#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

GtkWidget *num_nodes_spin;
GtkWidget *main_stack;
GtkWidget *distance_table_container;
GtkWidget *distance_input_grid;
GtkWidget *main_window;
int nodes = 0;

FILE *output_file;
int **D;
int **P;
int **changes;
char **node_names;


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

static void on_entry_changed(GtkEditable *editable, gpointer user_data) {
    const gchar *text = gtk_entry_get_text(GTK_ENTRY(editable));

    if (g_strcmp0(text, "-1") == 0) {
        // block validator + self
        g_signal_handlers_block_by_func(editable, on_insert_text, NULL);
        g_signal_handlers_block_by_func(editable, on_entry_changed, user_data);

        gtk_entry_set_text(GTK_ENTRY(editable), "∞");

        // unblock
        g_signal_handlers_unblock_by_func(editable, on_insert_text, NULL);
        g_signal_handlers_unblock_by_func(editable, on_entry_changed, user_data);
    }
}

static void on_name_changed(GtkEditable *editable, gpointer user_data) {
    GtkEntry *partner = GTK_ENTRY(user_data);
    const gchar *text = gtk_entry_get_text(GTK_ENTRY(editable));

    g_signal_handlers_block_by_func(partner, on_name_changed, editable);
    gtk_entry_set_text(partner, text);
    g_signal_handlers_unblock_by_func(partner, on_name_changed, editable);
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
                to_attach = gtk_entry_new();
                gtk_entry_set_width_chars(GTK_ENTRY(to_attach), 6);
                gtk_entry_set_max_length(GTK_ENTRY(to_attach), 5);
                snprintf(buf, sizeof(buf), "%c", 'A' + (j - 1));
                gtk_entry_set_text(GTK_ENTRY(to_attach), buf);

                g_object_set_data(G_OBJECT(to_attach), "node-index", GINT_TO_POINTER(j));
                gtk_grid_attach(GTK_GRID(distance_input_grid), to_attach, j, i, 1, 1);
                continue;
            }
            if (j == 0 && i != 0)
            {
                to_attach = gtk_entry_new();
                gtk_entry_set_width_chars(GTK_ENTRY(to_attach), 6);
                gtk_entry_set_max_length(GTK_ENTRY(to_attach), 5);
                snprintf(buf, sizeof(buf), "%c", 'A' + (i - 1));
                gtk_entry_set_text(GTK_ENTRY(to_attach), buf);

                GtkWidget *col_entry = gtk_grid_get_child_at(GTK_GRID(distance_input_grid), i, 0);

                g_signal_connect(to_attach, "changed", G_CALLBACK(on_name_changed), col_entry);
                g_signal_connect(col_entry, "changed", G_CALLBACK(on_name_changed), to_attach);

                gtk_grid_attach(GTK_GRID(distance_input_grid), to_attach, j, i, 1, 1);
                continue;
            }
            to_attach = gtk_entry_new();
            gtk_entry_set_width_chars(GTK_ENTRY(to_attach), 6);
            gtk_entry_set_max_length(GTK_ENTRY(to_attach), 5);
            gtk_entry_set_input_purpose(GTK_ENTRY(to_attach), GTK_INPUT_PURPOSE_DIGITS);
            
            // Connect the insert-text signal to validate input
            g_signal_connect(to_attach, "insert-text", G_CALLBACK(on_insert_text), NULL);

            g_signal_connect(to_attach, "changed", G_CALLBACK(on_entry_changed), NULL);
            
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
        double angle = 360.0 / nodes * i;
        fprintf(output_file, "    \\node[main] (%s) at (%.2f:3cm) {%s};\n",
                node_names[i], angle, node_names[i]);
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
                        "        (%s) edge[bend left=15] node[%s, fill=none] {%d} (%s)\n",
                        node_names[i], pos, D[i][j], node_names[j]);
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
        fprintf(output_file, "%s", node_names[j]);
        if (j < nodes - 1)
            fprintf(output_file, " & ");
    }
    fprintf(output_file, " \\\\\n\\hline\n");
    for (int i = 0; i < nodes; i++)
    {
        fprintf(output_file, "%s & ", node_names[i]);
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
        char slide_title[64];
        sprintf(slide_title, "Table D(%s)", node_names[k]);
        print_table_latex(slide_title, D);
        print_table_latex("Table P", P);
    }
}

void print_shortest_paths()
{
    for (int i = 0; i < nodes; i++)
    {
        char slide_title[64];
        sprintf(slide_title, "Shortest Paths from %s", node_names[i]);
        fprintf(output_file, "\\begin{frame}{%s}\n", slide_title);
        fprintf(output_file, "\\begin{itemize}\n    ");

        for (int j = 0; j < nodes; j++)
        {
            if (i == j)
                continue;

            fprintf(output_file, "\\item to %s (%d): %s $\\rightarrow$ ", node_names[j], D[i][j], node_names[i]);

            int z = i;
            int w = j;
            while (P[z][w] != 0)
            {
                fprintf(output_file, "%s $\\rightarrow$ ", node_names[P[z][w] - 1]);
                z = P[z][w] - 1;
            }
            fprintf(output_file, "%s\n", node_names[j]);
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
    // Allocate memory for node names
    node_names = (char **)malloc(nodes * sizeof(char *));
    for (int i = 0; i < nodes; i++)
        node_names[i] = (char *)malloc(11 * sizeof(char)); // 10 chars + null terminator (max 10 chars)
    
    // Extract node names from the first row
    for (int j = 1; j <= nodes; j++)
    {
        GtkWidget *child = gtk_grid_get_child_at(GTK_GRID(distance_input_grid), j, 0);
        const char *text = gtk_entry_get_text(GTK_ENTRY(child));
        strcpy(node_names[j - 1], text);
    }
    
    for (int i = 1; i <= nodes; i++)
    {
        for (int j = 1; j <= nodes; j++)
        {
            GtkWidget *child = gtk_grid_get_child_at(GTK_GRID(distance_input_grid), j, i);
            const char *text = gtk_entry_get_text(GTK_ENTRY(child));
            int ivalue;
            if (g_strcmp0(text, "∞") == 0) {
                ivalue = 99999;
            } else {
                ivalue = atoi(text);
                if (ivalue < 0) ivalue = 99999;
            }
            D[i - 1][j - 1] = ivalue;
            P[i - 1][j - 1] = 0;
            changes[i - 1][j - 1] = 0;
        }
    }
}

int validate_node_names()
{
    // Check for empty names and collect all names
    char temp_names[nodes][11];
    
    for (int j = 1; j <= nodes; j++)
    {
        GtkWidget *child = gtk_grid_get_child_at(GTK_GRID(distance_input_grid), j, 0);
        const char *text = gtk_entry_get_text(GTK_ENTRY(child));
        
        // Check if name is empty or contains only whitespace
        if (text == NULL || strlen(text) == 0 || strspn(text, " \t\n\r") == strlen(text))
        {
            GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(main_window),
                GTK_DIALOG_MODAL,
                GTK_MESSAGE_ERROR,
                GTK_BUTTONS_OK,
                "Node name cannot be empty. Please provide a name for all nodes.");
            gtk_dialog_run(GTK_DIALOG(dialog));
            gtk_widget_destroy(dialog);
            return 0; // Validation failed
        }
        
        // Store the name for duplicate checking
        strncpy(temp_names[j-1], text, 10);
        temp_names[j-1][10] = '\0'; // Ensure null termination
    }
    
    // Check for duplicates
    for (int i = 0; i < nodes; i++)
    {
        for (int j = i + 1; j < nodes; j++)
        {
            if (strcmp(temp_names[i], temp_names[j]) == 0)
            {
                GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(main_window),
                    GTK_DIALOG_MODAL,
                    GTK_MESSAGE_ERROR,
                    GTK_BUTTONS_OK,
                    "Duplicate node name '%s' found. Each node must have a unique name.",
                    temp_names[i]);
                gtk_dialog_run(GTK_DIALOG(dialog));
                gtk_widget_destroy(dialog);
                return 0; // Validation failed
            }
        }
    }
    
    return 1; // Validation passed
}

void on_runBtn_clicked(GtkButton *button, gpointer user_data)
{   
    if (!validate_node_names())
    {
        return; // Stop execution if validation fails
    }

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
        free(changes[i]);
        free(node_names[i]);
    }

    fprintf(output_file, "\\end{document}");
    free(D);
    free(P);
    free(changes);
    free(node_names);
    fclose(output_file);

    system("pdflatex output.tex");
    system("evince --presentation output.pdf");
}

void on_loadBtn_clicked(GtkButton *button, gpointer user_data)
{
    GtkWidget *dialog;
    GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_OPEN;
    gint res;

    dialog = gtk_file_chooser_dialog_new("Open Floyd File",
                                       NULL,
                                       action,
                                       "_Cancel", GTK_RESPONSE_CANCEL,
                                       "_Open", GTK_RESPONSE_ACCEPT,
                                       NULL);

    // Only .floyd files
    GtkFileFilter *filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, "Floyd files (*.floyd)");
    gtk_file_filter_add_pattern(filter, "*.floyd");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);

    res = gtk_dialog_run(GTK_DIALOG(dialog));
    if (res == GTK_RESPONSE_ACCEPT)
    {
        char *filename;
        GtkFileChooser *chooser = GTK_FILE_CHOOSER(dialog);
        filename = gtk_file_chooser_get_filename(chooser);

        FILE *file = fopen(filename, "r");
        if (file == NULL)
        {
            GtkWidget *error_dialog = gtk_message_dialog_new(NULL,
                GTK_DIALOG_MODAL,
                GTK_MESSAGE_ERROR,
                GTK_BUTTONS_OK,
                "Error opening file: %s", filename);
            gtk_dialog_run(GTK_DIALOG(error_dialog));
            gtk_widget_destroy(error_dialog);
            g_free(filename);
            gtk_widget_destroy(dialog);
            return;
        }

        // Read number of nodes
        int loaded_nodes;
        if (fscanf(file, "%d", &loaded_nodes) != 1)
        {
            GtkWidget *error_dialog = gtk_message_dialog_new(GTK_WINDOW(main_window),
                GTK_DIALOG_MODAL,
                GTK_MESSAGE_ERROR,
                GTK_BUTTONS_OK,
                "Invalid file format");
            gtk_dialog_run(GTK_DIALOG(error_dialog));
            gtk_widget_destroy(error_dialog);
            fclose(file);
            g_free(filename);
            gtk_widget_destroy(dialog);
            return;
        }

        // Set the spin button value
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(num_nodes_spin), loaded_nodes);
        nodes = loaded_nodes;

        // Create the table
        create_distance_table();

        // Read node names and distances
        for (int i = 0; i < nodes; i++)
        {
            char node_name[6];
            if (fscanf(file, "%5s", node_name) != 1)
            {
                GtkWidget *error_dialog = gtk_message_dialog_new(GTK_WINDOW(main_window),
                    GTK_DIALOG_MODAL,
                    GTK_MESSAGE_ERROR,
                    GTK_BUTTONS_OK,
                    "Error reading node names from file");
                gtk_dialog_run(GTK_DIALOG(error_dialog));
                gtk_widget_destroy(error_dialog);
                fclose(file);
                g_free(filename);
                gtk_widget_destroy(dialog);
                return;
            }

            // Set node name in row and column headers
            GtkWidget *col_entry = gtk_grid_get_child_at(GTK_GRID(distance_input_grid), i + 1, 0);
            GtkWidget *row_entry = gtk_grid_get_child_at(GTK_GRID(distance_input_grid), 0, i + 1);
            
            g_signal_handlers_block_by_func(col_entry, on_name_changed, row_entry);
            g_signal_handlers_block_by_func(row_entry, on_name_changed, col_entry);
            
            gtk_entry_set_text(GTK_ENTRY(col_entry), node_name);
            gtk_entry_set_text(GTK_ENTRY(row_entry), node_name);
            
            g_signal_handlers_unblock_by_func(col_entry, on_name_changed, row_entry);
            g_signal_handlers_unblock_by_func(row_entry, on_name_changed, col_entry);
        }

        // Read distances
        for (int i = 0; i < nodes; i++)
        {
            for (int j = 0; j < nodes; j++)
            {
                int distance;
                if (fscanf(file, "%d", &distance) != 1)
                {
                    GtkWidget *error_dialog = gtk_message_dialog_new(GTK_WINDOW(main_window),
                        GTK_DIALOG_MODAL,
                        GTK_MESSAGE_ERROR,
                        GTK_BUTTONS_OK,
                        "Error reading distances from file");
                    gtk_dialog_run(GTK_DIALOG(error_dialog));
                    gtk_widget_destroy(error_dialog);
                    fclose(file);
                    g_free(filename);
                    gtk_widget_destroy(dialog);
                    return;
                }

                GtkWidget *entry = gtk_grid_get_child_at(GTK_GRID(distance_input_grid), j + 1, i + 1);
                if (entry && GTK_IS_ENTRY(entry))
                {
                    g_signal_handlers_block_by_func(entry, on_entry_changed, NULL);
                    
                    if (distance >= 99999)
                        gtk_entry_set_text(GTK_ENTRY(entry), "∞");
                    else
                    {
                        char dist_str[10];
                        snprintf(dist_str, sizeof(dist_str), "%d", distance);
                        gtk_entry_set_text(GTK_ENTRY(entry), dist_str);
                    }
                    
                    g_signal_handlers_unblock_by_func(entry, on_entry_changed, NULL);
                }
            }
        }

        fclose(file);
        g_free(filename);

        // Switch to the distance input page
        gtk_stack_set_visible_child_name(GTK_STACK(main_stack), "page1");
    }

    gtk_widget_destroy(dialog);
}

void on_saveBtn_clicked(GtkButton *button, gpointer user_data)
{
    // Check if we have a valid table to save
    if (nodes == 0)
    {
        GtkWidget *error_dialog = gtk_message_dialog_new(GTK_WINDOW(main_window),
            GTK_DIALOG_MODAL,
            GTK_MESSAGE_WARNING,
            GTK_BUTTONS_OK,
            "No data to save. Please create a table first.");
        gtk_dialog_run(GTK_DIALOG(error_dialog));
        gtk_widget_destroy(error_dialog);
        return;
    }

    GtkWidget *dialog;
    GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_SAVE;
    gint res;

    dialog = gtk_file_chooser_dialog_new("Save Floyd File",
                                       NULL,
                                       action,
                                       "_Cancel", GTK_RESPONSE_CANCEL,
                                       "_Save", GTK_RESPONSE_ACCEPT,
                                       NULL);

    // Set file filter for .floyd files
    GtkFileFilter *filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, "Floyd files (*.floyd)");
    gtk_file_filter_add_pattern(filter, "*.floyd");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);

    // Set default extension
    gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dialog), TRUE);

    res = gtk_dialog_run(GTK_DIALOG(dialog));
    if (res == GTK_RESPONSE_ACCEPT)
    {
        char *filename;
        GtkFileChooser *chooser = GTK_FILE_CHOOSER(dialog);
        filename = gtk_file_chooser_get_filename(chooser);

        // Add .floyd extension
        char *final_filename;
        if (!g_str_has_suffix(filename, ".floyd"))
        {
            final_filename = g_strconcat(filename, ".floyd", NULL);
        }
        else
        {
            final_filename = g_strdup(filename);
        }

        FILE *file = fopen(final_filename, "w");
        if (file == NULL)
        {
            GtkWidget *error_dialog = gtk_message_dialog_new(GTK_WINDOW(main_window),
                GTK_DIALOG_MODAL,
                GTK_MESSAGE_ERROR,
                GTK_BUTTONS_OK,
                "Error creating file: %s", final_filename);
            gtk_dialog_run(GTK_DIALOG(error_dialog));
            gtk_widget_destroy(error_dialog);
            g_free(filename);
            g_free(final_filename);
            gtk_widget_destroy(dialog);
            return;
        }

        // Save number of nodes
        fprintf(file, "%d\n", nodes);

        // Save node names
        for (int i = 1; i <= nodes; i++)
        {
            GtkWidget *entry = gtk_grid_get_child_at(GTK_GRID(distance_input_grid), i, 0);
            const char *node_name = gtk_entry_get_text(GTK_ENTRY(entry));
            fprintf(file, "%s ", node_name);
        }
        fprintf(file, "\n");

        // Save distances
        for (int i = 1; i <= nodes; i++)
        {
            for (int j = 1; j <= nodes; j++)
            {
                GtkWidget *entry = gtk_grid_get_child_at(GTK_GRID(distance_input_grid), j, i);
                const char *text = gtk_entry_get_text(GTK_ENTRY(entry));
                
                int distance;
                if (g_strcmp0(text, "∞") == 0)
                    distance = 99999;
                else
                    distance = atoi(text);
                
                fprintf(file, "%d ", distance);
            }
            fprintf(file, "\n");
        }

        fclose(file);
        g_free(filename);
        g_free(final_filename);
    }

    gtk_widget_destroy(dialog);
}

int main(int argc, char *argv[])
{
    gtk_init(&argc, &argv);

    // GtkBuilder *builder = gtk_builder_new_from_file("Floyd/floyd.glade"); //Si se abre desde el menu
    GtkBuilder *builder = gtk_builder_new_from_file("floyd.glade"); // Si se abre SIN en menu

    main_window = GTK_WIDGET(gtk_builder_get_object(builder, "hWindow"));

    gtk_builder_connect_signals(builder, NULL);

    // exit
    g_signal_connect(main_window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    main_stack = GTK_WIDGET(gtk_builder_get_object(builder, "mainStack"));
    num_nodes_spin = GTK_WIDGET(gtk_builder_get_object(builder, "numNodesSpin"));
    distance_input_grid = GTK_WIDGET(gtk_builder_get_object(builder, "distance_input_grid"));

    gtk_widget_show_all(main_window);

    gtk_main();

    g_object_unref(builder);

    return 0;
}
