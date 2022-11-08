#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <pwd.h>
#include <grp.h>
#include <errno.h>
#include <arpa/inet.h>
#include <string.h>
#include <dirent.h>
#include <time.h>
#include <utime.h>
#include "mytar.h"
#include "parseFlag.h"

#define NAME 257
#define RWPERMS S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH
#define TWO 2
#define TIMESIZE 16

int tar_validation(unsigned char *header);
int check_end_of_tar(unsigned char *prev_block, unsigned char *curBlock);

void tar_extract(int tar_fd, int *flags, char *path_names[], int total_path) {
    int num_read;  
    char typeflag[TWO] = {'\0'};
    int size;
    char str_size[SIZE_SIZE] = {'\0'}; 
    struct tm* mtstruct;
    time_t mtime;
    char name[NAME] = {'\0'};
    char linkname[LINKNAME_SIZE + 1] = {'\0'};
    char permOctal[MODE_SIZE];
    int check;
    int i, perm, j = 0; 
    struct utimbuf utimestruct;
    int regFile, amount_to_write;
    char mtimeBuff[TIMESIZE];
    int end; /* end of file */
    int file_perm;

    /* The + 1 is for NULL termination */
    char currBuff[BLOCKSIZE + 1] = {'\0'};
    char prevBuff[BLOCKSIZE + 1] = {'\0'};

    /* Keep readinga and looping until you get to a corrupt file or 
     * end of file */
    while ((num_read = read(tar_fd, currBuff, BLOCKSIZE)) > 0) {
        if (tar_validation((unsigned char *)currBuff) == 1 && 
            !check_end_of_tar((unsigned char *)prevBuff, 
            (unsigned char *)currBuff)) {

            /* get the permission from the header and store in permOctal */
            strcpy(permOctal, &currBuff[MODE_OFFSET]);
            perm = strtol(permOctal, NULL, 8);

            /* Get the flag to check what kind of file it is */
            typeflag[0] = currBuff[TYPEFLAG_OFFSET];
            /* Get the size of the file */
            strcpy(str_size, &currBuff[SIZE_OFFSET]);
            size = strtol(str_size, NULL, 8);

            /* get the mtime to store into file later */
            strcpy(mtimeBuff, &currBuff[MTIME_OFFSET]);
            mtime =  strtol(mtimeBuff, NULL, 8);
            mtstruct = localtime(&mtime);

            /* If size == 0, meaning it's a directory, connect the prefix 
             * else just connect the name */
            j = 0;
            for (i = PREFIX_OFFSET; currBuff[i] != '\0' && i < BLOCKSIZE; 
                        i++) {
                    /* while the prefix index is not NULL and it's less 512 */
                if (currBuff[i]) {
                    name[j++] = currBuff[i];
                }
            }
            if (name[0]) {
                name[j++] = '/';
            }
            for (i = 0; currBuff[i] != '\0' && i < MODE_OFFSET; i++) {
                    /* while the prefix index is not NULL and it's less 512 */
                name[j++] = currBuff[i];
            }
            
            /* copy linkname to linkname for symbolic link */
            strncpy(linkname, &currBuff[LINKNAME_OFFSET], LINKNAME_SIZE);
        }
        else {
            /* check if it's end of file */
            /* set previous buffer as current buffer to check for end of file */
            memset(prevBuff, '\0', (BLOCKSIZE + 1));
            strncpy(prevBuff, currBuff, (BLOCKSIZE + 1));

            if((num_read = read(tar_fd, currBuff, BLOCKSIZE)) < 0) {
                perror("read");
                exit(EXIT_FAILURE);
            }
            close(tar_fd);
            if ((end = check_end_of_tar((unsigned char *)prevBuff, 
                (unsigned char *)currBuff))) {
                break;
            }
            else {
                fprintf(stderr, "Bad Tarfile\n");
                break;
            }
        }

        utimestruct.modtime = mtime;
        file_perm = perm;
        /* Extract the file. If there are arguments, check if the name is
         * in the occur of the filename, if yes, extract it out, if not
         * don't print it out. */
        if (total_path > 0) {
            /* Loop through the path and check if it's something that you 
             * have wanted to list */
            for (i = 0; i < total_path; i++) {
                /* if it's something you want to list, print it out */
                if (strstr(path_names[i], name) != NULL) {
                    if (flags[VFLAGPOS]) {
                        /* if v flag is on, print out the longer version */
                        printf("%s\n", name);
                    }

                    /* if it's a directory, make it */
                    if (!strcmp(typeflag, "5")) {
                        if ((check = mkdir(name, file_perm)) < 0 && 
                                errno != EEXIST) {
                            perror("mkdir");
                            exit(EXIT_FAILURE);
                        }
                    }
                    /* create symlink */
                    else if (!strcmp(typeflag, "2")) {
                        if ((check = symlink(name, linkname)) < 0) {
                            perror("symlink");
                            exit(EXIT_FAILURE);
                        }
                    }
                    /* if it's a regular file, create it and add data to it */
                    else if (!strcmp(typeflag, "0") || 
                                !strcmp(typeflag, "\0")) {
                        if ((regFile = open(name, O_WRONLY | O_CREAT | O_TRUNC,
                                file_perm) < 0)) {
                            perror("open");
                            exit(EXIT_FAILURE);
                        }
                        /* add block greater than 0 */
                        if (size % BLOCKSIZE > 0) {
                            size += BLOCKSIZE; 
                        }
                        for (i = 0; i < (size / BLOCKSIZE); i++) {
                            amount_to_write = 0;
                            if ((num_read = read(tar_fd, currBuff, BLOCKSIZE))
                                    > 0) {
                                /* Check and skip over NULL to write */
                                for (j = 0; j < num_read; j++) {
                                    if (currBuff[j]) {
                                        amount_to_write++;
                                    }
                                }
                                /* write the correct amount of word in file */
                                if (write(regFile, currBuff, amount_to_write) 
                                    < 0) {
                                    perror("write");
                                    exit(EXIT_FAILURE);
                                }
                            } 
                        }
                        if (close(regFile) < 0) {
                            perror("regFile");
                            exit(EXIT_FAILURE);
                        }
                    }
                    /* change utime of file at the end */
                    if ((check = utime(name, &utimestruct)) < 0) {
                        perror("utime");
                        exit(EXIT_FAILURE);
                    }
                }
            }
        }
        else {
            /* if no path is given, extract out every header */
            if (flags[VFLAGPOS]) {
                /* if v flag is on, print out the longer version */
                printf("%s\n", name);
            }
            /* if it's a directory, make it */
            if (!strcmp(typeflag, "5")) {
                if ((check = mkdir(name, file_perm)) < 0 && errno != EEXIST) {
                    perror("mkdir");
                    exit(EXIT_FAILURE);
                }
            }
            /* create symlink */
            else if (!strcmp(typeflag, "2")) {
                if ((check = symlink(name, linkname)) < 0) {
                    perror("symlink");
                    exit(EXIT_FAILURE);
                }
            }
            /* if it's a regular file, create it and add data to it */
            else if (!strcmp(typeflag, "0") || 
                        !strcmp(typeflag, "\0")) {
                if ((regFile = open(name, O_WRONLY | O_CREAT | O_TRUNC,
                        file_perm) < 0)) {
                    perror("open");
                    exit(EXIT_FAILURE);
                }
                /* add block greater than 0 */
                if (size % BLOCKSIZE > 0) {
                    size += BLOCKSIZE; 
                }
                for (i = 0; i < (size / BLOCKSIZE); i++) {
                    amount_to_write = 0;
                    if ((num_read = read(tar_fd, currBuff, BLOCKSIZE))
                            > 0) {
                        /* Check and skip over NULL to write */
                        for (j = 0; j < num_read; j++) {
                            if (currBuff[j]) {
                                amount_to_write++;
                            }
                        }
                        /* write the correct amount of word in file */
                        if (write(regFile, currBuff, amount_to_write) 
                            < 0) {
                            perror("write");
                            exit(EXIT_FAILURE);
                        }
                    } 
                }
                if (close(regFile) < 0) {
                    perror("regFile");
                    exit(EXIT_FAILURE);
                }
            }
            /* change utime of file at the end */
            if ((check = utime(name, &utimestruct)) < 0) {
                perror("utime");
                exit(EXIT_FAILURE);
            }

        }
        memset(str_size, '\0', SIZE_SIZE);
        memset(typeflag, '\0', TWO);
        memset(permOctal, '\0', MODE_SIZE + 1);
        memset(name, '\0', NAME);
        
        /* set previous buffer as current buffer to check for end of file */
        memset(prevBuff, '\0', (BLOCKSIZE + 1));
        strncpy(prevBuff, currBuff, (BLOCKSIZE + 1));
    }
}