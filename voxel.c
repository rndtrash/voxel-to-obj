#include <math.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

#include "uthash.h"

#define VOXEL_SIDE 32
//#define VOXEL_SIDE 64
// + 1 bit for the 64th index
#define BITS_PER_SIDE 7
#define VERTEX_MASK 0x7F

#define EXTRACT_BITS(value, offset, count) ((value >> offset) & (0xFFFFFFFF >> (32 - count)))

static char voxel[VOXEL_SIDE][VOXEL_SIDE][VOXEL_SIDE] = { 0 };

struct vertex_hash {
	int id;			/* key */
	int n;			/* value */
	UT_hash_handle hh;	/* hash */
};
struct vertex_hash *vertices = NULL;

struct face_list {
	int ns[3];		/* value */
	struct face_list *next;
};
struct face_list *faces_head = NULL;
struct face_list *faces_tail = NULL;

static inline const int voxel_valid(const int x, const int y, const int z)
{
	return x >= 0 && x < VOXEL_SIDE
		&& y >= 0 && y < VOXEL_SIDE
		&& z >= 0 && z < VOXEL_SIDE;
}

static inline const int vertex_compress(const int x, const int y, const int z)
{
	return (x & VERTEX_MASK) | ((y & VERTEX_MASK) << BITS_PER_SIDE) | ((z & VERTEX_MASK) << (BITS_PER_SIDE * 2));
}

static const int get_vertex_index(const int x, const int y, const int z)
{
	static int vertex_iota = 0;

	const int id = vertex_compress(x, y, z);
#ifdef DEBUG
	printf("Compressed: %i\n", id);
#endif
	struct vertex_hash *h;
	HASH_FIND_INT(vertices, &id, h);

	if (h == NULL)
	{
#ifdef DEBUG
		printf("Failed to find a vertex (%i %i %i), making a new one then!\n", x, y, z);
#endif
		h = (struct vertex_hash*) malloc(sizeof *h);
		h->id = id;
		h->n = vertex_iota++;
		HASH_ADD_INT(vertices, id, h);
	}

	return h->n;
}

static inline void make_face(const int id1, const int id2, const int id3, const int id4)
{
	struct face_list *face1 = (struct face_list*) malloc(sizeof *face1);
	//face1->ns = (int[3]) { id1, id2, id3 }; // LIFE COULD BE DREAM...
	face1->ns[0] = id1;
	face1->ns[1] = id2;
	face1->ns[2] = id3;

	struct face_list *face2 = (struct face_list*) malloc(sizeof *face2);
	//face2->ns = (int[3]) { id3, id4, id1 }; // LIFE COULD BE DREEEEAAAAAM.....
	face2->ns[0] = id3;
	face2->ns[1] = id4;
	face2->ns[2] = id1;
	face2->next = NULL;

	face1->next = face2;

	if (faces_tail != NULL)
	{
		faces_tail->next = face1;
		faces_tail = face2;
	}
	else
	{
		faces_head = face1;
		faces_tail = face2;
	}
}

static inline void make_cube(const int x, const int y, const int z)
{
#ifdef DEBUG
	printf("Block color: %i\n", (int) voxel[x][y][z]);
#endif
	if (voxel[x][y][z] == 0)
		return;

	/* Replace Y with - 1 if it points down */
	const int vertex_offsets[8][3] = {
		{ 0, 0, 0 },
		{ 0, 1, 0 },
		{ 0, 0, 1 },
		{ 0, 1, 1 },
		{ 1, 0, 0 },
		{ 1, 1, 0 },
		{ 1, 0, 1 },
		{ 1, 1, 1 }
	};

	int vertices_cache[8] = { -1, -1, -1, -1, -1, -1, -1, -1 };
	int _get_v(const int i)
	{
		if (vertices_cache[i] == -1)
			vertices_cache[i] = get_vertex_index(x + vertex_offsets[i][0], y + vertex_offsets[i][1], z + vertex_offsets[i][2]);
		return vertices_cache[i];
	}

	/* Front and back */
	if (!voxel_valid(x - 1, y, z) || voxel[x - 1][y][z] == 0)
		make_face(_get_v(0), _get_v(1), _get_v(3), _get_v(2));
	if (!voxel_valid(x + 1, y, z) || voxel[x + 1][y][z] == 0)
		make_face(_get_v(6), _get_v(7), _get_v(5), _get_v(4));

	/* Left and right */
	if (!voxel_valid(x, y, z - 1) || voxel[x][y][z - 1] == 0)
		make_face(_get_v(0), _get_v(4), _get_v(5), _get_v(1));
	if (!voxel_valid(x, y, z + 1) || voxel[x][y][z + 1] == 0)
		make_face(_get_v(2), _get_v(3), _get_v(7), _get_v(6));

	/* Top and bottom */
	if (!voxel_valid(x, y - 1, z) || voxel[x][y - 1][z] == 0)
		make_face(_get_v(4), _get_v(0), _get_v(2), _get_v(6));
	if (!voxel_valid(x, y + 1, z) || voxel[x][y + 1][z] == 0)
		make_face(_get_v(1), _get_v(5), _get_v(7), _get_v(3));
}

int main(int argc, char **argv)
{
	/* Generating voxel world */
	printf("Generating voxel world...\n");
	for (int x = 0; x < VOXEL_SIDE; x++)
	for (int y = 0; y < VOXEL_SIDE; y++)
	for (int z = 0; z < VOXEL_SIDE; z++)
	{
//		float lx = x - 16.f, ly = y - 16.f, lz = z - 16.f;
//		if (sqrt(lx * lx + ly * ly + lz * lz) < (float) (VOXEL_SIDE / 2))
//		if (x == 0 || x == VOXEL_SIDE - 1 || y == 0 || y == VOXEL_SIDE - 1 || z == 0 || z == VOXEL_SIDE - 1)
//		if (y == 0)
		if ((x & 1) ^ (y & 1) ^ (z & 1)) // Worst case scenario
			voxel[x][y][z] = 1;
	}

	/* Making a mesh */
	printf("Making a mesh...\n");

	struct timespec t_start;
	clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &t_start);

	for (int x = 0; x < VOXEL_SIDE; x++)
	{
		printf("x: %i\n", x);
		for (int y = 0; y < VOXEL_SIDE; y++)
		for (int z = 0; z < VOXEL_SIDE; z++)
			make_cube(x, y, z);
	}

	struct timespec t_finish;
	clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &t_finish);
	printf("Done. Time elapsed: %f seconds\n", (t_finish.tv_sec - t_start.tv_sec) + (t_finish.tv_nsec - t_start.tv_nsec) / 1000000000.0f);

	/* Writing the mesh to output.obj */
	if (argc > 1 && argv[1][0] == 's')
	{
		printf("Skipping mesh write...\n");
		goto finish;
	}

	printf("Making the mesh to \"output.obj\"...\n");
	FILE *f = fopen("output.obj", "w");

	/* Writing verticies */
	{
		struct vertex_hash *current = NULL;
		struct vertex_hash *temp = NULL;
		HASH_ITER(hh, vertices, current, temp)
		{
			fprintf(
				f,
				"v %i %i %i\n",
				EXTRACT_BITS(current->id, 0, BITS_PER_SIDE), // x
				EXTRACT_BITS(current->id, BITS_PER_SIDE, BITS_PER_SIDE), // y
				EXTRACT_BITS(current->id, BITS_PER_SIDE * 2, BITS_PER_SIDE) // z
			);
		}
	}

	/* Writing faces */
	{
		struct face_list *current = faces_head;
		while (current != NULL)
		{
			fprintf(
				f,
				"f %i %i %i\n",
				current->ns[0] + 1,
				current->ns[1] + 1,
				current->ns[2] + 1
			);
			current = current->next;
		}
	}

	fclose(f);

finish:
	printf("Done!\n");

	return 0;
}
