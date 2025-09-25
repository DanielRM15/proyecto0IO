#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <limits.h>

FILE *output_file;
GtkWidget *main_window;
GtkWidget *mainStack;
GtkWidget *equipmentContainer;
GtkWidget *equipmentWidget;
GtkWidget *costEntry;
GtkWidget *projectSpin;
GtkWidget *lifeSpin;

int init_equipment_cost;
int term;
int lifespan;

int *maintenance_costs;
int *resale_prices;
int *profits;
int *use_profits;

int **ctx_table;
int *g_values;
int **next_replace;
int *num_optimal;

GtkWidget **maintenance_entries;
GtkWidget **resale_entries;
GtkWidget **profit_entries;
GtkWidget **profit_checks;

void on_profit_check_toggled(GtkToggleButton *toggle_button, gpointer user_data);

void setup_latex()
{
    fprintf(output_file,
            "\\documentclass[12pt,a4paper]{article}\n"
            "\\usepackage[table]{xcolor}\n"
            "\\usepackage{amsmath}\n"
            "\\usepackage{amssymb}\n"
            "\\usepackage{longtable}\n"
            "\\usepackage{pdflscape}\n"
            "\\usepackage{graphicx}\n"
            "\\usepackage{geometry}\n"
            "\\usepackage{tikz}\n"
            "\\usetikzlibrary{arrows,positioning,shapes}\n"
            "\\geometry{margin=1in}\n"
            "\n"
            "\\begin{document}\n"
            "\\begin{titlepage}\n"
            "    \\centering\n"
            "    \\vspace*{3cm}\n"
            "    {\\Huge \\textbf{Instituto Tecnológico de Costa Rica}} \\\\[2cm]\n"
            "    {\\Large \\textbf{Operations Research - Semester II}} \\\\[2cm]\n"
            "    {\\LARGE \\textbf{Equipment Replacement Problem}} \\\\[3cm]\n"
            "    {\\Large Members:} \\\\[0.5cm]\n"
            "    {\\large Adrián Zamora Chavarría \\\\ Daniel Romero Murillo} \\\\[2cm]\n"
            "    {\\large Date: \\today}\n"
            "    \\vfill\n"
            "\\end{titlepage}\n"
            "\\newpage\n");
}

int calculate_ctx(int t, int x)
{
    if (x <= t || x - t > lifespan)
    {
        return INT_MAX;
    }

    int cost = init_equipment_cost;

    // Add maintenance costs from year t+1 to x
    for (int i = 1; i <= x - t; i++)
    {
        if (i <= lifespan)
        {
            cost += maintenance_costs[i - 1];
        }
    }

    // Subtract resale price at year x-t of useful life
    if (x - t <= lifespan)
    {
        cost -= resale_prices[x - t - 1];
    }

    // If there are profits, subtract them
    if (use_profits)
    {
        for (int i = 1; i <= x - t; i++)
        {
            if (i <= lifespan && profits[i - 1] > 0)
            {
                cost -= profits[i - 1];
            }
        }
    }

    return cost;
}

// Main algorithm to solve the equipment replacement problem
void solve_equipment_replacement()
{
    // Allocate memory for tables
    ctx_table = (int **)malloc((term + 1) * sizeof(int *));
    for (int i = 0; i <= term; i++)
    {
        ctx_table[i] = (int *)malloc((term + 1) * sizeof(int));
    }

    g_values = (int *)malloc((term + 1) * sizeof(int));
    next_replace = (int **)malloc((term + 1) * sizeof(int *));
    num_optimal = (int *)malloc((term + 1) * sizeof(int));

    // Allocate memory for multiple optimal options (maximum lifespan options)
    for (int i = 0; i <= term; i++)
    {
        next_replace[i] = (int *)malloc(lifespan * sizeof(int));
        num_optimal[i] = 0;
    }

    // Calculate Ctx table
    for (int t = 0; t < term; t++)
    {
        for (int x = t + 1; x <= term; x++)
        {
            ctx_table[t][x] = calculate_ctx(t, x);
        }
    }

    // Base case: G(term) = 0
    g_values[term] = 0;
    num_optimal[term] = 0;

    // Calculate G(t) for t = term-1, term-2, etc.
    for (int t = term - 1; t >= 0; t--)
    {
        g_values[t] = INT_MAX;
        num_optimal[t] = 0;

        // Check all possible replacement times x
        for (int x = t + 1; x <= term && x <= t + lifespan; x++)
        {
            if (ctx_table[t][x] != INT_MAX)
            {
                int cost = ctx_table[t][x] + g_values[x];

                if (cost < g_values[t])
                {
                    // New best option
                    g_values[t] = cost;
                    num_optimal[t] = 1;
                    next_replace[t][0] = x;
                }
                else if (cost == g_values[t])
                {
                    // Tie: add to optimal options
                    next_replace[t][num_optimal[t]] = x;
                    num_optimal[t]++;
                }
            }
        }
    }
}

void generate_step_by_step_calculations()
{

    fprintf(output_file, "\\newpage\n");
    fprintf(output_file, "\\subsection*{Step by Step Calculations}\n");

    for (int t = term; t >= 0; t--)
    {
        if (t == term)
        {
            fprintf(output_file, "$G(%d) = 0$ \\quad (Base case)\n\n", t);
        }
        else
        {
            fprintf(output_file, "$G(%d) = \\min\\{", t);

            int valid_options = 0;

            for (int x = t + 1; x <= term && x <= t + lifespan; x++)
            {
                if (ctx_table[t][x] != INT_MAX)
                {
                    if (valid_options > 0)
                    {
                        fprintf(output_file, ", ");
                    }

                    fprintf(output_file, "C_{%d%d} + G(%d) = ", t, x, x);

                    fprintf(output_file, "(");
                    fprintf(output_file, "%d", init_equipment_cost);

                    for (int i = 1; i <= x - t; i++)
                    {
                        if (i <= lifespan)
                        {
                            fprintf(output_file, " + %d", maintenance_costs[i - 1]);
                        }
                    }

                    if (x - t <= lifespan)
                    {
                        fprintf(output_file, " - %d", resale_prices[x - t - 1]);
                    }

                    if (use_profits)
                    {
                        for (int i = 1; i <= x - t; i++)
                        {
                            if (i <= lifespan && profits[i - 1] > 0)
                            {
                                fprintf(output_file, " - %d", profits[i - 1]);
                            }
                        }
                    }

                    fprintf(output_file, ") + %d = %d", g_values[x], ctx_table[t][x] + g_values[x]);
                    valid_options++;
                }
            }

            fprintf(output_file, "\\} = %d$\n\n", g_values[t]);

            if (num_optimal[t] > 1)
            {
                fprintf(output_file, "\\textbf{Multiple optimal choices:} ");
                for (int i = 0; i < num_optimal[t]; i++)
                {
                    if (i > 0)
                        fprintf(output_file, ", ");
                    fprintf(output_file, "sell at time %d", next_replace[t][i]);
                }
                fprintf(output_file, " (tie)\n\n");
            }
            else if (num_optimal[t] == 1)
            {
                fprintf(output_file, "\\textbf{Optimal choice:} sell at time %d\n\n", next_replace[t][0]);
            }
        }
    }
}

void generate_tikz_graph(int *plan, int plan_length) {
    fprintf(output_file, "\\begin{center}\n");
    fprintf(output_file, "\\begin{tikzpicture}[node distance=1.5cm, auto]\n");
    fprintf(output_file, "\\tikzstyle{time} = [circle, draw]\n");
    fprintf(output_file, "\\tikzstyle{arrow} = [thick,->]\n");

    // Draw all nodes
    for (int t = 0; t <= term; t++) {
        if (t == 0) {
            fprintf(output_file, "\\node[time] (t%d) {%d};\n", t, t);
        } else {
            fprintf(output_file, "\\node[time, right of=t%d] (t%d) {%d};\n", t-1, t, t);
        }
    }
    
    // Draw arrows for each equipment usage period
    for (int i = 0; i < plan_length; i++) {
        int buy_time = plan[2*i];
        int sell_time = plan[2*i + 1];
        
        // Draw arc from buy_time to sell_time
        fprintf(output_file, "\\draw[arrow] (t%d) to [bend left=30] (t%d);\n", 
                buy_time, sell_time);
    }
    
    fprintf(output_file, "\\end{tikzpicture}\n");
    fprintf(output_file, "\\end{center}\n\n");
}

void generate_all_plans(int current_time, int *plan_number, int *current_plan, int plan_length)
{
    if (current_time >= term)
    {
        fprintf(output_file, "\\item \\textbf{Plan %d:} ", (*plan_number)++);
        for (int i = 0; i < plan_length; i++)
        {
            if (i > 0)
                fprintf(output_file, " $\\rightarrow$ ");
            fprintf(output_file, "Buy at %d, sell at %d",
                    current_plan[2 * i], current_plan[2 * i + 1]);
        }
        fprintf(output_file, "\n");
        
        generate_tikz_graph(current_plan, plan_length);
        
        return;
    }

    for (int i = 0; i < num_optimal[current_time]; i++)
    {
        int next_time = next_replace[current_time][i];
        current_plan[2 * plan_length] = current_time;
        current_plan[2 * plan_length + 1] = next_time;

        generate_all_plans(next_time, plan_number, current_plan, plan_length + 1);
    }
}

void generate_latex_report()
{
    // Problem description
    fprintf(output_file,
            "\\newpage\n"
            "\\section*{Equipment Replacement Problem}\n"
            "The equipment replacement problem is an optimization problem that involves deciding the optimal time to replace a piece of equipment (sell and re-buy) to minimize business costs. There are four main components to this problem:\n"
            "\n"
            "\\begin{itemize}\n"
            "    \\item The project term (e.g. 5 years long project)\n"
            "    \\item Life span of the equipment\n"
            "    \\item Price of new equipment\n"
            "    \\item Maintenance costs (could increase as equipment ages)\n"
            "    \\item Sell prices (e.g. 4 year old equipment can be sold for \\$50)\n"
            "\\end{itemize}\n"
            "\n"
            "The problem involves determining the optimal number of equipment replacements and the timing of each, in order to minimize operating costs.\n"
            "\n"
            "Given the following variables:\n"
            "\\begin{itemize}\n"
            "    \\item $t_{max}$: Project term\n"
            "    \\item $t_{span}$: Lifes span of equipment\n"
            "    \\item $t$: A given instant through the project term.\n"
            "    \\item $C_{tx}$: The cost of buying at instant $t$ and selling at $x$\n"
            "    \\begin{itemize}\n"
            "        \\item Cost of new equipment + maintenance costs - sell price - profits\n"
            "    \\end{itemize}\n"
            "\\end{itemize}\n"
            "\n"
            "G(t) = optimal business costs from $t$ to $t_{max}$ (Buying new equipment at $t$).\n"
            "\n"
            "\\subsection{Trivial case}\n"
            "G($t_{max}$) = 0\n"
            "\\begin{itemize}\n"
            "    \\item From instant $t_{max}$ to instant $t_{max}$\n"
            "    \\item We don't buy any equipment\n"
            "\\end{itemize}\n"
            "\n"
            "\\subsection{Optimization}\n"
            "If new equipment is bought at $t$ we can sell it and buy new equipment at either $t + 1$, $t + 2$ , ..., or $t + t_{span}$. We should sell at the point where we get the least operation cost given the project term.\n"
            "\n"
            "\\subsection{Bellman Equation}\n"
            "Knowing this, we can establish the Bellman equation:\n"
            "\\begin{equation}\n"
            "    G(t) = \\min \\{C_{tx} + G(x)\\} \\\\\n"
            "\\end{equation}\n"
            "\\begin{centering}\n"
            "    From $t = t_{max}$ until $t = 0$.\n"
            "\\end{centering}\n",
            init_equipment_cost, term, lifespan);

    // Equipment life cycle costs table
    fprintf(output_file,
            "\\subsection*{Equipment Life Cycle Costs}\n"
            "\\begin{center}\n"
            "\\begin{tabular}{|c|c|c|}\n"
            "\\hline\n"
            "Year & Maintenance Cost & Resale Price \\\\\n"
            "\\hline\n");

    for (int i = 0; i < lifespan; i++)
    {
        fprintf(output_file, "%d & %d & %d \\\\\n",
                i + 1, maintenance_costs[i], resale_prices[i]);
    }

    fprintf(output_file,
            "\\hline\n"
            "\\end{tabular}\n"
            "\\end{center}\n\n");

    // Ctx calculations
    fprintf(output_file,
            "\\subsection*{$C_{tx}$ Calculations}\n\n");

    // Calculate costs by usage duration to avoid repetition
    for (int duration = 1; duration <= lifespan; duration++)
    {
        fprintf(output_file, "\\textbf{%d year(s):}\n", duration);

        int cost = init_equipment_cost;
        fprintf(output_file, "$C_{t,t+%d} = %d", duration, init_equipment_cost);

        // Add maintenance costs
        for (int i = 1; i <= duration; i++)
        {
            cost += maintenance_costs[i - 1];
            fprintf(output_file, " + %d", maintenance_costs[i - 1]);
        }

        // Subtract resale price
        cost -= resale_prices[duration - 1];
        fprintf(output_file, " - %d", resale_prices[duration - 1]);

        // Subtract profits if enabled
        if (use_profits)
        {
            for (int i = 1; i <= duration; i++)
            {
                if (profits[i - 1] > 0)
                {
                    cost -= profits[i - 1];
                    fprintf(output_file, " - %d", profits[i - 1]);
                }
            }
        }

        fprintf(output_file, " = %d$\n\n", cost);
    }

    // Ctx Table
    fprintf(output_file,
            "\\subsection*{$C_{tx}$ Table}\n"
            "\\begin{center}\n"
            "\\begin{tabular}{|c|");

    for (int x = 1; x <= term; x++)
    {
        fprintf(output_file, "c|");
    }
    fprintf(output_file, "}\n\\hline\nt/x");

    for (int x = 1; x <= term; x++)
    {
        fprintf(output_file, " & %d", x);
    }
    fprintf(output_file, " \\\\\n\\hline\n");

    for (int t = 0; t < term; t++)
    {
        fprintf(output_file, "%d", t);
        for (int x = 1; x <= term; x++)
        {
            if (x > t && ctx_table[t][x] != INT_MAX)
            {
                fprintf(output_file, " & %d", ctx_table[t][x]);
            }
            else
            {
                fprintf(output_file, " & --");
            }
        }
        fprintf(output_file, " \\\\\n");
    }

    fprintf(output_file,
            "\\hline\n"
            "\\end{tabular}\n"
            "\\end{center}\n\n");

    // Step by step calculations
    generate_step_by_step_calculations();

    // Result table
    fprintf(output_file,
            "\\section*{Result Table (Analysis table)}\n"
            "\\begin{center}\n"
            "\\begin{tabular}{|c|c|c|}\n"
            "\\hline\n"
            "t & G(t) & Next Replacement \\\\\n"
            "\\hline\n");

    for (int t = 0; t <= term; t++)
    {
        if (t == term)
        {
            fprintf(output_file, "%d & %d & -- \\\\\n", t, g_values[t]);
        }
        else
        {
            fprintf(output_file, "%d & %d & ", t, g_values[t]);

            // Show all optimal options
            for (int i = 0; i < num_optimal[t]; i++)
            {
                if (i > 0)
                    fprintf(output_file, ", ");
                fprintf(output_file, "%d", next_replace[t][i]);
            }

            fprintf(output_file, " \\\\\n");
        }
    }

    fprintf(output_file,
            "\\hline\n"
            "\\end{tabular}\n"
            "\\end{center}\n\n");

    // Optimal solution
    fprintf(output_file,
            "\\section*{Optimal Solution}\n"
            "\\textbf{Minimum Total Cost:} %d\n\n",
            g_values[0]);

    fprintf(output_file, "\\textbf{All Optimal Replacement Plans:}\n\\begin{itemize}\n");

    // Generate all optimal plans
    int current_plan[20]; // Buffer for the current plan (assuming max 10 replacements)
    int plan_counter = 1;
    generate_all_plans(0, &plan_counter, current_plan, 0);

    fprintf(output_file, "\\end{itemize}\n\n");
}

void save_data_to_file()
{
    GtkWidget *dialog;
    GtkFileChooser *chooser;
    GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_SAVE;
    gint res;

    dialog = gtk_file_chooser_dialog_new("Save Equipment Data",
                                         GTK_WINDOW(main_window),
                                         action,
                                         "_Cancel",
                                         GTK_RESPONSE_CANCEL,
                                         "_Save",
                                         GTK_RESPONSE_ACCEPT,
                                         NULL);

    chooser = GTK_FILE_CHOOSER(dialog);
    
    GtkFileFilter *filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, "Equipment files (*.equip)");
    gtk_file_filter_add_pattern(filter, "*.equip");
    gtk_file_chooser_add_filter(chooser, filter);

    GtkFileFilter *all_filter = gtk_file_filter_new();
    gtk_file_filter_set_name(all_filter, "All files");
    gtk_file_filter_add_pattern(all_filter, "*");
    gtk_file_chooser_add_filter(chooser, all_filter);

    res = gtk_dialog_run(GTK_DIALOG(dialog));
    
    if (res == GTK_RESPONSE_ACCEPT)
    {
        char *chosen_filename;
        char *final_filename;
        
        chosen_filename = gtk_file_chooser_get_filename(chooser);
        
        // Check if filename already has .equip extension
        if (!g_str_has_suffix(chosen_filename, ".equip"))
        {
            final_filename = g_strdup_printf("%s.equip", chosen_filename);
            g_free(chosen_filename);
        }
        else
        {
            final_filename = chosen_filename;
        }

        FILE *file = fopen(final_filename, "w");
        if (file == NULL)
        {
            g_free(final_filename);
            gtk_widget_destroy(dialog);
            return;
        }

        fprintf(file, "%d\n", init_equipment_cost);
        fprintf(file, "%d\n", term);
        fprintf(file, "%d\n", lifespan);

        // Save maintenance costs, resale prices, and profits
        for (int i = 0; i < lifespan; i++)
        {
            int maintenance = 0, resale = 0, profit = 0, has_profit = 0;
            
            if (maintenance_entries && maintenance_entries[i])
            {
                const char *maint_text = gtk_entry_get_text(GTK_ENTRY(maintenance_entries[i]));
                maintenance = atoi(maint_text);
            }
            
            if (resale_entries && resale_entries[i])
            {
                const char *resale_text = gtk_entry_get_text(GTK_ENTRY(resale_entries[i]));
                resale = atoi(resale_text);
            }
            
            if (profit_checks && profit_checks[i])
            {
                has_profit = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(profit_checks[i]));
                if (has_profit && profit_entries && profit_entries[i])
                {
                    const char *profit_text = gtk_entry_get_text(GTK_ENTRY(profit_entries[i]));
                    profit = atoi(profit_text);
                }
            }

            fprintf(file, "%d %d %d %d\n", maintenance, resale, profit, has_profit);
        }

        fclose(file);
        
        g_free(final_filename);
    }

    gtk_widget_destroy(dialog);
}

int load_data_from_file()
{
    GtkWidget *dialog;
    GtkFileChooser *chooser;
    GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_OPEN;
    gint res;

    dialog = gtk_file_chooser_dialog_new("Open Equipment Data",
                                         GTK_WINDOW(main_window),
                                         action,
                                         "_Cancel",
                                         GTK_RESPONSE_CANCEL,
                                         "_Open",
                                         GTK_RESPONSE_ACCEPT,
                                         NULL);

    chooser = GTK_FILE_CHOOSER(dialog);

    // Add file filters
    GtkFileFilter *filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, "Equipment files (*.equip)");
    gtk_file_filter_add_pattern(filter, "*.equip");
    gtk_file_chooser_add_filter(chooser, filter);

    GtkFileFilter *all_filter = gtk_file_filter_new();
    gtk_file_filter_set_name(all_filter, "All files");
    gtk_file_filter_add_pattern(all_filter, "*");
    gtk_file_chooser_add_filter(chooser, all_filter);

    res = gtk_dialog_run(GTK_DIALOG(dialog));

    if (res == GTK_RESPONSE_ACCEPT)
    {
        char *filename;
        filename = gtk_file_chooser_get_filename(chooser);

        FILE *file = fopen(filename, "r");
        if (file == NULL)
        {
            g_free(filename);
            gtk_widget_destroy(dialog);
            return 0;
        }

        int loaded_cost, loaded_term, loaded_lifespan;
        
        if (fscanf(file, "%d", &loaded_cost) != 1 ||
            fscanf(file, "%d", &loaded_term) != 1 ||
            fscanf(file, "%d", &loaded_lifespan) != 1)
        {
            fclose(file);
            g_free(filename);
            gtk_widget_destroy(dialog);
            return 0;
        }

        char cost_str[20];
        snprintf(cost_str, sizeof(cost_str), "%d", loaded_cost);
        gtk_entry_set_text(GTK_ENTRY(costEntry), cost_str);
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(projectSpin), loaded_term);
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(lifeSpin), loaded_lifespan);

        // Update global variables
        init_equipment_cost = loaded_cost;
        term = loaded_term;
        lifespan = loaded_lifespan;

        // Recreate the equipment widgets
        maintenance_entries = (GtkWidget **)malloc(lifespan * sizeof(GtkWidget *));
        resale_entries = (GtkWidget **)malloc(lifespan * sizeof(GtkWidget *));
        profit_entries = (GtkWidget **)malloc(lifespan * sizeof(GtkWidget *));
        profit_checks = (GtkWidget **)malloc(lifespan * sizeof(GtkWidget *));

        GList *children = gtk_container_get_children(GTK_CONTAINER(equipmentContainer));
        for (GList *iter = children; iter != NULL; iter = g_list_next(iter))
        {
            gtk_widget_destroy(GTK_WIDGET(iter->data));
        }
        g_list_free(children);

        for (int i = 1; i <= lifespan; i++)
        {
            GtkBuilder *temp_builder = gtk_builder_new_from_file("equipos.glade");
            GtkWidget *cloned_widget = GTK_WIDGET(gtk_builder_get_object(temp_builder, "equipmentWidget"));

            maintenance_entries[i - 1] = GTK_WIDGET(gtk_builder_get_object(temp_builder, "maintCostEntry"));
            resale_entries[i - 1] = GTK_WIDGET(gtk_builder_get_object(temp_builder, "resaleEntry"));
            profit_entries[i - 1] = GTK_WIDGET(gtk_builder_get_object(temp_builder, "profitEntry"));
            profit_checks[i - 1] = GTK_WIDGET(gtk_builder_get_object(temp_builder, "profitCheck"));

            gtk_widget_set_sensitive(profit_entries[i - 1], FALSE);

            g_signal_connect(profit_checks[i - 1], "toggled", G_CALLBACK(on_profit_check_toggled), profit_entries[i - 1]);

            GList *widget_children = gtk_container_get_children(GTK_CONTAINER(cloned_widget));
            if (widget_children)
            {
                GtkWidget *title_label = GTK_WIDGET(widget_children->data);
                char title[50];
                snprintf(title, sizeof(title), "Year %d of Equipment Life", i);
                gtk_label_set_text(GTK_LABEL(title_label), title);
            }
            g_list_free(widget_children);

            gtk_box_pack_start(GTK_BOX(equipmentContainer), cloned_widget, FALSE, FALSE, 0);
        }

        // Load data
        for (int i = 0; i < lifespan; i++)
        {
            int maintenance, resale, profit, has_profit;
            if (fscanf(file, "%d %d %d %d", &maintenance, &resale, &profit, &has_profit) != 4)
            {
                fclose(file);
                g_free(filename);
                gtk_widget_destroy(dialog);
                return 0;
            }

            // Set maintenance cost
            char value_str[20];
            snprintf(value_str, sizeof(value_str), "%d", maintenance);
            gtk_entry_set_text(GTK_ENTRY(maintenance_entries[i]), value_str);

            // Set resale price
            snprintf(value_str, sizeof(value_str), "%d", resale);
            gtk_entry_set_text(GTK_ENTRY(resale_entries[i]), value_str);

            // Set profit checkbox and value
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(profit_checks[i]), has_profit);
            if (has_profit)
            {
                gtk_widget_set_sensitive(profit_entries[i], TRUE);
                snprintf(value_str, sizeof(value_str), "%d", profit);
                gtk_entry_set_text(GTK_ENTRY(profit_entries[i]), value_str);
            }
            else
            {
                gtk_widget_set_sensitive(profit_entries[i], FALSE);
                gtk_entry_set_text(GTK_ENTRY(profit_entries[i]), "");
            }
        }

        fclose(file);
        gtk_widget_show_all(equipmentContainer);
        
        g_free(filename);
        gtk_widget_destroy(dialog);
        return 1;
    }

    gtk_widget_destroy(dialog);
    return 0;
}

void on_profit_check_toggled(GtkToggleButton *toggle_button, gpointer user_data)
{
    GtkWidget *profit_entry = GTK_WIDGET(user_data);
    gboolean is_active = gtk_toggle_button_get_active(toggle_button);

    gtk_widget_set_sensitive(profit_entry, is_active);
    if (!is_active)
    {
        gtk_entry_set_text(GTK_ENTRY(profit_entry), "");
    }
}

void on_continueBtn_clicked(GtkButton *button, gpointer user_data)
{
    const char *cost_text = gtk_entry_get_text(GTK_ENTRY(costEntry));
    init_equipment_cost = atoi(cost_text);

    term = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(projectSpin));
    lifespan = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(lifeSpin));

    if (strlen(cost_text) == 0 || init_equipment_cost <= 0)
    {
        GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(main_window),
                                                   GTK_DIALOG_MODAL,
                                                   GTK_MESSAGE_ERROR,
                                                   GTK_BUTTONS_OK,
                                                   "Please enter a valid initial equipment cost greater than 0.");
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        return;
    }

    maintenance_entries = (GtkWidget **)malloc(lifespan * sizeof(GtkWidget *));
    resale_entries = (GtkWidget **)malloc(lifespan * sizeof(GtkWidget *));
    profit_entries = (GtkWidget **)malloc(lifespan * sizeof(GtkWidget *));
    profit_checks = (GtkWidget **)malloc(lifespan * sizeof(GtkWidget *));

    GList *children = gtk_container_get_children(GTK_CONTAINER(equipmentContainer));
    for (GList *iter = children; iter != NULL; iter = g_list_next(iter))
    {
        gtk_widget_destroy(GTK_WIDGET(iter->data));
    }
    g_list_free(children);

    for (int i = 1; i <= lifespan; i++)
    {
        GtkBuilder *temp_builder = gtk_builder_new_from_file("equipos.glade");
        GtkWidget *cloned_widget = GTK_WIDGET(gtk_builder_get_object(temp_builder, "equipmentWidget"));

        maintenance_entries[i - 1] = GTK_WIDGET(gtk_builder_get_object(temp_builder, "maintCostEntry"));
        resale_entries[i - 1] = GTK_WIDGET(gtk_builder_get_object(temp_builder, "resaleEntry"));
        profit_entries[i - 1] = GTK_WIDGET(gtk_builder_get_object(temp_builder, "profitEntry"));
        profit_checks[i - 1] = GTK_WIDGET(gtk_builder_get_object(temp_builder, "profitCheck"));

        gtk_widget_set_sensitive(profit_entries[i - 1], FALSE);

        g_signal_connect(profit_checks[i - 1], "toggled", G_CALLBACK(on_profit_check_toggled), profit_entries[i - 1]);

        GList *widget_children = gtk_container_get_children(GTK_CONTAINER(cloned_widget));
        if (widget_children)
        {
            GtkWidget *title_label = GTK_WIDGET(widget_children->data);
            char title[50];
            snprintf(title, sizeof(title), "Year %d of Equipment Life", i);
            gtk_label_set_text(GTK_LABEL(title_label), title);
        }
        g_list_free(widget_children);

        gtk_box_pack_start(GTK_BOX(equipmentContainer), cloned_widget, FALSE, FALSE, 0);
    }

    gtk_widget_show_all(equipmentContainer);

    gtk_stack_set_visible_child_name(GTK_STACK(mainStack), "page1");
}

void on_runBtn_clicked(GtkButton *button, gpointer user_data)
{
    // Data for maintenance and resale
    maintenance_costs = (int *)malloc(lifespan * sizeof(int));
    resale_prices = (int *)malloc(lifespan * sizeof(int));
    profits = (int *)malloc(lifespan * sizeof(int));
    use_profits = 0;

    for (int i = 0; i < lifespan; i++)
    {
        const char *maint_text = gtk_entry_get_text(GTK_ENTRY(maintenance_entries[i]));
        const char *resale_text = gtk_entry_get_text(GTK_ENTRY(resale_entries[i]));

        maintenance_costs[i] = atoi(maint_text);
        resale_prices[i] = atoi(resale_text);

        // Check if profit is enabled
        if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(profit_checks[i])))
        {
            const char *profit_text = gtk_entry_get_text(GTK_ENTRY(profit_entries[i]));
            profits[i] = atoi(profit_text);
            use_profits = 1;
        }
        else
        {
            profits[i] = 0;
        }
    }

    // Run the algorithm
    solve_equipment_replacement();

    // Generate report
    output_file = fopen("output.tex", "w");
    if (output_file == NULL)
    {
        g_print("Failed to open LaTeX file");
        return;
    }

    setup_latex();
    generate_latex_report();

    fprintf(output_file, "\\end{document}");
    fclose(output_file);

    system("pdflatex output.tex");
    system("evince --presentation output.pdf &");

    g_print("Algorithm executed. Minimum cost: %d\n", g_values[0]);
}

void on_saveBtn_clicked(GtkButton *button, gpointer user_data)
{
    const char *cost_text = gtk_entry_get_text(GTK_ENTRY(costEntry));
    if (strlen(cost_text) == 0 || atoi(cost_text) <= 0)
    {
        GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(main_window),
                                                   GTK_DIALOG_MODAL,
                                                   GTK_MESSAGE_ERROR,
                                                   GTK_BUTTONS_OK,
                                                   "Please enter valid equipment data before saving.");
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        return;
    }

    init_equipment_cost = atoi(cost_text);
    term = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(projectSpin));
    lifespan = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(lifeSpin));

    save_data_to_file();
}

void on_loadBtn_clicked(GtkButton *button, gpointer user_data)
{
    if (load_data_from_file())
    {
        gtk_stack_set_visible_child_name(GTK_STACK(mainStack), "page1");
    }
}

int main(int argc, char *argv[])
{
    gtk_init(&argc, &argv);

    // GtkBuilder *builder = gtk_builder_new_from_file("Equipos/equipos.glade"); // Si se abre desde el menu
    GtkBuilder *builder = gtk_builder_new_from_file("equipos.glade"); // Si se abre SIN en menu

    main_window = GTK_WIDGET(gtk_builder_get_object(builder, "hWindow"));
    mainStack = GTK_WIDGET(gtk_builder_get_object(builder, "mainStack"));
    equipmentContainer = GTK_WIDGET(gtk_builder_get_object(builder, "equipmentContainer"));
    equipmentWidget = GTK_WIDGET(gtk_builder_get_object(builder, "equipmentWidget"));

    gtk_builder_connect_signals(builder, NULL);

    costEntry = GTK_WIDGET(gtk_builder_get_object(builder, "costEntry"));
    projectSpin = GTK_WIDGET(gtk_builder_get_object(builder, "projectSpin"));
    lifeSpin = GTK_WIDGET(gtk_builder_get_object(builder, "lifeSpin"));

    // exit
    g_signal_connect(main_window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    gtk_widget_show_all(main_window);

    gtk_main();

    g_object_unref(builder);

    return 0;
}
