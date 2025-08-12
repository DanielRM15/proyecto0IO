#include <gtk/gtk.h>

void on_quitBtn_clicked(GtkButton *button, gpointer user_data) {
    gtk_main_quit();
}

void on_not_implemented(GtkButton *button, gpointer user_data) {
    system("./Pending/pending.exe &");
}

int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);

    GtkBuilder *builder = gtk_builder_new_from_file("mainMenu.glade");

    GtkWidget *window = GTK_WIDGET(gtk_builder_get_object(builder, "hWindow"));

    gtk_builder_connect_signals(builder, NULL);

    g_object_unref(builder);

    gtk_widget_show_all(window);

    gtk_main();

    return 0;
}
