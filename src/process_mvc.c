#include <stdio.h>
#include <unistd.h>
#include <gtk/gtk.h>

#include "../include/utils.h"
#include "../include/process_mvc.h"

static inline void assign_row(GtkListStore *ls, GtkTreeIter *iter,
                                struct procstat *stat) {
    char vmem[32], rmem[32];
    sprintf(vmem, "%s", get_human_from_bytes(stat->vsize));
    sprintf(rmem, "%s", get_human_from_bytes((size_t)stat->rss * getpagesize()));
    gtk_list_store_set(
        ls, iter,
        COL_PID, stat->pid,
        COL_NAME, stat->comm,
        COL_STATE, stat->state,
        COL_PPID, stat->ppid,
        COL_NICE, stat->nice,
        COL_THREADS, stat->num_threads,
        COL_VMEM, vmem,
        COL_RMEM, rmem,
        -1
    );
    return;
}

/**
 * mvc_sync_shrink() traverses @ls, remove rows no longer present, and update
 * those still present.
 */
static void mvc_sync_shrink(GtkListStore *ls) {
    GtkTreeIter iter;
    if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(ls), &iter) == FALSE) {
        return;
    }
    int pid_at_iter;
    struct procstat *stat;
    while (1) {
        gtk_tree_model_get(GTK_TREE_MODEL(ls), &iter, COL_PID, &pid_at_iter, -1);
        if ((stat = proclist_find_by_pid(pid_at_iter)) != NULL) {
            assign_row(ls, &iter, stat);
            if (gtk_tree_model_iter_next(GTK_TREE_MODEL(ls), &iter) == FALSE) {
                return;
            }
        } else {
            if (gtk_list_store_remove(ls, &iter) == FALSE) {
                return;
            }
        }
    }
}

static int is_in_model_pid(GtkListStore *ls, int pid) {
    GtkTreeIter iter;
    if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(ls), &iter) == FALSE) {
        return 0;
    }
    int pid_at_iter;
    do {
        gtk_tree_model_get(GTK_TREE_MODEL(ls), &iter, COL_PID, &pid_at_iter, -1);
        if (pid_at_iter == pid) {
            return 1;
        }
    } while (gtk_tree_model_iter_next(GTK_TREE_MODEL(ls), &iter) == TRUE);
    return 0;
}

/**
 * mvc_sync_grow() traverses proclist and append rows currently not present in @ls.
 *
 * mvc_sync_grow() requires a previous mvc_sync_shrink() call to update rows
 * still present.
 */
static void mvc_sync_grow(GtkListStore *ls) {
    GtkTreeIter iter;
    for (struct procstat *stat = proclist_iter_begin();
            stat != proclist_iter_end();
            stat = proclist_iter_next()) {
        if (!is_in_model_pid(ls, stat->pid)) {
            gtk_list_store_append(ls, &iter);
            assign_row(ls, &iter, stat);
        } else {
            // NOTE: Already updated by previous mvc_sync_shrink() call!
        }
    }
}

int gsysmon_process_mvc_sync(GtkListStore *ls, struct proclist *proclist) {
    if (ls == NULL || proclist == NULL) {
        return -1;
    }
    mvc_sync_shrink(ls);
    mvc_sync_grow(ls);
    return 0;
}
