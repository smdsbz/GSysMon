#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>

#include <gtk/gtk.h>

#include "../include/utils.h"
#include "../include/system.h"
#include "../include/cpu.h"
#include "../include/process_mvc.h"
#include "../include/process_callbacks.h"
#include "../include/cpustat.h"
#include "../include/memory.h"
#include "../include/record.h"


static GtkBuilder *builder = NULL;      // making it global makes things a lot much easier

static inline GObject *get_widget_by_id(const char *id)
{   // {{{
    return gtk_builder_get_object(builder, id);
}   // }}}

/**** Search Related Callbacks ****/

static void on_search_choice_activate(GtkMenuItem *item, gpointer method)
{   // {{{
    char sbar_name[32];
    sprintf(sbar_name, "search-bar-%s", (char *)method);
    gtk_search_bar_set_search_mode(
        GTK_SEARCH_BAR(get_widget_by_id(sbar_name)),
        TRUE
    );
    if (strequ(method, "pid")) {
        gtk_search_bar_set_search_mode(
            GTK_SEARCH_BAR(get_widget_by_id("search-bar-name")),
            FALSE
        );
    } else {
        gtk_search_bar_set_search_mode(
            GTK_SEARCH_BAR(get_widget_by_id("search-bar-pid")),
            FALSE
        );
    }
}   // }}}

static void on_search_toggle_clicked(GtkToolButton *btn, gpointer _)
{   // {{{
    gtk_search_bar_set_search_mode(
        GTK_SEARCH_BAR(get_widget_by_id("search-bar-name")),
        TRUE
    );
    gtk_search_bar_set_search_mode(
        GTK_SEARCH_BAR(get_widget_by_id("search-bar-pid")),
        FALSE
    );
}   // }}}

static void (*process_filter_fn)(struct proclist *, void *) = NULL;
static char *process_filter_str = NULL;

static void on_search_entry_changed(GtkSearchEntry *entry, gpointer method)
{   // {{{
    if (process_filter_str != NULL) {
        g_free(process_filter_str);
        process_filter_str = NULL;
    }
    process_filter_str = gtk_editable_get_chars(GTK_EDITABLE(entry), 0, -1);
    // HACK: On clicking the 'X' button on the search bar, the search bar is set
    //       to invisible and search entry is emptied.
    //       (Is it that hard to emmit a "search-mode-changed" signal, GTK?)
    if (strequ(process_filter_str, "")) {
        g_free(process_filter_str);
        process_filter_str = NULL;
        process_filter_fn = NULL;
        return;
    }
    if (strequ(method, "pid")) {
        process_filter_fn = proclist_filter_by_pid_leading;
    } else {
        process_filter_fn = proclist_filter_by_name_fuzzy;
    }
}   // }}}

/**** Program Related Callbacks ****/

static void on_button_kill_clicked(GtkToolButton *btn, gpointer _)
{   // {{{
    GtkTreeSelection *sel = GTK_TREE_SELECTION(
        get_widget_by_id("process-tree-selection")
    );
    GtkTreeModel *mod = GTK_TREE_MODEL(
        get_widget_by_id("process-liststore")
    );
    GtkTreeIter iter;
    // if no process selected, prompt user
    if (!gtk_tree_selection_get_selected(sel, &mod, &iter)) {
        GtkWidget *msgdialog = gtk_message_dialog_new(
            GTK_WINDOW(get_widget_by_id("main-window")),
            GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_CLOSE,
            "Please specify the process to be deleted by selecting it!"
        );
        gtk_dialog_run(GTK_DIALOG(msgdialog));
        gtk_widget_destroy(msgdialog);
        return;
    }
    // else, ask for confirmation
    int pid;
    char *comm;
    gtk_tree_model_get(mod, &iter, COL_PID, &pid, COL_NAME, &comm, -1);
    GtkWidget *confdialog = gtk_message_dialog_new(
        GTK_WINDOW(get_widget_by_id("main-window")),
        GTK_DIALOG_MODAL, GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO,
        "Are you sure you want to terminate '%s' (%d)?", comm, pid
    );
    int choice = gtk_dialog_run(GTK_DIALOG(confdialog));
    gtk_widget_destroy(confdialog);
    g_free(comm);
    switch (choice) {
        case GTK_RESPONSE_YES: {
            if (kill(pid, SIGKILL) == -1) {
                perror("kill");
            }
            break;
        }
        default: {
            break;
        }
    }
}   // }}}

static GThreadPool *prog_pool = NULL;

static void new_program_fn(gpointer cmd_alloced, gpointer _)
{   // {{{
    int ret = system(cmd_alloced);      // blocked
    g_print("'%s' finished with code %d!\n", (char *)cmd_alloced, ret);
    free(cmd_alloced);
}   // }}}

static void on_button_new_program_clicked(gpointer _)
{   // {{{
    GtkDialog *dialog = GTK_DIALOG(get_widget_by_id("new-program-dialog"));
    int ret = gtk_dialog_run(dialog);
    switch (ret) {
        case GTK_RESPONSE_APPLY: {
            char *cmd_alloced = malloc(
                gtk_entry_get_text_length(
                    GTK_ENTRY(get_widget_by_id("new-cmdline-entry"))
                ) + 1
            );
            strcpy(
                cmd_alloced,
                gtk_entry_get_text(
                    GTK_ENTRY(get_widget_by_id("new-cmdline-entry"))
                )
            );
            g_thread_pool_push(prog_pool, cmd_alloced, NULL);
            break;
        }
        default: {
            break;
        }
    }
    gtk_widget_destroy(GTK_WIDGET(dialog));     // may not be correct, will see
}   // }}}

static void on_button_nuke_clicked(gpointer _)
{   // {{{
    GtkWidget *confdialog = gtk_message_dialog_new(
        GTK_WINDOW(get_widget_by_id("main-window")),
        GTK_DIALOG_MODAL, GTK_MESSAGE_WARNING, GTK_BUTTONS_YES_NO,
        "Are you sure you want to shutdown your computer?"
    );
    int choice = gtk_dialog_run(GTK_DIALOG(confdialog));
    gtk_widget_destroy(confdialog);
    switch (choice) {
        case GTK_RESPONSE_YES: {
            system("shutdown -h now");
            // will not survive this line
        }
        default: {
            break;
        }
    }
}   // }}}

/**** GUI Update Related Callbacks ****/

static guint timeout_halfsec_src = 0;
static struct cpusinfo *cpusinfo = NULL;
static struct record *cpurec = NULL;
static struct record *memrec = NULL;
static struct record *swaprec = NULL;

static gboolean timeout_halfsec_fn(gpointer _)
{   // {{{
    // update system-page
    char str[256];
    sprintf(str, "%.2lf", sysmon_get_uptime()->uptime);
    gtk_label_set_label(
        GTK_LABEL(get_widget_by_id("label-system-uptime")),
        str
    );
    cpusinfo = sysmon_get_cpusinfo();
    str[0] = '\0';
    for (int idx = 0; idx != cpusinfo->processor_count; ++idx) {
        char cpu_freq[16];
        sprintf(cpu_freq, "%.2lf, ", cpusinfo->cpuinfos[idx].cpu_mhz);
        strcat(str, cpu_freq);
    }
    /* str[strlen(str) - 1] = '\0'; */
    str[strlen(str) - 2] = '\0';
    gtk_label_set_label(
        GTK_LABEL(get_widget_by_id("label-cpu-frequency")),
        str
    );
    // update statusbar
    if (cpurec->data->head == NULL) {
        sprintf(str, "-- %%");
    } else {
        sprintf(str, "%.2lf %%", *(double *)cpurec->data->head->data * 100.0);
    }
    gtk_label_set_label(
        GTK_LABEL(get_widget_by_id("label-statusbar-cpu")),
        str
    );
    if (memrec->data->head == NULL) {
        sprintf(str, "-- %%");
    } else {
        sprintf(str, "%.2lf %%", *(double *)memrec->data->head->data * 100.0);
    }
    gtk_label_set_label(
        GTK_LABEL(get_widget_by_id("label-statusbar-mem")),
        str
    );
    time_t time_ = time(NULL);
    char *timestr = ctime(&time_);
    remove_trailing_newline(timestr);
    gtk_label_set_label(
        GTK_LABEL(get_widget_by_id("label-statusbar-time")),
        timestr
    );
    return G_SOURCE_CONTINUE;
}   // }}}

guint timeout_onesec_src = 0;

static gboolean timeout_onesec_fn(gpointer _)
{   // {{{
    // update process-page
    gsysmon_process_mvc_refresh(
        GTK_LIST_STORE(gtk_builder_get_object(builder, "process-liststore")),
        process_filter_fn, process_filter_str
    );
    // update resource-page
    struct cpustat *cpustat = sysmon_get_cpustat_diff();
    record_push_lf(cpurec, cpustat_get_usage_percentage(cpustat, -1) / 100.0);
    struct meminfo *meminfo = sysmon_get_meminfo();
    record_push_lf(memrec, (1.0 - (double)meminfo->mem_free / (double)meminfo->mem_total));
    record_push_lf(swaprec, (1.0 - (double)meminfo->swap_free / (double)meminfo->swap_total));
    gtk_widget_queue_draw(GTK_WIDGET(get_widget_by_id("draw-area-cpu")));
    gtk_widget_queue_draw(GTK_WIDGET(get_widget_by_id("draw-area-mem")));
    gtk_widget_queue_draw(GTK_WIDGET(get_widget_by_id("draw-area-swap")));
    return G_SOURCE_CONTINUE;
}   // }}}

static gboolean on_cpustat_draw(GtkWidget *widget, cairo_t *cr, gpointer _)
{   // {{{
    record_render(cpurec, cr, gtk_widget_get_allocated_width(widget),
                    gtk_widget_get_allocated_height(widget));
    return G_SOURCE_REMOVE;
}   // }}}

static gboolean on_memstat_draw(GtkWidget *widget, cairo_t *cr, gpointer _)
{   // {{{
    record_render(memrec, cr, gtk_widget_get_allocated_width(widget),
                    gtk_widget_get_allocated_height(widget));
    return G_SOURCE_REMOVE;
}   // }}}

static gboolean on_swapstat_draw(GtkWidget *widget, cairo_t *cr, gpointer _)
{   // {{{
    record_render(swaprec, cr, gtk_widget_get_allocated_width(widget),
                    gtk_widget_get_allocated_height(widget));
    return G_SOURCE_REMOVE;
}   // }}}

/**** Application Exit Callback ****/

static void on_destroy(GtkWidget *widget, gpointer _)
{   // {{{
    gtk_main_quit();
    if (timeout_halfsec_src != 0) {
        g_source_remove(timeout_halfsec_src);
        timeout_halfsec_src = 0;
    }
    if (process_filter_str != NULL) {
        g_free(process_filter_str);
    }
    sysmon_cpu_unload();
    gsysmon_process_mvc_unload();
    sysmon_cpustat_unload();
    if (cpurec != NULL) {
        record_destroy_with_data(cpurec);
    }
    if (memrec != NULL) {
        record_destroy_with_data(memrec);
    }
    if (swaprec != NULL) {
        record_destroy_with_data(swaprec);
    }
    if (prog_pool != NULL) {
        g_thread_pool_free(prog_pool, /*immediate=*/FALSE, /*wait_=*/TRUE);
        prog_pool = NULL;
    }
    return;
}   // }}}

/**** Main Entry ****/

int main(int argc, char **argv) {

    gtk_init(&argc, &argv);

    builder = gtk_builder_new_from_file("ui/main.glade");

    GObject *main_window = get_widget_by_id("main-window");
    g_signal_connect(main_window, "destroy", G_CALLBACK(on_destroy), NULL);

    // search {{{
    // #search-toggle
    g_signal_connect(
        get_widget_by_id("search-toggle"),
        "clicked", G_CALLBACK(on_search_toggle_clicked), NULL
    );

    // #search-choice-(pid|name)
    gtk_menu_tool_button_set_menu(
        GTK_MENU_TOOL_BUTTON(get_widget_by_id("search-toggle")),
        GTK_WIDGET(get_widget_by_id("search-method-choice"))
    );
    g_signal_connect(
        get_widget_by_id("search-choice-pid"),
        "activate", G_CALLBACK(on_search_choice_activate), "pid"
    );
    g_signal_connect(
        get_widget_by_id("search-choice-name"),
        "activate", G_CALLBACK(on_search_choice_activate), "name"
    );

    // #search-entry-(pid|name)
    g_signal_connect(
        GTK_SEARCH_ENTRY(gtk_builder_get_object(builder, "search-entry-pid")),
        "search-changed", G_CALLBACK(on_search_entry_changed), "pid"
    );
    g_signal_connect(
        GTK_SEARCH_ENTRY(gtk_builder_get_object(builder, "search-entry-name")),
        "search-changed", G_CALLBACK(on_search_entry_changed), "name"
    );
    // end search }}}

    // toolbar {{{
    g_signal_connect(
        get_widget_by_id("button-kill"),
        "clicked", G_CALLBACK(on_button_kill_clicked), NULL
    );
    g_signal_connect(
        get_widget_by_id("button-new-program"),
        "clicked", G_CALLBACK(on_button_new_program_clicked), NULL
    );
    g_signal_connect(
        get_widget_by_id("button-nuke"),
        "clicked", G_CALLBACK(on_button_nuke_clicked), NULL
    );
    // }}}

    // system-page {{{
    gtk_label_set_xalign(
        GTK_LABEL(get_widget_by_id("label-hostname-title")),
        1.0
    );
    gtk_label_set_xalign(
        GTK_LABEL(get_widget_by_id("label-hostname")),
        0.0
    );
    gtk_label_set_label(
        GTK_LABEL(get_widget_by_id("label-hostname")),
        sysmon_get_hostname()
    );

    gtk_label_set_xalign(
        GTK_LABEL(get_widget_by_id("label-system-boottime-title")),
        1.0
    );
    gtk_label_set_xalign(
        GTK_LABEL(get_widget_by_id("label-system-boottime")),
        0.0
    );
    ssize_t boottime = sysmon_get_boottime();
    char *boottime_str = ctime(&boottime);
    remove_trailing_newline(boottime_str);
    gtk_label_set_label(
        GTK_LABEL(get_widget_by_id("label-system-boottime")),
        boottime_str
    );

    gtk_label_set_xalign(
        GTK_LABEL(get_widget_by_id("label-system-uptime-title")),
        1.0
    );
    gtk_label_set_xalign(
        GTK_LABEL(get_widget_by_id("label-system-uptime")),
        0.0
    );

    gtk_label_set_xalign(
        GTK_LABEL(get_widget_by_id("label-system-version-title")),
        1.0
    );
    gtk_label_set_xalign(
        GTK_LABEL(get_widget_by_id("label-system-version")),
        0.0
    );
    gtk_label_set_label(
        GTK_LABEL(get_widget_by_id("label-system-version")),
        sysmon_get_system_version()
    );

    sysmon_cpu_load();
    cpusinfo = sysmon_get_cpusinfo();
    gtk_label_set_xalign(
        GTK_LABEL(get_widget_by_id("label-cpu-model-title")),
        1.0
    );
    gtk_label_set_xalign(
        GTK_LABEL(get_widget_by_id("label-cpu-model")),
        0.0
    );
    if (cpusinfo != NULL) {
        gtk_label_set_label(
            GTK_LABEL(get_widget_by_id("label-cpu-model")),
            cpusinfo->cpuinfos[0].model_name
        );
    } else {
        g_printerr("sysmon_get_cpusinfo() failed!\n");
        g_object_unref(builder);
        sysmon_cpu_unload();
        return 0;
    }

    gtk_label_set_xalign(
        GTK_LABEL(get_widget_by_id("label-cpu-frequency-title")),
        1.0
    );
    gtk_label_set_xalign(
        GTK_LABEL(get_widget_by_id("label-cpu-frequency")),
        0.0
    );
    // end system-page }}}

    // resource-page {{{
    g_signal_connect(
        get_widget_by_id("draw-area-cpu"),
        "draw", G_CALLBACK(on_cpustat_draw), NULL
    );
    g_signal_connect(
        get_widget_by_id("draw-area-mem"),
        "draw", G_CALLBACK(on_memstat_draw), NULL
    );
    g_signal_connect(
        get_widget_by_id("draw-area-swap"),
        "draw", G_CALLBACK(on_swapstat_draw), NULL
    );
    // }}}

    // statusbar {{{
    gtk_label_set_xalign(
        GTK_LABEL(get_widget_by_id("label-statusbar-cpu-title")),
        1.0
    );
    gtk_label_set_xalign(
        GTK_LABEL(get_widget_by_id("label-statusbar-cpu")),
        0.0
    );
    gtk_label_set_xalign(
        GTK_LABEL(get_widget_by_id("label-statusbar-mem-title")),
        1.0
    );
    gtk_label_set_xalign(
        GTK_LABEL(get_widget_by_id("label-statusbar-mem")),
        0.0
    );
    gtk_label_set_xalign(
        GTK_LABEL(get_widget_by_id("label-statusbar-time")),
        1.0
    );
    // }}}

    gsysmon_process_mvc_load();

    sysmon_cpustat_load();
    sysmon_get_cpustat_diff();
    cpurec = record_new(100, RECORD_TYPE_DOUBLE);
    memrec = record_new(100, RECORD_TYPE_DOUBLE);
    swaprec = record_new(100, RECORD_TYPE_DOUBLE);

    prog_pool = g_thread_pool_new(new_program_fn, NULL, -1, FALSE, NULL);

    g_timeout_add(500, timeout_halfsec_fn, NULL);
    g_timeout_add(1000, timeout_onesec_fn, NULL);

    gtk_widget_show_all(GTK_WIDGET(main_window));
    gtk_main();

    return 0;
}

