#include <stdio.h>
#include <string.h>

#include "../include/utils.h"


ssize_t open_and_read(const char *from, char *to, size_t bufsiz) {
    int retval;
    FILE *fp;
    // open and read from file
    if ((fp = fopen(from, "r")) == NULL) {
        perror("open_and_read/fopen");
        return -1;
    }
    if ((retval = fread(to, sizeof(char), bufsiz - 1, fp)) == 0) {
        printf("%s\n", "open_and_read/fread: Nothing can be read!");
        fclose(fp);
        return -1;
    }
    fclose(fp);
    to[retval] = '\0';
    if (retval == bufsiz - 1) {
        return -1;
    }
    return retval;
}

int freadline(FILE *fp, char *to, size_t bufsiz) {
    static char fmt[32];
    // NOTE: scanf-family adds '\0' to output string, therefore using bufsiz - 1
    sprintf(fmt, "%%%lu[^\n]\n", bufsiz - 1);
    int retval = fscanf(fp, fmt, to);
    if (retval != 1) {
        return -1;
    }
    ssize_t len = strlen(to); 
    if (len == 0) {
        return 0;
    }
    if (len == bufsiz - 1) {
        return -2;
    }
    return 1;
}

