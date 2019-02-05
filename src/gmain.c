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


static GtkBuilder *builder = NULL;      // making it global makes things a lot much easier

static inline GObject *get_widget_by_id(const char *id)
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

static void on_button_kill_clicked(GtkToolButton *btn, gpointer _)
{   // {{{
    GtkTreeSelection *sel = GTK_TREE_SELECTION(
        get_widget_by_id("process-tree-selection")
    );
    GtkTreeModel *mod = GTK_TREE_MODEL(
        get_widget_by_id("process-liststore")
    );
    GtkTreeIter iter;
    // TODO: Popup an alert box before the button actually does something!
    if (!gtk_tree_selection_get_selected(sel, &mod, &iter)) {
        g_print("on_search_entry_changed(): no selected row!\n");
        return;
    }
    int pid;
    gtk_tree_model_get(mod, &iter, COL_PID, &pid, -1);
    if (kill(pid, SIGKILL) == -1) {
        perror("kill");
        return;
    }
}   // }}}

static guint timeout_halfsec_src = 0;
static struct cpusinfo *cpusinfo = NULL;

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

guint timeout_onesec_src = 0;

static gboolean timeout_onesec_fn(gpointer _)
{   // {{{
    // update process-page
    gsysmon_process_mvc_refresh(
        GTK_LIST_STORE(gtk_builder_get_object(builder, "process-liststore")),
        process_filter_fn, process_filter_str
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
    if (process_filter_str != NULL) {
        g_free(process_filter_str);
    }
    sysmon_cpu_unload();
    gsysmon_process_mvc_unload();
    return;
}   // }}}

static void dummy(GtkWidget *_, gpointer __) {
    g_print("dummy(): called\n");
}

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

    g_timeout_add(500, timeout_halfsec_fn, NULL);
    g_timeout_add(1000, timeout_onesec_fn, NULL);

    gsysmon_process_mvc_load();

    gtk_widget_show_all(GTK_WIDGET(main_window));
    gtk_main();

    return 0;
}

