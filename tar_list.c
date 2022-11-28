#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <arpa/inet.h>
#include <string.h>
#include <dirent.h>
#include <time.h>
#include "mytar.h"

#define NAME 257
#define USERGROUP 19
#define TIMESIZE 16
#define PERMNUM 11
#define OCTALNUM 8
#define REGNUM 48
#define SYMNUM 50
#define DIRNUM 53
#define TIMELEN 18

void add_perm_to_list(int perm, char *list, int typeflag);
int check_end_of_tar(unsigned char *prev_block, unsigned char *curBlock);
int tar_validation(unsigned char *header);
int check_end_of_tar(unsigned char *prev_block, unsigned char *curBlock);

void tar_list(int tar_fd, int *flags, char *path_names[], int total_path) {
    
    char name[NAME] = {'\0'};
    char permOctal[MODE_SIZE];
    int i, perm, j = 0; 
    char permission[PERMNUM] = {'\0'};
    char uid[UID_SIZE], gid[GID_SIZE];
    char user_group[USERGROUP + 1] = {'\0'};
    char size_temp[SIZE_SIZE] = {'\0'};
    int size;
    char mtimeBuff[TIMESIZE];
    int num_read;
    struct tm* mtinfo;
    time_t mtime;
    int tar_val, end;
    char magic[MAGIC_SIZE + 1], version[VERSION_SIZE]; /* strict check */

    /* The + 1 is for NULL termination */
    char currBuff[BLOCKSIZE + 1] = {'\0'};
    char prevBuff[BLOCKSIZE + 1] = {'\0'};
    
    /* Keep reading 512 blocks of data */
    while ((num_read = read(tar_fd, currBuff, BLOCKSIZE)) > 0) {

        /* if it's not a corrupt header or end of file with two 512 
         * blocks of NULL, load in the necessary data. 
         * NOTE: This part can probably be made into an unpacking header
         * function, but I was too lazy since and I was soloing so yeah */
        if ((tar_val = tar_validation((unsigned char *)currBuff)) == 1 && 
            !(end = check_end_of_tar((unsigned char *)prevBuff, 
            (unsigned char *)currBuff))) {
            /* get the permission from the header and store in permOctal */
            strcpy(permOctal, &currBuff[MODE_OFFSET]);
            perm = strtol(permOctal, NULL, OCTALNUM);
            add_perm_to_list(perm, permission, currBuff[TYPEFLAG_OFFSET]);

            /* if user name is greater than 17 in length */
            strncpy(user_group, &currBuff[UNAME_OFFSET], USERGROUP);
            strcat(user_group, "/");
            strncat(user_group, &currBuff[GNAME_OFFSET], 
                USERGROUP - strlen(user_group) - 1);
           
           /* pad out the rest of the user/gropu with spaces */ 
            for (i = 0; i < 18 - strlen(user_group); i++) {
                strcat(user_group, " ");
            }

            /* get size and store in char as ASCII in octal */
            j = 0;
            for (i = SIZE_OFFSET; i < MTIME_OFFSET; i++) {
                size_temp[j++] = currBuff[i];
            }
            /* term octal size to long integer */
            size = strtol(size_temp, NULL, OCTALNUM);

            /* If size == 0, meaning it's a directory, connect the prefix 
             * else just connect the name*/
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
            /* copy to mtime */
            strcpy(mtimeBuff, &currBuff[MTIME_OFFSET]);
            mtime =  strtol(mtimeBuff, NULL, OCTALNUM);
            mtinfo = localtime(&mtime);
            strftime(mtimeBuff, TIMELEN, "%Y-%m-%d %H:%M", mtinfo);

            /* Check for strict compliance */
            strncpy(magic, &currBuff[MAGIC_OFFSET], MAGIC_SIZE + 1);
            strncpy(version, &currBuff[VERSION_OFFSET], VERSION_SIZE);
            /* check to see if it's Strict con*/
            if (flags[SFLAGPOS]) {
                if (check_strict(magic, version)) {
                    fprintf(stderr, "Not Conforming to Strict Mode\n");
                }
            }
        }
        /* if it's a courrupt file or it's end of file, STOP READING!!! */
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

        /* print out the thing. If there are arguments, check if the name is
         * in the occur of the filename, if yes, print it out, if not
         * don't print it out. */
        if (total_path > 0) {
            /* Loop through the path and check if it's something that you 
             * have wanted to list */
            for (i = 0; i < total_path; i++) {
                /* if it's something you want to list, print it out */
                if (strstr(name, path_names[i]) != NULL) {
                    if (flags[VFLAGPOS]) {
                        /* if v flag is on, print out the longer version */
                        printf("%s %s %8d %s %s\n", permission, user_group, 
                            size, mtimeBuff, name);
                        break;
                    }
                    else {
                        /* if not print out the name only. */
                        printf("%s\n", name);
                        break;
                    }
                }
            }
        }
        else {
            /* if no path is given, print out every header */
            if (flags[VFLAGPOS]) {
                /* if v flag is on, print out the longer version */
                printf("%s %s %8d %s %s\n", permission, user_group, 
                            size, mtimeBuff, name);
            }
            else {
                /* if not print out the name only. */
                printf("%s\n", name);
            }
        }
        /* reset to '\0' so nothing bad happens? */
        memset(permOctal, '\0', MODE_SIZE + 1);
        memset(uid, '\0', UID_SIZE + 1);
        memset(gid, '\0', GID_SIZE + 1);
        memset(user_group, '\0', USERGROUP + 1);
        memset(size_temp, '\0', SIZE_SIZE);
        memset(name, '\0', NAME);
        memset(permission, '\0', PERMNUM);

        /* set previous buffer as current buffer to check for end of file */
        memset(prevBuff, '\0', (BLOCKSIZE + 1));
        strncpy(prevBuff, currBuff, (BLOCKSIZE + 1));
        
        /* skip the data if it has a size > 0 */
        if (size) {
            /* if the data was greater than 0, but less than 512, skip 
             * only one block */
            if (size % BLOCKSIZE > 0) {
                size += BLOCKSIZE; /*add a block so it will skip at least 512*/
            }
            if (lseek(tar_fd, BLOCKSIZE * (size / BLOCKSIZE), SEEK_CUR) < 0) {
                perror("lseek");
                exit(EXIT_FAILURE);
            }
        }
    }
}

void add_perm_to_list(int perm, char *list, int typeflag) {
    int i = 0;

    if(typeflag == REGNUM || typeflag == 0)
    {
        list[i++] = '-';
    }
    else if(typeflag == SYMNUM)
    {
        list[i++] = 'l';
    }
    else if(typeflag == DIRNUM)
    {
        list[i++] = 'd';
    }
    if (perm & S_IRUSR) {
        list[i++] = 'r';
    }
    else {
        list[i++] = '-';
    }
    if (perm & S_IWUSR) {
        list[i++] = 'w';
    }
    else {
        list[i++] = '-';
    }
    if (perm & S_IXUSR) {
        list[i++] = 'x';
    }
    else {
        list[i++] = '-';
    }
    if (perm & S_IRGRP) {
        list[i++] = 'r';
    }
    else {
        list[i++] = '-';
    }
    if (perm & S_IWGRP) {
        list[i++] = 'w';
    }
    else {
        list[i++] = '-';
    }
    if (perm & S_IXGRP) {
        list[i++] = 'x';
    }
    else {
        list[i++] = '-';
    }    
    if (perm & S_IROTH) {
        list[i++] = 'r';
    }
    else {
        list[i++] = '-';
    }   
    if (perm & S_IWOTH) {
        list[i++] = 'w';
    }
    else {
        list[i++] = '-';
    }  
    if (perm & S_IXOTH) {
        list[i++] = 'x';
    }
    else {
        list[i++] = '-';
    }   
}

int tar_validation(unsigned char *header) {
    /* Function takes in 512 bytes. Goes through each byte and add it
     * up except for the chksum field. If it is the chksum field, add it
     * to a chksum buffer in order to convert it to int later. Check if 
     * the chksum field matches with the total bytes in the header. 
     * Return 1, if it does, else return 0 if it doesn't (not a tar header) */

    int i, j = 0; /* array counters */
    int total_chksum = 0, header_chksum;
    char chksum[CHKSUM_SIZE]; /* store chksum string in octal */
    for (i = 0; i < BLOCKSIZE; i++) {
        /* if it isn't chksum field, add the byte to total_chksum */
        if (i < CHKSUM_OFFSET || i >= TYPEFLAG_OFFSET) {
            total_chksum += header[i];
        }
        /* Else if it's chksum field, add it to chksum to convert to int
         * later */
        else {
            chksum[j++] = header[i];
        }
    }
    /* convert string to octal in number */
    header_chksum = strtol(chksum, NULL, OCTALNUM);
    total_chksum += 8 * ' ';

    /* if it's equal, return true else false */
    if (total_chksum == header_chksum) 
        return 1;
    else 
        return 0;
}

int check_end_of_tar(unsigned char *prev_block, unsigned char *curBlock) {
    /* Takes in two blocks of character buffers. Checks if both 
     * blocks are equal to NULL or 0 in all their fields. */
    
    int i;
    for (i = 0; i < BLOCKSIZE; i++) {
        if (prev_block[i] == '\0' && curBlock[i] == '\0') {
            continue;
        }
        else {
            return 0; /* it's not both end of file */
        }
    }
    return 1; /* checked all and it's all NULL */
}

int check_strict(char *magic, char *version) {
    /* does strict mode checks */
    char s_magic[MAGIC_SIZE + 1] = "ustar\0";
    char s_version[MAGIC_SIZE] = "00";

    return (strncmp(s_magic, magic, MAGIC_SIZE + 1) + 
            strncmp(s_version, version, MAGIC_SIZE));
}
