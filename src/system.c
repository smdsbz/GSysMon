#include <stdio.h>
#if SYSMON_SYSTEM_TEST
#include <stdlib.h>
#endif

#include "../include/utils.h"
#include "../include/system.h"


char *sysmon_get_hostname(void) {
    static char hostname[256];
    int retval;
    if ((retval = open_and_read("/proc/sys/kernel/hostname", hostname, 256)) == -1) {
        return NULL;
    }
    // remove trailing '\n'
    --retval;
    hostname[retval] = '\0';
    return hostname;
}

struct uptime *sysmon_get_uptime(void) {
    static struct uptime uptime;
    static char buf[64];
    int retval;
    if ((retval = open_and_read("/proc/uptime", buf, 64)) == -1) {
        return NULL;
    }
    if (sscanf(buf, "%lf %lf", &uptime.uptime, &uptime.idle) != 2) {
        return NULL;
    }
    return &uptime;
}


/******* Tests *******/

#if SYSMON_SYSTEM_TEST
int main(const int argc, const char **argv) {

    printf("Testing %s():\n", "sysmon_get_hostname");
    char *hostname = sysmon_get_hostname();
    if (hostname) {
        printf(SYSMON_TEST_SUCCESS ": %s\n", hostname);
    } else {
        printf(SYSMON_TEST_FAIL ": got %p\n", hostname);
    }

    printf("Testing %s():\n", "sysmon_get_uptime");
    struct uptime *uptime = sysmon_get_uptime();
    if (uptime) {
        printf(SYSMON_TEST_SUCCESS ": uptime: %.3f, idle: %.3f\n", uptime->uptime, uptime->idle);
    } else {
        printf(SYSMON_TEST_FAIL ": got %p\n", uptime);
    }

    return EXIT_SUCCESS;
}
#endif

