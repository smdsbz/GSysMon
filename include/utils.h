#ifndef _SYSMON_UTILS_H
#define _SYSMON_UTILS_H

#include <stdio.h>
#include <sys/types.h>
#include <string.h>

/**
 * some printf-able colors for prettier debugging
 */
#define RED   "\x1B[31m"
#define GRN   "\x1B[32m"
#define YEL   "\x1B[33m"
#define BLU   "\x1B[34m"
#define MAG   "\x1B[35m"
#define CYN   "\x1B[36m"
#define WHT   "\x1B[37m"
#define RESET "\x1B[0m"
#define SYSMON_TEST_SUCCESS "   " GRN "SUCCESS" RESET
#define SYSMON_TEST_FAIL    "   " RED "   FAIL" RESET
#define SYSMON_TEST_ESCAPE  "   "     "       "       "  "

/**
 * open_and_read() - open file at @from and copy its content to @to
 * @from: source file path
 * @to: destination buffer
 * @bufsiz: maximum capacity of the @to buffer
 *
 * open_and_read() returns count of characters copied, -1 if internal calls
 * failed or read content has filled buffer, which will be bufsiz - 1 characters
 * from the file @from and a null character. On the later case, a truncated
 * version will be left in @to.
 */
ssize_t open_and_read(const char *from, char *to, size_t bufsiz);

/**
 * freadline() - reads out a line from file @fp
 * @fp: FILE pointer
 * @to: destination buffer
 * @bufsiz: maximum capacity of the @to buffer
 *
 * freadline() returns
 *       1  on success
 *       0  on reading an empty line
 *      -1  on no match, i.e. end of file
 *      -2  on buffer too small, but a truncated version will be saved in @to
 */
int freadline(FILE *fp, char *to, size_t bufsiz);

/**
 * remove_trailing_newline() - removes trailing newline of a '\n\0' terminated
 * C-string
 * @s: the string to be modified
 */
static inline void remove_trailing_newline(char *s) {
    s[strlen(s) - 1] = '\0';
    return;
}

/**
 * strequ() - compares two string
 * @s1: the first string
 * @s2: the second string
 *
 * strequ() returns 1 if @s1 and @s2 do equal, otherwise returns 0.
 */
static inline int strequ(const char *s1, const char *s2) {
    return !(strcmp(s1, s2));
}

#endif
