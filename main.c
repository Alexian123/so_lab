#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#define BMP_HEADER_WIDTH_OFFSET 18

// reads an integer (4 bytes) from file and returns it
static int read_int(int fd);

int main(int argc, char **argv) {
    // check arguments
    if (argc != 2) {
        fprintf(stderr, "Wrong number of arguments.\n");
        fprintf(stderr, "Usage: %s <input_file.bmp>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    
    // open file
    int fd = open(argv[1], O_RDONLY);
    if (fd < 0) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }
    
    // move cursor to read width & height
    if (lseek(fd, BMP_HEADER_WIDTH_OFFSET, SEEK_SET) < 0) {
        perror("Error parsing file");
        exit(EXIT_FAILURE);
    }
    
    int ret_val = 0;
    int width = 0;
    if (ret_val = read(fd, &width, sizeof(width)) < 0) {
        perror("Error reading file");
        exit(EXIT_FAILURE);
    }
    if (ret_val != sizeof(width)) {
        fprintf("Invalid bitmap file header.\n");
        exit(EXIT_FAILURE);
    }
    
    
    
    
    struct stat st;
    if (fstat(fd, &st) < 0) {
        perror("Error reading file stats");
        exit(EXIT_FAILURE);
    }
    
    
    
    return 0;
}
