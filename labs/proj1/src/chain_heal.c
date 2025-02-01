#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>

typedef size_t ST;

typedef struct Node {
	char name[101], is_start, visited;
	int X, Y, cur_pp, max_pp, max_heal;
	ST adj_size;
	struct Node *prev, **adj;
} Node;


typedef struct Global {
	int jumps, init_heal, best;
	double heal_scale;
	Node *last;
} Global;

// ****************************** FUNCTIONS ****************************** 
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

		// Set Node default Values;
		n->adj_size = 0;
		n->visited = 0;
		n->is_start = 0;
		n->prev = last_n;	

		last_n = n;
		(*size)++;
	}
	free(line);					// Free temporary char*

	return last_n;;
}

Node** make_array(const ST size, Node **last_node) {
	Node **arr = (Node**)malloc(size * sizeof(Node));
	for(int i = size-1; i >= 0; i--) {			
		arr[i] = *last_node;
		*last_node = (*last_node)->prev;
	}

	return arr;
}

void make_adj_lists(Node **arr, const ST size, const int jump_ran) {
	/*	Initialize adjacency graph to keep track of connections
			REF - https://www.techiedelight.com/initialize-2d-array-with-zeroes-c/ */
	int matrix[size][size];
	memset(matrix, 0, sizeof(matrix));			// set every value as 0
	int dist_squared = jump_ran * jump_ran;

	// FIRST - Find connections with jump_ran
	for(ST i = 0; i < size - 1; i++) {
		Node *n1 = arr[i];
		for(ST j = i+1; j < size; j++) {
			Node *n2 = arr[j];
			int x_diff = n2->X - n1->X;					// Calculate distance
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

	// SECOND - Put edges into adj list
	for(ST i = 0; i < size; i++) {					
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
}

void find_start_Nodes(Node **arr, const ST size, const int init_ran) {
	int init_squared = init_ran * init_ran;
	arr[0]->is_start = 1;
	int x1 = arr[0]->X;
	int y1 = arr[0]->Y;
	
	// Set n2 as start node if within initial range
	for(ST i = 1; i < size; i++) {
		Node *n2 = arr[i];
		int x_diff = n2->X - x1;
		int y_diff = n2->Y - y1;
		int total = (x_diff * x_diff) + (y_diff * y_diff);
		
		if(total <= init_squared) {		n2->is_start = 1;		}
	}

}
// Caclulate healing on current Node
int calc_healing(Node *curr, const int hop_num, Global *v) {
	int h = rint( (double) v->init_heal * pow(v->heal_scale, hop_num-1));
	curr->max_heal = curr->max_pp - curr->cur_pp;
	if(h > curr->max_heal) {		h = curr->max_heal;		}
	else {curr->max_heal = h; }
	return h;
}

void dfs_rec(Node *curr, const int hop_num, int healing, Global **vars) {
	Global *v = *vars;	
	// BASE CASE - MAX JUMPS
	if(hop_num > v->jumps) { return;	}

	curr->visited = 1;											// Set current Node as visited
	// Caclulate Healing
	healing += calc_healing(curr, hop_num, *vars);
	if(healing > v->best) {
		(*vars)->best = healing;
		// Update last Node of longest healing
		(*vars)->last = curr;
	}
	// Index through neighbors of curr Node and call dfs_rec
	for(ST i = 0; i < curr->adj_size; i++) {
		Node *index = curr->adj[i];	
		// Only check for unvisited neighbors
		if(1 == index->visited) { continue; }

		index->prev = curr;
		dfs_rec(index, hop_num+1, healing, vars);
		index->visited = 0;										// Set as unvisited when backtracking
	}
}

void DFS(Node **arr, ST size, Global *vars) {
	for(ST i = 0; i < size; i++) {
		Node *n = arr[i];
		if(0 == n->is_start ) {	continue; }
		
		dfs_rec(n, 1, 0, &vars);
	}
}

// ****************************** MAIN ******************************
int main(int argc, char **argv) {
	// Return Error if inccorect usage 
	if(argc != 6) {
		printf("usage: ./bin/chain_heal initial_range jump_ran num_jumps ");
		printf("initial_power power_reduction < input_file\n");
		return 1;
	}	

	/*  Store argv values with sscanf
			REF - https://utk.instructure.com/courses/218225/pages/strings-in-c?module_item_id=5126893 */
	int init_ran, jump_ran, num_jumps, init_power;
	double power_red;
	sscanf(argv[1], "%d", &init_ran);	
	sscanf(argv[2], "%d", &jump_ran);	
	sscanf(argv[3], "%d", &num_jumps);	
	sscanf(argv[4], "%d", &init_power);	
	sscanf(argv[5], "%lf", &power_red);	

	
	ST size = 0;															// Node counter
	Node *last_n = read_stdin(&size);					// Last Node in the list 
	Node **arr = make_array(size, &last_n);		// Allocate and fill Node array
	make_adj_lists(arr, size, jump_ran);			// Allocate memory & make adjacency list;
	find_start_Nodes(arr, size, init_ran); 		// Starting Nodes will be checked off 
	// Store global variables in a struct
	Global vars = {num_jumps, init_power,0, 1.0-power_red, NULL};
	DFS(arr, size, &vars);											// Initial call for DFS

	Node *index = vars.last;
	Node **tmp_arr = (Node**)malloc(num_jumps * sizeof(Node));
	for(int i = num_jumps-1; i >= 0; i--) {
		tmp_arr[i] = index;
		index = index->prev;
	}

	// OUTPUT
	for(ST i = 0; i < (ST)num_jumps; i++) {
		printf("%s %d\n", tmp_arr[i]->name,tmp_arr[i]->max_heal);
	}
	free(tmp_arr);
	printf("Total_Healing %d\n", vars.best);

	// Deleting Nodes and other memory
	for(ST i = 0; i < size; i++) {	
		free(arr[i]->adj);
		free(arr[i]); 
	}
	free(arr);
}