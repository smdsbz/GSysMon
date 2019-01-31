#ifndef _SYSMON_CPUSTAT_H
#define _SYSMON_CPUSTAT_H

#include <unistd.h>
#include <sys/types.h>

static inline size_t USER_HZ(void) {
    return (size_t)sysconf(_SC_CLK_TCK);
}

struct single_cpustat {
    int         id;
    size_t      total;
    size_t      user;
    size_t      nice;
    size_t      system;
    size_t      idle;
    size_t      iowait;
    size_t      irq;
    size_t      softirq;
    size_t      steal;
    /* size_t      guest; */
    /* size_t      guest_nice; */
};

/**
 * cpustat_set_total() - calculates and sets the total field of a single_cpustat
 * struct
 * @stat: pointer to the single_cpustat struct
 *
 * cpustat_set_total() calculates the total uptime of the cpu given times in
 * @stat, assigns the calculated value to @stat's `total` field, and returns
 * that value, in case it is needed immediately.
 */
static inline size_t cpustat_set_total(struct single_cpustat *stat) {
    stat->total = (
        stat->user + stat->nice + stat->system + stat->idle + stat->iowait
        + stat->irq + stat->softirq + stat->steal
    );
    return stat->total;
}

/**
 * sysmon_get_cpustat() - gets cpu statistics from ``/proc/stat``
 * @cpu_id: the ID of the cpu querying, if passed -1, gets total stats
 *
 * sysmon_get_cpustat() returns a statically allocated single_cpustat struct on
 * success, or NULL on failure.
 */
struct single_cpustat *sysmon_get_cpustat(int cpu_id);

struct cpustat {
    struct single_cpustat   all;
    size_t                  ncpus;
    struct single_cpustat  *each;
};
int cpustat_init(void);
void cpustat_cleanup(void);


int sysmon_cpustat_load();
void sysmon_cpustat_unload();

#endif
