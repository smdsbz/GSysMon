#ifndef _SYSMON_SYSTEM_H
#define _SYSMON_SYSTEM_H

#include <sys/types.h>

/**
 * sysmon_get_hostname() - get hostname from ``/proc/sys/kernel/hostname``
 *
 * Returns a char pointer to a statically allocated string, DO NOT free()! If
 * the hostname is too long (longer than 255), NULL is returned.
 */
char *sysmon_get_hostname(void);

struct uptime {
  double    uptime;     // in seconds
  double    idle;       // summed idle time in seconds
};
/**
 * sysmon_get_uptime() - get uptime from ``/proc/uptime``
 *
 * Returns two numbers in a statically allocacted uptime struct containing two
 * doubles, uptime and idle time (in seconds).
 */
struct uptime *sysmon_get_uptime(void);

/**
 * [DEPRECATED: use sysmon_get_boottime() instead]
 * sysmon_convert_uptime_to_boottime() - converts @uptime to system boot time
 * @uptime: system uptime
 *
 * Returns a statically allocated string on success, else returns NULL.
 */
/* char *sysmon_convert_uptime_to_boottime(double uptime); */

/**
 * sysmon_get_boottime() - get boot time from ``/proc/stat``
 *
 * Returns boot time since 1970.01.01 0000 (UTC), if error, returns 0.
 */
size_t sysmon_get_boottime(void);

/**
 * sysmon_get_system_version() - get system version info from
 * ``/proc/sys/kernel/osrelease``
 *
 * Returns a statically allocated string on success, otherwise returns NULL.
 */
char *sysmon_get_system_version(void);

#endif
