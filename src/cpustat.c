#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "../include/utils.h"
#include "../include/cpustat.h"


struct single_cpustat *sysmon_get_cpustat(int id) {     // {{{
    static const char statfmt[] = (     // {{{
        "%*s "      // cpu name (cpu | cpuN)
        "%lu "      // user
        "%lu "      // nice
        "%lu "      // system
        "%lu "      // idle
        "%lu "      // iowait
        "%lu "      // irq
        "%lu "      // softirq
        "%lu "      // steal
        /* "%lu "      // guest */
        /* "%lu "      // guest_nice */
    );      // }}}
    static struct single_cpustat stat;
    FILE *fp;
    char buf[256];
    char cpu_pref[16] = "cpu ";
    int retval;
    stat.id = id;
    // get line leading identifier
    if (id >= 0) {
        sprintf(cpu_pref, "cpu%d ", id);
    }
    if ((fp = fopen("/proc/stat", "r")) == NULL) {
        perror("fopen");
        return NULL;
    }
    // find and read
    int found = 0;
    while (freadline(fp, buf, 256) == 1) {
        if (!str_starts_with(buf, cpu_pref)) {
            continue;
        }
        retval = sscanf(buf, statfmt, &stat.user, &stat.nice, &stat.system,
                        &stat.idle, &stat.iowait, &stat.irq, &stat.softirq,
                        &stat.steal);
        if (retval == 8) {
            cpustat_set_total(&stat);
            found = 1;
        }
        break;
    }
    fclose(fp);
    if (!found) {
        return NULL;
    }
    return &stat;
}   // }}}

static double __interval = 1.0;     // in seconds


/******* Test *******/

#if SYSMON_CPUSTAT_TEST
int main(const int argc, const char **argv) {

    printf("Testing %s():\n", "sysmon_get_cpustat");
    struct single_cpustat *stat = sysmon_get_cpustat(2);
    printf(SYSMON_TEST_SUCCESS ": id: %d\n", stat->id);
    printf(SYSMON_TEST_ESCAPE "total: %lu\n", stat->total);
    printf(SYSMON_TEST_ESCAPE "user: %lu\n", stat->user);
    printf(SYSMON_TEST_ESCAPE "nice: %lu\n", stat->nice);
    printf(SYSMON_TEST_ESCAPE "system: %lu\n", stat->system);
    printf(SYSMON_TEST_ESCAPE "idle: %lu\n", stat->idle);
    printf(SYSMON_TEST_ESCAPE "iowait: %lu\n", stat->iowait);
    printf(SYSMON_TEST_ESCAPE "irq: %lu\n", stat->irq);
    printf(SYSMON_TEST_ESCAPE "softirq: %lu\n", stat->softirq);
    printf(SYSMON_TEST_ESCAPE "steal: %lu\n", stat->steal);

    return 0;
}
#endif
