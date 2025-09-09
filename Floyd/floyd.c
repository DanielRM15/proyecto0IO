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

FILE *output_file;
int **D;
int **P;

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
            GtkAdjustment *adj = gtk_adjustment_new(i == j ? 0 : -1, -1, 1000, 1, 10, 0);
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

void setup_latex()
{
    fprintf(output_file,
            "\\documentclass{beamer}\n"
            "%% Theme settings\n"
            "\\usetheme{default}\n"
            "%% Define header template\n"
            "\\setbeamertemplate{headline}{%%\n"
            "    \\begin{beamercolorbox}[wd=\\paperwidth,ht=3ex,dp=1ex,center]{section in head/foot}%%\n"
            "        \\usebeamercolor[fg]{section in head/foot}%%\n"
            "        My Centered Header\n"
            "    \\end{beamercolorbox}%%\n"
            "}\n\n"
            "%% Customize header color\n"
            "\\setbeamercolor{section in head/foot}{bg=blue!50, fg=white}\n"
            "\\begin{document}\n\n");
}

void print_table_latex(const char *slide_title, int **table)
{
    fprintf(output_file, "\\begin{frame}{%s}\n", slide_title);
    fprintf(output_file, "    \\begin{center}\n");
    fprintf(output_file, "        \\begin{tabular}{c|");
    for (int j = 0; j < nodes; j++)
        fprintf(output_file, "c");
    fprintf(output_file, "}\n");
    fprintf(output_file, "         & ");
    for (int j = 0; j < nodes; j++)
    {
        fprintf(output_file, "v%d", j + 1);
        if (j < nodes - 1)
            fprintf(output_file, " & ");
    }
    fprintf(output_file, " \\\\\n\\hline\n");
    for (int i = 0; i < nodes; i++)
    {
        fprintf(output_file, "v%d & ", i + 1);
        for (int j = 0; j < nodes; j++)
        {
            if (table[i][j] < 99999)
                fprintf(output_file, "%d", table[i][j]);
            else
                fprintf(output_file, "$\\infty$");
            if (j < nodes - 1)
                fprintf(output_file, " & ");
        }
        fprintf(output_file, " \\\\\n");
    }

    fprintf(output_file, "        \\end{tabular}\n");
    fprintf(output_file, "    \\end{center}\n");
    fprintf(output_file, "\\end{frame}\n\n");
    fprintf(output_file, " ");
}

void floyd_tex()
{
    for (int k = 0; k < nodes; k++)
    {
        for (int i = 0; i < nodes; i++)
        {
            for (int j = 0; j < nodes; j++)
            {
                if (i == j)
                    continue;
                if (D[i][j] > D[i][k] + D[k][j])
                {
                    D[i][j] = D[i][k] + D[k][j];
                    P[i][j] = k + 1;
                }
            }
        }
        char slide_title[64];
        sprintf(slide_title, "Table D(%d)", k + 1);
        print_table_latex(slide_title, D);
        print_table_latex("Tabla P (Hasta el momento)", P);
    }
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

void build_D0()
{
    for (int i = 1; i <= nodes; i++)
    {
        for (int j = 1; j <= nodes; j++)
        {
            GtkWidget *child = gtk_grid_get_child_at(GTK_GRID(distance_input_grid), j, i);
            gdouble value = gtk_spin_button_get_value(GTK_SPIN_BUTTON(child));
            int ivalue = (int)value;
            if (ivalue < 0)
            {
                ivalue = 99999;
            }
            D[i - 1][j - 1] = ivalue;
            P[i - 1][j - 1] = 0;
        }
    }
    print_table_latex("Table D(0)", D);
}

void on_runBtn_clicked(GtkButton *button, gpointer user_data)
{
    output_file = fopen("output.tex", "w");
    setup_latex();
    if (output_file == NULL)
    {
        g_print("Failed to open LaTeX file");
        return;
    }

    D = (int **)malloc(nodes * sizeof(int *));
    for (int i = 0; i < nodes; i++)
        D[i] = (int *)malloc(nodes * sizeof(int));

    P = (int **)malloc(nodes * sizeof(int *));
    for (int i = 0; i < nodes; i++)
        P[i] = (int *)malloc(nodes * sizeof(int));

    build_D0();
    floyd_tex();

    for (int i = 0; i < nodes; i++)
    {
        free(D[i]);
        free(P[i]);
    }

    fprintf(output_file, "\\end{document}");
    free(D);
    free(P);
    fclose(output_file);
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
    distance_input_grid = GTK_WIDGET(gtk_builder_get_object(builder, "distance_input_grid"));

    gtk_widget_show_all(window);

    gtk_main();

    g_object_unref(builder);

    return 0;
}
