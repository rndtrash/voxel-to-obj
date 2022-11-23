#include <voxel.h>

#include <limits.h>
#include <voxel_utils.h>

void voxel_init(voxel_model *vm, const int size)
{
	vm->size = size;
	vm->vertexIota = 0;
	vm->vertices = NULL;
	vm->faces = vm->_facesTail = NULL;

	vm->_axisBits = bitwidth(size) + 1;
	vm->_axisMask = 0xFFFFFFFF >> (32 - vm->_axisBits);

	vm->_voxelsPP = calloc(size, sizeof(char**));
	vm->_voxelsP = calloc(size * size, sizeof(char*));
	vm->_voxels = calloc(size * size * size, sizeof(char));

	for (int i = 0; i < size; ++i) {
		vm->_voxelsPP[i] = vm->_voxelsP + (i * size);
	}

	for (uint32_t i = 0; i < size * size; ++i) {
		vm->_voxelsP[i] = vm->_voxels + (i * size);
	}

	vm->voxels = vm->_voxelsPP;

#ifdef DEBUG
	printf("voxel_init: _axisBits=%i", vm->_axisBits);
#endif
}

void voxel_free(voxel_model *vm)
{
	free(vm->_voxelsPP);
	free(vm->_voxelsP);
	free(vm->_voxels);

	{
		vertex_hash *current = NULL;
		vertex_hash *temp = NULL;
		HASH_ITER(hh, vm->vertices, current, temp)
		{
			HASH_DEL(vm->vertices, current);
			free(current);
		}
	}

	{
		face_list *current = vm->faces;
		while (current != NULL)
		{
				face_list *next = current->next;
				free(current);
				current = next;
		}
	}
}

void voxel_greedy(voxel_model *vm)
{
	typedef struct rect_list
	{
		rect rect;
		struct rect_list *next;
	}
	rect_list;

	inline void add_rect(rect_list **list, rect rect)
	{
		rect_list *r = malloc(sizeof *r);
		r->rect.x = rect.x;
		r->rect.y = rect.y;
		r->rect.z = rect.z;
		r->rect.w = rect.w;
		r->rect.h = rect.h;

		if (*list != NULL)
		{
			r->next = *list;
		}
		else
		{
			r->next = NULL;
		}
		*list = r;
	}

	typedef struct rect_hash
	{
		int id;			/* key */
		rect_list *r;		/* value */
		UT_hash_handle hh;	/* hash */
	}
	rect_hash;

	/* Front*/
	for (int x = 0; x < vm->size; x++)
	{
		rect_hash *rectHash = NULL;
		/* Extending up */
		for (int z = 0; z < vm->size; z++)
		{
			for (int y = 0; y < vm->size; y++)
			{
				if (vm->voxels[x][y][z] == 0 || (VERTEX_VALID(x - 1, y, z, vm->size) && vm->voxels[x - 1][y][z] != 0))
					continue;

				int localY = y + 1;
				while (localY < vm->size && vm->voxels[x][localY][z] == 1 && (!VERTEX_VALID(x - 1, localY, z, vm->size) || vm->voxels[x - 1][localY][z] == 0))
				{
					localY++;
				}
				int height = localY - y;

				rect_hash *current;
				HASH_FIND_INT(rectHash, &y, current);
				if (current == NULL)
				{
#ifdef DEBUG
					printf("Making a new %i,%i,%i entry!\n", x, y, z);
#endif
					current = malloc(sizeof *current);
					current->id = y;
					current->r = NULL;
					HASH_ADD_INT(rectHash, id, current);
				}

				add_rect(&current->r, (rect) { x, y, z, 1, height });

				y = localY;
			}
		}

		/* Extending right and pushing to the list */
		rect_hash *current = NULL;
		rect_hash *temp = NULL;
		HASH_ITER(hh, rectHash, current, temp)
		{
			rect_list *rect = current->r;
#ifdef DEBUG
			printf("RECT: XYZ:%i,%i,%i W=%i H=%i\n", rect->rect.x, rect->rect.y, rect->rect.z, rect->rect.w, rect->rect.h);
#endif
			while (rect != NULL)
			{
				rect_list *next = rect->next;
				if (next == NULL || abs(next->rect.z - rect->rect.z) != 1 || next->rect.h != rect->rect.h)
				{
#ifdef DEBUG
					if (next)
						printf("Pushing rect to the vertex list! WH %i,%i and %i,%i!\n", rect->rect.w, rect->rect.h, next->rect.w, next->rect.h);
					else
						printf("That's the last rect on this Z=%i! (WH %i,%i)\n", rect->rect.x, rect->rect.w, rect->rect.h);
#endif
					/* Append polygons and free the rect */
					voxel_make_face(
						vm,
						voxel_get_index(vm, rect->rect.x, rect->rect.y               , rect->rect.z + rect->rect.w),
						voxel_get_index(vm, rect->rect.x, rect->rect.y + rect->rect.h, rect->rect.z + rect->rect.w),
						voxel_get_index(vm, rect->rect.x, rect->rect.y + rect->rect.h, rect->rect.z               ),
						voxel_get_index(vm, rect->rect.x, rect->rect.y               , rect->rect.z               )
					);
					free(rect);
					rect = next;
				}
				else
				{
#ifdef DEBUG
					printf("Merging rects with WH %i,%i and %i,%i!\n", rect->rect.w, rect->rect.h, next->rect.w, next->rect.h);
#endif

					/* Merge two rects and free the latter one */
					rect->rect.w += next->rect.w;
					rect->rect.z = next->rect.z;

					next = next->next;
					free(rect->next);
					rect->next = next;
				}
			}

			HASH_DEL(rectHash, current);
			free(current);
		}
	}

	// TODO: Back, Left, Right, Top, Bottom
}

void voxel_simple(voxel_model *vm)
{
	/* Replace Y with - 1 if it points down */
	static const int vertex_offsets[8][3] = {
		{ 0, 0, 0 },
		{ 0, 1, 0 },
		{ 0, 0, 1 },
		{ 0, 1, 1 },
		{ 1, 0, 0 },
		{ 1, 1, 0 },
		{ 1, 0, 1 },
		{ 1, 1, 1 }
	};

	for (int x = 0; x < vm->size; x++)
	for (int y = 0; y < vm->size; y++)
	for (int z = 0; z < vm->size; z++)
	{
		if (vm->voxels[x][y][z] == 0)
			continue;

		int vertices_cache[8] = { -1, -1, -1, -1, -1, -1, -1, -1 };
		int _get_v(const int i)
		{
			if (vertices_cache[i] == -1)
				vertices_cache[i] = voxel_get_index(vm, x + vertex_offsets[i][0], y + vertex_offsets[i][1], z + vertex_offsets[i][2]);
			return vertices_cache[i];
		}

		/* Front and back */
		if (!VERTEX_VALID(x - 1, y, z, vm->size) || vm->voxels[x - 1][y][z] == 0)
			voxel_make_face(vm, _get_v(2), _get_v(3), _get_v(1), _get_v(0));
		if (!VERTEX_VALID(x + 1, y, z, vm->size) || vm->voxels[x + 1][y][z] == 0)
			voxel_make_face(vm, _get_v(4), _get_v(5), _get_v(7), _get_v(6));

		/* Left and right */
		if (!VERTEX_VALID(x, y, z - 1, vm->size) || vm->voxels[x][y][z - 1] == 0)
			voxel_make_face(vm, _get_v(0), _get_v(1), _get_v(5), _get_v(4));
		if (!VERTEX_VALID(x, y, z + 1, vm->size) || vm->voxels[x][y][z + 1] == 0)
			voxel_make_face(vm, _get_v(6), _get_v(7), _get_v(3), _get_v(2));

		/* Top and bottom */
		if (!VERTEX_VALID(x, y - 1, z, vm->size) || vm->voxels[x][y - 1][z] == 0)
			voxel_make_face(vm, _get_v(6), _get_v(2), _get_v(0), _get_v(4));
		if (!VERTEX_VALID(x, y + 1, z, vm->size) || vm->voxels[x][y + 1][z] == 0)
			voxel_make_face(vm, _get_v(3), _get_v(7), _get_v(5), _get_v(1));
	}
}
