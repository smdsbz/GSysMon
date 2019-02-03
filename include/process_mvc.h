#ifndef _SYSMON_PROCESS_MVC_H
#define _SYSMON_PROCESS_MVC_H

#include <gtk/gtk.h>

#include "process.h"

enum {
    COL_PID, COL_NAME, COL_STATE, COL_PPID, COL_NICE, COL_THREADS,
    COL_VMEM, COL_RMEM, NCOLS
};

int gsysmon_process_mvc_sync(GtkListStore *ls, struct proclist *proclist);

/**
 * gsysmon_process_mvc_refresh() - refreshes process list in GUI
 * @ls: the GtkListStore object to be refreshed
 * @cb
 * @cb_data
 *
 * gsysmon_process_mvc_refresh() returns 0 on success and -1 on failure.
 */
static inline int gsysmon_process_mvc_refresh(
        GtkListStore *ls,
        void (*cb)(struct proclist *, void *),
        void *cb_data) {
    return gsysmon_process_mvc_sync(ls, sysmon_process_refresh(cb, cb_data));
}

/**
 * Wraps the process module load and unload.
 */
static inline int gsysmon_process_mvc_load(void) {
    return sysmon_process_load();
}
static inline void gsysmon_process_mvc_unload(void) {
    sysmon_process_unload();
    return;
}

#endif
