#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

GtkWidget *main_window;
GtkWidget *main_stack;
GtkWidget *capacity_spin;
GtkWidget *objects_spin;
GtkWidget *objects_container;

int capacity = 0;
int objects_amount = 0;

typedef struct
{
	int value;	// Value gained at position ij
	int amount; // Amount of object i taken
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
			int take_amount = 0;
			while (Q * objects[i].cost <= j)
			{
				int newval;
				if (i > 0)
					newval = Q * objects[i].value + table[i - 1][j - Q * objects[i].cost].value;
				else
					newval = Q * objects[i].value;
				take_amount = newval > val ? Q : take_amount;
				val = newval > val ? newval : val;
				Q++;
			}
			TableItem table_item;
			table_item.value = val;
			table_item.amount = take_amount;
			table[i][j] = table_item;
		}
	}
}

GtkWidget *create_object_widget(int object_id)
{
	GtkBuilder *builder = gtk_builder_new_from_file("objectSetupWidget.glade");
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

void print_table()
{
	for (int i = 0; i < capacity + 1; i++)
	{
		for (int j = 0; j < objects_amount; j++)
		{
			printf("%d / %d  |  ", table[j][i].value, table[j][i].amount);
		}
		printf("\n");
	}
}

void on_runBtn_clicked(GtkButton *button, gpointer user_data)
{
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
	print_table();
}

int main(int argc, char *argv[])
{
	// Object water;
	// water.name = "Water";
	// water.value = 11;
	// water.cost = 4;
	// water.amount = 10;

	// Object socks;
	// socks.name = "Socks";
	// socks.value = 7;
	// socks.cost = 3;
	// socks.amount = 10;

	// Object cookies;
	// cookies.name = "Cookies";
	// cookies.value = 12;
	// cookies.cost = 5;
	// cookies.amount = 10;

	// objects[0] = water;
	// objects[1] = socks;
	// objects[2] = cookies;

	gtk_init(&argc, &argv);

	// GtkBuilder *builder = gtk_builder_new_from_file("Knapsack/knapsack.glade"); //Si se abre desde el menu
	GtkBuilder *builder = gtk_builder_new_from_file("knapsack.glade"); // Si se abre SIN en menu

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
