#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>

typedef struct Node {
	char name[101];
	int x_crd, y_crd;
	int cur_pp, max_pp;
	int visited;
	struct Node *prev;
} Node;

// Print out array
void print_arr(Node **a, const size_t size) {
	for(size_t i = 0; i < size; i++) {				
		printf("%s\n", a[i]->name);
	}
}

// DFS somehow

int main(int argc, char **argv) {
	// Error check argv values
	if(argc != 6) {
		printf("usage: ./bin/chain_heal initial_range jump_range num_jumps ");
		printf("initial_power power_reduction < input_file\n");
		return 1;
	}	

	// argv variables 
	int init_ran, jump_ran, num_jumps, init_power;
	double power_red;


	/*  Store argv values with sscanf
			REF - https://utk.instructure.com/courses/218225/pages/strings-in-c?module_item_id=5126893 */
	sscanf(argv[1], "%d", &init_ran);	
	sscanf(argv[2], "%d", &jump_ran);	
	sscanf(argv[3], "%d", &num_jumps);	
	sscanf(argv[4], "%d", &init_power);	
	sscanf(argv[5], "%lf", &power_red);	


	char *tmp_line = NULL;			// For getline
	size_t tmp_size = 0;				// For getline
	Node *last_n = NULL; 				// previous Node for chaining Nodes
	size_t arr_size = 0;				// Keep count of Nodes


	// While loop to read in stdin and Malloc Nodes 
	// REF - Keith Scroggs talked about using getline instead of just scanf for each line
	while(getline(&tmp_line, &tmp_size, stdin) > -1) {
		Node *n = (Node*)malloc(sizeof(Node));
		sscanf(tmp_line, "%d %d %d %d %s", &(n->x_crd), &(n->y_crd), &(n->cur_pp), &(n->max_pp), n->name);		

		n->prev = last_n;			// Set Node links
		last_n = n;
		arr_size++;
	}
	free(tmp_line);					// Free temporary char*


	// Malloc Node array and put Nodes in them
	Node **arr_nodes = (Node**)malloc(arr_size * sizeof(Node));
	for(size_t i = arr_size-1; i >= 0; i--) {			
		arr_nodes[i] = last_n;
		last_n = last_n->prev;
	}


	// TODO - Make Graph

	// Deleting all Nodes
	for(size_t i = 0; i < arr_size; i++) {	free(arr_nodes[i]); }
	free(arr_nodes);
}