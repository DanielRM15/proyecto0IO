#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

FILE *output_file;
GtkWidget *main_window;
GtkWidget *main_stack;
GtkWidget *variables_container;
GtkWidget *objective_container;
GtkWidget *constraints_container;
GtkWidget *problem_input; // problem name
GtkWidget *variables_spin;
GtkWidget *constraints_spin;
GtkWidget *constraint_page_label;

GtkWidget *variable_widgets[15];
GtkWidget *coefficient_widgets[15];
char *variable_names[15];

char *problem_name;
int variable_amount;
int constraint_amount;
int mode = 0;				 // 0 for max, 1 for min;
int intermediate_tables = 0; // 1 yes, 0 no
int is_degenerate = 0;		 // 0 no, 1 yes
int constraint_page_count = 0;

double **simplex_table;
int table_cols;
int table_rows;

void setup_latex()
{
	fprintf(output_file,
			"\\documentclass[12pt,a4paper]{article}\n"
			"\\usepackage[table]{xcolor}\n"
			"\\usepackage{float}\n"
			"\\usepackage{geometry}\n"
			"\\geometry{margin=1in}\n"
			"\n"
			"\\begin{document}\n"
			"\\begin{titlepage}\n"
			"    \\centering\n"
			"    \\vspace*{3cm}\n"
			"    {\\Huge \\textbf{Instituto Tecnológico de Costa Rica}} \\\\[2cm]\n"
			"    {\\Large \\textbf{Operations Research - Semester II}} \\\\[2cm]\n"
			"    {\\LARGE \\textbf{Knapsack Problem}} \\\\[3cm]\n"
			"    {\\Large Members:} \\\\[0.5cm]\n"
			"    {\\large Adrián Zamora Chavarría \\\\ Daniel Romero Murillo} \\\\[2cm]\n"
			"    {\\large Date: \\today}\n"
			"    \\vfill\n"
			"\\end{titlepage}\n"
			"\\newpage\n");
}

void print_simplex_table(int highlight_row, int highlight_col, int print_fractions)
{
	fprintf(output_file, "\\begin{table}[H]\n");
	fprintf(output_file, "\\centering\n");
	fprintf(output_file, "\\begin{tabular}{c|");
	for (int i = 0; i < table_cols; i++)
		fprintf(output_file, "r");
	if (print_fractions)
		fprintf(output_file, "r");

	fprintf(output_file, "}\n");
	fprintf(output_file, "& Z ");
	for (int i = 0; i < variable_amount; i++)
		fprintf(output_file, "& %s", variable_names[i]);
	for (int i = 0; i < constraint_amount; i++)
		fprintf(output_file, "& $s_%d$", i + 1);
	fprintf(output_file, "& b");
	if (print_fractions)
		fprintf(output_file, "& Frac");
	fprintf(output_file, "\\\\ \\hline\n");

	for (int i = 0; i < table_rows; i++)
	{
		for (int j = 0; j < table_cols; j++)
		{
			fprintf(output_file, "& ");
			if ((highlight_row != -1 && highlight_col != -1 && i == highlight_row && j == highlight_col) ||
				(highlight_row != -1 && highlight_col == -1 && i == highlight_row) ||
				(highlight_col != -1 && highlight_row == -1 && j == highlight_col))
			{
				fprintf(output_file, "\\cellcolor{cyan!30} ");
			}
			fprintf(output_file, "%.2f", simplex_table[i][j]);
		}
		if (print_fractions)
		{
			fprintf(output_file, "& ");
			if (i > 0)
				fprintf(output_file, "%.2f", simplex_table[i][table_cols - 1] / simplex_table[i][print_fractions]);
		}
		fprintf(output_file, "\\\\\n");
	}
	fprintf(output_file, "\\end{tabular}\n");
	fprintf(output_file, "\\end{table}\n");
}

void print_problem_model()
{
	fprintf(output_file, "\\section*{");
	fprintf(output_file, problem_name);
	fprintf(output_file, "}\n");
	if (mode)
		fprintf(output_file, "Minimize\n\\begin{center}\nZ = ");
	else
		fprintf(output_file, "Maximize\n\\begin{center}\nZ = ");

	for (int i = 1; i <= variable_amount; i++)
	{
		double val = -simplex_table[0][i];
		if (val < 0)
		{
			fprintf(output_file, "- ");
			val *= -1;
		}
		else
		{
			if (i > 1)
				fprintf(output_file, "+ ");
		}
		fprintf(output_file, "%.2f%s ", val, variable_names[i - 1]);
	}
	fprintf(output_file, "\n\\end{center}\nSubject to\n\\begin{center}\n");
	for (int i = 1; i <= constraint_amount; i++)
	{
		for (int j = 0; j < variable_amount; j++)
		{
			double val = simplex_table[i][j + 1];
			if (val < 0)
			{
				fprintf(output_file, "- ");
				val *= -1;
			}
			else
			{
				if (j != 0)
					fprintf(output_file, "+ ");
			}

			if (j < variable_amount)
				fprintf(output_file, "%.2f%s ", val, variable_names[j]);
			// else // Print slack variables
			// 	fprintf(output_file, "%.2f$s_%d$ ", val, j - variable_amount + 1);
		}
		double val = simplex_table[i][table_cols - 1];
		fprintf(output_file, "$\\leq$ %.2f \\\\\n", val);
	}
	fprintf(output_file, "\\end{center}\n");
	fprintf(output_file, "Simplex Table\n");
	print_simplex_table(-1, -1, 0);
}

void print_results()
{
	fprintf(output_file, "\\section*{Results}\n");
	print_simplex_table(-1, -1, 0);
	fprintf(output_file, "\n\\subsection*{Objective Value}\n");
	fprintf(output_file, "Z = %.2f\n", simplex_table[0][table_cols - 1]);
	fprintf(output_file, "\n\\subsection*{Variables}\n");
	for (int i = 0; i < variable_amount; i++)
	{
		double var_value = 0;
		for (int j = 0; j < constraint_amount + 1; j++)
		{
			if (simplex_table[j][i + 1] != 0)
			{
				if (simplex_table[j][i + 1] != 1)
				{
					var_value = 0;
					break;
				}
				else
					var_value = simplex_table[j][table_cols - 1];
			}
		}
		fprintf(output_file, variable_names[i]);
		fprintf(output_file, " = %.2f \\\\\n", var_value);
	}
	fprintf(output_file, "\n\\subsection*{Slack or Surplus}\n");
	for (int i = 0; i < constraint_amount; i++)
	{
		double var_value = 0;
		for (int j = 0; j < constraint_amount + 1; j++)
		{
			if (simplex_table[j][i + variable_amount + 1] != 0)
			{
				if (simplex_table[j][i + variable_amount + 1] != 1)
				{
					var_value = 0;
					break;
				}
				else
					var_value = simplex_table[j][table_cols - 1];
			}
		}
		fprintf(output_file, "$s_%d$", i + 1);
		fprintf(output_file, " = %.2f \\\\\n", var_value);
	}
}

void report_unbounded()
{
	fprintf(output_file, "\n\\section*{Unbounded Problem!}\n");
	fprintf(output_file, "This problema is unbounded. Please re-model it and try again!\n");
}

void check_degeneracy()
{
	for (int j = 0; j < variable_amount + constraint_amount; j++)
	{
		double var_value = 0;
		int is_basic = 0;
		for (int i = 1; i < table_rows; i++)
		{
			double val = simplex_table[i][j + 1];
			if (fabs(val) < 1e-9)
				val = 0.0;
			if (val != 0.0)
			{
				if (fabs(val - 1.0) < 1e-9)
				{
					var_value = simplex_table[i][table_cols - 1];
					is_basic = 1;
					if (fabs(var_value) < 1e-9)
						var_value = 0.0;
				}
				else
				{
					is_basic = 0;
					break;
				}
			}
		}
		if (is_basic && fabs(var_value) < 1e-9) // var_value == 0
		{
			fprintf(output_file, "\n\\subsection*{Degenerate Base}\n");
			if (j + 1 < variable_amount + 1)
				fprintf(output_file, "The variable %s is part of the base but has a value of 0. \n", variable_names[j]);
			else
				fprintf(output_file, "The variable $s_%d$ is part of the base but has a value of 0. ", j - variable_amount + 1);
			fprintf(output_file, "Therefore, this is a degenerate Basic Feasible Solution (BFS).\n\n", j - variable_amount + 1);
			break;
		}
	}
}

void multiple_solutions(int pivoting)
{
	// Check for multiple solutions
	int is_basic = 1;
	int pivot_col = 0;
	for (int i = 0; i < variable_amount + constraint_amount; i++)
	{
		if (fabs(simplex_table[0][i + 1]) > 1e-9)
			continue;
		for (int j = 1; j < table_rows; j++)
		{
			double val = simplex_table[j][i + 1];
			if (fabs(val) < 1e-9)
				val = 0.0;
			if (val != 0.0)
			{
				if (fabs(val - 1.0) < 1e-9)
				{
					is_basic = 1;
				}
				else
				{
					is_basic = 0;
					pivot_col = i + 1;
					break;
				}
			}
		}
		if (!is_basic)
			break;
	}

	// pivot again if possible
	if (!is_basic)
	{
		fprintf(output_file, "\\newpage\n\\section*{Multiple Solutions}\n");
		fprintf(output_file, "A basic variable has a 0 on its first row, allowing us to pivot again and find another optimal solution.\n\n");

		if (intermediate_tables) // Print intermediate tables data
		{
			fprintf(output_file, "\\subsection*{Pivoting %d}", pivoting);
			if (mode)
				fprintf(output_file, "\\subsection*{Most Positive}\n");
			else
				fprintf(output_file, "\\subsubsection*{Most Negative}\n");
			fprintf(output_file, "Column %d (%.2f)\n", pivot_col + 1, simplex_table[0][pivot_col]);
			print_simplex_table(0, pivot_col, 0);
			fprintf(output_file, "\\subsubsection*{Fractions}\n");
		}

		if (intermediate_tables)
			print_simplex_table(-1, -1, pivot_col);
		int smallest_frac = 1; // The smallest fraction row
		int is_unbounded = 1;
		for (int i = 1; i < table_rows; i++)
		{
			double frac = simplex_table[i][table_cols - 1] / simplex_table[i][pivot_col];
			if (intermediate_tables)
				fprintf(output_file, "$%.2f / %.2f = %.2f$ \\\\\n", simplex_table[i][table_cols - 1], simplex_table[i][pivot_col], frac);
			if (frac <= 1e-4)
				continue;
			else
				is_unbounded = 0;
			if (frac <= simplex_table[smallest_frac][table_cols - 1] / simplex_table[smallest_frac][pivot_col] ||
				simplex_table[smallest_frac][table_cols - 1] / simplex_table[smallest_frac][pivot_col] < 0)
				smallest_frac = i;
		}
		if (is_unbounded)
		{
			report_unbounded();
			return;
		}
		if (intermediate_tables)
		{
			fprintf(output_file, "Smallest fraction: %.2f", simplex_table[smallest_frac][table_cols - 1] / simplex_table[smallest_frac][pivot_col]);
			fprintf(output_file, " $\\rightarrow$ Pivot: row %d", smallest_frac + 1);
			fprintf(output_file, "\\subsubsection*{Pivot}\n");
			print_simplex_table(smallest_frac, pivot_col, 0);
		}

		// Make a 1 on smallest frac cell
		double div_value = simplex_table[smallest_frac][pivot_col];
		for (int i = 0; i < table_cols; i++)
		{
			simplex_table[smallest_frac][i] = simplex_table[smallest_frac][i] / div_value;
		}
		if (intermediate_tables)
		{
			fprintf(output_file, "\\subsubsection*{Canonization}\n", pivoting);
			fprintf(output_file, "$R_%d \\leftarrow R_%d/%.2f$ \\\\", smallest_frac + 1, smallest_frac + 1, div_value);
			print_simplex_table(smallest_frac, -1, 0);
		}

		// Convert col to 0s
		for (int i = 0; i < table_rows; i++)
		{
			if (i == smallest_frac)
				continue;

			double mult_value = -simplex_table[i][pivot_col];
			for (int j = 0; j < table_cols; j++)
			{
				simplex_table[i][j] = simplex_table[i][j] + mult_value * simplex_table[smallest_frac][j];
			}
			if (intermediate_tables)
			{
				fprintf(output_file, "$R_%d \\leftarrow R_%d + %.2f R_%d$ \\\\", i + 1, i + 1, mult_value, smallest_frac + 1);
				print_simplex_table(i, -1, 0);
			}
		}
		if (intermediate_tables)
		{
			fprintf(output_file, "\\subsubsection*{Pivot Result}\n");
			print_simplex_table(-1, -1, 0);
		}
		check_degeneracy();
		if (intermediate_tables)
			fprintf(output_file, "\n\\newpage\n");
		print_results();
	}
}

void simplex()
{
	if (intermediate_tables)
		fprintf(output_file, "\\newpage\n\\section*{Intermediate Tables}\n");

	is_degenerate = 0;
	int safe = 0;
	int pivoting = 0;
	while (1)
	{
		pivoting++;
		int pivot_col = 1; // The most negative column (index, not value)
		for (int i = 2; i < table_cols - 1; i++)
		{
			if (mode == 0)
			{
				if (simplex_table[0][i] < simplex_table[0][pivot_col])
					pivot_col = i;
			}
			else
			{
				if (simplex_table[0][i] > simplex_table[0][pivot_col])
					pivot_col = i;
			}
		}

		if (mode == 0 && simplex_table[0][pivot_col] >= 0)
			break;
		if (mode == 1 && simplex_table[0][pivot_col] <= 0)
			break;

		if (intermediate_tables) // Print intermediate tables data
		{
			fprintf(output_file, "\\subsection*{Pivoting %d}", pivoting);
			if (mode)
				fprintf(output_file, "\\subsection*{Most Positive}\n");
			else
				fprintf(output_file, "\\subsubsection*{Most Negative}\n");
			fprintf(output_file, "Column %d (%.2f)\n", pivot_col + 1, simplex_table[0][pivot_col]);
			print_simplex_table(0, pivot_col, 0);
			fprintf(output_file, "\\subsubsection*{Fractions}\n");
		}

		if (intermediate_tables)
			print_simplex_table(-1, -1, pivot_col);
		int smallest_frac = 1; // The smallest fraction row
		int is_unbounded = 1;
		for (int i = 1; i < table_rows; i++)
		{
			double frac = simplex_table[i][table_cols - 1] / simplex_table[i][pivot_col];
			if (intermediate_tables)
				fprintf(output_file, "$%.2f / %.2f = %.2f$ \\\\", simplex_table[i][table_cols - 1], simplex_table[i][pivot_col], frac);
			if (simplex_table[i][table_cols - 1] < 0 || simplex_table[i][pivot_col] < 0)
				continue;
			else
				is_unbounded = 0;
			if (frac <= simplex_table[smallest_frac][table_cols - 1] / simplex_table[smallest_frac][pivot_col] ||
				simplex_table[smallest_frac][table_cols - 1] < 0 || simplex_table[smallest_frac][pivot_col] < 0)
				smallest_frac = i;
		}
		if (is_unbounded)
		{
			report_unbounded();
			return;
		}
		if (intermediate_tables)
		{
			fprintf(output_file, "Smallest fraction: %.2f", simplex_table[smallest_frac][table_cols - 1] / simplex_table[smallest_frac][pivot_col]);
			fprintf(output_file, " $\\rightarrow$ Pivot: row %d", smallest_frac + 1);
			fprintf(output_file, "\\subsubsection*{Pivot}\n");
			print_simplex_table(smallest_frac, pivot_col, 0);
		}

		// Make a 1 on smallest frac cell
		double div_value = simplex_table[smallest_frac][pivot_col];
		for (int i = 0; i < table_cols; i++)
		{
			simplex_table[smallest_frac][i] = simplex_table[smallest_frac][i] / div_value;
		}
		if (intermediate_tables)
		{
			fprintf(output_file, "\\subsubsection*{Canonization}\n", pivoting);
			fprintf(output_file, "$R_%d \\leftarrow R_%d/%.2f$ \\\\", smallest_frac + 1, smallest_frac + 1, div_value);
			print_simplex_table(smallest_frac, -1, 0);
		}

		// Convert col to 0s
		for (int i = 0; i < table_rows; i++)
		{
			if (i == smallest_frac)
				continue;

			double mult_value = -simplex_table[i][pivot_col];
			for (int j = 0; j < table_cols; j++)
			{
				simplex_table[i][j] = simplex_table[i][j] + mult_value * simplex_table[smallest_frac][j];
			}
			if (intermediate_tables)
			{
				fprintf(output_file, "$R_%d \\leftarrow R_%d + %.2f R_%d$ \\\\", i + 1, i + 1, mult_value, smallest_frac + 1);
				print_simplex_table(i, -1, 0);
			}
		}
		if (intermediate_tables)
		{
			fprintf(output_file, "\\subsubsection*{Pivot Result}\n");
			print_simplex_table(-1, -1, 0);
		}
		check_degeneracy();
		if (intermediate_tables)
			fprintf(output_file, "\n\\newpage\n");
		safe++;
		if (safe > 50)
			break;
	}
	print_results();
	multiple_solutions(pivoting);
}

void fill_simplex_row(int row) // Fills a row with coefficients from coefficient_widgets
{
	if (!simplex_table)
	{
		table_rows = constraint_amount + 1;
		table_cols = 2 + variable_amount + constraint_amount;
		simplex_table = malloc(table_rows * sizeof(*simplex_table));
		for (int i = 0; i < table_rows; i++)
			simplex_table[i] = malloc(table_cols * sizeof(double));
	}

	for (int j = 0; j < table_cols; j++)
	{
		if (j == 0) // First cell
		{
			if (row == 0)
				simplex_table[row][j] = 1;
			else
				simplex_table[row][j] = 0;
			continue;
		}
		if (j == table_cols - 1) // last cell
		{
			if (row == 0)
				simplex_table[row][j] = 0;
			else
				simplex_table[row][j] = (double)gtk_spin_button_get_value(GTK_SPIN_BUTTON(coefficient_widgets[variable_amount]));
			break;
		}
		if (j > variable_amount) // slack cells
		{
			if (row == j - variable_amount)
				simplex_table[row][j] = 1;
			else
				simplex_table[row][j] = 0;
			continue;
		}

		if (row == 0)
			simplex_table[row][j] = (double)-gtk_spin_button_get_value(GTK_SPIN_BUTTON(coefficient_widgets[j - 1]));
		else
			simplex_table[row][j] = (double)gtk_spin_button_get_value(GTK_SPIN_BUTTON(coefficient_widgets[j - 1]));
	}
}

void on_mode_radio_toggled(GtkToggleButton *toggle_button, gpointer user_data)
{
	const gchar *name = gtk_buildable_get_name(GTK_BUILDABLE(toggle_button));

	if (gtk_toggle_button_get_active(toggle_button))
	{
		if (g_strcmp0(name, "maximize_radio") == 0)
			mode = 0;
		else
			mode = 1;
	}
}

void on_intermediate_radio_toggled(GtkToggleButton *toggle_button, gpointer user_data)
{
	const gchar *name = gtk_buildable_get_name(GTK_BUILDABLE(toggle_button));

	if (gtk_toggle_button_get_active(toggle_button))
	{
		if (g_strcmp0(name, "no_intermediate_radio") == 0)
			intermediate_tables = 0;
		else
			intermediate_tables = 1;
	}
}

GtkWidget *create_variable_widget(int var_id)
{
	GtkBuilder *builder = gtk_builder_new_from_file("objectSetupWidget.glade");
	GtkWidget *elem = GTK_WIDGET(gtk_builder_get_object(builder, "objectSetupWidget"));
	g_object_ref(elem);

	GtkWidget *title_label = GTK_WIDGET(gtk_builder_get_object(builder, "varLabel"));
	if (title_label)
	{
		char title_text[50];
		sprintf(title_text, "Variable %d", var_id + 1);
		gtk_label_set_text(GTK_LABEL(title_label), title_text);
	}

	// Store references to the widgets
	variable_widgets[var_id] = GTK_WIDGET(gtk_builder_get_object(builder, "name_txt"));
	char default_name[6];
	sprintf(default_name, "$x_%d$", var_id + 1);
	gtk_entry_set_text(GTK_ENTRY(variable_widgets[var_id]), default_name);

	g_object_unref(builder);
	return elem;
}

GtkWidget *create_objective_variable_widget(int var_id)
{
	GtkBuilder *builder = gtk_builder_new_from_file("objectiveValueWidget.glade");
	GtkWidget *elem = GTK_WIDGET(gtk_builder_get_object(builder, "objectiveValueWidget"));
	g_object_ref(elem);

	GtkWidget *title_label = GTK_WIDGET(gtk_builder_get_object(builder, "varLabel"));
	if (title_label)
		gtk_label_set_text(GTK_LABEL(title_label), variable_names[var_id]);

	// Store references to the widgets
	coefficient_widgets[var_id] = GTK_WIDGET(gtk_builder_get_object(builder, "value_spin"));

	g_object_unref(builder);
	return elem;
}

void on_continueBtn_variables(GtkButton *button, gpointer user_data) // Continue button in first page
{
	variable_amount = (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(variables_spin));
	constraint_amount = (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(constraints_spin));
	problem_name = gtk_entry_get_text(GTK_ENTRY(problem_input));
	if (!variable_amount || !constraint_amount)
	{
		return;
	}

	// Remove all variable widgets
	GList *children, *iter;
	children = gtk_container_get_children(GTK_CONTAINER(variables_container));
	for (iter = children; iter != NULL; iter = g_list_next(iter))
	{
		gtk_widget_destroy(GTK_WIDGET(iter->data));
	}
	g_list_free(children);

	// Create new variable widgets
	for (int i = 0; i < variable_amount; i++)
	{
		GtkWidget *object_widget = create_variable_widget(i);
		gtk_box_pack_start(GTK_BOX(variables_container), object_widget, FALSE, FALSE, 5);
	}
	gtk_stack_set_visible_child_name(GTK_STACK(main_stack), "page1");
	gtk_widget_show_all(variables_container);
}

void on_continueBtn_objective(GtkButton *button, gpointer user_data)
{
	for (int i = 0; i < variable_amount; i++)
		variable_names[i] = gtk_entry_get_text(GTK_ENTRY(variable_widgets[i]));

	GList *children, *iter;
	children = gtk_container_get_children(GTK_CONTAINER(objective_container));
	for (iter = children; iter != NULL; iter = g_list_next(iter))
	{
		gtk_widget_destroy(GTK_WIDGET(iter->data));
	}
	g_list_free(children);

	for (int i = 0; i < variable_amount; i++)
	{
		GtkWidget *object_widget = create_objective_variable_widget(i);
		gtk_box_pack_start(GTK_BOX(objective_container), object_widget, FALSE, FALSE, 5);
	}
	gtk_stack_set_visible_child_name(GTK_STACK(main_stack), "page2");
	gtk_widget_show_all(objective_container);
}

void on_continueBtn_constraints(GtkButton *button, gpointer user_data)
{
	fill_simplex_row(constraint_page_count);

	GList *children, *iter;
	children = gtk_container_get_children(GTK_CONTAINER(constraints_container));
	for (iter = children; iter != NULL; iter = g_list_next(iter))
	{
		gtk_widget_destroy(GTK_WIDGET(iter->data));
	}
	g_list_free(children);

	for (int i = 0; i < variable_amount; i++)
	{
		GtkWidget *object_widget = create_objective_variable_widget(i);
		gtk_box_pack_start(GTK_BOX(constraints_container), object_widget, FALSE, FALSE, 5);
	}
	variable_names[variable_amount] = "Constant term (Right-Hand Side)";
	GtkWidget *object_widget = create_objective_variable_widget(variable_amount);
	gtk_box_pack_start(GTK_BOX(constraints_container), object_widget, FALSE, FALSE, 5);

	gtk_stack_set_visible_child_name(GTK_STACK(main_stack), "page3");
	gtk_widget_show_all(constraints_container);
}

void on_next_constraintBtn(GtkButton *button, gpointer user_data)
{
	constraint_page_count++;
	fill_simplex_row(constraint_page_count);
	if (constraint_page_count >= constraint_amount)
	{
		gtk_stack_set_visible_child_name(GTK_STACK(main_stack), "page4");
		return;
	}
	else
	{
		char title_text[50];
		sprintf(title_text, "Constraint %d", constraint_page_count + 1);
		gtk_label_set_text(GTK_LABEL(constraint_page_label), title_text);
	}

	// Remove current constraint widgets
	GList *children, *iter;
	children = gtk_container_get_children(GTK_CONTAINER(constraints_container));
	for (iter = children; iter != NULL; iter = g_list_next(iter))
	{
		gtk_widget_destroy(GTK_WIDGET(iter->data));
	}
	g_list_free(children);

	// Create new constraint widgets
	variable_names[variable_amount] = "Constant term (Right-Hand Side)";
	for (int i = 0; i <= variable_amount; i++)
	{
		GtkWidget *object_widget = create_objective_variable_widget(i);
		gtk_box_pack_start(GTK_BOX(constraints_container), object_widget, FALSE, FALSE, 5);
	}

	gtk_widget_show_all(constraints_container);
}

void on_solveBtn(GtkButton *button, gpointer user_data)
{
	output_file = fopen("output.tex", "w");
	if (output_file == NULL)
	{
		g_print("Failed to open LaTeX file");
		return;
	}
	setup_latex();
	print_problem_model();
	simplex();
	fprintf(output_file, "\\end{document}\n");
	fclose(output_file);

	system("pdflatex output.tex");
	system("evince --presentation output.pdf");

	for (int i = 0; i < table_rows; i++)
		free(simplex_table[i]);
	free(simplex_table);
	simplex_table = NULL;

	gtk_spin_button_set_value(GTK_SPIN_BUTTON(variables_spin), 1);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(constraints_spin), 1);
	gtk_entry_set_text(GTK_ENTRY(problem_input), "");
	gtk_stack_set_visible_child_name(GTK_STACK(main_stack), "page0");
}

void save_data_to_file(const char *filename)
{
	FILE *file = fopen(filename, "w");
	if (file == NULL)
	{
		return;
	}

	// Save metadata
	fprintf(file, "problem=%s\n", problem_name);
	fprintf(file, "var_amount=%d\n", variable_amount);
	fprintf(file, "contraint_amount=%d\n", constraint_amount);
	fprintf(file, "mode=%d\n", mode);

	fprintf(file, "VARIABLES\n");
	for (int i = 0; i < variable_amount; i++)
	{
		fprintf(file, "%s\n", variable_names[i]);
	}
	fprintf(file, "END_VARIABLES\n");

	fprintf(file, "table_rows=%d\n", table_rows);
	fprintf(file, "table_cols=%d\n", table_cols);
	fprintf(file, "SIMPLEX_TABLE\n");
	for (int i = 0; i < table_rows; i++)
	{
		fprintf(file, "START_ROW\n");
		for (int j = 0; j < table_cols; j++)
		{
			fprintf(file, "%.2f\n", simplex_table[i][j]);
		}
		fprintf(file, "END_ROW\n");
	}
	fprintf(file, "END_SIMPLEX_TABLE\n");

	fclose(file);
}

void load_data_from_file(const char *filename)
{
	FILE *file = fopen(filename, "r");
	if (file == NULL)
	{
		return;
	}

	char line[512];
	char trimmed[512];

	while (fgets(line, sizeof(line), file) != NULL)
	{
		strncpy(trimmed, line, sizeof(trimmed));
		trimmed[sizeof(trimmed) - 1] = '\0';
		size_t l = strlen(trimmed);
		if (l > 0 && trimmed[l - 1] == '\n')
			trimmed[l - 1] = '\0';

		if (strncmp(trimmed, "problem=", 8) == 0)
		{
			char *val = trimmed + 8;
			if (problem_name)
			{
				problem_name = strdup(val);
			}
			else
			{
				problem_name = strdup(val);
			}
		}
		else if (strncmp(trimmed, "var_amount=", 11) == 0)
		{
			sscanf(trimmed + 11, "%d", &variable_amount);
		}
		else if (strncmp(trimmed, "contraint_amount=", 17) == 0)
		{
			sscanf(trimmed + 17, "%d", &constraint_amount);
		}
		else if (strncmp(trimmed, "mode=", 5) == 0)
		{
			sscanf(trimmed + 5, "%d", &mode);
		}
		else if (strcmp(trimmed, "VARIABLES") == 0)
		{
			for (int i = 0; i < variable_amount; i++)
			{
				if (fgets(line, sizeof(line), file) == NULL)
					break;
				strncpy(trimmed, line, sizeof(trimmed));
				trimmed[sizeof(trimmed) - 1] = '\0';
				size_t ll = strlen(trimmed);
				if (ll > 0 && trimmed[ll - 1] == '\n')
					trimmed[ll - 1] = '\0';
				variable_names[i] = strdup(trimmed);
			}
			if (fgets(line, sizeof(line), file) != NULL)
			{
			}
		}
		else if (strncmp(trimmed, "table_rows=", 11) == 0)
		{
			sscanf(trimmed + 11, "%d", &table_rows);
		}
		else if (strncmp(trimmed, "table_cols=", 11) == 0)
		{
			sscanf(trimmed + 11, "%d", &table_cols);
		}
		else if (strcmp(trimmed, "SIMPLEX_TABLE") == 0)
		{
			if (simplex_table)
			{
				for (int i = 0; i < table_rows; i++)
				{
					if (simplex_table[i])
						free(simplex_table[i]);
				}
				free(simplex_table);
				simplex_table = NULL;
			}

			simplex_table = malloc(table_rows * sizeof(*simplex_table));
			for (int i = 0; i < table_rows; i++)
				simplex_table[i] = malloc(table_cols * sizeof(double));

			for (int i = 0; i < table_rows; i++)
			{
				if (fgets(line, sizeof(line), file) == NULL)
					break;
				while (strncmp(line, "START_ROW", 9) != 0)
				{
					if (fgets(line, sizeof(line), file) == NULL)
						break;
				}

				for (int j = 0; j < table_cols; j++)
				{
					if (fgets(line, sizeof(line), file) == NULL)
						break;
					char tmp[128];
					strncpy(tmp, line, sizeof(tmp));
					tmp[sizeof(tmp) - 1] = '\0';
					size_t lt = strlen(tmp);
					if (lt > 0 && tmp[lt - 1] == '\n')
						tmp[lt - 1] = '\0';
					simplex_table[i][j] = atof(tmp);
				}

				if (fgets(line, sizeof(line), file) == NULL)
					break;
			}
		}
	}

	fclose(file);
}

void on_saveBtn_clicked(GtkButton *button, gpointer user_data)
{
	GtkWidget *dialog = gtk_file_chooser_dialog_new("Save Simplex Data",
													GTK_WINDOW(main_window),
													GTK_FILE_CHOOSER_ACTION_SAVE,
													"_Cancel", GTK_RESPONSE_CANCEL,
													"_Save", GTK_RESPONSE_ACCEPT,
													NULL);

	// Set default filename
	gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog), "LP_problem.smplx");

	// Add file filter
	GtkFileFilter *filter = gtk_file_filter_new();
	gtk_file_filter_set_name(filter, "Simplex Files (*.smplx)");
	gtk_file_filter_add_pattern(filter, "*.smplx");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);

	gint result = gtk_dialog_run(GTK_DIALOG(dialog));
	if (result == GTK_RESPONSE_ACCEPT)
	{
		char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));

		// Add .ksp extension if not present
		if (!g_str_has_suffix(filename, ".smplx"))
		{
			char *new_filename = g_strdup_printf("%s.smplx", filename);
			g_free(filename);
			filename = new_filename;
		}

		save_data_to_file(filename);
		g_free(filename);
	}

	gtk_widget_destroy(dialog);
}

void on_loadBtn_clicked(GtkButton *button, gpointer user_data)
{
	GtkWidget *dialog = gtk_file_chooser_dialog_new("Load Simplex Data",
													GTK_WINDOW(main_window),
													GTK_FILE_CHOOSER_ACTION_OPEN,
													"_Cancel", GTK_RESPONSE_CANCEL,
													"_Load", GTK_RESPONSE_ACCEPT,
													NULL);

	// Add file filter
	GtkFileFilter *filter = gtk_file_filter_new();
	gtk_file_filter_set_name(filter, "Simplex Files (*.smplx)");
	gtk_file_filter_add_pattern(filter, "*.smplx");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);

	gint result = gtk_dialog_run(GTK_DIALOG(dialog));
	if (result == GTK_RESPONSE_ACCEPT)
	{
		char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
		load_data_from_file(filename);
		g_free(filename);
	}

	gtk_widget_destroy(dialog);
	gtk_stack_set_visible_child_name(GTK_STACK(main_stack), "page4");
}

int main(int argc, char *argv[])
{
	// simplex();
	gtk_init(&argc, &argv);

	// GtkBuilder *builder = gtk_builder_new_from_file("Simplex/simplex.glade"); // Si se abre desde el menu
	GtkBuilder *builder = gtk_builder_new_from_file("simplex.glade"); // Si se abre SIN en menu

	main_window = GTK_WIDGET(gtk_builder_get_object(builder, "hWindow"));

	gtk_builder_connect_signals(builder, NULL);

	// exit
	g_signal_connect(main_window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

	main_stack = GTK_WIDGET(gtk_builder_get_object(builder, "mainStack"));
	problem_input = GTK_WIDGET(gtk_builder_get_object(builder, "problem_name"));
	variables_spin = GTK_WIDGET(gtk_builder_get_object(builder, "variables_spin"));
	constraints_spin = GTK_WIDGET(gtk_builder_get_object(builder, "constraints_spin"));
	variables_container = GTK_WIDGET(gtk_builder_get_object(builder, "variables_container"));
	objective_container = GTK_WIDGET(gtk_builder_get_object(builder, "objective_container"));
	constraints_container = GTK_WIDGET(gtk_builder_get_object(builder, "constraints_container"));
	constraint_page_label = GTK_WIDGET(gtk_builder_get_object(builder, "constraint_label"));

	GtkWidget *problem_input; // problem name
	GtkWidget *variables_spin;
	GtkWidget *constraints_spin;

	gtk_widget_show_all(main_window);

	gtk_main();

	g_object_unref(builder);

	return 0;
}
