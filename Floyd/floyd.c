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
int **changes;

// Validate numeric input
void on_insert_text(GtkEditable *editable, const gchar *text, gint length, gint *position, gpointer user_data)
{
    const gchar *current_text = gtk_entry_get_text(GTK_ENTRY(editable));
    
    // Check if all characters in the text are digits or minus sign
    for (int i = 0; i < length; i++)
    {
        if (!g_ascii_isdigit(text[i]) && text[i] != '-')
        {
            g_signal_stop_emission_by_name(editable, "insert-text");
            return;
        }
    }
    
    // Only allow minus at the beginning
    if (strchr(text, '-') && (*position > 0 || strlen(current_text) > 0))
    {
        g_signal_stop_emission_by_name(editable, "insert-text");
        return;
    }
    
    if (strchr(text, '-') || strchr(current_text, '-'))
    {

        for (int i = 0; i < length; i++)
        {
            if (g_ascii_isdigit(text[i]) && text[i] != '1')
            {
                g_signal_stop_emission_by_name(editable, "insert-text");
                return;
            }
        }
        
        // If current text already has -1
        if (strlen(current_text) >= 2 || (strlen(current_text) == 1 && current_text[0] == '-' && strchr(text, '1') && length > 1))
        {
            g_signal_stop_emission_by_name(editable, "insert-text");
            return;
        }
    }
        return;
}

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
            to_attach = gtk_entry_new();
            gtk_entry_set_width_chars(GTK_ENTRY(to_attach), 6);
            gtk_entry_set_max_length(GTK_ENTRY(to_attach), 5);
            gtk_entry_set_input_purpose(GTK_ENTRY(to_attach), GTK_INPUT_PURPOSE_DIGITS);
            
            // Connect the insert-text signal to validate input
            g_signal_connect(to_attach, "insert-text", G_CALLBACK(on_insert_text), NULL);
            
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

void print_cover_slide()
{
    fprintf(output_file, "\\begin{frame}[plain]\n\\titlepage\\end{frame}\n\n");
}

void setup_latex()
{
    fprintf(output_file,
        "\\PassOptionsToPackage{table}{xcolor}\n"
        "\\documentclass[xcolor=table]{beamer}\n"
        "\\usetheme{Madrid}\n"
        "\\usepackage{tikz, xcolor}\n"
        "\\usetikzlibrary{arrows.meta}\n"
        "\\usepackage{hyperref}\n"
        "\\usepackage{graphicx}\n"
        "\\title{Floyd's Algorithm: Shortest Path Problem}\n"
        "\\subtitle{Project 1}\n"
        "\\author{Daniel Romero - 2023059668 \\break Adrián Zamora - 2023083307}\n"
        "\\institute[TEC]{\\break Escuela de Ingeniería en Computación \\break Instituto Tecnológico de Costa Rica \\break II Semestre 2025}\n"
        "\\date{September 12, 2025}\n"
        "\\begin{document}\n\n"
    );
    print_cover_slide();
}

void print_graph_latex()
{
    fprintf(output_file, "\\begin{frame}{%s}\n", "Graph");
    fprintf(output_file, "\\begin{center}\n");
    fprintf(output_file, "\\begin{tikzpicture}[->, >=Stealth, thick, main/.style={circle, draw, minimum size=1cm}]\n\n");

    // Place nodes on a circle
    for (int i = 0; i < nodes; i++)
    {
        char node_name = 'A' + i;
        double angle = 360.0 / nodes * i;
        fprintf(output_file, "    \\node[main] (%c) at (%.2f:3cm) {%c};\n",
                node_name, angle, node_name);
    }

    fprintf(output_file, "\\path\n");

    // Draw edges where D[i][j] <= 9999
    for (int i = 0; i < nodes; i++)
    {
        for (int j = 0; j < nodes; j++)
        {
            if (i != j && D[i][j] <= 9999)
            {
                // Choose label position dynamically based on node positions
                const char *pos = "above"; // default
                if ((j - i + nodes) % nodes > nodes / 2)
                    pos = "below";

                fprintf(output_file,
                        "        (%c) edge[bend left=15] node[%s, fill=none] {%d} (%c)\n",
                        'A' + i, pos, D[i][j], 'A' + j);
            }
        }
    }

    fprintf(output_file, ";\n\\end{tikzpicture}\n");
    fprintf(output_file, "\\end{center}\n");
    fprintf(output_file, "\\end{frame}\n\n");
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
            if (table[i][j] < 99999 && changes[i][j] == 0) // No change
                fprintf(output_file, "%d", table[i][j]);
            else if (table[i][j] < 99999) // Blue highlight
                fprintf(output_file, "\\textcolor{blue}{%d}", table[i][j]);
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
}

void floyd()
{
    for (int k = 0; k < nodes; k++)
    {
        for (int i = 0; i < nodes; i++)
        {
            for (int j = 0; j < nodes; j++)
            {
                changes[i][j] = 0;
                if (i == j)
                    continue;
                if (D[i][j] > D[i][k] + D[k][j])
                {
                    D[i][j] = D[i][k] + D[k][j];
                    P[i][j] = k + 1;
                    changes[i][j] = 1;
                }
            }
        }
        char slide_title[32];
        sprintf(slide_title, "Table D(%d)", k + 1);
        print_table_latex(slide_title, D);
        print_table_latex("Table P", P);
    }
}

void print_shortest_paths()
{
    for (int i = 0; i < nodes; i++)
    {
        char slide_title[32];
        sprintf(slide_title, "Shortest Paths from v%d", i + 1);
        fprintf(output_file, "\\begin{frame}{%s}\n", slide_title);
        fprintf(output_file, "\\begin{itemize}\n    ");

        for (int j = 0; j < nodes; j++)
        {
            if (i == j)
                continue;

            fprintf(output_file, "\\item to v%d (%d): v%d $\\rightarrow$ ", j + 1, D[i][j], i + 1);

            int z = i;
            int w = j;
            while (P[z][w] != 0)
            {
                fprintf(output_file, "v%d $\\rightarrow$ ", P[z][w]);
                z = P[z][w] - 1;
            }
            fprintf(output_file, "v%d\n", j + 1);
        }
        fprintf(output_file, "\\end{itemize}\n");
        fprintf(output_file, "\\end{frame}\n\n");
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
            const char *text = gtk_entry_get_text(GTK_ENTRY(child));
            int ivalue = atoi(text);
            if (ivalue < 0)
            {
                ivalue = 99999;
            }
            D[i - 1][j - 1] = ivalue;
            P[i - 1][j - 1] = 0;
            changes[i - 1][j - 1] = 0;
        }
    }
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

    changes = (int **)malloc(nodes * sizeof(int *));
    for (int i = 0; i < nodes; i++)
        changes[i] = (int *)malloc(nodes * sizeof(int));

    build_D0();
    print_graph_latex();
    print_table_latex("Table D(0)", D);
    floyd();
    print_shortest_paths();

    for (int i = 0; i < nodes; i++)
    {
        free(D[i]);
        free(P[i]);
    }

    fprintf(output_file, "\\end{document}");
    free(D);
    free(P);
    fclose(output_file);

    system("pdflatex output.tex");
    system("evince --presentation output.pdf");
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
