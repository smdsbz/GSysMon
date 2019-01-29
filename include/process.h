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

struct procstat *sysmon_get_procstat(int pid);

struct procnode {
    struct procstat    *procstat;
    struct procnode    *prev;
    struct procnode    *next;
};
struct proclist {
    size_t              count;
    struct procnode    *head;
    struct procnode    *tail;
} __proclist;
int proclist_init(void);
void proclist_cleanup(void);

struct proclist *sysmon_process_refresh(void);

static inline int sysmon_process_load(void) {
    return proclist_init();
}
static inline void sysmon_process_unload(void) {
    proclist_cleanup();
    return;
}


#endif

