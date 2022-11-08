#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include "parseFlag.h"
#include "mytar.h"

#define FLAGSIZE 6
#define ARGSIZE 3
#define MAX_PATH_LEN 256

#define RWPERMS S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH

int main(int argc, char *argv[]) {

    /* Each position is c, t, x, v, f, and S flags if not 0 */
    int flag[FLAGSIZE] = {0, 0, 0, 0, 0, 0};
    char *tarname = argv[2], *tar_path[argc - ARGSIZE];
    int tar_fd, i, total_path = argc - ARGSIZE;
    
    /* error out if the usage is not at least 3 */
    if (argc < ARGSIZE) {
        usageError();
    }

    /* Get the flags into the flag arrays */
    parse_flags(argv[1], flag);

    for (i = 0; i < total_path; i++) {
        if (strlen(argv[i + ARGSIZE]) > MAX_PATH_LEN) {
            fprintf(stderr, "Path name cannot be longer than 256 characters\n");
            exit(EXIT_FAILURE);
        }
        tar_path[i] = argv[i + ARGSIZE];
    }

    if (flag[CFLAGPOS]) {
        tar_fd = open(tarname, O_WRONLY | O_CREAT | O_TRUNC, RWPERMS);
        if (tar_fd < 0) {
            perror("open");
            exit(EXIT_FAILURE);
        }
        tar_create(tar_fd, flag, tar_path, total_path);
        if (close(tar_fd) < 0) {
            perror("close");
            exit(EXIT_FAILURE);
        }
    }
    else if (flag[TFLAGPOS]) {
        tar_fd = open(tarname, O_RDONLY);
        if (tar_fd < 0) {
            perror("open");
            exit(EXIT_FAILURE);
        }
        tar_list(tar_fd, flag, tar_path, total_path);
    }
    else if (flag[XFLAGPOS]) {
        tar_fd = open(tarname, O_RDONLY);
        if (tar_fd < 0) {
            perror("open");
            exit(EXIT_FAILURE);
        }
        tar_extract(tar_fd, flag, tar_path, total_path);
    }

    return 0;
}