#include <stdio.h>
#include <string.h>
#if SYSMON_SYSTEM_TEST
#include <stdlib.h>
#endif

#include "../include/utils.h"
#include "../include/process.h"


/******* struct procstat Helpers *******/

static int procstat_fill(struct procstat *procstat) {
    return 0;
}

/******* struct proclist Helpers *******/

static struct procnode *proclist_add(int pid);

static inline int proclist_fill(struct procnode *proc) {
    return procstat_fill(proc->procstat);
}

static void proclist_del(struct procnode *proc);

