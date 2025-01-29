#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>

typedef struct Node {
	char *name;
	int x_crd, y_crd;
	int cur_pp, max_pp;
	int visited;
	struct Node *prev;
} Node;

int main(int argc, char **argv) {
	// Error check argv values
	if(argc != 6) {
		printf("usage: ./bin/chain_heal initial_range jump_range num_jumps ");
		printf("initial_power power_reduction < input_file\n");
		return 1;
	}	

	int init_ran, jump_ran, num_jumps, init_power;
	double power_red;

	/*  Store argv values with sscanf
			REF - https://utk.instructure.com/courses/218225/pages/strings-in-c?module_item_id=5126893 */

	sscanf(argv[1], "%d", &init_ran);	
	sscanf(argv[2], "%d", &jump_ran);	
	sscanf(argv[3], "%d", &num_jumps);	
	sscanf(argv[4], "%d", &init_power);	
	sscanf(argv[5], "%lf", &power_red);	

	// TODO - Read In Input From stdin

	int node_count = 0;
	char *line = NULL;
	size_t size = 0;
	Node *last_node = NULL;

	while(getline(&line, &size, stdin) > -1) {
		Node *n = (Node*)malloc(sizeof(Node));

		char temp[50];			 			// Use temp string for n->name
		sscanf(line, "%d %d %d %d %s", &n->x_crd, &n->y_crd, &n->cur_pp, &n->max_pp, temp);		
		n->name = temp;
		n->prev = last_node;			// Set Node links
		last_node = n;

		node_count++;
	}
	printf("%d %d %d %d %s\n", last_node->x_crd, last_node->y_crd, last_node->cur_pp, last_node->max_pp, last_node->name);
	printf("Number of Nodes: %d\n", node_count);
	free(line);

	// TODO - Create Nodes into array

	// Deleting all Nodes
	Node *curr = last_node;
	while(curr != NULL) {
		Node *next = curr->prev;
		free(curr);
		curr = next;
	}
	// TODO - Print out input 


}