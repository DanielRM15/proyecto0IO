#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <mpfr.h>

FILE *output_file;
GtkBuilder *builder;
GtkWidget *main_window;
GtkWidget *objective_container;
GtkWidget *constraints_container;
GtkWidget *problem_input; // problem name
GtkWidget *variables_spin;
GtkWidget *constraints_spin;
GtkWidget *max_min_combo;
GtkWidget **variable_entries;
char *variable_names[15];

GtkWidget ***constraint_label_widgets = NULL;
int current_constraints = 0;
int current_variables = 0;
GtkWidget ***spin_table = NULL;
GtkWidget **constraint_combos = NULL; // combos with constraint type (<=, >=, =)

double *sol1;
double *sol2;

char *problem_name;
int variable_amount;		 // LP model variables amount
int constraint_amount;		 // LP model constraints amount
int slackv_amount;			 // slack variables amount
int excessv_amount;			 // excess variables amount
int artificialv_amount;		 // artificial variables amount
int mode = 0;				 // 0 for max, 1 for min;
int intermediate_tables = 1; // 1 yes, 0 no
int is_degenerate = 0;		 // 0 no, 1 yes
int constraint_page_count = 0;

double **simplex_table;
mpfr_t M;
int table_cols;
int table_rows;

void setup_latex()
{
	fprintf(output_file,
			"\\documentclass[12pt,a4paper]{article}\n"
			"\\usepackage[table]{xcolor}\n"
			"\\usepackage{float}\n"
			"\\usepackage{geometry}\n"
			"\\usepackage{adjustbox}\n"
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
			"\\newpage\n"
			"\\section*{Simplex}\n"
			"The simplex algorithm was invented by Robert Dantzig in 1947 to solve Linear Programming problems. Dantzig proved that the feasible region (area that satisfies all constraints) of a Linear Programming problem forms a convex set. He also proved that the optimal solution of a LP problem is located at a vertex of the convex body of the feasible region. \\\\\n"
			"\n"
			"Dantzig then used this theorems to create Simplex, an algorithm that moves through the vertices of the feasible convex body until finding the optimal solution to the problem. Simplex is a mechanical method that uses matrix operations to determine the solution. \\\\\n"
			"\n"
			"To setup the simplex algorithm we will:\n"
			"\\begin{itemize}\n"
			"    \\item For each $\\leq$ constraint add a slack variable $s_i$\n"
			"    \\item For each $\\geq$ constraint add an excess variable $e_i$ and an artificial variable $a_i$.\n"
			"    \\item for each $=$ constraint add an artificial variable $a_i$\n"
			"    \\item for each artificial variable added:\n"
			"    \\begin{itemize}\n"
			"        \\item Add $Ma_i$ to the objective function if minimizing.\n"
			"        \\item Add $-Ma_i$ to the objective function if maximizing.\n"
			"        \\item M is a constant of a large value, but not infinite.\n"
			"    \\end{itemize}\n"
			"\\end{itemize}\n"
			"\n"
			"\\subsection*{Example}\n"
			"\\begin{center}\n"
			"Maximize\n"
			"Z = $x_1$ + $x_2$ \\\\\n"
			"$x_1$ + 2$x_2$ $\\leq$ 3 \\\\\n"
			"4$x_1$ + 5$x_2$ $\\geq$ 6 \\\\\n"
			"7$x_1$ + 8$x_2$ $=$ 9 \\\\\n"
			"\\end{center}\n"
			"\n"
			"Converts to\n"
			"\n"
			"\\begin{center}\n"
			"Maximize\n"
			"Z = $x_1$ + $x_2$ $- Ma_1 - Ma_2$ \\\\\n"
			"$x_1$ + 2$x_2$ + $s_1$ $\\leq$ 3 \\\\\n"
			"4$x_1$ + 5$x_2$ + $e_1$ + $a_1$ $\\geq$ 6 \\\\\n"
			"7$x_1$ + 8$x_2$ + $a_2$ $=$ 9 \\\\\n"
			"\\end{center}\n"
			"\n"
			"\\subsection*{Simplex Table}\n"
			"Using the same example as above, we should construct a matrix called Simplex Table as follows:\n"
			"\\begin{table}[H]\n"
			"\\centering\n"
			"\\begin{adjustbox}{max width=\\textwidth}\n"
			"\\begin{tabular}{c|rrrrrrrr}\n"
			"& Z & $x_1$& $x_2$& $s_1$& $e_1$& $a_1$& $a_2$& b\\\\ \\hline\n"
			"& 1& -1& -1& 0& 0& M& M& 0\\\\\n"
			"& 0& 1& 2& 1& 0& 0& 0& 3\\\\\n"
			"& 0& 4& 5& 0& -1& 1& 0& 6\\\\\n"
			"& 0& 7& 8& 0& 0& 0& 1& 9\\\\\n"
			"\\end{tabular}\n"
			"\\end{adjustbox}\n"
			"\\end{table}\n"
			"\\newpage\n"
			"Execution of Simplex algorithm:\n"
			"\\begin{enumerate}\n"
			"    \\item If there are any artificial variables, our first step should be to remove the Ms in their columns through matrix operations, leaving canonical vectors on artificial variable columns.\n"
			"    \\item if there are any negative\\\\textbackslash positive values in the first row (excluding Z and b) when maximizing\\\\textbackslash minimizing (respectively)\n"
			"    \\begin{enumerate}\n"
			"        \\item Identify the pivot\n"
			"        \\begin{enumerate}\n"
			"            \\item Indetify a column X with the most negative\\\\textbackslash positive (maximizing\\\\textbackslash minimizing), divide all values on column b with their respective value on X.\n"
			"            \\item Exclude 0s and negative values\n"
			"            \\item The pivot will be the cell on column X on the row with the \\textbf{smallest} division result.\n"
			"        \\end{enumerate}\n"
			"        \\item Through matrix operations, create a canonical vector on the column of the pivot, leaving $1$ exactly on the pivot cell.\n"
			"        \\item Go to 2\n"
			"    \\end{enumerate}\n"
			"    \\item The simplex table will contain the optimal solution.\n"
			"    \\begin{itemize}\n"
			"        \\item In column b, the first cell shows the value of Z.\n"
			"        \\item Every variable with a non-canonical vector in their column has a value of 0.\n"
			"        \\item Variables with a canonical vector in their column have a value of what the b column has on the row where they have a 1.\n"
			"    \\end{itemize}\n"
			"\\end{enumerate}\n");
}

void print_with_M(FILE *f, mpfr_t x)
{
	mpfr_prec_t prec = mpfr_get_prec(x);
	mpfr_t abs_x, k;
	mpfr_inits2(prec, abs_x, k, NULL);

	mpfr_abs(abs_x, x, MPFR_RNDN);
	mpfr_t threshold;
	mpfr_init_set_ui(threshold, 99999, MPFR_RNDN);

	if (mpfr_cmp(abs_x, threshold) <= 0)
	{
		mpfr_fprintf(f, "%.6Rg", x);
	}
	else
	{
		mpfr_div(k, x, M, MPFR_RNDN);
		double k_d = mpfr_get_d(k, MPFR_RNDN);

		if (k_d == 1.0)
			fprintf(f, "M");
		else if (k_d == -1.0)
			fprintf(f, "-M");
		else
			fprintf(f, "%.3gM", k_d);
	}

	mpfr_clears(abs_x, k, threshold, NULL);
}

void print_simplex_table(mpfr_t **table, int highlight_row, int highlight_col, int print_fractions)
{
	fprintf(output_file, "\\begin{table}[H]\n");
	fprintf(output_file, "\\centering\n");
	fprintf(output_file, "\\begin{adjustbox}{max width=\\textwidth}");
	fprintf(output_file, "\\begin{tabular}{c|");
	for (int i = 0; i < table_cols; i++)
		fprintf(output_file, "r");
	if (print_fractions)
		fprintf(output_file, "r");

	fprintf(output_file, "}\n");
	fprintf(output_file, "& Z ");
	for (int i = 0; i < variable_amount; i++)
		fprintf(output_file, "& %s", variable_names[i]);
	for (int i = 0; i < slackv_amount; i++)
		fprintf(output_file, "& $s_%d$", i + 1);
	for (int i = 0; i < excessv_amount; i++)
		fprintf(output_file, "& $e_%d$", i + 1);
	for (int i = 0; i < artificialv_amount; i++)
		fprintf(output_file, "& $a_%d$", i + 1);
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
			// mpfr_fprintf(output_file, "%.2Rf", table[i][j]);
			print_with_M(output_file, table[i][j]);
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
	fprintf(output_file, "\\end{adjustbox}\n");
	fprintf(output_file, "\\end{table}\n");
}

void print_problem_model(mpfr_t **table)
{
	fprintf(output_file, "\\newpage\n\\section*{");
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

		int option = gtk_combo_box_get_active(GTK_COMBO_BOX(constraint_combos[i - 1]));
		if (option == 0)
			mpfr_fprintf(output_file, "$\\leq$ ", val);
		if (option == 1)
			mpfr_fprintf(output_file, "$\\geq$ ", val);
		if (option == 2)
			mpfr_fprintf(output_file, "$=$ ", val);
		mpfr_fprintf(output_file, "%.2Rf \\\\\n", val);
		mpfr_clear(val);
	}
	fprintf(output_file, "\\end{center}\n");
	fprintf(output_file, "Simplex Table\n");
	print_simplex_table(table, -1, -1, 0);
}

int feasible(mpfr_t **table) // 0 No, 1 Yes
{
	// Check for artificial vars in BFS
	mpfr_t zero, one;
	mpfr_inits2(256, zero, one, NULL);
	mpfr_set_ui(zero, 0, MPFR_RNDN);
	mpfr_set_ui(one, 1, MPFR_RNDN);
	int is_basic = 1;
	int c = variable_amount + slackv_amount + excessv_amount + 1;
	for (; c <= variable_amount + slackv_amount + excessv_amount + artificialv_amount; c++)
	{
		for (int r = 0; r < table_rows; r++)
		{
			if (mpfr_cmp(table[r][c], zero) != 0)
			{
				if (mpfr_cmp(table[r][c], one) != 0)
				{
					is_basic = 0;
				}
			}
		}
		if (is_basic)
		{
			fprintf(output_file, "\\newpage\n\\section*{Non-Feasible}\n");
			fprintf(output_file, "The artificial variable $a_%d$ is in the base. Therefore, a BFS (basic-feasible-solution) does not exist for this problem.\n", c - variable_amount - slackv_amount - excessv_amount);
			return 0;
		}
	}

	// Check for Ms
	for (c = 0; c < variable_amount + slackv_amount + excessv_amount + 1; c++)
	{
		mpfr_t val;
		mpfr_init2(val, mpfr_get_prec(table[0][c]));
		mpfr_abs(val, table[0][c], MPFR_RNDN);
		if (mpfr_cmp(val, M) > 0)
		{
			fprintf(output_file, "\\newpage\n\\section*{Non-Feasible}\n");
			fprintf(output_file, "The column $%d$ contains an M.Therefore, a BFS (basic-feasible-solution) does not exist for this problem.\n", c + 1);
			return 0;
		}
	}
	return 1;
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
	fprintf(output_file, "\n\\subsection*{Excess}\n");
	for (int i = 0; i < excessv_amount; i++)
	{
		mpfr_set_ui(var_value, 0, MPFR_RNDN); // default = 0

		for (int j = 0; j < constraint_amount + 1; j++)
		{
			if (mpfr_cmp(table[j][i + variable_amount + slackv_amount + 1], zero) != 0)
			{
				if (mpfr_cmp(table[j][i + variable_amount + slackv_amount + 1], one) != 0)
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
		fprintf(output_file, "$e_%d$ = ", i + 1);
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

void check_degeneracy(mpfr_t **table)
{
	for (int j = 0; j < variable_amount + slackv_amount + excessv_amount; j++)
	{
		mpfr_t var_value, is_basic;
		mpfr_inits2(mpfr_get_prec(table[0][0]), var_value, is_basic, NULL);
		mpfr_set_ui(var_value, 0, MPFR_RNDN);
		mpfr_set_ui(is_basic, 0, MPFR_RNDN);

		for (int i = 1; i < table_rows; i++)
		{
			mpfr_t val;
			mpfr_init2(val, mpfr_get_prec(table[0][0]));
			mpfr_set(val, table[i][j + 1], MPFR_RNDN);

			if (mpfr_cmp_ui(val, 0) != 0)
			{
				if (mpfr_cmp_ui(val, 1) == 0)
				{
					mpfr_set(var_value, table[i][table_cols - 1], MPFR_RNDN);
					mpfr_set_ui(is_basic, 1, MPFR_RNDN);
				}
				else
				{
					mpfr_set_ui(is_basic, 0, MPFR_RNDN);
					mpfr_clear(val);
					break;
				}
			}
			mpfr_clear(val);
		}
		if (mpfr_cmp_ui(is_basic, 0) != 0 && mpfr_cmp_ui(var_value, 0) == 0)
		{
			fprintf(output_file, "\n\\subsection*{Degenerate Base}\n");
			if (j + 1 < variable_amount + 1)
				fprintf(output_file, "The variable %s is part of the base but has a value of 0. \n", variable_names[j]);
			else
				fprintf(output_file, "The variable $s_%d$ is part of the base but has a value of 0. ", j - variable_amount + 1);
			fprintf(output_file, "Therefore, this is a degenerate Basic Feasible Solution (BFS).\n\n");
			break;
		}
		mpfr_clears(var_value, is_basic, (mpfr_ptr)0);
	}
}

// Return 1 if can pivot again. Return 0 if cannot pivot again. -1 if unbounded
int pivot(mpfr_t **table, int pivot_col, int pivoting) // ARGS: simplex table, pivot #
{
	if (intermediate_tables) // Print intermediate tables data
	{
		fprintf(output_file, "\\subsection*{Pivoting %d}", pivoting);
		if (mode)
			fprintf(output_file, "\\subsection*{Most Positive}\n");
		else
			fprintf(output_file, "\\subsubsection*{Most Negative}\n");
		fprintf(output_file, "Column %d (", pivot_col + 1, table[0][pivot_col]);
		print_with_M(output_file, table[0][pivot_col]);
		fprintf(output_file, ")\n", pivot_col + 1, table[0][pivot_col]);
		print_simplex_table(table, 0, pivot_col, 0);
		fprintf(output_file, "\\subsubsection*{Fractions}\n");
		print_simplex_table(table, -1, -1, pivot_col);
	}

	int smallest_frac = 1; // The smallest fraction row
	int is_unbounded = 1;
	mpfr_t frac, tmp, zero;
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
		else if (pivot_col < variable_amount + slackv_amount + 1)
		{
			snprintf(varbuf, sizeof(varbuf), "$s_%d$", pivot_col - variable_amount);
			var_name = varbuf;
		}
		else if (pivot_col < variable_amount + slackv_amount + excessv_amount + 1)
		{
			snprintf(varbuf, sizeof(varbuf), "$e_%d$", pivot_col - variable_amount - slackv_amount);
			var_name = varbuf;
		}
		else
		{
			snprintf(varbuf, sizeof(varbuf), "unknown");
			var_name = varbuf;
		}
		report_unbounded(var_name);
		return -1; // -> unbounded
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
		check_degeneracy(table);
	}
	if (intermediate_tables)
		fprintf(output_file, "\n\\newpage\n");

	return 1; // Return 1 -> pivot again
}

void multiple_solutions(mpfr_t **table, int pivoting)
{
	// Check for multiple solutions
	int is_basic = 1;
	int pivot_col = 0;
	for (int c = 0; c < variable_amount + slackv_amount + excessv_amount; c++)
	{
		if (mpfr_cmp_ui(table[0][c + 1], 0) != 0)
			continue;
		for (int r = 1; r < table_rows; r++)
		{
			mpfr_t val;
			mpfr_init_set(val, table[r][c + 1], MPFR_RNDN);
			if (mpfr_cmp_ui(val, 0) != 0)
			{
				if (mpfr_cmp_si(val, 1) == 0)
				{
					is_basic = 1;
				}
				else
				{
					is_basic = 0;
					pivot_col = c + 1;
					break;
				}
			}
			mpfr_clear(val);
		}
		if (!is_basic)
			break;
	}

	// pivot again if possible
	if (!is_basic)
	{
		fprintf(output_file, "\\newpage\n\\section*{Multiple Solutions}\n");
		fprintf(output_file, "A non-basic variable has a 0 on its first row, allowing us to pivot again and find another optimal solution.\n\n");
		pivot(table, pivot_col, pivoting);
		print_results(table);
	}
}

void simplex(mpfr_t **table)
{
	is_degenerate = 0;
	int pivoting = 0;

	// Artificial variables canonization
	int j = variable_amount + slackv_amount + excessv_amount + 1;
	if (intermediate_tables && artificialv_amount > 0)
		fprintf(output_file, "\\newpage\n\\section*{Artificial Variables \"Canonization\"}\n");
	for (int i = 0; i < artificialv_amount; i++)
	{
		mpfr_t a_val;
		mpfr_init2(a_val, 256);
		mpfr_set(a_val, table[0][j], MPFR_RNDN);

		int one_row = -1;

		for (int r = 1; r < table_rows; r++)
		{
			if (mpfr_cmp_si(table[r][j], 1) == 0)
			{
				one_row = r;
				break;
			}
		}

		if (one_row == -1)
		{
			g_print("Error: failed canonization of artificial variables.\n");
			return;
		}

		mpfr_t neg_a, to_add;
		mpfr_inits2(256, neg_a, to_add, (mpfr_ptr)0);
		mpfr_neg(neg_a, a_val, MPFR_RNDN);
		for (int c = 1; c < table_cols; c++)
		{
			mpfr_mul(to_add, neg_a, table[one_row][c], MPFR_RNDN);
			mpfr_add(table[0][c], table[0][c], to_add, MPFR_RNDN);
		}
		if (intermediate_tables)
		{
			fprintf(output_file, "\\subsection*{Variable $a_%d$}\n", i + 1);
			fprintf(output_file, "$R_1 \\leftarrow R_1+");
			print_with_M(output_file, to_add);
			fprintf(output_file, "R_%d$ \\\\\n", one_row + 1);
			print_simplex_table(table, -1, -1, 0);
		}

		mpfr_clear(neg_a);
		mpfr_clear(to_add);
		mpfr_clear(a_val);

		j++;
	}

	if (intermediate_tables)
		fprintf(output_file, "\\newpage\n\\section*{Intermediate Tables}\n");

	int can_pivot = 1;
	while (can_pivot == 1)
	{
		pivoting++;
		int pivot_col = 1; // The most negative/positive column (index, not value)
		int pivoting_cols = variable_amount + slackv_amount + excessv_amount;
		for (int i = 2; i <= pivoting_cols; i++)
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
			can_pivot = 0;
			break;
		}
		if (mode == 1 && mpfr_cmp(table[0][pivot_col], zero) <= 0)
		{
			mpfr_clear(zero);
			can_pivot = 0;
			break;
		}
		mpfr_clear(zero);

		can_pivot = pivot(table, pivot_col, pivoting);

		if (pivoting > 51)
			break;
	}
	if (!can_pivot)
	{
		if (feasible(table))
		{
			print_results(table);
			multiple_solutions(table, pivoting);
		}
	}
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
		gtk_spin_button_set_digits(GTK_SPIN_BUTTON(coef_spin), 3);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(coef_spin), 1.000);
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
	constraint_amount = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(constraints_spin));
	variable_amount = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(variables_spin));

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

	current_constraints = constraint_amount;
	current_variables = variable_amount;
	if (constraint_amount > 0 && variable_amount > 0)
	{
		constraint_label_widgets = malloc(sizeof(GtkWidget **) * constraint_amount);
		for (int c = 0; c < constraint_amount; c++)
		{
			constraint_label_widgets[c] = malloc(sizeof(GtkWidget *) * variable_amount);
			for (int v = 0; v < variable_amount; v++)
				constraint_label_widgets[c][v] = NULL;
		}
	}
	for (int c = 0; c < constraint_amount; ++c)
	{

		GtkWidget *row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);

		for (int v = 0; v < variable_amount; ++v)
		{
			GtkWidget *coef_spin = gtk_spin_button_new_with_range(-99999, 99999, 1);
			gtk_spin_button_set_digits(GTK_SPIN_BUTTON(coef_spin), 3);
			gtk_spin_button_set_value(GTK_SPIN_BUTTON(coef_spin), 1.000);
			gtk_box_pack_start(GTK_BOX(row), coef_spin, FALSE, FALSE, 0);

			if (spin_table)
				spin_table[c + 1][v] = coef_spin;

			const char *var_name = gtk_entry_get_text(GTK_ENTRY(variable_entries[v]));

			gchar *label_text;
			if (v < variable_amount - 1)
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
		constraint_combos[c] = comp_combo;

		GtkWidget *rhs_spin = gtk_spin_button_new_with_range(-99999, 99999, 1);
		gtk_spin_button_set_digits(GTK_SPIN_BUTTON(rhs_spin), 2);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(rhs_spin), 0.00);
		gtk_box_pack_start(GTK_BOX(row), rhs_spin, FALSE, FALSE, 0);

		if (spin_table)
			spin_table[c + 1][variable_amount] = rhs_spin;

		gtk_box_pack_start(GTK_BOX(constraints_container), row, FALSE, FALSE, 4);
	}

	gtk_widget_show_all(constraints_container);
}

// TODO: call this every time setup spins change, and remove "Update" button
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

	if (constraint_combos != NULL)
	{
		free(constraint_combos);
		constraint_combos = NULL;
	}
	constraint_combos = malloc(sizeof(GtkWidget *) * num_constraints);

	build_objective();
	build_constraints();
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

	mpfr_set_d(mtable[0][0], 1.0, MPFR_RNDN);
	int slack_count = 1, excess_count = 1, artificial_count = 1;
	for (int i = 0; i < table_rows; i++)
	{
		for (int j = 1; j < table_cols; j++)
		{
			if (j <= variable_amount) // variable columns
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
			else if (slackv_amount > 0 && j <= variable_amount + slackv_amount && i > 0) // slack columns
			{
				int option = gtk_combo_box_get_active(GTK_COMBO_BOX(constraint_combos[i - 1]));
				if (option == 0)
				{
					if (j - variable_amount == slack_count)
					{
						mpfr_set_d(mtable[i][j], 1.0, MPFR_RNDN);
						slack_count++;
						j = variable_amount + slackv_amount;
					}
				}
			}
			else if (excessv_amount > 0 && j <= variable_amount + slackv_amount + excessv_amount && i > 0) // excess columns
			{
				int option = gtk_combo_box_get_active(GTK_COMBO_BOX(constraint_combos[i - 1]));
				if (option == 1)
				{
					if (j - variable_amount - slackv_amount == excess_count)
					{
						mpfr_set_d(mtable[i][j], -1.0, MPFR_RNDN);
						excess_count++;
						j = variable_amount + slackv_amount + excessv_amount;
					}
				}
			}
			else if (artificialv_amount > 0 && j < table_cols - 1 && i > 0) // artificial columns
			{
				int option = gtk_combo_box_get_active(GTK_COMBO_BOX(constraint_combos[i - 1]));
				if (option == 1 || option == 2)
				{
					if (j - variable_amount - slackv_amount - excessv_amount == artificial_count)
					{
						mpfr_set_d(mtable[i][j], 1.0, MPFR_RNDN);
						artificial_count++;
						j = variable_amount + slackv_amount + excessv_amount + artificialv_amount;
					}
				}
			}
			else if (artificialv_amount > 0 &&
					 j <= variable_amount + slackv_amount + excessv_amount + artificialv_amount &&
					 j > variable_amount + slackv_amount + excessv_amount) // artificial columns in first row
			{
				if (mode == 1)
					mpfr_neg(mtable[i][j], M, MPFR_RNDN);
				else
					mpfr_set(mtable[i][j], M, MPFR_RNDN);
			}
			else if (j == table_cols - 1) // b column
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
	// Init variables
	mpfr_init2(M, 256);
	mpfr_set_ui(M, 1000000, MPFR_RNDN);
	slackv_amount = 0;
	excessv_amount = 0;
	artificialv_amount = 0;

	// constraint types
	for (int i = 0; i < constraint_amount; i++)
	{
		int option = gtk_combo_box_get_active(GTK_COMBO_BOX(constraint_combos[i]));
		if (option == 0)
		{
			slackv_amount++;
		}
		else if (option == 1)
		{
			excessv_amount++;
			artificialv_amount++;
		}
		else if (option == 2)
		{
			artificialv_amount++;
		}
	}

	table_rows = constraint_amount + 1;
	table_cols = 2 + variable_amount + slackv_amount + excessv_amount + artificialv_amount;

	GtkComboBox *combo = GTK_COMBO_BOX(max_min_combo);
	mode = gtk_combo_box_get_active(combo);

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
	GtkWidget *popover = GTK_WIDGET(button);
	while (popover && !GTK_IS_POPOVER(popover))
		popover = gtk_widget_get_parent(popover);
	if (popover)
		gtk_popover_popdown(GTK_POPOVER(popover)); // Hide popover menu

	GtkWidget *steps_btn = GTK_WIDGET(gtk_builder_get_object(builder, "btn_steps"));
	GtkWidget *solution_btn = GTK_WIDGET(gtk_builder_get_object(builder, "btn_solution"));

	if (GTK_WIDGET(button) == steps_btn)
		intermediate_tables = 1;
	else if (GTK_WIDGET(button) == solution_btn)
		intermediate_tables = 0;

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

void save_data_to_file(const char *filename)
{
	FILE *file = fopen(filename, "w");
	if (file == NULL)
	{
		return;
	}

	// Save metadata
	constraint_amount = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(constraints_spin));
	variable_amount = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(variables_spin));
	problem_name = gtk_entry_get_text(GTK_ENTRY(problem_input));
	fprintf(file, "problem=%s\n", problem_name);
	fprintf(file, "var_amount=%d\n", variable_amount);
	fprintf(file, "contraint_amount=%d\n", constraint_amount);

	GtkComboBox *combo = GTK_COMBO_BOX(max_min_combo);
	mode = gtk_combo_box_get_active(combo);
	fprintf(file, "mode=%d\n", mode);

	fprintf(file, "VARIABLES\n");
	for (int i = 0; i < variable_amount; i++)
	{
		const char *txt = gtk_entry_get_text(GTK_ENTRY(variable_entries[i]));
		fprintf(file, "%s\n", txt);
	}

	fprintf(file, "OBJECTIVE\n");
	for (int i = 0; i < variable_amount; i++)
	{
		double val = gtk_spin_button_get_value(GTK_SPIN_BUTTON(spin_table[0][i]));
		fprintf(file, "%.3f\n", val);
	}
	fprintf(file, "CONSTRAINTS\n");
	for (int i = 1; i <= constraint_amount; i++)
	{
		for (int j = 0; j <= variable_amount; j++)
		{
			double val = gtk_spin_button_get_value(GTK_SPIN_BUTTON(spin_table[i][j]));
			fprintf(file, "%.3f\n", val);
		}
		int option = gtk_combo_box_get_active(GTK_COMBO_BOX(constraint_combos[i - 1]));
		fprintf(file, "%d\n", option);
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
			gtk_entry_set_text(GTK_ENTRY(problem_input), strdup(val));
		}
		else if (strncmp(trimmed, "var_amount=", 11) == 0)
		{
			sscanf(trimmed + 11, "%d", &variable_amount);
			gtk_spin_button_set_value(GTK_SPIN_BUTTON(variables_spin), variable_amount);
		}
		else if (strncmp(trimmed, "contraint_amount=", 17) == 0)
		{
			sscanf(trimmed + 17, "%d", &constraint_amount);
			gtk_spin_button_set_value(GTK_SPIN_BUTTON(constraints_spin), constraint_amount);
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
			setup_input_page();
		}
		else if (strcmp(trimmed, "OBJECTIVE") == 0)
		{
			for (int j = 0; j < variable_amount; j++)
			{
				fgets(line, sizeof(line), file);
				char tmp[128];
				strncpy(tmp, line, sizeof(tmp));
				tmp[sizeof(tmp) - 1] = '\0';
				size_t lt = strlen(tmp);
				if (lt > 0 && tmp[lt - 1] == '\n')
					tmp[lt - 1] = '\0';
				gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin_table[0][j]), atof(tmp));
			}
		}
		else if (strcmp(trimmed, "CONSTRAINTS") == 0)
		{
			for (int i = 0; i < constraint_amount; i++)
			{
				for (int j = 0; j <= variable_amount; j++)
				{
					fgets(line, sizeof(line), file);
					char tmp[128];
					strncpy(tmp, line, sizeof(tmp));
					tmp[sizeof(tmp) - 1] = '\0';
					size_t lt = strlen(tmp);
					if (lt > 0 && tmp[lt - 1] == '\n')
						tmp[lt - 1] = '\0';
					gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin_table[i + 1][j]), atof(tmp));
				}
				fgets(line, sizeof(line), file);
				char tmp[128];
				strncpy(tmp, line, sizeof(tmp));
				tmp[sizeof(tmp) - 1] = '\0';
				size_t lt = strlen(tmp);
				if (lt > 0 && tmp[lt - 1] == '\n')
					tmp[lt - 1] = '\0';
				gtk_combo_box_set_active(GTK_COMBO_BOX(constraint_combos[i]), atoi(tmp));
			}
		}
	}

	fclose(file);
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

int main(int argc, char *argv[])
{
	gtk_init(&argc, &argv);

	// GtkBuilder *builder = gtk_builder_new_from_file("Simplex/simplex.glade"); // Si se abre desde el menu
	builder = gtk_builder_new_from_file("simplex.glade"); // Si se abre SIN en menu

	main_window = GTK_WIDGET(gtk_builder_get_object(builder, "hWindow"));

	gtk_builder_connect_signals(builder, NULL);

	// exit
	g_signal_connect(main_window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

	problem_input = GTK_WIDGET(gtk_builder_get_object(builder, "problem_name"));
	variables_spin = GTK_WIDGET(gtk_builder_get_object(builder, "variables_spin"));
	constraints_spin = GTK_WIDGET(gtk_builder_get_object(builder, "constraints_spin"));
	objective_container = GTK_WIDGET(gtk_builder_get_object(builder, "objective_container"));
	constraints_container = GTK_WIDGET(gtk_builder_get_object(builder, "constraints_container"));
	max_min_combo = GTK_WIDGET(gtk_builder_get_object(builder, "max_min_combo"));

	setup_input_page();
	gtk_widget_show_all(main_window);

	gtk_main();

	g_object_unref(builder);

	return 0;
}
