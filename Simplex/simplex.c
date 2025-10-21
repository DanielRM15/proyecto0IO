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
		fprintf(output_file, "& $s_%d$", i);
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
		for (int j = 0; j < variable_amount + constraint_amount; j++)
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
	fprintf(output_file, "DESPUES TERMINO ESTO");
}

void simplex()
{
	if (intermediate_tables)
		fprintf(output_file, "\\newpage\n\\section*{Intermediate Tables}\n");

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

		print_simplex_table(-1, -1, pivot_col);
		int smallest_frac = 1; // The smallest fraction row
		for (int i = 1; i < table_rows; i++)
		{
			double frac = simplex_table[i][table_cols - 1] / simplex_table[i][pivot_col];
			if (intermediate_tables)
				fprintf(output_file, "$%.2f / %.2f = %.2f$ \\\\", simplex_table[i][table_cols - 1], simplex_table[i][pivot_col], frac);
			if (frac < 0)
				continue;
			if (frac < simplex_table[smallest_frac][table_cols - 1] / simplex_table[smallest_frac][pivot_col])
				smallest_frac = i;
		}
		fprintf(output_file, "\\subsubsection*{Pivot}\n");
		print_simplex_table(smallest_frac, pivot_col, 0);

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
				fprintf(output_file, "$R_%d \\leftarrow R_%d + %.2f R_%d$ \\\\", i + 1, i + 1, mult_value, smallest_frac);
				print_simplex_table(i, -1, 0);
			}
		}
		if (intermediate_tables)
		{
			fprintf(output_file, "\\subsubsection*{Pivot Result}\n", pivoting);
			print_simplex_table(-1, -1, 0);
			fprintf(output_file, "\n\\newpage\n", pivoting);
		}
		safe++;
		if (safe > 50)
			break;
	}
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

void on_continueBtn_variables(GtkButton *button, gpointer user_data) // Continue button in variable names page
{
	variable_amount = (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(variables_spin));
	constraint_amount = (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(constraints_spin));
	problem_name = gtk_entry_get_text(GTK_ENTRY(problem_input));
	if (!variable_amount || !constraint_amount)
	{
		return;
	}

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
	print_results();
	fprintf(output_file, "\\end{document}\n");
	fclose(output_file);

	system("pdflatex output.tex");
	system("evince --presentation output.pdf &");
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
