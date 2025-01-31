#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>

typedef size_t ST;

typedef struct Node {
	char name[101], is_start, visited;
	int X, Y, cur_pp, max_pp;
	ST adj_size;
	struct Node *prev, **adj;
} Node;

typedef struct Global {
	ST num_jumps, power_red;
} Global;

void print_arr(Node **arr, const ST size) {
	for(ST i = 0; i < size; i++) {				
		Node *n = arr[i];
		printf("%s:		", n->name);
		for(ST j = 0; j < n->adj_size; j++) {
			printf("%s	|	 ", n->adj[j]->name);
		}
		printf("\n");
	}
}

Node* read_stdin(ST *size) {

	Node *last_n = NULL;
	char *line = NULL;			// For getline
	ST line_size = 0;				// For getline

	// While loop to read in stdin and Malloc Nodes 
	// REF - Keith Scroggs talked about using getline instead of just scanf for each line
	while(getline(&line, &line_size, stdin) > -1) {
		Node *n = (Node*)malloc(sizeof(Node));
		sscanf(line, "%d %d %d %d %s", &(n->X), &(n->Y), &(n->cur_pp), &(n->max_pp), n->name);		

		n->adj_size = 0;
		n->visited = 0;
		n->is_start = 0;
		n->prev = last_n;			// Set Node links
		last_n = n;
		(*size)++;
	}
	free(line);					// Free temporary char*

	return last_n;;
}
// Allocate memory for array of Nodes
Node** make_array(const ST size, Node **last_node) {

	Node **arr = (Node**)malloc(size * sizeof(Node));
	for(int i = size-1; i >= 0; i--) {			
		arr[i] = *last_node;
		*last_node = (*last_node)->prev;
	}

	return arr;
}
// Allocate and set every Node's adj list
void create_adj(Node **arr, const ST size, const int jump_ran) {
	/*	Initialize adjacency graph to keep track of connections
			REF - https://www.techiedelight.com/initialize-2d-array-with-zeroes-c/ */
	int matrix[size][size];
	memset(matrix, 0, sizeof(matrix));					// set every value as 0
	int dist_squared = jump_ran * jump_ran;

	for(ST i = 0; i < size - 1; i++) {			// Find connections with jump_ran
		Node *n1 = arr[i];

		for(ST j = i+1; j < size; j++) {
			Node *n2 = arr[j];
			int x_diff = n2->X - n1->X;			// Calculate distance
			int y_diff = n2->Y - n1->Y;
			int total = (x_diff * x_diff) + (y_diff * y_diff);

			// Continue if jump_ran is not enough	
			if(total > dist_squared) { continue; }

			// Adjust Node's adjacency size and adjacency graph
			n1->adj_size++;
			n2->adj_size++;
			matrix[i][j] = 1;
			matrix[j][i] = 1;
		}
	}

	for(ST i = 0; i < size; i++) {					// Put edges into adjacency list
		
		// Allocate memory for each list
		Node *n = arr[i];
		n->adj = (Node**)malloc(n->adj_size * sizeof(Node));

		// Use matrix to connect Nodes with edges 
		ST index = 0;
		for(ST j = 0; j < size; j++) {
			if(matrix[i][j] == 0) { continue; }	// 0 means no edge
			n->adj[index] = arr[j];
			// If adjacency list is full, then break loop
			if(index++ == n->adj_size) { break;}
		}
	}
	
	// print_arr(arr, size);
}
// Find and check off nodes from start jump 
void find_starting_nodes(Node **arr, const ST size, const int init_ran) {
	int init_squared = init_ran * init_ran;
	arr[0]->is_start = 1;
	int x1 = arr[0]->X;
	int y1 = arr[0]->Y;
	

	for(ST i = 1; i < size; i++) {
		Node *n2 = arr[i];
		int x_diff = n2->X - x1;
		int y_diff = n2->Y - y1;
		int total = (x_diff * x_diff) + (y_diff * y_diff);
		
		if(total <= init_squared) {
			n2->is_start = 1;
			// printf("%s -- is strt\n",n2->name);
		}
	}

}
// DFS recursion call
void dfs_rec(Node *curr, ST hop_num, Global vars) {

	if(hop_num > vars.num_jumps) return;	
	curr->visited = 1;	
	printf("Node:%s		Hop %zu\n", curr->name, hop_num);

	for(ST i = 0; i < curr->adj_size; i++) {
		Node *index = curr->adj[i];	
		if(0 == index->visited) {
			dfs_rec(index, hop_num+1, vars);
			index->visited = 0;
		}
	}
}
// Wrapper DFS call
void DFS(Node **arr, ST size, Global vars) {
	for(ST i = 0; i < size; i++) {
		Node *n = arr[i];
		if(n->is_start) {	dfs_rec(n, 1, vars);}
	}

}

int main(int argc, char **argv) {
	// Error check argv values
	if(argc != 6) {
		printf("usage: ./bin/chain_heal initial_range jump_ran num_jumps ");
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

	
	ST size = 0;															// Keep count of Nodes
	Node *last_n = read_stdin(&size);					// previous Node for chaining Nodes
	Node **arr = make_array(size, &last_n);		// Malloc Node array and put Nodes in them

	create_adj(arr, size, jump_ran);					// Create adjacency list
	find_starting_nodes(arr, size, init_ran); // Find and check starting nodes
	Global vars = {num_jumps, power_red}; 		// Store global variables in a struct
	DFS(arr, size, vars);											// Initial call for DFS

	// Deleting all Nodes
	for(ST i = 0; i < size; i++) {	
		free(arr[i]->adj);
		free(arr[i]); 
	}
	free(arr);
}