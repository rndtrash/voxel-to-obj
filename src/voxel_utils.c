#include <voxel_utils.h>

int voxel_get_index(voxel_model *vm, const int x, const int y, const int z)
{
	const int id = VERTEX_COMPRESS(x, y, z, vm->_axisBits, vm->_axisMask);
#ifdef DEBUG
	printf("Compressed: %i\n", id);
#endif
	vertex_hash *vertex;
	HASH_FIND_INT(vm->vertices, &id, vertex);

	if (vertex == NULL)
	{
#ifdef DEBUG
		printf("Failed to find a vertex (%i %i %i), making a new one then!\n", x, y, z);
#endif
		if ((vertex = malloc(sizeof *vertex)) == NULL)
			return -1;
		vertex->id = id;
		vertex->index = vm->vertexIota++;
#ifdef DEBUG
		printf("IOTA: %i\n", vm->vertexIota - 1);
#endif
		HASH_ADD_INT(vm->vertices, id, vertex);
	}

	return vertex->index;
}

void voxel_make_face(voxel_model *vm, const int id1, const int id2, const int id3, const int id4)
{
	if (id1 < 0 || id2 < 0 || id3 < 0 || id4 < 0)
		return;

	face_list *face1;
	if ((face1 = malloc(sizeof *face1)) == NULL)
		return;
	face1->indices[0] = id1;
	face1->indices[1] = id2;
	face1->indices[2] = id3;

	face_list *face2;
	if ((face2 = malloc(sizeof *face2)) == NULL)
		return;
	face2->indices[0] = id3;
	face2->indices[1] = id4;
	face2->indices[2] = id1;
	face2->next = NULL;

	face1->next = face2;

	if (vm->_facesTail != NULL)
	{
		vm->_facesTail->next = face1;
		vm->_facesTail = face2;
	}
	else
	{
		vm->faces = face1;
		vm->_facesTail = face2;
	}
}

int bitwidth(int i)
{
	int bits = 0;
	while (i >>= 1)
		bits++;

	return bits;
}
