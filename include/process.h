#ifndef _SYSMON_PROCESS_H
#define _SYSMON_PROCESS_H

#include <string.h>

struct procstat {
    int             pid;
    char            comm[256];      // filename of the executable
    char            state;          // process state
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
 * ``/proc/[pid]/stat``
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
};
int proclist_init(void);
void proclist_cleanup(void);

/**
 * proclist_del() - deletes a procnode struct from proclist
 * @proc: the procnode to be removed
 *
 * After calling proclist_del(), @proc will be free()-d thus no longer valid.
 *
 * Fields of proclist is maintained by this function.
 *
 * This interface is opened for later filter callback functions' development.
 * You may first call sysmon_process_refresh() to get a full list of running
 * processes, and then traverse the list and call this function to filter out
 * the unwanted ones.
 */
void proclist_del(struct procnode *procnode);

/**
 * proclist_begin/next/end() - proclist iterator family
 *
 * Call proclist iterator family to get a pointer to procstat struct.
 *
 * Typical usecase - traversing the list
 *
 *      for (struct procstat *stat = proclist_iter_begin();
 *              stat != proclist_iter_end();
 *              stat = proclist_iter_next()) {
 *          // use stat
 *          // modifications to stat is allowed but discouraged
 *      }
 *
 * The pointer returned CANNOT be free()-d.
 */
struct procstat *proclist_iter_begin(void);
struct procstat *proclist_iter_next(void);
static inline const struct procstat *proclist_iter_end(void) {
    return NULL;
}

/**
 * proclist_find_by_pid() - finds the procstat struct whose PID is @pid
 *
 * proclist_find_by_pid() returns pointer to procstat struct if exists,
 * otherwise returns NULL.
 *
 * The pointer returned CANNOT be free()-d.
 */
struct procstat *proclist_find_by_pid(int pid);

/**
 * sysmon_process_refresh() - refreshed and maintain the proclist
 * @cb: a callback interface, for future use
 * @cb_data: data to be passed to @cb
 *
 * sysmon_process_refresh() returns pointer to the modules proclist struct on
 * success and NULL on failure.
 */
struct proclist *sysmon_process_refresh(void (*cb)(struct proclist *, void *),
                                        void *cb_data);

/**
 * sysmon_process_load/unload() - module initialize and cleanup
 *
 * sysmon_process_load() returns 0 on success.
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

