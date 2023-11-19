#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/wait.h>

#define MAX_FILE_PATH_LEN 128

#define OUT_FLE_PATH_SUFFIX "stats.txt"
#define MAX_OUT_FILE_ENTRY_SIZE 512   // bytes
#define MAX_DATETIME_LEN 32

#define BMP_HEADER_WIDTH_OFFSET 18

static char tbuf[MAX_DATETIME_LEN] = "";    // buffer for storing date & time
static char buf[MAX_OUT_FILE_ENTRY_SIZE] = ""; // output buffer

/* 
    Try to read an integer (4B) from current pos in file and print error message if it fails.
*/
int read_next_int(int fd);


/*
    Handlers for different file types
*/
void write_regular_file_stats(const char *path, struct stat *stp);
void write_bmp_file_stats(const char *path, struct stat *stp);
void write_link_stats(const char *path, struct stat *stp);
void write_dir_stats(const char *path, struct stat *stp);

void convert_bmp_to_grayscale(const char *path);

/*
    Write 'buf' to 'out_fd'
*/
void write_to_output_file();


int main(int argc, char **argv) {
    // check arguments
    if (argc != 3) {
        fprintf(stderr, "Wrong number of arguments.\n");
        fprintf(stderr, "Usage: %s <input_directory> <output_directory>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // open input dir
    DIR *dirp = opendir(argv[1]);
    if (dirp == NULL) {
        perror("Error opening input directory");
        exit(EXIT_FAILURE);
    }


    // loop through all files from the input dir
    char file_path[MAX_FILE_PATH_LEN] = "";
    struct stat fst;
    int pid = 0;    // temporarely store child pid
    for (struct dirent *dr = readdir(dirp); dr != NULL; dr = readdir(dirp)) {
        // skip "." and ".." implicit dirs
        if (strcmp(dr->d_name, ".") == 0 || strcmp(dr->d_name, "..") == 0) {
            continue;
        }

        // build relative file path
        strcpy(file_path, "");
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

        // create child process
        if ((pid = fork()) < 0) {
            perror("Error creating process");
            exit(EXIT_FAILURE);
        }

        // handle file based on its type
        if (file_path[path_len - 5] == '.' && file_path[path_len - 4] == 'b' && 
            file_path[path_len - 3] == 'm' && file_path[path_len - 2] == 'p') {
            if (pid == 0) { // 1st child
                write_bmp_file_stats(file_path, &fst);
                goto KILL_CHILDREN;
            } else {    // parent
                if ((pid = fork()) < 0) {   // create 2nd child
                    perror("Error creating process");
                    exit(EXIT_FAILURE);
                }
                if (pid == 0) { // 2nd child
                    convert_bmp_to_grayscale(file_path);
                    goto KILL_CHILDREN;
                }
            }
        }

        if (pid != 0) { 
            continue;   // only parent process contiunes
        }

        if (S_ISREG(fst.st_mode)) {
            write_regular_file_stats(file_path, &fst);
        } else if (S_ISLNK(fst.st_mode)) {
            write_link_stats(file_path, &fst);
        } else if (S_ISDIR(fst.st_mode)) {
            write_dir_stats(file_path, &fst);
        }

    KILL_CHILDREN:
        exit(0);
    }

    // wait for all children to die
    int status = 0;     // store child process status
    while ((pid = wait(&status)) > 0) {
        printf("Process with pid %d ended with code %d\n", pid, status);
    }
    
    // close input dir
    if (closedir(dirp) < 0) {
        perror("Error closing input directory");
    }


    return 0;
}

int read_next_int(int fd) {
    int val = 0;
    int count = read(fd, &val, sizeof(val));
    if (count < 0) {
        perror("Error reading int from file");
        return -1;
    }
    if (count != sizeof(val)) {
        fprintf(stderr, "Error reading int from file: Data may be corrupted\n");
        return -1;
    };
    return val;
}

void write_regular_file_stats(const char *path, struct stat *stp) {
    // write stats to buffer
    snprintf(buf, MAX_OUT_FILE_ENTRY_SIZE, 
    "file name: %s\nsize: %ld\nuser id: %d\n"
    "time of last modification: %s\nnumber of hard links: %ld\nuser permissions: %c%c%c\n"
    "group permissions: %c%c%c\nothers permissions: %c%c%c\n\n",
    path, stp->st_size, stp->st_uid, tbuf, stp->st_nlink,
    (stp->st_mode & S_IRUSR) ? 'R' : '-', (stp->st_mode & S_IWUSR) ? 'W' : '-', (stp->st_mode & S_IXUSR) ? 'X' : '-',
    (stp->st_mode & S_IRGRP) ? 'R' : '-', (stp->st_mode & S_IWGRP) ? 'W' : '-', (stp->st_mode & S_IXGRP) ? 'X' : '-',
    (stp->st_mode & S_IROTH) ? 'R' : '-', (stp->st_mode & S_IWOTH) ? 'W' : '-', (stp->st_mode & S_IXOTH) ? 'X' : '-');

    write_to_output_file(path);
}

void write_bmp_file_stats(const char *path, struct stat *stp) {
    // open file
    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        perror("Error opening file");
        return;
    }
    
    // move cursor to read width & height
    if (lseek(fd, BMP_HEADER_WIDTH_OFFSET, SEEK_SET) < 0) {
        perror("Error seeking through file");
        return;
    }
    
    // read image width and height
    int width = read_next_int(fd);
    int height = read_next_int(fd);

    // close file
    if (close(fd) < 0) {
        perror("Error closing output file");
    }

    if (width < 0 || height < 0) { // error reading bmp header
        return;
    }
    
    // write stats to buffer
    snprintf(buf, MAX_OUT_FILE_ENTRY_SIZE, 
    "file name: %s\nheight: %d\nwidth: %d\nsize: %ld\nuser id: %d\n"
    "time of last modification: %s\nnumber of hard links: %ld\nuser permissions: %c%c%c\n"
    "group permissions: %c%c%c\nothers permissions: %c%c%c\n\n",
    path, height, width, stp->st_size, stp->st_uid, tbuf, stp->st_nlink,
    (stp->st_mode & S_IRUSR) ? 'R' : '-', (stp->st_mode & S_IWUSR) ? 'W' : '-', (stp->st_mode & S_IXUSR) ? 'X' : '-',
    (stp->st_mode & S_IRGRP) ? 'R' : '-', (stp->st_mode & S_IWGRP) ? 'W' : '-', (stp->st_mode & S_IXGRP) ? 'X' : '-',
    (stp->st_mode & S_IROTH) ? 'R' : '-', (stp->st_mode & S_IWOTH) ? 'W' : '-', (stp->st_mode & S_IXOTH) ? 'X' : '-');

    write_to_output_file(path);

}

void write_link_stats(const char *path, struct stat *stp) {
    // read stats of target file
    struct stat tgf;
    if (stat(path, &tgf) < 0) {
        perror("Error reading target file stats");
        return;
    }

    // write stats to buffer
    snprintf(buf, MAX_OUT_FILE_ENTRY_SIZE, 
    "link name: %s\nlink size: %ld\ntarget file size: %ld\n"
    "time of last modification: %s\nuser permissions: %c%c%c\n"
    "group permissions: %c%c%c\nothers permissions: %c%c%c\n\n",
    path, stp->st_size, tgf.st_size, tbuf,
    (stp->st_mode & S_IRUSR) ? 'R' : '-', (stp->st_mode & S_IWUSR) ? 'W' : '-', (stp->st_mode & S_IXUSR) ? 'X' : '-',
    (stp->st_mode & S_IRGRP) ? 'R' : '-', (stp->st_mode & S_IWGRP) ? 'W' : '-', (stp->st_mode & S_IXGRP) ? 'X' : '-',
    (stp->st_mode & S_IROTH) ? 'R' : '-', (stp->st_mode & S_IWOTH) ? 'W' : '-', (stp->st_mode & S_IXOTH) ? 'X' : '-');

    write_to_output_file(path);
}

void write_dir_stats(const char *path, struct stat *stp) {
    // write stats to buffer
    snprintf(buf, MAX_OUT_FILE_ENTRY_SIZE, 
    "directory name: %s\nuser id: %d\n"
    "time of last modification: %s\nuser permissions: %c%c%c\n"
    "group permissions: %c%c%c\nothers permissions: %c%c%c\n\n",
    path, stp->st_uid, tbuf,
    (stp->st_mode & S_IRUSR) ? 'R' : '-', (stp->st_mode & S_IWUSR) ? 'W' : '-', (stp->st_mode & S_IXUSR) ? 'X' : '-',
    (stp->st_mode & S_IRGRP) ? 'R' : '-', (stp->st_mode & S_IWGRP) ? 'W' : '-', (stp->st_mode & S_IXGRP) ? 'X' : '-',
    (stp->st_mode & S_IROTH) ? 'R' : '-', (stp->st_mode & S_IWOTH) ? 'W' : '-', (stp->st_mode & S_IXOTH) ? 'X' : '-');

    write_to_output_file(path);
}

void convert_bmp_to_grayscale(const char *path) {

}

void write_to_output_file(const char *path) {
    // create output file -> rw-rw-r--
    int out_fd = creat(OUT_FLE_PATH_SUFFIX, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
    if (out_fd < 0) {
        perror("Error creating output file");
        return;
    }
    size_t len = strlen(buf);
    int ret_val = write(out_fd, buf, len);
    if (ret_val < 0) {
        perror("Error writing output file");
    } else if (ret_val != len) {
        fprintf(stderr, "Error writing output file: Wrote %d bytes instead of %lu\n", ret_val, len);
    }
    // close output file
    if (close(out_fd) < 0) {
        perror("Error closing output file");
    }
}