#include <math.h>
#include <stdio.h>
#include <time.h>

#include <uthash.h>

#include <voxel.h>
#include <voxel_utils.h>

//#define VOXEL_SIDE 16
#define VOXEL_SIDE 64

#define EXTRACT_BITS(value, offset, count) ((value >> offset) & (0xFFFFFFFF >> (32 - count)))

int main(int argc, char **argv)
{
	voxel_model *vm = malloc(sizeof *vm);
	if (!voxel_init(vm, VOXEL_SIDE))
		return -1;

	/* Generating voxel world */
	printf("Generating voxel world...\n");
	for (int x = 0; x < VOXEL_SIDE; x++)
	for (int y = 0; y < VOXEL_SIDE; y++)
	for (int z = 0; z < VOXEL_SIDE; z++)
	{
		const float vs2 = VOXEL_SIDE / 2.0f;
		float lx = x - vs2, ly = y - vs2, lz = z - vs2;
		if (sqrt(lx * lx + ly * ly + lz * lz) < (float) (VOXEL_SIDE / 2))
//		if (sqrt(lx * lx + ly * ly + lz * lz) < (float) (VOXEL_SIDE / 2)
//			|| (x == 0 || x == VOXEL_SIDE - 1) ^ (y == 0 || y == VOXEL_SIDE - 1) ^ (z == 0 || z == VOXEL_SIDE - 1) ^ 1)
//		if (x == 0 || x == VOXEL_SIDE - 1 || y == 0 || y == VOXEL_SIDE - 1 || z == 0 || z == VOXEL_SIDE - 1)
//		if (y == 0)
//		if ((x & 1) ^ (y & 1) ^ (z & 1)) // Worst case scenario
		if (y != VOXEL_SIDE / 2)
			vm->voxels[x][y][z] = 1;
	}

	/* Making a mesh */
	printf("Making a mesh...\n");

	struct timespec t_start;
	clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &t_start);

	if (argc > 1 && argv[1][0] == 's')
		voxel_simple(vm);
	else
		voxel_greedy(vm);

	struct timespec t_finish;
	clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &t_finish);
	printf("Done. Time elapsed: %f seconds\n", (t_finish.tv_sec - t_start.tv_sec) + (t_finish.tv_nsec - t_start.tv_nsec) / 1000000000.0f);

	/* Writing the mesh to output.obj */
	printf("Making the mesh to \"output.obj\"...\n");
	FILE *f = fopen("output.obj", "w");

	/* Writing verties */
	{
		vertex_hash *current = NULL;
		vertex_hash *temp = NULL;
		HASH_ITER(hh, vm->vertices, current, temp)
		{
			fprintf(
				f,
				"v %i %i %i\n",
				EXTRACT_BITS(current->id, 0, vm->_axisBits), // x
				EXTRACT_BITS(current->id, vm->_axisBits, vm->_axisBits), // y
				EXTRACT_BITS(current->id, vm->_axisBits * 2, vm->_axisBits) // z
			);
		}
	}

	/* Writing faces */
	{
		face_list *current = vm->faces;
		while (current != NULL)
		{
			fprintf(
				f,
				"f %i %i %i\n",
				current->indices[0] + 1,
				current->indices[1] + 1,
				current->indices[2] + 1
			);
			current = current->next;
		}
	}
	fclose(f);

	voxel_free(vm);
	free(vm);

	printf("Done!\n");

	return 0;
}
