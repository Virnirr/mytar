#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include "mytar.h"

#define FLAGSIZE 6
#define ARGSIZE 3
#define MAX_PATH_LEN 256

#define RWPERMS S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH

void get_flags(char *flagArg, int *flag_store);

int main(int argc, char *argv[]) {

    /* Each position is c, t, x, v, f, and S flags if not 0 */
    int flag[FLAGSIZE] = {0, 0, 0, 0, 0, 0};
    char *tarname = argv[2];
    char *tar_path[argc - ARGSIZE];
    int tar_fd, i, total_path = argc - ARGSIZE;
    
    /* Get the flags into the flag arrays */
    get_flags(argv[1], flag);
    /* Place the path into the tar_path in order to compile later */
    for (i = 0; i < total_path; i++) {
        /* checks if the path name is less than or greater than 256 */
        if (strlen(argv[i + ARGSIZE]) > MAX_PATH_LEN) {
            fprintf(stderr, "Path name is too long\n");
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

/* parse the flag and check */
void get_flags(char *flagArg, int *flag_store) {
    /* Loop through flagArg until it reaches the end. If there are more than 
     * one of the same flag that isn't v flag, error out and exit. If there
     * is no flag, also error out. If the flag is not [cvtfS], also error 
     * out. */
    int flagPos = 0;
    int argPos = 0;
    while (flagArg[argPos]) {
        switch(flagArg[argPos]) {
            case 'c':
                if (flag_store[CFLAGPOS] == 0) {
                    flag_store[CFLAGPOS]++;
                    flagPos++;
                }
                else {
                    fprintf(stderr, "Invalid amount of flag [ctxvS]f\n");
                    exit(EXIT_FAILURE);
                }
                argPos++;
                break;
            case 't':
                if (flag_store[TFLAGPOS] == 0) {
                    flag_store[TFLAGPOS]++;
                    flagPos++;
                }
                else {
                    fprintf(stderr, "Invalid amount of flag [ctxvS]f\n");
                    exit(EXIT_FAILURE);
                }
                argPos++;
                break;
            case 'x':
                if (flag_store[XFLAGPOS] == 0) {
                    flag_store[XFLAGPOS]++;
                    flagPos++;
                }
                else {
                    fprintf(stderr, "Invalid amount of flag [ctxvS]f\n");
                    exit(EXIT_FAILURE);
                }
                argPos++;
                break;
            case 'v':
                flag_store[VFLAGPOS]++;
                flagPos++;
                argPos++;
                break;
            case 'S':
                if (flag_store[SFLAGPOS] == 0) {
                    flag_store[SFLAGPOS]++;
                    flagPos++;
                }
                else {
                    fprintf(stderr, "Invalid amount of flag [ctxvS]f\n");
                    exit(EXIT_FAILURE);
                }
                argPos++;
                break;
            case 'f':
                if (flag_store[FFLAGPOS] == 0) {
                    flag_store[FFLAGPOS]++;
                    flagPos++;
                }
                else {
                    fprintf(stderr, "Invalid amount of flag [ctxvS]f\n");
                    exit(EXIT_FAILURE);
                }
                argPos++;
                break;
            /* if the flag is anything else, then run a usage Error */
            default:
                fprintf(stderr, 
                        "usage: mytar [ctxvS]f tarfile [ path [ ... ] ]\n");
                exit(EXIT_FAILURE);
        }
    }
    if (flagPos == 0) {
        fprintf(stderr, "usage: mytar [ctxvS]f tarfile [ path [ ... ] ]\n");
        exit(EXIT_FAILURE);
    }
}