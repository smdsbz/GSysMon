#ifndef _SYSMON_CPUSTAT_H
#define _SYSMON_CPUSTAT_H

#include <sys/types.h>
#include <unistd.h>
#include <string.h>

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
static inline void single_cpustat_cpy(struct single_cpustat *dst,
                                      const struct single_cpustat *src) {
    memcpy(dst, src, sizeof(struct single_cpustat));
    return;
}

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

/**
 * sysmon_get_cpustat_all() - gets cpustat for all cpus
 *
 * sysmon_get_cpustat_all() returns a statically allocated cpustat struct on
 * success, NULL on error.
 *
 * You may NOT want to explicitly call this while a cpustat daemon is up and
 * running, see cpustat_get_diff() bug.
 */
struct cpustat *sysmon_get_cpustat_all(void);

/**
 * cpustatcpy() - copy cpustat struct from @src to @dst
 * @dst
 * @src
 *
 * cpustatcpy() returns @dst on success, NULL on error.
 *
 * Since the `ncpus` field may not be the same, and `each` field of @dst may
 * not be given, memory allocations may happen.
 */
struct cpustat *cpustatcpy(struct cpustat *dst, const struct cpustat *src);

/**
 * cpustat_del() - frees @cpustat
 * @cpustat
 */
void cpustat_del(struct cpustat *cpustat);

/**
 * cpustat_get_diff() - get difference between the current cpustat and the last
 *
 * cpustat_get_diff() internally uses the module's private cpustat struct, if
 * it hasn't been given values, NULL will return; otherwise, it returns a
 * statically allocated cpustat struct containing the diffs.
 *
 * If other calls tempting to get cpustat on all cpus, namely
 * sysmon_get_cpustat_all(), happen between two cpustat_get_diff() calls, the
 * module's private cpustat sturct will be refreshed, causing total time of the
 * diff output to be smaller, thus yielding less accurate calculated percentage
 * value. It's a structure design bug.
 */
struct cpustat *sysmon_get_cpustat_diff(void);

/**
 * cpustat_get_*_percentage() - percentage calculation helper family
 * @diff: diff result
 * @cpuid: the target cpu, -1 to calculate on all
 *
 * Returns percentage values in range [0.0, 100.0].
 */
double cpustat_get_user_percentage(const struct cpustat *diff, int cpuid);
double cpustat_get_kernel_percentage(const struct cpustat *diff, int cpuid);
double cpustat_get_idle_percentage(const struct cpustat *diff, int cpuid);
double cpustat_get_usage_percentage(const struct cpustat *diff, int cpuid);

static inline int sysmon_cpustat_load(void) {
    return cpustat_init();
}
static inline void sysmon_cpustat_unload(void) {
    cpustat_cleanup();
    return;
}

#endif
