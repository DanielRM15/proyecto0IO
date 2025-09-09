#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

GtkWidget *num_nodes_spin;
GtkWidget *main_stack;
GtkWidget *distance_table_container;
GtkWidget *distance_input_grid;
int nodes = 0;

// Function to create the dynamic distance table
void create_distance_table()
{
    char buf[16];
    for (int i = 0; i <= nodes; i++)
    {
        for (int j = 0; j <= nodes; j++)
        {
            GtkWidget *to_attach;
            if (i == 0 && j == 0)
                continue;
            if (i == 0 && j != 0)
            {
                snprintf(buf, sizeof(buf), "v%d", j);
                to_attach = gtk_label_new(buf);
                gtk_grid_attach(GTK_GRID(distance_input_grid), to_attach, j, i, 1, 1);
                continue;
            }
            if (j == 0 && i != 0)
            {
                snprintf(buf, sizeof(buf), "v%d", i);
                to_attach = gtk_label_new(buf);
                gtk_grid_attach(GTK_GRID(distance_input_grid), to_attach, j, i, 1, 1);
                continue;
            }
            GtkAdjustment *adj = gtk_adjustment_new(0, -1000, 1000, 1, 10, 0);
            to_attach = gtk_spin_button_new(adj, 1, 0);
            if (i == j)
            {
                gtk_entry_set_text(GTK_ENTRY(to_attach), "0");
                gtk_widget_set_sensitive(to_attach, FALSE);
            }
            else
                gtk_entry_set_text(GTK_ENTRY(to_attach), "-1");
            gtk_grid_attach(GTK_GRID(distance_input_grid), to_attach, j, i, 1, 1);
        }
    }
    gtk_widget_show_all(distance_input_grid);
}

void on_quitBtn_clicked(GtkButton *button, gpointer user_data)
{
    gtk_main_quit();
}

void on_continueBtn_clicked(GtkButton *button, gpointer user_data)
{
    nodes = (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(num_nodes_spin));
    if (!nodes)
    {
        return;
    }

    // Create the dynamic table
    create_distance_table();

    // Switch to the distance input page
    gtk_stack_set_visible_child_name(GTK_STACK(main_stack), "page1");
}

void on_loadBtn_clicked(GtkButton *button, gpointer user_data)
{
    g_print("Load button clicked\n");
}

int main(int argc, char *argv[])
{
    gtk_init(&argc, &argv);

    // GtkBuilder *builder = gtk_builder_new_from_file("Floyd/floyd.glade"); //Si se abre desde el menu
    GtkBuilder *builder = gtk_builder_new_from_file("floyd.glade"); // Si se abre SIN en menu

    GtkWidget *window = GTK_WIDGET(gtk_builder_get_object(builder, "hWindow"));

    gtk_builder_connect_signals(builder, NULL);

    // exit
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    main_stack = GTK_WIDGET(gtk_builder_get_object(builder, "mainStack"));
    num_nodes_spin = GTK_WIDGET(gtk_builder_get_object(builder, "numNodesSpin"));
    distance_table_container = GTK_WIDGET(gtk_builder_get_object(builder, "distanceTableContainer"));
    distance_input_grid = GTK_WIDGET(gtk_builder_get_object(builder, "distance_input_grid"));

    g_object_unref(builder);

    gtk_widget_show_all(window);

    gtk_main();

    return 0;
}
