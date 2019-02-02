#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gtk/gtk.h>

#include "../include/utils.h"
#include "../include/system.h"


void on_search_choice_select(GtkMenuItem *item, gpointer sbar) {
    gtk_search_bar_set_search_mode(GTK_SEARCH_BAR(sbar), TRUE);
}

void on_search_toggle_clicked(GtkToolButton *btn, gpointer sbar) {
    gtk_search_bar_set_search_mode(GTK_SEARCH_BAR(sbar), TRUE);
}

void on_destroy(GtkWidget *widget, gpointer data) {
    g_print("on_destroy: ...\n");
    gtk_main_quit();
    return;
}


int main(int argc, char **argv) {

    gtk_init(&argc, &argv);

    GtkBuilder *builder = gtk_builder_new_from_file("ui/main.glade");

    GObject *main_window = gtk_builder_get_object(builder, "main-window");
    g_signal_connect(main_window, "destroy", G_CALLBACK(on_destroy), NULL);
    gtk_window_set_default_size(GTK_WINDOW(main_window), 800, 500);

    gtk_menu_tool_button_set_menu(
        GTK_MENU_TOOL_BUTTON(gtk_builder_get_object(builder, "search-toggle")),
        GTK_WIDGET(gtk_builder_get_object(builder, "search-method-choice"))
    );

    g_signal_connect(
        gtk_builder_get_object(builder, "search-toggle"),
        "clicked", G_CALLBACK(on_search_toggle_clicked),
        gtk_builder_get_object(builder, "search-bar-name")
    );

    g_signal_connect(
        gtk_builder_get_object(builder, "search-choice-pid"),
        "activate", G_CALLBACK(on_search_choice_select),
        gtk_builder_get_object(builder, "search-bar-pid")
    );
    g_signal_connect(
        gtk_builder_get_object(builder, "search-choice-name"),
        "activate", G_CALLBACK(on_search_choice_select),
        gtk_builder_get_object(builder, "search-bar-name")
    );

    gtk_widget_show_all(GTK_WIDGET(main_window));
    gtk_main();

    return 0;
}

