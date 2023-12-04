#ifndef FILESTATS_H
#define FILESTATS_H
    #include <stdio.h>
    #include <stdlib.h>
    #include <stdint.h>
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

    #define OUT_ARCHIVE_NAME "statistics"

    static char tbuf[MAX_DATETIME_LEN] = "";    // buffer for storing date & time
    static char buf[MAX_OUT_FILE_ENTRY_SIZE] = ""; // output buffer
    static char out_path[MAX_FILE_PATH_LEN] = "";   // ouutput file path buffer

    void write_regular_file_stats(const char *name, struct stat *stp);

    void write_bmp_file_stats(const char *name, const char *path, struct stat *stp);
    void convert_bmp_to_grayscale(const char *path);

    void write_link_stats(const char *name, const char *path, struct stat *stp);

    void write_dir_stats(const char *name, struct stat *stp);

    // Write 'buf' to 'out_path'
    void write_to_output_file();

#endif