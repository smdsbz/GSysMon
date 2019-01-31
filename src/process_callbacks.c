#include <stdio.h>
#include <string.h>

#include "../include/utils.h"
#include "../include/process.h"
#include "../include/process_callbacks.h"


void proclist_filter_by_pid_leading(struct proclist *proclist, void *pid_lead) {
    struct procnode *node = proclist->head, *todel = NULL;
    char pid[32];
    while (node != NULL) {
        sprintf(pid, "%d", node->procstat.pid);
        if (!str_starts_with(pid, pid_lead)) {
            todel = node;
        }
        node = node->next;
        if (todel != NULL) {
            proclist_del(todel);
            todel = NULL;
        }
    }
    return;
}

void proclist_filter_by_name_fuzzy(struct proclist *proclist, void *name) {
    struct procnode *node = proclist->head, *todel = NULL;
    while (node != NULL) {
        if (!strstr_fuzzy(node->procstat.comm, name)) {
            todel = node;
        }
        node = node->next;
        if (todel != NULL) {
            proclist_del(todel);
            todel = NULL;
        }
    }
    return;
}


/******* Test *******/

#if SYSMON_PROCESS_CALLBACKS_TEST
int main(const int argc, const char **argv) {
    sysmon_process_load();

    printf("Testing callback %s:\n", "filter_by_pid_leading");
    if (sysmon_process_refresh(proclist_filter_by_pid_leading, "16") == NULL) {
        puts(SYSMON_TEST_FAIL);
        return 0;
    }
    puts(SYSMON_TEST_SUCCESS ": listing results ...");
    for (struct procstat *stat = proclist_iter_begin();
            stat != proclist_iter_end();
            stat = proclist_iter_next()) {
        printf(SYSMON_TEST_ESCAPE "%s (%d)\n", stat->comm, stat->pid);
    }

    printf("Testing callback %s:\n", "filter_by_name_fuzzy");
    if (sysmon_process_refresh(proclist_filter_by_name_fuzzy, "crme") == NULL) {
        puts(SYSMON_TEST_FAIL);
        return 0;
    }
    puts(SYSMON_TEST_SUCCESS ": listing results ...");
    for (struct procstat *stat = proclist_iter_begin();
            stat != proclist_iter_end();
            stat = proclist_iter_next()) {
        printf(SYSMON_TEST_ESCAPE "%s (%d)\n", stat->comm, stat->pid);
    }

    sysmon_process_unload();
    return 0;
}
#endif

