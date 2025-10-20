#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

GtkWidget *main_window;
GtkWidget *main_stack;
GtkWidget *variables_container;
GtkWidget *objective_container;
GtkWidget *constraints_container;
GtkWidget *problem_input; // problem name
GtkWidget *variables_spin;
GtkWidget *constraints_spin;

GtkWidget *variable_widgets[15];
GtkWidget *coefficient_widgets[15];
char *variable_names[15];

char *problem_name;
int variable_amount;
int constraint_amount;
int mode = 0; // 0 for max, 1 for min;
int constraint_page_count = 0;

double **simplex_table;
int table_cols;
int table_rows;

void simplex()
{
	while (1)
	{
		for (int i = 0; i < table_rows; i++)
		{
			for (int j = 0; j < table_cols; j++)
			{
				g_print("%f  ", simplex_table[i][j]);
			}
			g_print("\n");
		}
		g_print("\n");
		int most_negative = 0; // The most negative column (index, not value)
		for (int i = 0; i < table_cols; i++)
		{
			if (simplex_table[0][i] < simplex_table[0][most_negative])
				most_negative = i;
		}
		if (simplex_table[0][most_negative] >= 0)
			break;

		int smallest_frac = 1; // The most negative fraction row
		for (int i = 2; i < table_rows; i++)
		{
			if (simplex_table[i][table_cols - 1] / simplex_table[i][most_negative] < simplex_table[smallest_frac][table_cols - 1] / simplex_table[smallest_frac][most_negative])
				smallest_frac = i;
		}

		// Make a 1 on smallest frac cell
		double div_value = simplex_table[smallest_frac][most_negative];
		for (int i = 0; i < table_cols; i++)
		{
			simplex_table[smallest_frac][i] = simplex_table[smallest_frac][i] / div_value;
		}

		// Convert col to 0s
		for (int i = 0; i < table_rows; i++)
		{
			if (i == smallest_frac)
				continue;

			double mult_value = -simplex_table[i][most_negative];
			for (int j = 0; j < table_cols; j++)
			{
				simplex_table[i][j] = simplex_table[i][j] + mult_value * simplex_table[smallest_frac][j];
			}
		}
	}

	// print table
	for (int i = 0; i < table_rows; i++)
	{
		for (int j = 0; j < table_cols; j++)
		{
			g_print("%f  ", simplex_table[i][j]);
		}
		g_print("\n");
	}
}

void fill_simplex_row(int row) // Fills a row with coefficients from coefficient_widgets
{
	if (!simplex_table)
	{
		table_rows = constraint_amount + 1;
		table_cols = (2 * variable_amount) + 2;
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
			{
				simplex_table[row][j] = 0;
				break;
			}
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
			simplex_table[row][j] = (double)-gtk_spin_button_get_value(GTK_SPIN_BUTTON(coefficient_widgets[j]));
		else
			simplex_table[row][j] = (double)gtk_spin_button_get_value(GTK_SPIN_BUTTON(coefficient_widgets[j]));
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
		simplex();
		return;
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
	for (int i = 0; i < variable_amount; i++)
	{
		GtkWidget *object_widget = create_objective_variable_widget(i);
		gtk_box_pack_start(GTK_BOX(constraints_container), object_widget, FALSE, FALSE, 5);
	}
	variable_names[variable_amount] = "Constant term (Right-Hand Side)";
	GtkWidget *object_widget = create_objective_variable_widget(variable_amount);
	gtk_box_pack_start(GTK_BOX(constraints_container), object_widget, FALSE, FALSE, 5);
	gtk_widget_show_all(constraints_container);
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
	variables_spin = GTK_WIDGET(gtk_builder_get_object(builder, "variables_spin"));
	constraints_spin = GTK_WIDGET(gtk_builder_get_object(builder, "constraints_spin"));
	variables_container = GTK_WIDGET(gtk_builder_get_object(builder, "variables_container"));
	objective_container = GTK_WIDGET(gtk_builder_get_object(builder, "objective_container"));
	constraints_container = GTK_WIDGET(gtk_builder_get_object(builder, "constraints_container"));

	GtkWidget *problem_input; // problem name
	GtkWidget *variables_spin;
	GtkWidget *constraints_spin;

	gtk_widget_show_all(main_window);

	gtk_main();

	g_object_unref(builder);

	return 0;
}
