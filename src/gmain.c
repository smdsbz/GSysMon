#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <gtk/gtk.h>

#include "../include/utils.h"
#include "../include/system.h"
#include "../include/cpu.h"
#include "../include/process.h"


GtkBuilder *builder = NULL;         // making it global makes things a lot more easier
guint timeout_halfsec_src = 0;
guint timeout_onesec_src = 0;
struct cpusinfo *cpusinfo = NULL;

static GObject *get_widget_by_id(const char *id)
{   // {{{
    return gtk_builder_get_object(builder, id);
}   // }}}

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

static void refresh_process_liststore(GtkListStore *ls)
{   // {{{
    struct proclist *proclist = sysmon_process_refresh(NULL, NULL);
    if (proclist == NULL) {
        g_printerr("sysmon_process_refresh() failed!\n");
        return;
    }
    enum {
        COL_PID, COL_NAME, COL_STATE, COL_PPID, COL_NICE, COL_THREADS,
        COL_VMEM, COL_RMEM, NCOLS
    };
    gtk_list_store_clear(ls);
    GtkTreeIter treeiter;
    for (struct procstat *stat = proclist_iter_begin();
            stat != proclist_iter_end();
            stat = proclist_iter_next()) {
        gtk_list_store_append(ls, &treeiter);
        char vmem[32], rmem[32];
        sprintf(vmem, "%s", get_human_from_bytes(stat->vsize));
        sprintf(rmem, "%s", get_human_from_bytes((size_t)stat->rss * getpagesize()));
        gtk_list_store_set(
            ls, &treeiter,
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
    }
    return;
}   // }}}

static gboolean timeout_halfsec_fn(gpointer _)
{   // {{{
    // update system-page
    char uptime_str[32];
    sprintf(uptime_str, "%.2lf", sysmon_get_uptime()->uptime);
    gtk_label_set_label(
        GTK_LABEL(get_widget_by_id("label-system-uptime")),
        uptime_str
    );
    cpusinfo = sysmon_get_cpusinfo();
    char cpus_freq[256] = "";
    for (int idx = 0; idx != cpusinfo->processor_count; ++idx) {
        char cpu_freq[16];
        sprintf(cpu_freq, "%.2lf, ", cpusinfo->cpuinfos[idx].cpu_mhz);
        strcat(cpus_freq, cpu_freq);
    }
    cpus_freq[strlen(cpus_freq) - 1] = '\0';
    cpus_freq[strlen(cpus_freq) - 2] = '\0';
    gtk_label_set_label(
        GTK_LABEL(get_widget_by_id("label-cpu-frequency")),
        cpus_freq
    );
    return G_SOURCE_CONTINUE;
}   // }}}

static gboolean timeout_onesec_fn(gpointer _)
{   // {{{
    // update process-page
    refresh_process_liststore(
        GTK_LIST_STORE(gtk_builder_get_object(builder, "process-liststore"))
    );
    return G_SOURCE_CONTINUE;
}   // }}}

static void on_destroy(GtkWidget *widget, gpointer _)
{   // {{{
    gtk_main_quit();
    if (timeout_halfsec_src != 0) {
        g_source_remove(timeout_halfsec_src);
        timeout_halfsec_src = 0;
    }
    sysmon_cpu_unload();
    sysmon_process_unload();
    return;
}   // }}}


int main(int argc, char **argv) {

    gtk_init(&argc, &argv);

    builder = gtk_builder_new_from_file("ui/main.glade");

    GObject *main_window = get_widget_by_id("main-window");
    g_signal_connect(main_window, "destroy", G_CALLBACK(on_destroy), NULL);
    /* gtk_window_set_default_size(GTK_WINDOW(main_window), 800, 500); */

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

    // system-page: #label-* (static)
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

    g_timeout_add(500, timeout_halfsec_fn, NULL);
    g_timeout_add(1000, timeout_onesec_fn, NULL);

    sysmon_process_load();

    gtk_widget_show_all(GTK_WIDGET(main_window));
    gtk_main();

    return 0;
}

