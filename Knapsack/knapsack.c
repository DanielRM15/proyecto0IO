#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

FILE *output_file;
GtkWidget *main_window;
GtkWidget *main_stack;
GtkWidget *capacity_spin;
GtkWidget *objects_spin;
GtkWidget *objects_container;
GtkWidget *bounded_radio;
GtkWidget *unbounded_radio;
GtkWidget *onezero_radio;

int capacity = 0;
int objects_amount = 0;
char knapsack_type = 'B'; //'B': bounded; 'U': unbounded; 'O': 1/0

typedef struct
{
	int value;	 // Value gained at position ij
	int amount;	 // Amount of object i taken
	int amount2; // Amount2 if there is a tie
} TableItem;
TableItem **table;

typedef struct
{
	GtkWidget *name_txt;
	GtkWidget *value_spin;
	GtkWidget *cost_spin;
	GtkWidget *available_spin;
	GtkWidget *main_widget;
} ObjectWidgetData;
ObjectWidgetData object_widgets[12];

typedef struct
{
	char *name;
	int value;
	int cost;
	int amount;
} Object;
Object *objects;

void knapsack()
{
	for (int i = 0; i < objects_amount; i++)
	{
		table[i] = malloc((capacity + 1) * sizeof(TableItem));
		for (int j = 0; j <= capacity; j++)
		{
			int val;
			if (i > 0)
				val = table[i - 1][j].value;
			else
				val = 0;

			int Q = 0;
			int take_amount = -1;
			int take_amount2 = 0;
			while (Q * objects[i].cost <= j && Q <= objects[i].amount)
			{
				int newval;
				if (i > 0)
					newval = Q * objects[i].value + table[i - 1][j - Q * objects[i].cost].value;
				else
					newval = Q * objects[i].value;

				if (newval > val)
				{
					take_amount = Q;
					val = newval;
					take_amount2 = -1;
				}
				else if (newval == val)
				{
					if (Q > 0)
						take_amount = Q;
				}
				Q++;
			}
			if (take_amount == -1)
			{
				take_amount = take_amount2;
				take_amount2 = -1;
			}
			TableItem table_item;
			table_item.value = val;
			table_item.amount = take_amount;
			table_item.amount2 = take_amount2;
			table[i][j] = table_item;
		}
	}
}

GtkWidget *create_object_widget(int object_id)
{
	GtkBuilder *builder = gtk_builder_new_from_file("Knapsack/objectSetupWidget.glade");
	GtkWidget *elem = GTK_WIDGET(gtk_builder_get_object(builder, "objectSetupWidget"));
	g_object_ref(elem);

	GtkWidget *title_label = GTK_WIDGET(gtk_builder_get_object(builder, "objectLabel"));
	if (title_label)
	{
		char title_text[50];
		sprintf(title_text, "Object %d", object_id + 1);
		gtk_label_set_text(GTK_LABEL(title_label), title_text);
	}

	// Store references to the widgets
	object_widgets[object_id].main_widget = elem;
	object_widgets[object_id].name_txt = GTK_WIDGET(gtk_builder_get_object(builder, "name_txt"));
	object_widgets[object_id].value_spin = GTK_WIDGET(gtk_builder_get_object(builder, "value_spin"));
	object_widgets[object_id].cost_spin = GTK_WIDGET(gtk_builder_get_object(builder, "cost_spin"));
	object_widgets[object_id].available_spin = GTK_WIDGET(gtk_builder_get_object(builder, "available_spin"));

	g_object_unref(builder);
	return elem;
}

void on_continueBtn_clicked(GtkButton *button, gpointer user_data)
{
	capacity = (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(capacity_spin));
	objects_amount = (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(objects_spin));
	if (!capacity || !objects_amount)
	{
		return;
	}

	for (int i = 0; i < objects_amount; i++)
	{
		GtkWidget *object_widget = create_object_widget(i);
		gtk_box_pack_start(GTK_BOX(objects_container), object_widget, FALSE, FALSE, 5);
	}

	gtk_stack_set_visible_child_name(GTK_STACK(main_stack), "page1");
	gtk_widget_show_all(objects_container);
}

void on_radio_toggled(GtkToggleButton *toggle_button, gpointer user_data)
{
	const gchar *name = gtk_buildable_get_name(GTK_BUILDABLE(toggle_button));

	if (gtk_toggle_button_get_active(toggle_button))
	{
		if (g_strcmp0(name, "bounded_radio") == 0)
		{
			for (int i = 0; i < objects_amount; i++)
			{
				gtk_spin_button_set_value(GTK_SPIN_BUTTON(object_widgets[i].available_spin), 1);
				gtk_widget_set_sensitive(object_widgets[i].available_spin, TRUE);
				knapsack_type = 'B';
			}
		}
		else if (g_strcmp0(name, "unbounded_radio") == 0)
		{
			for (int i = 0; i < objects_amount; i++)
			{
				gtk_spin_button_set_value(GTK_SPIN_BUTTON(object_widgets[i].available_spin), 999);
				gtk_entry_set_text(GTK_ENTRY(object_widgets[i].available_spin), "∞");
				gtk_widget_set_sensitive(object_widgets[i].available_spin, FALSE);
				knapsack_type = 'U';
			}
		}
		else if (g_strcmp0(name, "onezero_radio") == 0)
		{
			for (int i = 0; i < objects_amount; i++)
			{
				gtk_spin_button_set_value(GTK_SPIN_BUTTON(object_widgets[i].available_spin), 1);
				gtk_widget_set_sensitive(object_widgets[i].available_spin, FALSE);
				knapsack_type = 'O';
			}
		}
	}
}

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
			"\\newpage\n"
			"\\section*{The Knapsack Problem}\n"
			"The \\textbf{Knapsack Problem} is a classic optimization problem in computer science and operations research. It consists of having a set of items, each with a weight and a value. We also have a knapsack with limited capacity. The goal is to select a subset of items so that the total weight does not exceed the capacity of the knapsack, while the total value gained is maximized.\n"
			"\\[\n"
			"\\max(Z) = \\sum x_i v_i  \\\n"
			"\\]\n"
			"\\[\\text{subject to} \\quad \\sum x_i c_i \\leq C\n"
			"\\]\n"
			"\\begin{itemize}\n"
			"\\item Z: gained value (we want to maximize it)\n"
			"\\item C: knapsack capacity\n"
			"\\item $v_i$: value of item i\n"
			"\\item $c_i$: cost of item i\n"
			"\\item $x_i$: amount of item i taken\n"
			"\\end{itemize}\n"
			"\\subsection*{0/1 Knapsack Problem}\n"
			"In the \\textbf{0/1 Knapsack Problem}, items exist only once. We can either take it or leave it. We cannot take multiple of the same item.\n"
			"\\subsection*{Bounded Knapsack Problem}\n"
			"In the \\textbf{Bounded Knapsack Problem}, we can take multiple copies of each item. The items are limited and we have to choose how many to take of each.\n"
			"\\subsection*{Unbounded Knapsack Problem}\n"
			"In the \\textbf{Unbounded Knapsack Problem}, there are infinite copies of each item. We can take as much as we please of any item.\n"
			"\\subsection*{Knapsack algorithm}\n"
			"\\begin{itemize}\n"
			"\\item We have $k_i$ copies of item i\n"
			"\\item We can include up to Q copies of item i\n"
			"\\end{itemize}\n"
			"\\[\n"
			"Q = \\min(k_j, \\left\\lfloor \\frac{i}{c_j} \\right\\rfloor)\n"
			"\\]\n"
			"\\newpage\n"
			"\\begin{itemize}\n"
			"\\item Now we fill up a $C \\times k_i$ sized table like this:\n"
			"\\end{itemize}\n"
			"\\begin{align*}\n"
			"\\text{Table}[i][j] = \\max \\Big(&\n"
			"\\text{Table}[i][j-1], \\\\\n"
			"&1 \\cdot v_i + \\text{Table}[i-1 \\cdot c_i][j-1], \\\\\n"
			"&2 \\cdot v_i + \\text{Table}[i-2 \\cdot c_i][j-1], \\\\\n"
			"&\\dots, \\\\\n"
			"&Q \\cdot v_i + \\text{Table}[i-Q \\cdot c_i][j-1]\n"
			"\\Big)\n"
			"\\end{align*}\n"
			"\\begin{itemize}\n"
			"\\item In each cell we save the maximum possible value, and the amount of the item we should take to get it.\n"
			"\\end{itemize}\n");
}

void print_problem()
{
	fprintf(output_file, "\\newpage\n");
	fprintf(output_file, "\\section*{Problem}\n");
	fprintf(output_file, "\\begin{large}\n");
	fprintf(output_file, "\\noindent\n");
	fprintf(output_file, "\\textbf{Maximize} \\\\\n");
	fprintf(output_file, "\\hspace*{1cm}$Z = ");
	for (int i = 0; i < objects_amount; i++)
	{
		if (objects[i].value > 1)
			fprintf(output_file, "%d", objects[i].value);
		fprintf(output_file, "x_%d ", i + 1);
		if (i < objects_amount - 1)
			fprintf(output_file, "+ ");
	}
	fprintf(output_file, "$ \\\\[1em]\n");
	fprintf(output_file, "\\noindent\n");
	fprintf(output_file, "\\textbf{Subject to} \\\\\n");
	fprintf(output_file, "\\hspace*{1cm}$");
	for (int i = 0; i < objects_amount; i++)
	{
		if (objects[i].cost > 1)
			fprintf(output_file, "%d", objects[i].cost);
		fprintf(output_file, "x_{%d} ", i + 1);
		if (i < objects_amount - 1)
			fprintf(output_file, "+ ");
	}
	fprintf(output_file, "\\leq %d$\\\\\n", capacity);
	for (int i = 0; i < objects_amount; i++)
	{
		fprintf(output_file, "\\hspace*{1cm}$0 \\leq x_%d ", i + 1);
		if (knapsack_type != 'U')
			fprintf(output_file, "\\leq %d", objects[i].amount);
		fprintf(output_file, "$\\\\\n");
	}
	fprintf(output_file, "\\end{large}\n");
}

void print_knapsack_latex(int start_j, int end_j)
{
	// if (objects_amount > 7)
	// 	fprintf(output_file, "\\begin{landscape}\n");
	// else
	fprintf(output_file, "\\newpage\n");
	fprintf(output_file, "\\section*{Table}\n");
	// fprintf(output_file, "\\resizebox{\\textwidth}{!}{%%\n");
	fprintf(output_file, "\\begin{longtable}{|c|");
	for (int i = start_j; i < end_j; i++)
		fprintf(output_file, "c|");
	fprintf(output_file, "}\n");
	fprintf(output_file, "\\hline\n");
	for (int i = start_j; i < end_j; i++)
		fprintf(output_file, "&%s", objects[i].name);
	fprintf(output_file, "\\\\\n\\hline\n");
	for (int i = 0; i <= capacity; i++)
	{
		fprintf(output_file, "%d ", i);
		for (int j = start_j; j < end_j; j++)
		{
			if (table[j][i].amount2 != -1)
				fprintf(output_file, "& \\cellcolor{orange!30} ");
			else if (table[j][i].amount > 0)
				fprintf(output_file, "& \\cellcolor{green!30} ");
			else
				fprintf(output_file, "& \\cellcolor{red!30} ");
			fprintf(output_file, "%d/", table[j][i].value);
			if (table[j][i].amount == 0)
				fprintf(output_file, "$x_%d=%d$ ", j, 0);
			else if (table[j][i].amount2 == -1)
				fprintf(output_file, "$x_%d=%d$ ", j, table[j][i].amount);
			else
				fprintf(output_file, "$x_%d=%d-%d$ ", j, table[j][i].amount, table[j][i].amount2);
		}
		fprintf(output_file, "\\\\\n");
	}
	fprintf(output_file, "\\hline\n");
	fprintf(output_file, "\\end{longtable}\n");
	// if (objects_amount > 7)
	// 	fprintf(output_file, "\\end{landscape}\n");
}

void print_solution_latex(int track_table[objects_amount][capacity + 1])
{
	fprintf(output_file, "\\newpage");
	fprintf(output_file, "\\section*{Optimal Solution}\n");
	fprintf(output_file, "\\begin{large}\n");
	fprintf(output_file, "Z = %d\\\\\n", table[objects_amount - 1][capacity].value);

	int last_decision_i = -1;
	int last_decision_j = -1;
	int consumed = 0;
	for (int i = 0; i < objects_amount; i++)
	{
		TableItem cell = table[objects_amount - 1 - i][capacity - consumed];
		if (cell.amount2 != -1)
		{
			if (track_table[objects_amount - 1 - i][capacity - consumed] == 0)
			{
				last_decision_i = objects_amount - 1 - i;
				last_decision_j = capacity - consumed;
				fprintf(output_file, "$x_%d$ = %d\\\\\n", objects_amount - i, cell.amount2);
				consumed += cell.amount2 * objects[objects_amount - 1 - i].cost;
			}
			else
			{
				fprintf(output_file, "$x_%d$ = %d\\\\\n", objects_amount - i, cell.amount);
				consumed += cell.amount * objects[objects_amount - 1 - i].cost;
			}
		}
		else
		{
			fprintf(output_file, "$x_%d$ = %d\\\\\n", objects_amount - i, cell.amount);
			consumed += cell.amount * objects[objects_amount - 1 - i].cost;
		}
	}
	fprintf(output_file, "\\end{large}\n");

	if (last_decision_i != -1)
	{
		track_table[last_decision_i][last_decision_j] = 1;
		print_solution_latex(track_table);
	}
}

void on_runBtn_clicked(GtkButton *button, gpointer user_data)
{
	output_file = fopen("Knapsack/output.tex", "w");
	if (output_file == NULL)
	{
		g_print("Failed to open LaTeX file");
		return;
	}
	table = malloc(objects_amount * sizeof(TableItem *));
	objects = malloc(objects_amount * sizeof(Object));
	for (int i = 0; i < objects_amount; i++)
	{
		Object obj;
		obj.name = gtk_entry_get_text(GTK_ENTRY(object_widgets[i].name_txt));
		obj.value = (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(object_widgets[i].value_spin));
		obj.cost = (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(object_widgets[i].cost_spin));
		obj.amount = (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(object_widgets[i].available_spin));
		objects[i] = obj;
	}

	knapsack();
	setup_latex();
	print_problem();
	int x = 0;
	while (x < objects_amount)
	{
		int until = x + 5;
		print_knapsack_latex(x, until > objects_amount ? objects_amount : until);
		x = until;
	}
	int track_table[objects_amount][capacity + 1];
	memset(track_table, 0, sizeof track_table);
	print_solution_latex(track_table);
	fprintf(output_file, "\\end{document}");
	fclose(output_file);

	for (int i = 0; i < objects_amount; i++)
		free(table[i]);
	free(table);
	free(objects);

	system("pdflatex Knapsack/output.tex");
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
	fprintf(file, "KNAPSACK_DATA_V1\n");
	fprintf(file, "capacity=%d\n", capacity);
	fprintf(file, "objects_amount=%d\n", objects_amount);
	fprintf(file, "knapsack_type=%c\n", knapsack_type);

	// Save objects data
	for (int i = 0; i < objects_amount; i++)
	{
		const char *name = gtk_entry_get_text(GTK_ENTRY(object_widgets[i].name_txt));
		int value = (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(object_widgets[i].value_spin));
		int cost = (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(object_widgets[i].cost_spin));
		int amount = (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(object_widgets[i].available_spin));

		fprintf(file, "object_%d_name=%s\n", i, name);
		fprintf(file, "object_%d_value=%d\n", i, value);
		fprintf(file, "object_%d_cost=%d\n", i, cost);
		fprintf(file, "object_%d_amount=%d\n", i, amount);
	}

	fclose(file);
}

void load_data_from_file(const char *filename)
{
	FILE *file = fopen(filename, "r");
	if (file == NULL)
	{
		return;
	}

	char line[256];
	char header[256];

	// Read and verify header
	if (fgets(header, sizeof(header), file) == NULL ||
		strncmp(header, "KNAPSACK_DATA_V1", 16) != 0)
	{
		fclose(file);
		return;
	}

	// Clear existing objects first
	GList *children = gtk_container_get_children(GTK_CONTAINER(objects_container));
	for (GList *iter = children; iter != NULL; iter = g_list_next(iter))
	{
		gtk_widget_destroy(GTK_WIDGET(iter->data));
	}
	g_list_free(children);

	// Initialize object_widgets array
	for (int i = 0; i < 10; i++)
	{
		object_widgets[i].main_widget = NULL;
		object_widgets[i].name_txt = NULL;
		object_widgets[i].value_spin = NULL;
		object_widgets[i].cost_spin = NULL;
		object_widgets[i].available_spin = NULL;
	}

	// Read metadata
	while (fgets(line, sizeof(line), file))
	{
		// Remove newline
		line[strcspn(line, "\n")] = 0;

		if (strncmp(line, "capacity=", 9) == 0)
		{
			capacity = atoi(line + 9);
			gtk_spin_button_set_value(GTK_SPIN_BUTTON(capacity_spin), capacity);
		}
		else if (strncmp(line, "objects_amount=", 15) == 0)
		{
			objects_amount = atoi(line + 15);
			gtk_spin_button_set_value(GTK_SPIN_BUTTON(objects_spin), objects_amount);
		}
		else if (strncmp(line, "knapsack_type=", 14) == 0)
		{
			knapsack_type = line[14];
		}
		else if (strncmp(line, "object_", 7) == 0)
		{
			// Parse object data
			int obj_id;
			char property[50];
			char value[256];

			if (sscanf(line, "object_%d_%49[^=]=%255s", &obj_id, property, value) == 3)
			{
				// Ensure we have created the object widget
				if (obj_id < objects_amount && object_widgets[obj_id].main_widget == NULL)
				{
					GtkWidget *object_widget = create_object_widget(obj_id);
					gtk_box_pack_start(GTK_BOX(objects_container), object_widget, FALSE, FALSE, 5);
				}

				if (obj_id < objects_amount)
				{
					if (strcmp(property, "name") == 0)
					{
						gtk_entry_set_text(GTK_ENTRY(object_widgets[obj_id].name_txt), value);
					}
					else if (strcmp(property, "value") == 0)
					{
						gtk_spin_button_set_value(GTK_SPIN_BUTTON(object_widgets[obj_id].value_spin), atoi(value));
					}
					else if (strcmp(property, "cost") == 0)
					{
						gtk_spin_button_set_value(GTK_SPIN_BUTTON(object_widgets[obj_id].cost_spin), atoi(value));
					}
					else if (strcmp(property, "amount") == 0)
					{
						gtk_spin_button_set_value(GTK_SPIN_BUTTON(object_widgets[obj_id].available_spin), atoi(value));
					}
				}
			}
		}
	}

	fclose(file);

	// Switch to the objects page and update radio buttons
	if (objects_amount > 0)
	{
		gtk_stack_set_visible_child_name(GTK_STACK(main_stack), "page1");
		gtk_widget_show_all(objects_container);

		// Update radio button selection based on knapsack_type
		if (knapsack_type == 'B')
		{
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(bounded_radio), TRUE);
		}
		else if (knapsack_type == 'U')
		{
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(unbounded_radio), TRUE);
		}
		else if (knapsack_type == 'O')
		{
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(onezero_radio), TRUE);
		}
	}
}

void on_saveBtn_clicked(GtkButton *button, gpointer user_data)
{
	GtkWidget *dialog = gtk_file_chooser_dialog_new("Save Knapsack Data",
													GTK_WINDOW(main_window),
													GTK_FILE_CHOOSER_ACTION_SAVE,
													"_Cancel", GTK_RESPONSE_CANCEL,
													"_Save", GTK_RESPONSE_ACCEPT,
													NULL);

	// Set default filename
	gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog), "knapsack_data.ksp");

	// Add file filter
	GtkFileFilter *filter = gtk_file_filter_new();
	gtk_file_filter_set_name(filter, "Knapsack Files (*.ksp)");
	gtk_file_filter_add_pattern(filter, "*.ksp");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);

	gint result = gtk_dialog_run(GTK_DIALOG(dialog));
	if (result == GTK_RESPONSE_ACCEPT)
	{
		char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));

		// Add .ksp extension if not present
		if (!g_str_has_suffix(filename, ".ksp"))
		{
			char *new_filename = g_strdup_printf("%s.ksp", filename);
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
	GtkWidget *dialog = gtk_file_chooser_dialog_new("Load Knapsack Data",
													GTK_WINDOW(main_window),
													GTK_FILE_CHOOSER_ACTION_OPEN,
													"_Cancel", GTK_RESPONSE_CANCEL,
													"_Load", GTK_RESPONSE_ACCEPT,
													NULL);

	// Add file filter
	GtkFileFilter *filter = gtk_file_filter_new();
	gtk_file_filter_set_name(filter, "Knapsack Files (*.ksp)");
	gtk_file_filter_add_pattern(filter, "*.ksp");
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

	GtkBuilder *builder = gtk_builder_new_from_file("Knapsack/knapsack.glade"); // Si se abre desde el menu
	// GtkBuilder *builder = gtk_builder_new_from_file("knapsack.glade"); // Si se abre SIN en menu

	main_window = GTK_WIDGET(gtk_builder_get_object(builder, "hWindow"));

	gtk_builder_connect_signals(builder, NULL);

	// exit
	g_signal_connect(main_window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

	main_stack = GTK_WIDGET(gtk_builder_get_object(builder, "mainStack"));
	capacity_spin = GTK_WIDGET(gtk_builder_get_object(builder, "capacitySpin"));
	objects_spin = GTK_WIDGET(gtk_builder_get_object(builder, "objectsSpin"));
	objects_container = GTK_WIDGET(gtk_builder_get_object(builder, "objectsContainer"));

	bounded_radio = GTK_WIDGET(gtk_builder_get_object(builder, "bounded_radio"));
	unbounded_radio = GTK_WIDGET(gtk_builder_get_object(builder, "unbounded_radio"));
	onezero_radio = GTK_WIDGET(gtk_builder_get_object(builder, "onezero_radio"));

	gtk_widget_show_all(main_window);

	gtk_main();

	g_object_unref(builder);

	return 0;
}
