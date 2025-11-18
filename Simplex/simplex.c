#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <mpfr.h>

FILE *output_file;
GtkWidget *main_window;
GtkWidget *objective_container;
GtkWidget *constraints_container;
GtkWidget *problem_input; // problem name
GtkWidget *variables_spin;
GtkWidget *constraints_spin;
GtkWidget **variable_entries;
char *variable_names[15];

GtkWidget ***constraint_label_widgets = NULL;
int current_constraints = 0;
int current_variables = 0;
GtkWidget ***spin_table = NULL;

double *sol1;
double *sol2;

char *problem_name;
int variable_amount;
int constraint_amount;
int mode = 0;				 // 0 for max, 1 for min;
int intermediate_tables = 1; // 1 yes, 0 no
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
			"    {\\LARGE \\textbf{Simplex}} \\\\[3cm]\n"
			"    {\\Large Members:} \\\\[0.5cm]\n"
			"    {\\large Adrián Zamora Chavarría \\\\ Daniel Romero Murillo} \\\\[2cm]\n"
			"    {\\large Date: \\today}\n"
			"    \\vfill\n"
			"\\end{titlepage}\n"
			"\\newpage\n");
}

void print_simplex_table(mpfr_t **table, int highlight_row, int highlight_col, int print_fractions)
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
			mpfr_fprintf(output_file, "%.2Rf", table[i][j]);
		}
		if (print_fractions)
		{
			fprintf(output_file, "& ");
			if (i > 0)
			{
				mpfr_t result;
				mpfr_init(result);
				mpfr_div(result, table[i][table_cols - 1], table[i][print_fractions], MPFR_RNDN);
				mpfr_fprintf(output_file, "%.2Rf", result);
				mpfr_clear(result);
			}
		}
		fprintf(output_file, "\\\\\n");
	}
	fprintf(output_file, "\\end{tabular}\n");
	fprintf(output_file, "\\end{table}\n");
}

void print_problem_model(mpfr_t **table)
{
	g_print("Print!\n");
	fprintf(output_file, "\\section*{");
	fprintf(output_file, problem_name);
	fprintf(output_file, "}\n");
	if (mode)
		fprintf(output_file, "Minimize\n\\begin{center}\nZ = ");
	else
		fprintf(output_file, "Maximize\n\\begin{center}\nZ = ");

	for (int i = 1; i <= variable_amount; i++)
	{
		mpfr_t val;
		mpfr_init_set(val, table[0][i], MPFR_RNDN);
		mpfr_neg(val, val, MPFR_RNDN);
		if (mpfr_sgn(val) < 0)
		{
			fprintf(output_file, "- ");
			mpfr_neg(val, val, MPFR_RNDN);
		}
		else
		{
			if (i > 1)
				fprintf(output_file, "+ ");
		}
		mpfr_fprintf(output_file, "%.2Rf%s ", val, variable_names[i - 1]);
		mpfr_clear(val);
	}
	fprintf(output_file, "\n\\end{center}\nSubject to\n\\begin{center}\n");
	for (int i = 1; i <= constraint_amount; i++)
	{
		for (int j = 0; j < variable_amount; j++)
		{
			mpfr_t val;
			mpfr_init_set(val, table[i][j + 1], MPFR_RNDN);
			if (mpfr_sgn(val) < 0)
			{
				fprintf(output_file, "- ");
				mpfr_neg(val, val, MPFR_RNDN);
			}
			else
			{
				if (j != 0)
					fprintf(output_file, "+ ");
			}

			if (j < variable_amount)
				mpfr_fprintf(output_file, "%.2Rf%s ", val, variable_names[j]);
			// else // Print slack variables
			// 	fprintf(output_file, "%.2f$s_%d$ ", val, j - variable_amount + 1);
			mpfr_clear(val);
		}
		mpfr_t val;
		mpfr_init_set(val, table[i][table_cols - 1], MPFR_RNDN);
		mpfr_fprintf(output_file, "$\\leq$ %.2Rf \\\\\n", val);
		mpfr_clear(val);
	}
	fprintf(output_file, "\\end{center}\n");
	fprintf(output_file, "Simplex Table\n");
	print_simplex_table(table, -1, -1, 0);
}

void print_results(mpfr_t **table)
{
	fprintf(output_file, "\\section*{Results}\n");
	print_simplex_table(table, -1, -1, 0);
	fprintf(output_file, "\n\\subsection*{Objective Value}\n");
	mpfr_fprintf(output_file, "Z = %.2Rf\n", table[0][table_cols - 1]);
	fprintf(output_file, "\n\\subsection*{Variables}\n");

	mpfr_t var_value, zero, one;
	mpfr_inits2(256, var_value, zero, one, NULL);
	mpfr_set_ui(zero, 0, MPFR_RNDN);
	mpfr_set_ui(one, 1, MPFR_RNDN);
	for (int i = 0; i < variable_amount; i++)
	{
		mpfr_set_ui(var_value, 0, MPFR_RNDN); // var_value = 0

		for (int j = 0; j < constraint_amount + 1; j++)
		{
			if (mpfr_cmp(table[j][i + 1], zero) != 0)
			{
				if (mpfr_cmp(table[j][i + 1], one) != 0)
				{
					mpfr_set_ui(var_value, 0, MPFR_RNDN); // not a basic variable
					break;
				}
				else
				{
					mpfr_set(var_value, table[j][table_cols - 1], MPFR_RNDN);
				}
			}
		}
		fprintf(output_file, "%s = ", variable_names[i]);
		mpfr_fprintf(output_file, "%.2Rf \\\\\n", var_value);
	}

	fprintf(output_file, "\n\\subsection*{Slack or Surplus}\n");
	for (int i = 0; i < constraint_amount; i++)
	{
		mpfr_set_ui(var_value, 0, MPFR_RNDN); // default = 0

		for (int j = 0; j < constraint_amount + 1; j++)
		{
			if (mpfr_cmp(table[j][i + variable_amount + 1], zero) != 0)
			{
				if (mpfr_cmp(table[j][i + variable_amount + 1], one) != 0)
				{
					mpfr_set_ui(var_value, 0, MPFR_RNDN);
					break;
				}
				else
				{
					mpfr_set(var_value, table[j][table_cols - 1], MPFR_RNDN);
				}
			}
		}
		fprintf(output_file, "$s_%d$ = ", i + 1);
		mpfr_fprintf(output_file, "%.2Rf \\\\\n", var_value);
	}
	mpfr_clears(var_value, zero, one, NULL);
}

void report_unbounded(char *var_name)
{
	fprintf(output_file, "\n\\section*{Unbounded Problem!}\n");
	if (mode)
		fprintf(output_file, "This problem is unbounded. The variable %s can be infinitely decreased, making Z have an infinitely negative value.\n", var_name);
	else
		fprintf(output_file, "This problem is unbounded. The variable %s can be infinitely increased, making Z have an infinite value.\n", var_name);

	fprintf(output_file, "This can be solved by adding restrictions that cap the value of %s.\n", var_name);
	fprintf(output_file, "Please re-model it and try again!\n");
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

void simplex(mpfr_t **table)
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
				if (mpfr_cmp(table[0][i], table[0][pivot_col]) < 0)
					pivot_col = i;
			}
			else
			{
				if (mpfr_cmp(table[0][i], table[0][pivot_col]) > 0)
					pivot_col = i;
			}
		}

		mpfr_t zero;
		mpfr_init_set_ui(zero, 0, MPFR_RNDN);
		if (mode == 0 && mpfr_cmp(table[0][pivot_col], zero) >= 0)
		{
			mpfr_clear(zero);
			break;
		}
		if (mode == 1 && mpfr_cmp(table[0][pivot_col], zero) <= 0)
		{
			mpfr_clear(zero);
			break;
		}

		if (intermediate_tables) // Print intermediate tables data
		{
			fprintf(output_file, "\\subsection*{Pivoting %d}", pivoting);
			if (mode)
				fprintf(output_file, "\\subsection*{Most Positive}\n");
			else
				fprintf(output_file, "\\subsubsection*{Most Negative}\n");
			mpfr_fprintf(output_file, "Column %d (%.2Rf)\n", pivot_col + 1, table[0][pivot_col]);
			print_simplex_table(table, 0, pivot_col, 0);
			fprintf(output_file, "\\subsubsection*{Fractions}\n");
			print_simplex_table(table, -1, -1, pivot_col);
		}

		int smallest_frac = 1; // The smallest fraction row
		int is_unbounded = 1;
		mpfr_t frac, tmp;
		mpfr_inits2(256, frac, tmp, zero, NULL);
		mpfr_set_ui(zero, 0, MPFR_RNDN);
		for (int i = 1; i < table_rows; i++)
		{
			mpfr_div(frac, table[i][table_cols - 1], table[i][pivot_col], MPFR_RNDN);

			if (intermediate_tables)
			{
				mpfr_fprintf(output_file,
							 "$%.2Rf / %.2Rf = %.2Rf$ \\\\\n",
							 table[i][table_cols - 1],
							 table[i][pivot_col],
							 frac);
			}
			if (mpfr_cmp(table[i][table_cols - 1], zero) < 0 ||
				mpfr_cmp(table[i][pivot_col], zero) < 0)
			{
				continue;
			}
			else
			{
				is_unbounded = 0;
			}
			mpfr_div(tmp,
					 table[smallest_frac][table_cols - 1],
					 table[smallest_frac][pivot_col],
					 MPFR_RNDN);
			if (mpfr_cmp(frac, tmp) <= 0 ||
				mpfr_cmp(table[smallest_frac][table_cols - 1], zero) < 0 ||
				mpfr_cmp(table[smallest_frac][pivot_col], zero) < 0)
			{
				smallest_frac = i;
			}
		}
		mpfr_clears(frac, tmp, zero, NULL);

		if (is_unbounded)
		{
			char varbuf[12];
			char *var_name;
			if (pivot_col < variable_amount + 1)
				var_name = variable_names[pivot_col - 1];
			else
			{
				snprintf(varbuf, sizeof(varbuf), "$s_%d$", pivot_col - variable_amount);
				var_name = varbuf;
			}
			// report_unbounded(var_name);
			return;
		}
		if (intermediate_tables)
		{
			mpfr_t ratio;
			mpfr_init2(ratio, 256);
			mpfr_div(ratio,
					 table[smallest_frac][table_cols - 1],
					 table[smallest_frac][pivot_col],
					 MPFR_RNDN);
			mpfr_fprintf(output_file, "Smallest fraction: %.2Rf", ratio);
			mpfr_clear(ratio);
			fprintf(output_file, " $\\rightarrow$ Pivot: row %d", smallest_frac + 1);
			fprintf(output_file, "\\subsubsection*{Pivot}\n");
			print_simplex_table(table, smallest_frac, pivot_col, 0);
		}

		// Make a 1 on smallest frac cell
		mpfr_t div_value;
		mpfr_init2(div_value, 256);
		mpfr_set(div_value, table[smallest_frac][pivot_col], MPFR_RNDN);
		for (int i = 0; i < table_cols; i++)
		{
			mpfr_div(table[smallest_frac][i],
					 table[smallest_frac][i],
					 div_value,
					 MPFR_RNDN);
		}
		if (intermediate_tables)
		{
			fprintf(output_file, "\\subsubsection*{Canonization}\n", pivoting);
			mpfr_fprintf(output_file, "$R_%d \\leftarrow R_%d/%.2Rf$ \\\\", smallest_frac + 1, smallest_frac + 1, div_value);
			print_simplex_table(table, smallest_frac, -1, 0);
		}
		mpfr_clear(div_value);

		// Convert col to 0s
		for (int i = 0; i < table_rows; i++)
		{
			if (i == smallest_frac)
				continue;

			mpfr_t mult_value, tmp;
			mpfr_inits2(256, mult_value, tmp, NULL);
			mpfr_neg(mult_value, table[i][pivot_col], MPFR_RNDN);
			for (int j = 0; j < table_cols; j++)
			{
				mpfr_mul(tmp, mult_value, table[smallest_frac][j], MPFR_RNDN);
				mpfr_add(table[i][j], table[i][j], tmp, MPFR_RNDN);
			}

			if (intermediate_tables)
			{
				mpfr_fprintf(output_file,
							 "$R_%d \\leftarrow R_%d + %.2Rf R_%d$ \\\\\n",
							 i + 1, i + 1, mult_value, smallest_frac + 1);
				print_simplex_table(table, i, -1, 0);
			}
			mpfr_clears(mult_value, tmp, NULL);
		}
		if (intermediate_tables)
		{
			fprintf(output_file, "\\subsubsection*{Pivot Result}\n");
			print_simplex_table(table, -1, -1, 0);
		}
		// check_degeneracy();
		if (intermediate_tables)
			fprintf(output_file, "\n\\newpage\n");
		safe++;
		if (safe > 50)
			break;
	}
	print_results(table);
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
}

static void adapt_entry_width(GtkEditable *editable, gpointer user_data)
{
	const char *txt = gtk_entry_get_text(GTK_ENTRY(editable));
	int len = strlen(txt);

	if (len < 1)
		len = 1;
	gtk_entry_set_width_chars(GTK_ENTRY(editable), len);
}

// Update constraint labels when a variable name entry changes
static void on_variable_name_changed(GtkEditable *editable, gpointer user_data)
{
	int var_index = GPOINTER_TO_INT(user_data);
	const char *new_name = gtk_entry_get_text(GTK_ENTRY(editable));

	if (!constraint_label_widgets)
		return;

	for (int c = 0; c < current_constraints; c++)
	{
		if (var_index >= current_variables)
			continue;
		GtkWidget *label = constraint_label_widgets[c][var_index];
		if (!label)
			continue;

		gchar *label_text;
		if (var_index < current_variables - 1)
			label_text = g_strdup_printf("%s +", new_name);
		else
			label_text = g_strdup(new_name);

		gtk_label_set_text(GTK_LABEL(label), label_text);
		g_free(label_text);
	}
}

void build_objective(void)
{
	int num_variables = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(variables_spin));

	GList *children = gtk_container_get_children(GTK_CONTAINER(objective_container));

	int index = 0;
	for (GList *iter = children; iter != NULL; iter = iter->next, index++)
	{
		if (index >= 2)
		{
			gtk_widget_destroy(GTK_WIDGET(iter->data));
		}
	}
	g_list_free(children);

	for (int i = 0; i < num_variables; ++i)
	{

		GtkWidget *row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
		GtkWidget *coef_spin = gtk_spin_button_new_with_range(-99999, 99999, 1);
		gtk_spin_button_set_digits(GTK_SPIN_BUTTON(coef_spin), 2);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(coef_spin), 1.00);
		gtk_box_pack_start(GTK_BOX(row), coef_spin, FALSE, FALSE, 0);

		if (spin_table && spin_table[0])
			spin_table[0][i] = coef_spin;

		gchar *default_name = g_strdup_printf("$x_%d$", i + 1);
		GtkWidget *entry = gtk_entry_new();
		gtk_entry_set_text(GTK_ENTRY(entry), default_name);
		gtk_entry_set_has_frame(GTK_ENTRY(entry), FALSE);
		g_signal_connect(entry, "changed", G_CALLBACK(adapt_entry_width), NULL);
		g_signal_connect(entry, "changed", G_CALLBACK(on_variable_name_changed), GINT_TO_POINTER(i));
		adapt_entry_width(GTK_EDITABLE(entry), NULL);
		g_free(default_name);

		gtk_box_pack_start(GTK_BOX(row), entry, FALSE, FALSE, 4);

		variable_entries[i] = entry;

		if (i < num_variables - 1)
		{
			GtkWidget *plus = gtk_label_new("+");
			gtk_box_pack_start(GTK_BOX(row), plus, FALSE, FALSE, 4);
		}
		gtk_box_pack_start(GTK_BOX(objective_container), row, FALSE, FALSE, 4);
	}
	spin_table[0][num_variables] = NULL;
	gtk_widget_show_all(objective_container);
}

void build_constraints()
{
	int num_constraints = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(constraints_spin));
	int num_variables = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(variables_spin));

	GList *children = gtk_container_get_children(GTK_CONTAINER(constraints_container));
	for (GList *iter = children; iter != NULL; iter = iter->next)
	{
		gtk_widget_destroy(GTK_WIDGET(iter->data));
	}
	g_list_free(children);

	if (constraint_label_widgets != NULL)
	{
		for (int i = 0; i < current_constraints; i++)
		{
			if (constraint_label_widgets[i])
				free(constraint_label_widgets[i]);
		}
		free(constraint_label_widgets);
		constraint_label_widgets = NULL;
	}

	current_constraints = num_constraints;
	current_variables = num_variables;

	if (num_constraints > 0 && num_variables > 0)
	{
		constraint_label_widgets = malloc(sizeof(GtkWidget **) * num_constraints);
		for (int c = 0; c < num_constraints; c++)
		{
			constraint_label_widgets[c] = malloc(sizeof(GtkWidget *) * num_variables);
			for (int v = 0; v < num_variables; v++)
				constraint_label_widgets[c][v] = NULL;
		}
	}

	for (int c = 0; c < num_constraints; ++c)
	{

		GtkWidget *row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);

		for (int v = 0; v < num_variables; ++v)
		{
			GtkWidget *coef_spin = gtk_spin_button_new_with_range(-9999, 9999, 1);
			gtk_spin_button_set_digits(GTK_SPIN_BUTTON(coef_spin), 2);
			gtk_spin_button_set_value(GTK_SPIN_BUTTON(coef_spin), 1.00);
			gtk_box_pack_start(GTK_BOX(row), coef_spin, FALSE, FALSE, 0);

			if (spin_table)
				spin_table[c + 1][v] = coef_spin;

			const char *var_name = gtk_entry_get_text(GTK_ENTRY(variable_entries[v]));

			gchar *label_text;
			if (v < num_variables - 1)
				label_text = g_strdup_printf("%s +", var_name);
			else
				label_text = g_strdup(var_name);

			GtkWidget *label = gtk_label_new(label_text);
			g_free(label_text);

			if (constraint_label_widgets && c < current_constraints && v < current_variables)
				constraint_label_widgets[c][v] = label;

			gtk_box_pack_start(GTK_BOX(row), label, FALSE, FALSE, 4);
		}

		GtkWidget *comp_combo = gtk_combo_box_text_new();
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(comp_combo), "<=");
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(comp_combo), ">=");
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(comp_combo), "=");
		gtk_combo_box_set_active(GTK_COMBO_BOX(comp_combo), 0);
		gtk_box_pack_start(GTK_BOX(row), comp_combo, FALSE, FALSE, 6);

		GtkWidget *rhs_spin = gtk_spin_button_new_with_range(-99999, 99999, 1);
		gtk_spin_button_set_digits(GTK_SPIN_BUTTON(rhs_spin), 2);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(rhs_spin), 0.00);
		gtk_box_pack_start(GTK_BOX(row), rhs_spin, FALSE, FALSE, 0);

		if (spin_table)
			spin_table[c + 1][num_variables] = rhs_spin;

		gtk_box_pack_start(GTK_BOX(constraints_container), row, FALSE, FALSE, 4);
	}

	gtk_widget_show_all(constraints_container);
}

void setup_input_page(void)
{
	int num_variables = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(variables_spin));

	int num_constraints = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(constraints_spin));

	if (variable_entries != NULL)
		free(variable_entries);
	variable_entries = malloc(sizeof(GtkWidget *) * num_variables);

	if (spin_table != NULL)
	{
		for (int r = 0; r < current_constraints + 1; r++)
		{
			if (spin_table[r])
				free(spin_table[r]);
		}
		free(spin_table);
		spin_table = NULL;
	}

	int rows = num_constraints + 1;
	if (rows > 0 && num_variables > 0)
	{
		spin_table = malloc(sizeof(GtkWidget **) * rows);
		for (int r = 0; r < rows; r++)
		{
			spin_table[r] = malloc(sizeof(GtkWidget *) * (num_variables + 1));
			for (int c = 0; c < num_variables; c++)
				spin_table[r][c] = NULL;
		}
	}

	build_objective();
	build_constraints();
}

void fill_simplex_table()
{
	table_rows = constraint_amount + 1;
	table_cols = variable_amount + constraint_amount + 2;

	simplex_table = malloc(table_rows * sizeof(double *));
	for (int i = 0; i < table_rows; i++)
		simplex_table[i] = calloc(table_cols, sizeof(double));

	for (int i = 0; i < table_rows; i++)
	{
		for (int j = 1; j < table_cols; j++)
		{
			if (j <= variable_amount)
			{
				if (spin_table[i][j - 1] != NULL && i == 0)
					simplex_table[i][j] = -1 * gtk_spin_button_get_value(GTK_SPIN_BUTTON(spin_table[i][j - 1]));
				else if (spin_table[i][j - 1] != NULL)
					simplex_table[i][j] = gtk_spin_button_get_value(GTK_SPIN_BUTTON(spin_table[i][j - 1]));
			}
			else if (j <= variable_amount + constraint_amount)
			{
				if (i == j - variable_amount)
					simplex_table[i][j] = 1;
			}
			else if (j == table_cols - 1 && spin_table[i][variable_amount] != NULL)
			{
				simplex_table[i][j] = gtk_spin_button_get_value(GTK_SPIN_BUTTON(spin_table[i][variable_amount]));
			}
		}
	}
}

mpfr_t **fill_simplex_table_mpfr(mpfr_prec_t prec)
{
	mpfr_t **mtable = malloc(table_rows * sizeof(mpfr_t *));
	for (int i = 0; i < table_rows; i++)
	{
		mtable[i] = malloc(table_cols * sizeof(mpfr_t));
		for (int j = 0; j < table_cols; j++)
		{
			mpfr_init2(mtable[i][j], prec);
			mpfr_set_d(mtable[i][j], 0.0, MPFR_RNDN);
		}
	}

	for (int i = 0; i < table_rows; i++)
	{
		for (int j = 1; j < table_cols; j++)
		{
			if (j <= variable_amount)
			{
				if (spin_table && spin_table[i] && spin_table[i][j - 1] != NULL && i == 0)
				{
					double v = -1 * gtk_spin_button_get_value(GTK_SPIN_BUTTON(spin_table[i][j - 1]));
					mpfr_set_d(mtable[i][j], v, MPFR_RNDN);
				}
				else if (spin_table && spin_table[i] && spin_table[i][j - 1] != NULL)
				{
					double v = gtk_spin_button_get_value(GTK_SPIN_BUTTON(spin_table[i][j - 1]));
					mpfr_set_d(mtable[i][j], v, MPFR_RNDN);
				}
			}
			else if (j <= variable_amount + constraint_amount)
			{
				if (i == j - variable_amount)
					mpfr_set_d(mtable[i][j], 1.0, MPFR_RNDN);
			}
			else if (j == table_cols - 1)
			{
				if (spin_table && spin_table[i] && spin_table[i][variable_amount] != NULL)
				{
					double v = gtk_spin_button_get_value(GTK_SPIN_BUTTON(spin_table[i][variable_amount]));
					mpfr_set_d(mtable[i][j], v, MPFR_RNDN);
				}
			}
		}
	}

	return mtable;
}

void setup_simplex()
{
	variable_amount = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(variables_spin));
	constraint_amount = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(constraints_spin));
	table_rows = constraint_amount + 1;
	table_cols = variable_amount + constraint_amount + 2;

	// Get variable names
	for (int i = 0; i < variable_amount; i++)
	{
		const char *txt = gtk_entry_get_text(GTK_ENTRY(variable_entries[i]));
		if (variable_names[i] != NULL)
			free(variable_names[i]);
		variable_names[i] = strdup(txt);
	}
}

void on_solveBtn(GtkButton *button, gpointer user_data)
{
	output_file = fopen("output.tex", "w");
	if (output_file == NULL)
	{
		g_print("Failed to open LaTeX file");
		return;
	}
	problem_name = gtk_entry_get_text(GTK_ENTRY(problem_input));
	setup_simplex();
	mpfr_t **table = fill_simplex_table_mpfr(256);
	setup_latex();
	print_problem_model(table);
	simplex(table);
	fprintf(output_file, "\\end{document}\n");
	fclose(output_file);

	if (table)
	{
		for (int i = 0; i < table_rows; i++)
		{
			if (table[i])
			{
				for (int j = 0; j < table_cols; j++)
				{
					mpfr_clear(table[i][j]);
				}
				free(table[i]);
			}
		}
		free(table);
		table = NULL;
	}

	system("pdflatex output.tex");
	system("evince --presentation output.pdf &");
}

int main(int argc, char *argv[])
{
	gtk_init(&argc, &argv);

	// GtkBuilder *builder = gtk_builder_new_from_file("Simplex/simplex.glade"); // Si se abre desde el menu
	GtkBuilder *builder = gtk_builder_new_from_file("simplex.glade"); // Si se abre SIN en menu

	main_window = GTK_WIDGET(gtk_builder_get_object(builder, "hWindow"));

	gtk_builder_connect_signals(builder, NULL);

	// exit
	g_signal_connect(main_window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

	problem_input = GTK_WIDGET(gtk_builder_get_object(builder, "problem_name"));
	variables_spin = GTK_WIDGET(gtk_builder_get_object(builder, "variables_spin"));
	constraints_spin = GTK_WIDGET(gtk_builder_get_object(builder, "constraints_spin"));
	objective_container = GTK_WIDGET(gtk_builder_get_object(builder, "objective_container"));
	constraints_container = GTK_WIDGET(gtk_builder_get_object(builder, "constraints_container"));

	setup_input_page();
	gtk_widget_show_all(main_window);

	gtk_main();

	g_object_unref(builder);

	return 0;
}
