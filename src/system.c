#include <stdio.h>
#include <string.h>
#include <time.h>
#if SYSMON_SYSTEM_TEST
#include <stdlib.h>
#endif

#include "../include/utils.h"
#include "../include/system.h"


char *sysmon_get_hostname(void) {
    static char hostname[256];
    int retval;
    if ((retval = open_and_read("/proc/sys/kernel/hostname",
                                hostname, 256)) == -1) {
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

char *sysmon_convert_uptime_to_str(double uptime) {
    time_t boottime = time(NULL) - (ssize_t)uptime;
    char *timestr = ctime(&boottime);
    if (timestr == NULL) {
        perror("ctime");
        return NULL;
    }
    remove_trailing_newline(timestr);
    return timestr;
}

char *sysmon_get_system_version(void) {
    static char sysver[256];
    char ostype[32];
    char osrelease[128];
    if (open_and_read("/proc/sys/kernel/ostype", ostype, 32) == -1) {
        return NULL;
    }
    remove_trailing_newline(ostype);
    if (open_and_read("/proc/sys/kernel/osrelease", osrelease, 128) == -1) {
        return NULL;
    }
    remove_trailing_newline(osrelease);
    sprintf(sysver, "%s %s", ostype, osrelease);
    return sysver;
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

    printf("Testing %s():\n", "sysmon_convert_uptime_to_str");
    char *boottime = sysmon_convert_uptime_to_str(uptime->uptime);
    if (boottime) {
        printf(SYSMON_TEST_SUCCESS ": %s\n", boottime);
    } else {
        printf(SYSMON_TEST_FAIL ": got %p\n", boottime);
    }

    printf("Testing %s():\n", "sysmon_get_system_version");
    char *sysver = sysmon_get_system_version();
    if (sysver) {
        printf(SYSMON_TEST_SUCCESS ": %s\n", sysver);
    } else {
        printf(SYSMON_TEST_FAIL ": got %p\n", sysver);
    }

    return EXIT_SUCCESS;
}
#endif

