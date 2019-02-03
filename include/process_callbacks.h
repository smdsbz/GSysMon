#ifndef _SYSMON_PROCESS_CALLBACKS_H
#define _SYSMON_PROCESS_CALLBACKS_H

#include "../include/process.h"     // struct proclist

/* typedef void (*proclist_callback_t)(struct proclist *proclist, void *data); */

/**
 * proclist_filter_by_pid_leading() - filters proclist, leaving only procnodes
 * whose PIDs start with @pid_lead
 * @proclist: (process module private member)
 * @pid_lead: a c-string contains leading PID ascii chars
 */
void proclist_filter_by_pid_leading(struct proclist *proclist, void *pid_lead);

/**
 * proclist_filter_by_name_fuzzy() - filters proclist, leaving only procnodes
 * whose comm contains all chars presented in @name, but may not be consecutive
 */
void proclist_filter_by_name_fuzzy(struct proclist *proclist, void *name);

#endif
