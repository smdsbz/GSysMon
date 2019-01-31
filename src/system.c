#include <stdio.h>
#include <string.h>
#include <time.h>

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
    char buf[64];
    int retval;
    if ((retval = open_and_read("/proc/uptime", buf, 64)) == -1) {
        return NULL;
    }
    if (sscanf(buf, "%lf %lf", &uptime.uptime, &uptime.idle) != 2) {
        return NULL;
    }
    return &uptime;
}

char *sysmon_convert_uptime_to_boottime(double uptime) {
    time_t boottime = time(NULL) - (ssize_t)uptime;
    char *timestr = ctime(&boottime);
    if (timestr == NULL) {
        perror("ctime");
        return NULL;
    }
    remove_trailing_newline(timestr);
    return timestr;
}

size_t sysmon_get_boottime(void) {
    size_t boottime;
    char line[1024];
    FILE *fp;
    if ((fp = fopen("/proc/stat", "r")) == NULL) {
        perror("fopen");
        return 0;
    }
    while (freadline(fp, line, 1024) != -1) {
        if (sscanf(line, "btime %lu", &boottime) == 1) {
            break;
        }
    }
    fclose(fp);
    return boottime;
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

    /* printf("Testing %s():\n", "sysmon_convert_uptime_to_boottime"); */
    /* char *strboottime = sysmon_convert_uptime_to_boottime(uptime->uptime); */
    /* if (strboottime) { */
    /*     printf(SYSMON_TEST_SUCCESS ": %s\n", strboottime); */
    /* } else { */
    /*     printf(SYSMON_TEST_FAIL ": got %p\n", strboottime); */
    /* } */

    printf("Testing %s():\n", "sysmon_get_boottime");
    size_t btime = sysmon_get_boottime();
    if (btime) {
        char *boottime = ctime(&btime);
        remove_trailing_newline(boottime);
        printf(SYSMON_TEST_SUCCESS ": %lu (%s)\n", btime, boottime);
    } else {
        printf(SYSMON_TEST_FAIL ": got %lu\n", btime);
    }

    printf("Testing %s():\n", "sysmon_get_system_version");
    char *sysver = sysmon_get_system_version();
    if (sysver) {
        printf(SYSMON_TEST_SUCCESS ": %s\n", sysver);
    } else {
        printf(SYSMON_TEST_FAIL ": got %p\n", sysver);
    }

    return 0;
}
#endif

