#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>

/* Node struct
			name
			x_coord
			y_coord
			curr_pacification_points
			max_pacification_points
			prev node
*/

typedef struct Node {
	char name[101];
	int x_coord, y_coord;
	int cur_pp, max_pp;
	struct Node *prev_Node;
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

	// TODO - Read in input

	// Store argv values
	sscanf(argv[1], "%d", &init_ran);	
	sscanf(argv[2], "%d", &jump_ran);	
	sscanf(argv[3], "%d", &num_jumps);	
	sscanf(argv[4], "%d", &init_power);	
	sscanf(argv[5], "%lf", &power_red);	

	printf("initial range: %d\n", init_ran);
	printf("initial range: %d\n", jump_ran);
	printf("initial range: %d\n", num_jumps);
	printf("initial range: %d\n", init_power);
	printf("initial range: %lf\n", power_red);

	// TODO - Print out input 

	// TODO - Create Nodes into array
}