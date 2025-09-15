#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

int capacity = 10;
int objects_amount = 3; 

typedef struct {
		int value; //Value gained at position ij
		int amount; //Amount of object i taken
}TableItem;
TableItem **table;

typedef struct {
		char *name;
		int value;
		int cost;
		int amount;
}Object;
Object *objects;

void knapsack() {
	for (int i = 0; i < objects_amount; i++) {
		table[i] = malloc((capacity + 1) * sizeof(TableItem));
		for (int j = 0; j <= capacity; j++) {
			int val;
			if (i > 0)
				val = table[i-1][j].value;
			else
				val = 0;
				
			int Q = 0;
			int take_amount = 0;
			while (Q * objects[i].cost <= j) {
				int newval;
				if (i > 0)
					newval = Q * objects[i].value + table[i-1][j - Q * objects[i].cost].value;
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

void print_table() {
	for (int i = 0; i < capacity + 1; i++){
		for (int j = 0; j < objects_amount; j++) {
			printf("%d / %d  |  ", table[j][i].value, table[j][i].amount);
		}
		printf("\n");
	}
}

int main(int argc, char *argv[]) {
	table = malloc(objects_amount * sizeof(TableItem*));
	objects = malloc(objects_amount * sizeof(Object));
	
	Object water;
	water.name = "Water";
	water.value = 11;
	water.cost = 4;
	water.amount = 10;
	
	Object socks;
	socks.name = "Socks";
	socks.value = 7;
	socks.cost = 3;
	socks.amount = 10;
	
	Object cookies;
	cookies.name = "Cookies";
	cookies.value = 12;
	cookies.cost = 5;
	cookies.amount = 10;
	
	objects[0] = water;
	objects[1] = socks;
	objects[2] = cookies;
	
	knapsack();
	print_table();
	
    return 0;
}
