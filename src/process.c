#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/types.h>
#include <dirent.h>

#include "../include/utils.h"
#include "../include/process.h"


/******* Useful Constants *******/

const static size_t B_PER_KB = 1024;
const static size_t B_PER_MB = 1024 * 1024;
const static size_t B_PER_GB = 1024 * 1024 * 1024;

/******* struct procstat Helpers *******/

/**
 * procstat_fill() - fills a procstat struct with info read from
 *                   ``/proc/[pid]/stat``
 * @procstat: a struct procstat whose `pid` field is given
 *
 * procstat_fill() returns 0 on success, -1 on failure.
 *
 * The `pid` field of the procstat must be given.
 */
static int procstat_fill(struct procstat *procstat) {   // {{{
    const static char statfmt[] = (
        /* "%*d "          //  1. pid */
        /* // NOTE: the comm field may itself contain parenthesis!!! */
        /* "(%255[^)] "    //  2. comm */
        "%*c "          //  3. state
        "%d "           //  4. ppid
        "%*d "          //  5. pgrp
        "%*d "          //  6. session
        "%*d "          //  7. tty_nr
        "%*d "          //  8. tpgid
        "%*u "          //  9. flags
        "%*u "          // 10. minflt
        "%*u "          // 11. cminflt
        "%*u "          // 12. majflt
        "%*u "          // 13. cmajflt
        "%*u "          // 14. utime
        "%*u "          // 15. stime
        "%*d "          // 16. cutime
        "%*d "          // 17. cstime
        "%ld "          // 18. priority
        "%ld "          // 19. nice
        "%ld "          // 20. num_threads
        "%*d "          // 21. itrealvalue
        "%*u "          // 22. starttime
        "%lu "          // 23. vsize
        "%ld "          // 24. rss
                        //     etc.
    );
    FILE *fp;
    char path[512];
    int retval;
    sprintf(path, "/proc/%d/stat", procstat->pid);
    if ((fp = fopen(path, "r")) == NULL) {
        perror("fopen");
        return -1;
    }
    // extract comm field
    fscanf(fp, "%*d (");
    int paren_level = 1;
    char *dstp = procstat->comm;
    for (; paren_level != 0; ++dstp) {
        *dstp = fgetc(fp);
        if (*dstp == '(') {
            ++paren_level;
        } else if (*dstp == ')') {
            --paren_level;
        }
    }
    *(--dstp) = '\0';
    fgetc(fp);      // consumes a space
    // extract other fields
    retval = fscanf(fp, statfmt, &procstat->ppid, &procstat->priority,
                    &procstat->nice, &procstat->num_threads, &procstat->vsize,
                    &procstat->rss);
    if (retval != 6) {
        fclose(fp);
        return -1;
    }
    fclose(fp);
    return 0;
}   // }}}

/******* struct proclist Helpers *******/

static inline int procnode_fill(struct procnode *proc) {
    return procstat_fill(&proc->procstat);
}

/**
 * proclist_append() - adds a record to proclist
 * @pid: the pid of the process to be added
 *
 * proclist_append() returns a pointer to the newly added procnode on success,
 * returns NULL on failure, e.g. no process identified by @pid, and no procnode
 * will be added.
 *
 * Fields of proclist is maintained by this function.
 */
static struct procnode *proclist_append(int pid) {      // {{{
    struct procnode *procnode = malloc(sizeof(struct procnode));
    if (procnode == NULL) {
        perror("malloc");
        return NULL;
    }
    memset(procnode, 0, sizeof(struct procnode));
    // get stat
    procnode->procstat.pid = pid;
    if (procnode_fill(procnode) != 0) {
        printf("procnode_fill() with pid = %d failed\n", procnode->procstat.pid);
        free(procnode);
        return NULL;
    }
    // append to proclist
    if (__proclist.head == NULL) {
        __proclist.head = procnode;
    } else {
        procnode->prev = __proclist.tail;
        __proclist.tail->next = procnode;
    }
    __proclist.tail = procnode;
    return procnode;
}   // }}}

/**
 * proclist_del() - deletes a procnode struct from proclist
 * @proc: the procnode to be removed
 *
 * After calling proclist_del(), @proc will be free()-d and no longer valid.
 *
 * Fields of proclist is maintained by this function.
 */
static void proclist_del(struct procnode *proc) {   // {{{
    if (proc->next != NULL) {
        proc->next->prev = proc->prev;
    }
    if (proc->prev != NULL) {
        proc->prev->next = proc->next;
    }
    if (proc == __proclist.head) {
        __proclist.head = proc->next;
    }
    if (proc == __proclist.tail) {
        __proclist.tail = proc->prev;
    }
    free(proc);
    return;
}   // }}}

/******* sysmon_process_refresh() Helpers *******/

static int isprocdir(struct dirent *entry) {     // {{{
    if (entry->d_type != DT_DIR) {
        return 0;
    }
    size_t ch = 0;
    for (; entry->d_name[ch] != '\0' && isdigit(entry->d_name[ch]); ++ch) ;
    return (entry->d_name[ch] == '\0');
}   // }}}

struct proclist *sysmon_process_hard_refresh(void) {    // {{{
    DIR *procdp;
    struct dirent *entry;
    proclist_cleanup();
    if ((procdp = opendir("/proc")) == NULL) {
        perror("opendir");
        return NULL;
    }
    while ((entry = readdir(procdp)) != NULL) {
        if (!isprocdir(entry)) {
            continue;
        }
        if (proclist_append(strtod(entry->d_name, NULL)) == NULL) {
            return NULL;
        }
    }
    closedir(procdp);
    return &__proclist;
}   // }}}

static struct procstat *proclist_find_pid(int pid) {    // {{{
    struct procstat *procstat = proclist_iter_begin();
    for (; procstat != proclist_iter_end(); procstat = proclist_iter_next()) {
        if (procstat->pid == pid) {
            return procstat;
        }
    }
    return NULL;
}   // }}}

/**
 * sysmon_process_refresh_inc() - incremental refresh
 *
 * sysmon_process_refresh_inc() returns NULL on interal call failure.
 *
 * sysmon_process_refresh_inc() traverses the ``/proc`` directory, adds a node
 * into proclist if not present. Note this function does NOT refresh existing
 * nodes.
 */
static struct proclist *sysmon_process_refresh_inc(void) {      // {{{
    DIR *procdp;
    struct dirent *entry;
    if ((procdp = opendir("/proc")) == NULL) {
        perror("opendir");
        return NULL;
    }
    while ((entry = readdir(procdp)) != NULL) {
        if (!isprocdir(entry)) {
            continue;
        }
        int pid = strtod(entry->d_name, NULL);
        if (proclist_find_pid(pid) == NULL) {
            if (proclist_append(pid) == NULL) {
                return NULL;
            }
        }
    }
    closedir(procdp);
    return  &__proclist;
}   // }}}

/**
 * sysmon_process_refresh_dec() - decremental refresh
 *
 * sysmon_process_refresh_dec() returns NULL on interal call failure.
 *
 * sysmon_process_refresh_dec() traverses the existing proclist, updates fields
 * and delete procnodes no longer found in ``/proc``.
 */
static struct proclist *sysmon_process_refresh_dec(void) {      // {{{
    struct procnode *procnode = __proclist.head;
    struct procnode *delnode = NULL;
    while (procnode != NULL) {
        if (procstat_fill(&procnode->procstat) != 0) {
            delnode = procnode;
        }
        procnode = procnode->next;
        if (delnode != NULL) {
            proclist_del(delnode);
            delnode = NULL;
        }
    }
    return &__proclist;
}   // }}}

/******* Module Interfaces *******/

struct procstat *sysmon_get_procstat(int pid) {     // {{{
    static struct procstat procstat;
    procstat.pid = pid;
    if (procstat_fill(&procstat) != 0) {
        return NULL;
    }
    return &procstat;
}   // }}}

int proclist_init(void) {   // {{{
    memset(&__proclist, 0, sizeof(struct proclist));
    return 0;
}   // }}}

void proclist_cleanup(void) {   // {{{
    if (__proclist.head == NULL) {
        return;
    }
    while (__proclist.head != NULL) {
        proclist_del(__proclist.head);
    }
    return;
}   // }}}

static struct procnode *__proclist_iter_ptr;    // iter pointer as private member

struct procstat *proclist_iter_begin(void) {     // {{{
    __proclist_iter_ptr = __proclist.head;
    if (__proclist_iter_ptr == NULL) {
        return NULL;
    }
    return &__proclist_iter_ptr->procstat;
}   // }}}

struct procstat *proclist_iter_next(void) {      // {{{
    __proclist_iter_ptr = __proclist_iter_ptr->next;
    if (__proclist_iter_ptr == NULL) {
        return NULL;
    }
    return &__proclist_iter_ptr->procstat;
}   // }}}

struct proclist *sysmon_process_refresh(void (*cb)(void)) {     // {{{
    if (sysmon_process_refresh_dec() == NULL) {
        return NULL;
    }
    if (sysmon_process_refresh_inc() == NULL) {
        return NULL;
    }
    if (cb != NULL) {
        cb();
    }
    return &__proclist;
}   // }}}

/******* Test *******/

#if SYSMON_PROCESS_TEST
int main(const int argc, const char **argv) {

    /* printf("Testing %s():\n", "sysmon_get_procstat"); */
    /* struct procstat *procstat = sysmon_get_procstat(2545); */
    /* if (procstat != NULL) { */
    /*     printf(SYSMON_TEST_SUCCESS ": pid: %d\n", procstat->pid); */
    /*     printf(SYSMON_TEST_ESCAPE "comm: %s\n", procstat->comm); */
    /*     printf(SYSMON_TEST_ESCAPE "ppid: %d\n", procstat->ppid); */
    /*     printf(SYSMON_TEST_ESCAPE "priority: %ld\n", procstat->priority); */
    /*     printf(SYSMON_TEST_ESCAPE "nice: %ld\n", procstat->nice); */
    /*     printf(SYSMON_TEST_ESCAPE "num_threads: %ld\n", procstat->num_threads); */
    /*     printf(SYSMON_TEST_ESCAPE "vsize: %luB (%.2lfMB)\n", procstat->vsize, */
    /*             (double)procstat->vsize / (double)B_PER_MB); */
    /*     printf(SYSMON_TEST_ESCAPE "rss: %ldB (%.2lfMB)\n", procstat->rss, */
    /*             (double)procstat->rss * getpagesize() / (double)B_PER_MB); */
    /* } else { */
    /*     printf(SYSMON_TEST_FAIL ": %s\n", "sysmon_get_procstat()"); */
    /* } */

    printf("Testing %s():\n", "sysmon_process_refresh");
    sysmon_process_load();
    struct proclist *proclist = sysmon_process_hard_refresh();
    proclist = sysmon_process_refresh(NULL);
    if (proclist != NULL) {
        for (struct procstat *procstat = proclist_iter_begin();
                procstat != proclist_iter_end();
                procstat = proclist_iter_next()) {
            printf(SYSMON_TEST_SUCCESS ": pid: %d\n", procstat->pid);
            printf(SYSMON_TEST_ESCAPE "comm: %s\n", procstat->comm);
            printf(SYSMON_TEST_ESCAPE "ppid: %d\n", procstat->ppid);
            printf(SYSMON_TEST_ESCAPE "priority: %ld\n", procstat->priority);
            printf(SYSMON_TEST_ESCAPE "nice: %ld\n", procstat->nice);
            printf(SYSMON_TEST_ESCAPE "num_threads: %ld\n", procstat->num_threads);
            printf(SYSMON_TEST_ESCAPE "vsize: %luB (%.2lfMB)\n", procstat->vsize,
                    (double)procstat->vsize / (double)B_PER_MB);
            printf(SYSMON_TEST_ESCAPE "rss: %ldB (%.2lfMB)\n", procstat->rss,
                    (double)procstat->rss * getpagesize() / (double)B_PER_MB);
        }
    } else {
        printf(SYSMON_TEST_FAIL ": got %p\n", proclist);
    }
    sysmon_process_unload();

    return 0;
}
#endif

