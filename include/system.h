#ifndef _SYSMON_SYSTEM_H
#define _SYSMON_SYSTEM_H

/**
 * sysmon_get_hostname() - get hostname from ``/proc/sys/kernel/hostname``
 *
 * Returns a char pointer to a statically allocated string, DO NOT free()! If
 * the hostname is too long (longer than 255), NULL is returned.
 */
char *sysmon_get_hostname(void);

struct uptime {
  double    uptime;
  double    idle;
};
/**
 * sysmon_get_uptime() - get uptime from ``/proc/uptime``
 *
 * Returns two numbers in a statically allocacted uptime struct containing two
 * doubles, uptime and idle time (in seconds).
 */
struct uptime *sysmon_get_uptime(void);

#endif
