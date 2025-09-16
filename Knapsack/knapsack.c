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
ObjectWidgetData object_widgets[10];

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
		sprintf(title_text, "Objeto %d", object_id + 1);
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
			"\\usepackage{graphicx}\n"
			"\\usepackage{geometry}\n"
			"\\geometry{margin=1in}\n"
			"\n"
			"\\begin{document}\n"
			"\\begin{titlepage}\n"
			"    \\centering\n"
			"    \\vspace*{3cm}\n"
			"    {\\Huge \\textbf{Instituto Tecnológico de Costa Rica}} \\\\[2cm]\n"
			"    {\\LARGE \\textbf{Knapsack Problem}} \\\\[3cm]\n"
			"    {\\Large Members:} \\\\[0.5cm]\n"
			"    {\\large Adrián Zamora Chavarría \\\\ Daniel Romero Murillo} \\\\[2cm]\n"
			"    {\\large Date: \\today}\n"
			"    \\vfill\n"
			"\\end{titlepage}\n");
}

void print_problem()
{
	fprintf(output_file, "\\newpage\n");
	fprintf(output_file, "\\section*{Problem}\n");
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
		fprintf(output_file, "x_%d ", i + 1);
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
}

void print_knapsack_latex()
{
	fprintf(output_file, "\\newpage\n");
	fprintf(output_file, "\\section*{Data Table}\n");
	fprintf(output_file, "\\resizebox{\\textwidth}{!}{%%\n");
	fprintf(output_file, "\\begin{tabular}{|c|");
	for (int i = 0; i < objects_amount; i++)
		fprintf(output_file, "c|");
	fprintf(output_file, "}\n");
	fprintf(output_file, "\\hline\n");
	for (int i = 0; i < objects_amount; i++)
		fprintf(output_file, "&%s", objects[i].name);
	fprintf(output_file, "\\\\\n\\hline\n");
	for (int i = 0; i <= capacity; i++)
	{
		fprintf(output_file, "%d ", i);
		for (int j = 0; j < objects_amount; j++)
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
	fprintf(output_file, "\\end{tabular}\n}\n");
}

void print_solution_latex(int track_table[objects_amount][capacity + 1])
{
	fprintf(output_file, "\\newpage");
	fprintf(output_file, "\\section*{Optimal Solution}\n");
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
	print_knapsack_latex();
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

	gtk_widget_show_all(main_window);

	gtk_main();

	g_object_unref(builder);

	return 0;
}
