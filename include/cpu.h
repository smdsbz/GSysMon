#ifndef _SYSMON_CPU_H
#define _SYSMON_CPU_H

struct cpuinfo {
    char    model_name[256];
    double  cpu_mhz;
};
/**
 * sysmon_get_cpuinfo() - get cpu info from ``/proc/cpuinfo``
 *
 * Returns a pointer to a statically allocated cpuinfo struct, otherwise returns
 * NULL on error. If fails to read corresponding fields, they will stay zero-
 * initialized.
 */
struct cpuinfo *sysmon_get_cpuinfo(void);

#endif
