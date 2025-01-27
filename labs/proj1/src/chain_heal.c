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
	const int init_range = (int) *argv[1] - 48;
	const int jump_range = (int) *argv[2] - 48;
	const int num_jumps = (int) *argv[3] - 48;
	const int init_power = (int) *argv[4] - 48;
	const double power_reduc = (double) (*argv[5]);

	printf("initial range: %d\n", init_range);
	printf("jump range: %d\n", jump_range);
	printf("num jumps: %d\n", num_jumps);
	printf("initial power: %d\n", init_power);
	printf("power reduction: %f\n", power_reduc);
	// TODO - Read in input
	
	// TODO - Print out input 

	// TODO - Create Nodes into array
}