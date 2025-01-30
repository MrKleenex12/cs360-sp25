#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>

typedef struct Node {
	char name[101];
	int x_crd, y_crd;
	int cur_pp, max_pp;
	size_t visited, adj_size;
	struct Node *prev;
	struct Node **adj;
} Node;

// Print out array
void print_arr(Node **arr, const size_t arr_size) {
	for(size_t i = 0; i < arr_size; i++) {				
		printf("%s\n", arr[i]->name);
	}
}

void create_adj(Node **arr, const size_t size, const int jump_range) {
	/*	Initialize adjacency graph to keep track of connections
			REF - https://www.techiedelight.com/initialize-2d-array-with-zeroes-c/ */
	int matrix[size][size];
	memset(matrix, 0, sizeof(matrix));		// set every value as 0
	int dist_squared = jump_range * jump_range;

	// Find connections with jump_range
	for(size_t i = 0; i < size - 1; i++) {
		Node *n1 = arr[i];
		for(size_t j = i+1; j < size; j++) {
			Node *n2 = arr[j];
			// Calculate distance
			int x_diff = n2->x_crd - n1->x_crd;
			int y_diff = n2->y_crd - n1->y_crd;
			int total = (x_diff * x_diff) + (y_diff * y_diff);
			// Continue if jump_range is not enough	
			if(total > dist_squared) { continue; }

			// Adjust Node's adjacency size and adjacency graph
			n1->adj_size++;
			n2->adj_size++;
			matrix[i][j] = 1;
			matrix[j][i] = 1;
		}
	}

	
	// Put edges into adjacency list
	for(size_t i = 0; i < size; i++) {
		Node *n = arr[i];												// Allocate memory for each list
		n->adj = (Node**)malloc(n->adj_size * sizeof(Node));

		// Use matrix to connect Nodes with edges 
		size_t index = 0;
		for(size_t j = 0; j < size; j++) {
			if(matrix[i][j] == 0) { continue; }	// 0 means no edge
			n->adj[index] = arr[j];
			// If adjacency list is full, then break loop
			if(index++ == n->adj_size) { break;}
		}
	}
	

	// /*	Print out adjacency list
	for(size_t i = 0; i < size; i++) {
		Node *n = arr[i];
		printf("%s:		", n->name);
		for(size_t j = 0; j < n->adj_size; j++) {
			printf("%s, ", n->adj[j]->name);
		}
		printf("\n");
	}
	// */
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

		n->adj_size = 0;
		n->visited = 0;
		n->prev = last_n;			// Set Node links
		last_n = n;
		arr_size++;
	}
	free(tmp_line);					// Free temporary char*

	// printf("ln: %s		ln->p: %s\n", last_n->name, last_n->prev->name);

	// Malloc Node array and put Nodes in them
	Node **arr_nodes = (Node**)malloc(arr_size * sizeof(Node));
	for(int i = arr_size-1; i >= 0; i--) {			
		arr_nodes[i] = last_n;
		last_n = last_n->prev;
	}

	// print_arr(arr_nodes, arr_size);

	// TODO - Make Graph
	create_adj(arr_nodes, arr_size, jump_ran);

	// Deleting all Nodes
	for(size_t i = 0; i < arr_size; i++) {	free(arr_nodes[i]); }
	free(arr_nodes);
}