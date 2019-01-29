#ifndef _SYSMON_CPU_H
#define _SYSMON_CPU_H

#include <string.h>
#include <stdlib.h>

struct cpuinfo {
    unsigned    processor;          // processor id
    char        model_name[256];
    double      cpu_mhz;
    unsigned    core_id;            // physical id
};
static inline void cpuinfocpy(struct cpuinfo *dst,
                              const struct cpuinfo *src) {
    memcpy(dst, src, sizeof(struct cpuinfo));
    return;
}

/**
 * sysmon_get_cpuinfo() - get cpu info from ``/proc/cpuinfo``
 * @processor: processor ID
 *
 * Returns a pointer to a statically allocated cpuinfo struct, otherwise returns
 * NULL on error. If fails to read corresponding fields, they will stay zero-
 * initialized.
 */
struct cpuinfo *sysmon_get_cpuinfo(int processor);

/**
 * In order to make this module portable, the number of `cpuinfo`s has to vary,
 * therefore introducing cpusinfo_init/del().
 */
struct cpusinfo {
    unsigned        processor_count;
    struct cpuinfo *cpuinfos;
} __cpusinfo;
int cpusinfo_init(void);
void cpusinfo_del(void);

/**
 * sysmon_get_cpusinfo() - get infos for all logical processors
 *
 * sysmon_get_cpusinfo() returns a statically allocated cpusinfo struct, which
 * is a private member of this module, or NULL if failed.
 *
 * A leading sysmon_cpu_load() call and a subsequent sysmon_cpu_unload() call
 * are required!
 */
struct cpusinfo *sysmon_get_cpusinfo(void);

/**
 * sysmon_cpu_load/unload() - module initalize and cleanup
 *
 * You MUST call this two functions if you are using this module.
 */
static inline int sysmon_cpu_load(void) {
    return cpusinfo_init();
}
static inline void sysmon_cpu_unload(void) {
    cpusinfo_del();
    return;
}

#endif
