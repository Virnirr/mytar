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
#include "mytar.h"
#include "parseFlag.h"

#define MODEMASK  S_ISUID | S_ISGID | S_ISVTX | S_IRWXU | S_IRWXG | S_IRWXO
#define SPACES 32
#define ABSOLUTE_PATH 4096

void fill_data(int tar_fd, char *path,struct stat *sb, int *flags);
int insert_special_int(char *where, size_t size, int32_t val);
int fill_header(Ustar_Header *header, struct stat *fstat,char *path,int *flags);
void checksum(unsigned char *buffer, int size, int *chksum);

void tar_create(int tar_fd, int *flags, char *path_names[], int total_path) {
    
    int i;
    uint8_t end_of_file_archieve[BLOCKSIZE * 2]; /* end of archieve*/
    struct stat psb; /* Used for lstat current directory's file */
    char abspath[ABSOLUTE_PATH];

    memset(end_of_file_archieve, '\0', BLOCKSIZE * 2);

    /* Get absolute path of current working directory (cwd) */
    if (!(getcwd(abspath, MAXPATH))) {
        perror("getcwd");
        exit(EXIT_FAILURE);
    }

    /* Loop through all the given paths and archieve into tar file in
     * tar_fd file descriptor that has been opened */
    for (i = 0; i < total_path; i++) {
        if (lstat(path_names[i], &psb) < 0) {
            /* When writing, skip files you can’t read,
             * but go on. (report, of course). If it's an actual
             * lstat error, perror and exit */
            if (errno == ENOENT) {
                fprintf(stderr, "%s: No such file or directory\n", 
                        path_names[i]);
                continue;
            }
            else {
                perror("lstat");
                exit(EXIT_FAILURE);
            }
        }
        else {
            /* Try to fill data */
            fill_data(tar_fd, path_names[i], &psb, flags);
        }

        /* Go back to original file before you check next path */
        if (chdir(abspath) < 0) {
            perror("chdir");
            exit(EXIT_FAILURE);
        }
    }

    /* write end of file which is 2, 512 blcok of int_8 of 0 integers */
    if (write(tar_fd, end_of_file_archieve, BLOCKSIZE * 2) < 0) {
        perror("write");
        exit(EXIT_FAILURE);
    } 

    /* close tar file */
    if (close(tar_fd) < 0) {
        perror("close");
        exit(EXIT_FAILURE);
    }
}

void fill_data(int tar_fd, char *path, struct stat *sb, int *flags) 
{
    struct stat currSB, temp;
    DIR *dir;
    struct dirent *dirent;
    Ustar_Header header;
    int check, child_fd, num_read;
    char newPath[MAXPATH], buffer[BLOCKSIZE];

    /* if path is greater than 256, then just return and don't store */
    if (strlen(path) > MAXPATH) {
        fprintf(stderr, "%s: unable to construct header. Skipping.\n", path);
        return; 
    }

    /* initialize buffer */
    memset(buffer, '\0', BLOCKSIZE);
    /* initialize newPath */
    memset(newPath, '\0', MAXPATH);
    strcpy(newPath, path); /* concatenate path to newpath */

    /* reset whole header of blocksize */
    memset(&header, '\0', BLOCKSIZE);
    /* lstat the path to get the inode information */
    if (lstat(path, &currSB) < 0) {
        perror("lstat");
        exit(EXIT_FAILURE);
    }
    
    if ((strchr(newPath, '/') == 0) && S_ISDIR(currSB.st_mode)) {
        strcat(newPath, "/");
    }
    /* fill up header and then write it */
    /* Name is greater than 100, don't store */
    check = fill_header(&header, &currSB, newPath, flags);
    if (check == -1) {
        return; 
    }
    if (write(tar_fd, &header, BLOCKSIZE) < 0) {
        perror("write");
        exit(EXIT_FAILURE);
    }

    if (S_ISDIR(currSB.st_mode)) {
        /* if newPath does not have a "/" add it. Why does this work? */
        if ((dir = opendir(path)) < 0) {
            perror("opendir");
            exit(EXIT_FAILURE);
        }

        while ((dirent = readdir(dir)) != NULL) {
            if (strchr(newPath, '/') == 0) {
                strcat(newPath, "/");
            }
            if (strcmp(dirent -> d_name, ".") && 
                    strcmp(dirent -> d_name, "..")) {
                /* concatenate new directory path to go into 
                 * and the do DFS on all the data through the directories */
                strcat(newPath, dirent -> d_name);
                if (lstat(newPath, &temp) < 0) {
                    perror("lstat");
                    exit(EXIT_FAILURE);
                }
                if (S_ISDIR(temp.st_mode)) {
                    strcat(newPath, "/");
                }
                /* recursive create DFS */
                fill_data(tar_fd, newPath, &currSB, flags);
            }
            /* reset to original path */
            memset(newPath, '\0', MAXPATH);
            strcpy(newPath, path);
        }
        /* close opened directory */
        if (closedir(dir) < 0) {
            perror("closedir");
            exit(EXIT_FAILURE);
        }
    }
    else if (S_ISREG(currSB.st_mode)) {
        if ((child_fd = open(path, O_RDONLY)) < 0) {
            perror("open");
            exit(EXIT_FAILURE);
        }
        /* read from the file and write to tar archieve file */
        while ((num_read = read(child_fd, buffer, BLOCKSIZE)) > 0) {
            if (write(tar_fd, buffer, BLOCKSIZE) < 0) {
                perror("write");
                exit(EXIT_FAILURE);
            }
            memset(buffer, '\0', BLOCKSIZE); /* reset buffer to '\0' */
        }
        if (num_read < 0) {
            perror("read");
            exit(EXIT_FAILURE);
        }
        if (close(child_fd) < 0) {
            perror("close");
            exit(EXIT_FAILURE);
        }
    }
}

int fill_header(Ustar_Header *header, struct stat *fstat, char *path, 
                int *flags) {
    /* Takes in a Ustar_Header struct and populate it with the
     * header information from the file given by sb and path_name */

    int i = 0, chksum = 0;
    char buffer[PREFIX_SIZE] = {'\0'};
    char *path_pointer = path;
    struct passwd *pwduid;
    struct group *grpid;
    /* check actual pathname and prefix. Prefix can only contain directory
     * files. While the actual name can be directories (if it can fit) 
     * and the file name. Have to check if the actual file name is greater
     * than 100 characters long. */

   /* If the file size is greater than alloted file size */
    if (strlen(path) > NAME_SIZE) {
        
        /* check if the path name size is greater than the alloted 
         * name + prefix + 1 for the / of directory */
        if (strlen(path) > NAME_SIZE + 1 + PREFIX_SIZE) {
            fprintf(stderr,"%s: unable to construct header. Skipping.\n", path);
            return -1; 
        }
        /* I'm going insane working on this assignment ;-; Please Thank ME
         * in the future, the future ME. I know you will do well, keep it 
         * up. */
        /* Note: only directories are stored in prefix. Any file name
         * that is greater than 100 bytes should be skipped over. 
         * Read up to '/' which is the end of a directory and store it into
         * prefix. If the rest can fit into name, we're done, if not, keep
         * on going until either you reached max threshold or it fits. */
        while (strlen(header -> prefix) < PREFIX_SIZE) {
            /* while prefix is not max continue until name can fit into 
             * name header */

            /* reset buffer so you can add onto prefix later and check 
             * reset prefix so you can add onto prefix from buffer */
            memset(buffer, '\0', PREFIX_SIZE);
            memset(header -> prefix, '\0', PREFIX_SIZE);
            path_pointer = strchr(path_pointer, '/');
            /* if there is no '/' meaning it's a regular file check 
             * check if it fits or not to the name. If it doesn't 
             * just error out */
            if (path_pointer == NULL) {
                if (strlen(path_pointer) > NAME_SIZE) {
                    fprintf(stderr, "%s: (Name too long?) Skipping.\n", path);
                    return -1;
                }
                break;
            }
            /* check to see if the directory fits the prefix_size, 
             * fits the prefix or not. If it doesn't error out. */
            if (path_pointer - path > PREFIX_SIZE) {
                /* Directory can't be longer than 155 */
                fprintf(stderr, "%s: (Name too long?) Skipping.\n", path);
                return -1;
            }

            /* copy the name all the way up to the point you found the
             * first '/' which the first directory */
            for (i = 0; i < path_pointer - path; i++) {
                buffer[i] = path[i]; 
            }
            /* store buffer into header */
            sprintf(header -> prefix, "%s", buffer);
            /* Not sure about this, but I think it's about storing
             * \0 byte at the end of the buffer if there is any? */
            header -> prefix[PREFIX_SIZE - 1] = buffer[PREFIX_SIZE - 1];
            /* increment file_pointer to check for the next occuring '/' 
             * if there is any (i.e. could be no more directories.)*/
            path_pointer++;

            /* check if current length is good, if it is, then get out of
             * this loop */
            if (strlen(path_pointer) <= NAME_SIZE) {
                break;             
            }
        }
    }
    /* Check if it's a directory without '/' at the end. If it is, 
     * check if it fits into name with a '/' or not */
    if (S_ISDIR(fstat -> st_mode)) {
        if (strlen(path) == NAME_SIZE && path[NAME_SIZE - 1] != '/') {
            /* store in prefix if it doesn't have a / and is 
             * equal to 100 bytes exactly. */
            sprintf(header -> prefix, "%s", path_pointer);
        }
        else {
            /* if it's less than size of name, then add it to name.
             * Check if you're able to add '/' at the end or not. 
             * If yes, add, else don't add. */
             sprintf(header -> name, "%s", path_pointer);
        }
    }
    else {
        /* it fits into name NICE! */
        sprintf(header -> name, "%s", path_pointer);
    }
    /* fill mode with the already existing modes from befre */
    sprintf(header -> mode, "%07o", (fstat -> st_mode) & (MODEMASK));
    /* Fill up UID (User ID of the file owner) */
    insert_special_int(header -> uid, UID_SIZE, fstat -> st_uid);
    /* Fill GID (User Group ID of the file owner) */
    sprintf(header -> gid, "%07o", fstat -> st_gid);
    /* Fill Size. If it's Directory or Symlink, set size as 0 */
    if (S_ISDIR(fstat -> st_mode) || S_ISLNK(fstat -> st_mode)) {
        sprintf(header -> size, "%011o", 0);
    } 
    else {
        sprintf(header -> size, "%011o", (int)fstat -> st_size);
    }
    /* Fill mtime (last modified time)*/
    sprintf(header -> mtime, "%011o", (int)fstat -> st_mtime);

    /* Set typeflag for header for regular files, regular files (alternate),
     * Directories, and symlinks f*/
    if (S_ISREG(fstat -> st_mode)) {
        header -> typeflag[0] = REGFILE; /* '0' */
    } 
    else if (S_ISFIFO(fstat -> st_mode)) {
        header -> typeflag[0] = AREGFILE; /* '\0'*/
    }
    else if (S_ISDIR(fstat -> st_mode)) {
        header -> typeflag[0] = DIRFILE; /* '5' */
    }
    else if (S_ISLNK(fstat -> st_mode)) {
        header -> typeflag[0] = SYMFILE; /* '2' */
    }
    /* fill link of header */
    readlink(path, header -> linkname, LINKNAME_SIZE);
    /* Fills up mytar */
    sprintf(header -> magic, "%s", MAGIC);
    /* Just copy to version "00"*/
    strcpy(header -> version, TVERSION);
    /* Grab uname from UID */
    if (!(pwduid = getpwuid(fstat -> st_uid))) {
        perror("getpwduid");
        exit(EXIT_FAILURE);
    }
    /* Store only 32 characters and if possible, NULL terminating into uname */
    strncpy(header -> uname, pwduid -> pw_name, UNAME_SIZE);
    /*grab gname from GID */
    if (!(grpid = getgrgid(fstat -> st_gid))) {
        perror("getgrpid");
        exit(EXIT_FAILURE);
    }
    /* Store only 32 characters, and if possible, NULL terminating into gname */
    strncpy(header -> gname, grpid -> gr_name, GNAME_SIZE);
    /* dev major and minor will empty or '\0' */
    /* check sum add everything in unsigned 8 bit order or by 1 byte each */

    for (i = 0; i < BLOCKSIZE; i++) {
        chksum += ((unsigned char *)(header))[i];
    }
    /* chksum acts as spaces by itself, so add up check sum with spaces */
    chksum += 8 * ' ';

    sprintf(header -> chksum, "%07o", chksum);
    /* printf("chksum of %s: %d\n", path, chksum); */

    if (flags[VFLAGPOS]) {
        printf("%s\n", path);
    }

    return 1; /* success */
}

int insert_special_int(char *where, size_t size, int32_t val) {
/* For interoperability with GNU tar. GNU seems to
 * * set the high–order bit of the first byte, then
 * * treat the rest of the field as a binary integer
 * * in network byte order.
 * * Insert the given integer into the given field
 * * using this technique. Returns 0 on success, nonzero
 * * otherwise
 * */
    int err=0;
    if (val < 0 || (size < sizeof(val))) {
    /* if it's negative, bit 31 is set and we can't use the flag
 *      * if len is too small, we can't write it. Either way, we're
 *           * done.
 **/
        err++;
    } else {
    /* game on... */
        memset(where, 0, size); /* Clear out the buffer */
        *(int32_t *)(where+size-sizeof(val)) = htonl(val); /*place the int */
        *where |= 0x80; /* set that high-order bit */
    }
    return err;
}
