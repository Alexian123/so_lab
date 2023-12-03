#include "filestats.h"
#include "bitmap.h"

int main(int argc, char **argv) {
    // check arguments
    if (argc != 4) {
        fprintf(stderr, "Wrong number of arguments.\n");
        fprintf(stderr, "Usage: %s <input_directory> <output_directory> <c>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // open input dir
    DIR *dirp = opendir(argv[1]);
    if (dirp == NULL) {
        perror("Error opening input directory");
        exit(EXIT_FAILURE);
    }

    // count how many correct sentnces there are in all regular files
    int num_correct_senteces = 0;

    // skip first 2 entries from input dir ('.' and '..')
    struct dirent *dr = readdir(dirp);
    dr = readdir(dirp);
    dr = readdir(dirp);

    // loop through all files from the input dir
    char file_path[MAX_FILE_PATH_LEN] = "";
    struct stat fst;
    int pid = 0;    // temporarely store child pid
    for (; dr != NULL; dr = readdir(dirp)) {
        // build relative file path
        memset(file_path, 0, MAX_FILE_PATH_LEN);
        strncat(file_path, argv[1], MAX_FILE_PATH_LEN - 1);
        strncat(file_path, "/", MAX_FILE_PATH_LEN - strlen(file_path) - 1);
        strncat(file_path, dr->d_name, MAX_FILE_PATH_LEN - strlen(file_path) - 1);
        size_t path_len = strlen(file_path);

        // read file stats
        if (lstat(file_path, &fst) < 0) {
            perror("Error reading file stats");
            exit(EXIT_FAILURE);
        }

        // get date & time of last modification as formatted string
        time_t t = fst.st_mtime;
        struct tm *lt = localtime(&t);
        strftime(tbuf, MAX_DATETIME_LEN, "%c", lt);

        // generate output file path
        memset(out_path, 0, MAX_FILE_PATH_LEN);
        strncat(out_path, argv[2], MAX_FILE_PATH_LEN - 1);
        strncat(out_path, "/", MAX_FILE_PATH_LEN - strlen(out_path) - 1);
        strncat(out_path, dr->d_name, MAX_FILE_PATH_LEN - strlen(out_path) - 1);
        strncat(out_path, "_", MAX_FILE_PATH_LEN - strlen(out_path) - 1);
        strncat(out_path, OUT_FLE_PATH_SUFFIX, MAX_FILE_PATH_LEN - strlen(out_path) - 1);

        // create pipes (pipe1: child1 -> child2; pipe2: child2 -> parent)
        int p1fd[2], p2fd[2];
        if ((pipe(p1fd) < 0) || (pipe(p2fd) < 0)) {
            perror("Error creating pipes");
            exit(EXIT_FAILURE);
        }

        // create 1st child process
        if ((pid = fork()) < 0) {
            perror("Error creating process");
            exit(EXIT_FAILURE);
        }

        if (S_ISLNK(fst.st_mode)) { /* SYMLINK */

            if (pid == 0) { // 1st child
                write_link_stats(dr->d_name, file_path, &fst);
                exit(EXIT_SUCCESS);
            }

        } else if (S_ISDIR(fst.st_mode)) {  /* DIRECTORY */

            if (pid == 0) { // 1st child
                write_dir_stats(dr->d_name, &fst);
                exit(EXIT_SUCCESS);
            }

        } else if (file_path[path_len - 4] == '.' && file_path[path_len - 3] == 'b' && 
            file_path[path_len - 2] == 'm' && file_path[path_len - 1] == 'p') { /* BITMAP */

            if (pid == 0) { // 1st child
                write_bmp_file_stats(dr->d_name, file_path, &fst);
                exit(EXIT_SUCCESS);
            } else {    // parent
                if ((pid = fork()) < 0) {   // create 2nd child
                    perror("Error creating process");
                    exit(EXIT_FAILURE);
                }
                if (pid == 0) { // 2nd child
                    convert_bmp_to_grayscale(file_path);
                    exit(EXIT_SUCCESS);
                }
            }

        } else if (S_ISREG(fst.st_mode)) {  /* REGULAR FILE */

            if (pid == 0) { // 1st child
                write_regular_file_stats(dr->d_name, &fst);

                // close both ends of pipe2
                close(p2fd[0]);
                close(p2fd[1]);

                // close read end of pipe1
                close(p1fd[0]);

                // redirect stdout to write end of pipe1
                if (dup2(p1fd[1], 1) < 0) {
                    perror("Error redirecting stdout");
                    exit(EXIT_FAILURE);
                }

                // send file contents through pipe1
                execlp("cat", "cat", file_path, NULL);

                exit(EXIT_FAILURE); // safety (should not return here after execlp)
            } else {    // parent
                if ((pid = fork()) < 0) {   // create 2nd child
                    perror("Error creating process");
                    exit(EXIT_FAILURE);
                }
                if (pid == 0) { // 2nd child
                    // close write end of pipe1
                    close(p1fd[1]);

                    // redirect stdin to read end of pipe1
                    if (dup2(p1fd[0], 0) < 0) {
                        perror("Error redirecting stdin");
                        exit(EXIT_FAILURE);
                    }

                    // close read end of pipe2
                    close(p2fd[0]);

                    // redirect stdout to write end of pipe2
                    if (dup2(p2fd[1], 1) < 0) {
                        perror("Error redirecting stdout");
                        exit(EXIT_FAILURE);
                    }
                    
                    // call script which reads from stdin and writes to stdout
                    execlp("/bin/sh", "/bin/sh", "./count_correct_sentences.sh", argv[3], NULL);

                    exit(EXIT_FAILURE); // safety (should not return here after execlp!)
                }
                // parent
                
                // close both ends of pipe1
                close(p1fd[0]);
                close(p1fd[1]);

                // close write end of pipe2
                close(p2fd[1]);

                // read number from pipe2 as string
                char bf[16] = "";
                if (read(p2fd[0], bf, sizeof(bf)) < 0) {
                    perror("Error reading from pipe");
                    exit(EXIT_FAILURE);
                }

                // update total number of correct sentences
                num_correct_senteces += atoi(bf);
            }

        }

        // make sure all pipes are closed
        close(p1fd[0]);
        close(p1fd[1]);
        close(p2fd[0]);
        close(p2fd[1]);
    }

    // wait for all the children to die
    int status = 0; // store child process status
    while ((pid = wait(&status)) > 0) {
        printf("Process with pid %d ended with code %d\n", pid, status);
    }
    
    // close input dir
    closedir(dirp);

    printf("%d correct sentences which contain the character \'%c\' have been identified\n", num_correct_senteces, argv[3][0]);

    return 0;
}

void write_regular_file_stats(const char *name, struct stat *stp) {
    // write stats to buffer
    snprintf(buf, MAX_OUT_FILE_ENTRY_SIZE, 
    "file name: %s\nsize: %ld\nuser id: %d\n"
    "time of last modification: %s\nnumber of hard links: %ld\nuser permissions: %c%c%c\n"
    "group permissions: %c%c%c\nothers permissions: %c%c%c\n\n",
    name, stp->st_size, stp->st_uid, tbuf, stp->st_nlink,
    (stp->st_mode & S_IRUSR) ? 'R' : '-', (stp->st_mode & S_IWUSR) ? 'W' : '-', (stp->st_mode & S_IXUSR) ? 'X' : '-',
    (stp->st_mode & S_IRGRP) ? 'R' : '-', (stp->st_mode & S_IWGRP) ? 'W' : '-', (stp->st_mode & S_IXGRP) ? 'X' : '-',
    (stp->st_mode & S_IROTH) ? 'R' : '-', (stp->st_mode & S_IWOTH) ? 'W' : '-', (stp->st_mode & S_IXOTH) ? 'X' : '-');

    write_to_output_file();
}

void write_bmp_file_stats(const char *name, const char *path, struct stat *stp) {
    // open file
    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }
    
    // move cursor to InfoHeader section in bitmap
    if (lseek(fd, sizeof(struct BMPHeader), SEEK_SET) < 0) {
        perror("Error seeking through file");
        exit(EXIT_FAILURE);
    }
    
    // read InfoHeader
    struct BMPInfoHeader info;
    if (read(fd, &info, sizeof(info)) < 0)  {
        perror("Error reading bitmap header");
        exit(EXIT_FAILURE);
    }
    
    // close file
    close(fd);
    
    // write stats to buffer
    snprintf(buf, MAX_OUT_FILE_ENTRY_SIZE, 
    "file name: %s\nheight: %d\nwidth: %d\nsize: %ld\nuser id: %d\n"
    "time of last modification: %s\nnumber of hard links: %ld\nuser permissions: %c%c%c\n"
    "group permissions: %c%c%c\nothers permissions: %c%c%c\n\n",
    name, info.height, info.width, stp->st_size, stp->st_uid, tbuf, stp->st_nlink,
    (stp->st_mode & S_IRUSR) ? 'R' : '-', (stp->st_mode & S_IWUSR) ? 'W' : '-', (stp->st_mode & S_IXUSR) ? 'X' : '-',
    (stp->st_mode & S_IRGRP) ? 'R' : '-', (stp->st_mode & S_IWGRP) ? 'W' : '-', (stp->st_mode & S_IXGRP) ? 'X' : '-',
    (stp->st_mode & S_IROTH) ? 'R' : '-', (stp->st_mode & S_IWOTH) ? 'W' : '-', (stp->st_mode & S_IXOTH) ? 'X' : '-');

    write_to_output_file();
}

void write_link_stats(const char *name, const char *path, struct stat *stp) {
    // read stats of target file
    struct stat tgf;
    if (stat(path, &tgf) < 0) {
        perror("Error reading target file stats");
        exit(EXIT_FAILURE);
    }

    // write stats to buffer
    snprintf(buf, MAX_OUT_FILE_ENTRY_SIZE, 
    "link name: %s\nlink size: %ld\ntarget file size: %ld\n"
    "time of last modification: %s\nuser permissions: %c%c%c\n"
    "group permissions: %c%c%c\nothers permissions: %c%c%c\n\n",
    name, stp->st_size, tgf.st_size, tbuf,
    (stp->st_mode & S_IRUSR) ? 'R' : '-', (stp->st_mode & S_IWUSR) ? 'W' : '-', (stp->st_mode & S_IXUSR) ? 'X' : '-',
    (stp->st_mode & S_IRGRP) ? 'R' : '-', (stp->st_mode & S_IWGRP) ? 'W' : '-', (stp->st_mode & S_IXGRP) ? 'X' : '-',
    (stp->st_mode & S_IROTH) ? 'R' : '-', (stp->st_mode & S_IWOTH) ? 'W' : '-', (stp->st_mode & S_IXOTH) ? 'X' : '-');

    write_to_output_file();
}

void write_dir_stats(const char *name, struct stat *stp) {
    // write stats to buffer
    snprintf(buf, MAX_OUT_FILE_ENTRY_SIZE, 
    "directory name: %s\nuser id: %d\n"
    "time of last modification: %s\nuser permissions: %c%c%c\n"
    "group permissions: %c%c%c\nothers permissions: %c%c%c\n\n",
    name, stp->st_uid, tbuf,
    (stp->st_mode & S_IRUSR) ? 'R' : '-', (stp->st_mode & S_IWUSR) ? 'W' : '-', (stp->st_mode & S_IXUSR) ? 'X' : '-',
    (stp->st_mode & S_IRGRP) ? 'R' : '-', (stp->st_mode & S_IWGRP) ? 'W' : '-', (stp->st_mode & S_IXGRP) ? 'X' : '-',
    (stp->st_mode & S_IROTH) ? 'R' : '-', (stp->st_mode & S_IWOTH) ? 'W' : '-', (stp->st_mode & S_IXOTH) ? 'X' : '-');

    write_to_output_file();
}

void convert_bmp_to_grayscale(const char *path) {
    // open file
    int fd = open(path, O_RDWR);
    if (fd < 0) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    // read Header & InfoHeader
    struct BMPFile f;
    if ((read(fd, &f.header, sizeof(f.header)) < 0) || (read(fd, &f.info, sizeof(f.info)) < 0)) {
        perror("Error reading bitmap header");
        exit(EXIT_FAILURE);
    }

    if (f.info.bitCount <= 8) {
        fprintf(stderr, "Unsupported bitmap format\n");
        exit(EXIT_FAILURE);
    }

    // read pixel data
    int imgSize = f.info.width * f.info.height * 3;
    f.data = (uint8_t*) malloc(imgSize);
    if (f.data == NULL) {
        perror("Malloc error");
        exit(EXIT_FAILURE);
    }
    if (read(fd, f.data, imgSize) < 0) {
        perror("Error reading bitmap pixel data");
        exit(EXIT_FAILURE);
    }


    // convert to grayscale
    // p_grey = 0.299*p_red + 0.587*p_green + 0.114*p_blue
    for (int i = 0; i < f.info.height; ++i) {
        for (int j = 0; j < f.info.width; ++j) {
            uint8_t *pixel = &f.data[(i*f.info.width + j) * 3];
            uint8_t gray = (uint8_t) (0.299 * pixel[2] + 0.587 * pixel[1] + 0.114 * pixel[0]);
            pixel[0] = pixel[1] = pixel[2] = gray;
        }
    }

    // go to pixel data section of file (skip Header and InfoHeader)
    if (lseek(fd, sizeof(struct BMPHeader) + sizeof(struct BMPInfoHeader), SEEK_SET) < 0) {
        perror("Error seeking through file");
        exit(EXIT_FAILURE);
    }

    // overwrite file
    if (write(fd, f.data, imgSize) < 0) {
        perror("Error converting bitmap to grayscale");
        free(f.data);
        exit(EXIT_FAILURE);
    }
    free(f.data);

    // clsoe file
    close(fd);
}

void write_to_output_file() {
    // create output file -> rw-rw-r--
    int out_fd = creat(out_path, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
    if (out_fd < 0) {
        perror("Error creating output file");
        exit(EXIT_FAILURE);
    }

    // write data
    size_t len = strlen(buf);
    int ret_val = write(out_fd, buf, len);
    if (ret_val < 0) {
        perror("Error writing output file");
        exit(EXIT_FAILURE);
    } else if (ret_val != len) {
        fprintf(stderr, "Error writing output file: Wrote %d bytes instead of %lu\n", ret_val, len);
        exit(EXIT_FAILURE);
    }

    // close file
    close(out_fd);
}