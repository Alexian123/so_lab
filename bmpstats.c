#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>

#define OUT_FILE_PATH "stats.txt"
#define MAX_OUT_FILE_SIZE 512   // bytes
#define MAX_DATETIME_LEN 32

#define BMP_HEADER_WIDTH_OFFSET 18

/* 
    Read integer (4B) from current pos in file and
    exit with error message if it fails.
*/
int read_int_or_fail(int fd) {
    int val = 0;
    int count = read(fd, &val, sizeof(val));
    if (count < 0) {
        perror("Error reading bmp file");
        exit(EXIT_FAILURE);
    }
    if (count != sizeof(val)) {
        fprintf(stderr, "Error reading bmp file: Data may be corrupted\n");
        exit(EXIT_FAILURE);
    };
    return val;
}

int main(int argc, char **argv) {
    // check arguments
    if (argc != 2) {
        fprintf(stderr, "Wrong number of arguments.\n");
        fprintf(stderr, "Usage: %s <input_file.bmp>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    
    // open bmp file
    int bmp_fd = open(argv[1], O_RDONLY);
    if (bmp_fd < 0) {
        perror("Error opening bmp file");
        exit(EXIT_FAILURE);
    }

    // create output file -> rw-rw-r--
    int out_fd = creat(OUT_FILE_PATH, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
    if (out_fd < 0) {
        perror("Error creating output file");
        exit(EXIT_FAILURE);
    }
    
    // move cursor to read width & height
    if (lseek(bmp_fd, BMP_HEADER_WIDTH_OFFSET, SEEK_SET) < 0) {
        perror("Error parsing bmp file");
        exit(EXIT_FAILURE);
    }
    
    // read bmp image width and height
    int width = read_int_or_fail(bmp_fd);
    int height = read_int_or_fail(bmp_fd);
    
    // read bmp file stats
    struct stat st;
    if (fstat(bmp_fd, &st) < 0) {
        perror("Error reading bmp file stats");
        exit(EXIT_FAILURE);
    }

    // convert time of last modification from time_t to struct tm
    char tbuf[MAX_DATETIME_LEN] = "";
    time_t t = st.st_mtime;
    struct tm *lt = localtime(&t);
    strftime(tbuf, MAX_DATETIME_LEN, "%c", lt);


    // write stats to buffer
    char buf[MAX_OUT_FILE_SIZE] = "";
    memset(buf, 0, MAX_OUT_FILE_SIZE);
    snprintf(buf, MAX_OUT_FILE_SIZE, 
    "file name: %s\nheight: %d\nwidth: %d\nsize: %ld\nuser id: %d\n"
    "time of last modification: %s\nuser permissions: %c%c%c\n"
    "group permissions: %c%c%c\nothers permissions: %c%c%c",
    argv[1], height, width, st.st_size, st.st_uid, tbuf, 
    (st.st_mode & S_IRUSR) ? 'R' : '-', (st.st_mode & S_IWUSR) ? 'W' : '-', (st.st_mode & S_IXUSR) ? 'X' : '-',
    (st.st_mode & S_IRGRP) ? 'R' : '-', (st.st_mode & S_IWGRP) ? 'W' : '-', (st.st_mode & S_IXGRP) ? 'X' : '-',
    (st.st_mode & S_IROTH) ? 'R' : '-', (st.st_mode & S_IWOTH) ? 'W' : '-', (st.st_mode & S_IXOTH) ? 'X' : '-');

    // write buffer to output file
    size_t len = strlen(buf);
    int ret_val = write(out_fd, buf, len);
    if (ret_val < 0) {
        perror("Error writing output file");
        exit(EXIT_FAILURE);
    }
    if (ret_val != len) {
        fprintf(stderr, "Error writing output file: Wrote %d bytes instead of %lu\n", ret_val, len);
        exit(EXIT_FAILURE);
    }
    
    // close files
    if (close(bmp_fd) < 0) {
        perror("Error closing bmp file");
        exit(EXIT_FAILURE);
    }
    if (close(out_fd) < 0) {
        perror("Error closing output file");
        exit(EXIT_FAILURE);
    }

    return 0;
}
