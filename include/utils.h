#ifndef _SYSMON_UTILS_H
#define _SYSMON_UTILS_H

#include <sys/types.h>

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
#define SYSMON_TEST_SUCCESS "   " GRN "SUCCESS" RESET " "
#define SYSMON_TEST_FAIL    "   " RED "  FAIL " RESET " "

/**
 * open_and_read() - open file at @from and copy its content to @to
 * @from: source file path
 * @to: destination buffer
 * @maxlen: maximum capacity of the @to buffer
 *
 * open_and_read() returns count of characters copied, -1 if internal calls
 * failed.
 */
ssize_t open_and_read(const char *from, char *to, size_t maxlen);

#endif
