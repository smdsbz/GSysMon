#ifndef _SYSMON_PROCESS_H
#define _SYSMON_PROCESS_H

#include <string.h>

struct procstat {
    int             pid;
    char            comm[256];      // filename of the executable
    int             ppid;           // parent PID
    long            priority;       // negated, minus one
    long            nice;           // higher nice means lower priority
    long            num_threads;
    unsigned long   vsize;          // virtual memory size in bytes
    long            rss;            // resident set size: number of pages in
                                    // real memory
};
static inline void procstatcpy(struct procstat *dst,
                                const struct procstat *src) {
    memcpy(dst, src, sizeof(struct procstat));
    return;
}

/**
 * sysmon_get_procstat() - gets the status of a process by reading its
 *                         ``/proc/[pid]/stat``
 * @pid: the PID of the process
 * 
 * sysmon_get_procstat() returns a statically allocated procstat struct, or NULL
 * if operation failed.
 */
struct procstat *sysmon_get_procstat(int pid);

struct procnode {
    struct procstat     procstat;
    struct procnode    *prev;
    struct procnode    *next;
};
struct proclist {
    struct procnode    *head;
    struct procnode    *tail;
} __proclist;
int proclist_init(void);
void proclist_cleanup(void);

// TODO: proclist iterater family

/**
 * sysmon_process_refresh() - refreshed and maintain the proclist
 * @cb: a callback interface, for future use
 *
 * sysmon_process_refresh() returns pointer to the modules proclist struct on
 * success and NULL on failure.
 */
struct proclist *sysmon_process_refresh(void (*cb)(void));

/**
 * sysmon_process_load/unload() - module initialize and cleanup
 *
 * You MUST call this pair of functions on entering and leaving this module.
 */
static inline int sysmon_process_load(void) {
    return proclist_init();
}
static inline void sysmon_process_unload(void) {
    proclist_cleanup();
    return;
}

#endif

