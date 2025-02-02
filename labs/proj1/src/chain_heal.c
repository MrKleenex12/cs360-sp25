#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>

typedef size_t ST;

typedef struct Node {
	char name[101], is_start, visited;
	int X, Y, cur_pp, max_pp, max_heal;
	int id;
	ST adj_size;
	struct Node *prev, **adj;
} Node;


// Command Line values
typedef struct Command_Line_Variables {
	int init_ran, jump_ran, jumps, init_power;
	double scale;
} CLV;

typedef struct Best_Path_Variables {
	int best_heal, length, *heal_arr, *path_arr;
	// Node **path_arr;
} BPV;

// ****************************** FUNCTIONS ****************************** 
void print_arr(Node **arr, const ST size) {
	for(ST i = 0; i < size; i++) {				
		Node *n = arr[i];
		printf("%s:		", n->name);
		for(ST j = 0; j < n->adj_size; j++) {
			printf("%s | ", n->adj[j]->name);
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
		n->is_start = 0;
		n->visited = 0;
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
		arr[i]->id = i;
		*last_node = (*last_node)->prev;
	}
	return arr;
}

int find_dist(Node *n1, Node *n2) {
	int x_diff = n2->X - n1->X;					// Calculate distance
	int y_diff = n2->Y - n1->Y;
	return ((x_diff * x_diff) + (y_diff * y_diff));
}

void make_adj_lists(Node **arr, const ST size, const int jump_ran) {
	/*	Initialize adjacency graph to keep track of connections
			REF - https://www.techiedelight.com/initialize-2d-array-with-zeroes-c/ */
	int matrix[size][size];
	memset(matrix, 0, sizeof(matrix));			// set every value as 0
	// FIRST - Find connections with jump_ran
	for(ST i = 0; i < size - 1; i++) {
		Node *n1 = arr[i];
		for(ST j = i+1; j < size; j++) {
			Node *n2 = arr[j];
			// Continue if jump_ran is not enough	
			if( find_dist(n1, n2) > jump_ran * jump_ran) { continue; }
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
	arr[0]->is_start = 1;
	// Set n2 as start node if within initial range
	for(ST i = 1; i < size; i++) {
		Node *n2 = arr[i];
		if( find_dist(arr[0], n2) <= init_ran * init_ran ) {		n2->is_start = 1;		}
	}
}

void reset_visited(Node **arr, const ST size) {
	for(ST i = 0; i < size; i++) {	arr[i]->visited = 0; }
}

// Caclulate healing on current Node
int calc_healing(Node **current, const int hop_num, CLV *cl) {
	Node *curr = *current;
	int h = rint( (double) cl->init_power * pow(cl->scale, hop_num-1));
	curr->max_heal = curr->max_pp - curr->cur_pp;
	// Set correct max_heal
	if(h > curr->max_heal) {	h = curr->max_heal; }
	else { curr->max_heal = h; }
	return h;
}

void dfs_rec(Node *curr, const int hop_num, int healing, CLV *cl, BPV **vars) {
	BPV *v = *vars;
	curr->visited = 1;											// Set current Node as visited
	if(hop_num > cl->jumps) { return;	}			// BASE CASE - MAX JUMPS
	// Calculate Healing
	healing += calc_healing(&curr, hop_num, cl);
	if(healing >= v->best_heal) {						// Update Best path if 
		v->best_heal = healing;
		v->length = hop_num;
		// Set new best path
		Node *index = curr;
		for(int i = hop_num-1; i >= 0; i--) {
			v->path_arr[i] = index->id;
			v->heal_arr[i] = index->max_heal;
			index = index->prev;
		}
	}
	// Index through adjcency list 
	for(ST i = 0; i < curr->adj_size; i++) {
		Node *index = curr->adj[i];	
		if(1 == index->visited) { continue; }	// Only check for unvisited neighbors
		index->prev = curr;
		dfs_rec(index, hop_num+1, healing, cl, vars);
		index->visited = 0;										// Set as unvisited when backtracking
	}
}

void DFS(Node **arr, ST size, CLV *cl, BPV **vars) {
	// Allocate array for Best path
	BPV *v = *vars;
	v->path_arr = (int*)malloc(cl->jumps * sizeof(int));
	v->heal_arr = (int*)malloc(cl->jumps * sizeof(int));
	for(ST i = 0; i < size; i++) {
		Node *n = arr[i];
		if(0 == n->is_start ) {	continue; }
		dfs_rec(n, 1, 0, cl, vars);					// Call DFS and reset for next call
		reset_visited(arr, size);
	}
}

void free_memory(BPV *v, CLV *c, Node **arr, const ST size) {
	free(v->heal_arr);
	free(v->path_arr);
	free(v);
	free(c);
	for(ST i = 0; i < size; i++) {	
		free(arr[i]->adj);
		free(arr[i]); 
	}
	free(arr);
}

void final_output(BPV *v, Node **arr) {
	for(int i = 0; i < v->length; i++) {
		printf("%s %d\n", arr[v->path_arr[i]]->name, v->heal_arr[i]);
	}
	printf("Total_Healing %d\n", v->best_heal);
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
	CLV *cl = (CLV*)malloc(sizeof(CLV));				// Store BPV variables in a struct
	sscanf(argv[1], "%d", &(cl->init_ran));	
	sscanf(argv[2], "%d", &(cl->jump_ran));	
	sscanf(argv[3], "%d", &(cl->jumps));	
	sscanf(argv[4], "%d", &(cl->init_power));	
	sscanf(argv[5], "%lf", &(cl->scale));	
	cl->scale = 1.0 - cl->scale;
	
	ST size = 0;																// Node counter
	Node *last_n = read_stdin(&size);						// Last Node in the list 
	Node **arr = make_array(size, &last_n);			// Allocate and fill Node array
	// Make Graph
	make_adj_lists(arr, size, cl->jump_ran);		// Allocate memory & make adjacency list;
	find_start_Nodes(arr, size, cl->init_ran); 	// Starting Nodes will be checked off 
	// DFS
	BPV *vars = (BPV*)malloc(sizeof(BPV));
	DFS(arr, size, cl, &vars);

	// FINAL STEP
	final_output(vars, arr);										// Output	
	free_memory(vars, cl, arr, size);						// Freeing memory
}