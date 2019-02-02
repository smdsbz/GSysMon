#include <stdio.h>
#include <string.h>
#include <ctype.h>

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
    size_t len = strlen(to); 
    if (len == 0) {
        return 0;
    }
    if (len == bufsiz - 1) {
        return -2;
    }
    return 1;
}

int strstr_fuzzy(const char *s1, const char *s2) {
    const char *ps1 = s1, *ps2 = s2;
    for (; *ps1 != '\0' && *ps2 != '\0'; ++ps1) {
        if (*ps1 == *ps2) {
            ++ps2;
        }
    }
    return (*ps2 == '\0');
}

void lstrip(char *s) {
    // find first non-space character
    size_t nospace = 0;
    for (; s[nospace] != '\0' && isspace(s[nospace]); ++nospace) ;
    // now s[nospace] is either '\0' or the first non-space char
    // move forward
    size_t idx = nospace;
    for (; s[idx] != '\0'; ++idx) {
        s[idx - nospace] = s[idx];
    }
    s[idx - nospace] = '\0';
    return;
}

void rstrip(char *s) {
    ssize_t nospace = strlen(s) - 1;
    for (; nospace >= 0 && isspace(s[nospace]); --nospace) ;
    s[nospace + 1] = '\0';
    return;
}

char *get_human_from_bytes(size_t b) {
    static char str[256];
    char unit[3] = { 'B', '\0', '\0' };
    double disp = b;
    if (disp / 1024.0 > 1.0) {
        disp /= 1024.0;
        unit[0] = 'K';
        unit[1] = 'B';
    }
    if (disp / 1024.0 > 1.0) {
        disp /= 1024.0;
        unit[0] = 'M';
    }
    if (disp / 1024.0 > 1.0) {
        disp /= 1024.0;
        unit[0] = 'G';
    }
    sprintf(str, "%.2lf%s", disp, unit);
    return str;
}

