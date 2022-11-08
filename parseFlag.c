#include "parseFlag.h"
#include <stdio.h>
#include <stdlib.h>

/* parse the flag and check */
void parse_flags(char *flagArg, int *flag_store) {
    int flagPos = 0;

    while (*flagArg) {
        switch(*flagArg) {
            case 'c':
                if (!flag_store[CFLAGPOS]) {
                    flag_store[CFLAGPOS]++;
                    flagPos++;
                }
                else {
                    flagError();
                }
                flagArg++;
                break;
            case 't':
                if (!flag_store[TFLAGPOS]) {
                    flag_store[TFLAGPOS]++;
                    flagPos++;
                }
                else {
                    flagError();
                }
                flagArg++;
                break;
            case 'x':
                if (!flag_store[XFLAGPOS]) {
                    flag_store[XFLAGPOS]++;
                    flagPos++;
                }
                else {
                    flagError();
                }
                flagArg++;
                break;
            case 'v':
                if (!flag_store[VFLAGPOS]) {
                    flag_store[VFLAGPOS]++;
                    flagPos++;
                }
                else {
                    flagError();
                }
                flagArg++;
                break;
            case 'S':
                if (!flag_store[SFLAGPOS]) {
                    flag_store[SFLAGPOS]++;
                    flagPos++;
                }
                else {
                    flagError();
                }
                flagArg++;
                break;
            case 'f':
                if (!flag_store[FFLAGPOS]) {
                    flag_store[FFLAGPOS]++;
                    flagPos++;
                }
                else {
                    flagError();
                }
                flagArg++;
                break;
            default:
                usageError();
        }
    }
    if (!flagPos) {
        usageError();
    }
}

/* Print the usage error out if command line is wrong */
void usageError() {
    fprintf(stderr, "usage: mytar [ctxvS]f tarfile [ path [ ... ] ]\n");
    exit(EXIT_FAILURE);
}

/* Print flag error if they input more than one of each flag*/
void flagError() {
    fprintf(stderr, "Invalid amount of flag [ctxvS]f\n");
    exit(EXIT_FAILURE);
}

