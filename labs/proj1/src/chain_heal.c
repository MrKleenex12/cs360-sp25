#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>

typedef size_t ST;

typedef struct Node {
	char name[101], is_start, visited;
	int id;
	int X, Y, cur_pp, max_pp, max_heal;
	ST adj_size;
	struct Node *prev, **adj;
} Node;


// Command Line values
typedef struct Command_Line_Variables {
	int init_ran, jump_ran, jumps, init_power;
	double scale;
} CL;

typedef struct Global_Variables {
	int best_heal, *heal_arr, *path_arr;
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
		(*last_node)->id = i;
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
int calc_healing(Node **current, const int hop_num, CL *cl) {
	Node *curr = *current;
	int h = rint( (double) cl->init_power * pow(cl->scale, hop_num-1));
	curr->max_heal = curr->max_pp - curr->cur_pp;
	if(h > curr->max_heal) {
		h = curr->max_heal;
	}		else {
		curr->max_heal = h;
	}
	return h;
}

void dfs_rec(Node *curr, const int hop_num, int healing, CL *cl, Global **vars) {
	Global *dv = *vars;
	// BASE CASE - MAX JUMPS
	if(hop_num > cl->jumps) { return;	}
	// printf("%s(%d) - Hop: %d\n", curr->name, curr->id, hop_num);

	curr->visited = 1;											// Set current Node as visited
	// Caclulate Healing
	healing += calc_healing(&curr, hop_num, cl);
	if(healing > dv->best_heal) {
		dv->best_heal = healing;
		// printf("\nNew Best Healing: %d\n\n", healing);

		Node *index = curr;
		for(int i = hop_num-1; i >= 0; i--) {
			dv->path_arr[i] = index->id;
			dv->heal_arr[i] = index->max_heal;
			index = index->prev;
		}
	}
	// Index through neighbors of curr Node and call dfs_rec
	for(ST i = 0; i < curr->adj_size; i++) {
		Node *index = curr->adj[i];	
		// Only check for unvisited neighbors
		if(1 == index->visited) { continue; }

		index->prev = curr;
		// printf("	%s %d -> %s\n", curr->name, hop_num, index->name);
		dfs_rec(index, hop_num+1, healing, cl, vars);
		index->visited = 0;										// Set as unvisited when backtracking
	}
}

void DFS(Node **arr, ST size, CL *cl, Global **vars) {
	Global *dv = *vars;
	dv->path_arr = (int*)malloc(cl->jumps * sizeof(int));
	dv->heal_arr = (int*)malloc(cl->jumps * sizeof(int));

	for(ST i = 0; i < size; i++) {
		Node *n = arr[i];
		if(0 == n->is_start ) {	continue; }
		
		dfs_rec(n, 1, 0, cl, vars);
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
	// Store global variables in a struct
	CL *cl = (CL*)malloc(sizeof(CL));
	sscanf(argv[1], "%d", &(cl->init_ran));	
	sscanf(argv[2], "%d", &(cl->jump_ran));	
	sscanf(argv[3], "%d", &(cl->jumps));	
	sscanf(argv[4], "%d", &(cl->init_power));	
	sscanf(argv[5], "%lf", &(cl->scale));	
	cl->scale = 1.0 - cl->scale;
	
	ST size = 0;															// Node counter
	Node *last_n = read_stdin(&size);					// Last Node in the list 
	Node **arr = make_array(size, &last_n);		// Allocate and fill Node array
	make_adj_lists(arr, size, cl->jump_ran);			// Allocate memory & make adjacency list;
	find_start_Nodes(arr, size, cl->init_ran); 		// Starting Nodes will be checked off 
	Global *vars = (Global*)malloc(sizeof(Global));
	DFS(arr, size, cl, &vars);											// Initial call for DFS

	for(int i = 0; i < cl->jumps; i++) {
		int index = vars->path_arr[i];
		printf("%s %d\n", arr[index]->name, vars->heal_arr[i]);
	}
	printf("Total_Healing %d\n", vars->best_heal);

	// Freeing memory
	free(vars->heal_arr);
	free(vars->path_arr);
	free(vars);
	free(cl);
	for(ST i = 0; i < size; i++) {	
		free(arr[i]->adj);
		free(arr[i]); 
	}
	free(arr);
}