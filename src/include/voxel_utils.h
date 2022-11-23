#pragma once

#include <voxel_types.h>

#define VERTEX_COMPRESS(x, y, z, bits, mask) \
	((x & mask) | ((y & mask) << bits) | ((z & mask) << (bits * 2)))
#define VERTEX_VALID(x, y, z, size) \
	(x > 0 && x < size && y > 0 && y < size && z > 0 && z < size)

int voxel_get_index(voxel_model *vm, const int x, const int y, const int z);
void voxel_make_face(voxel_model *vm, const int id1, const int id2, const int id3, const int id4);
int bitwidth(int i);
