#include <stdio.h>

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

