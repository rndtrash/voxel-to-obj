#pragma once

#include <uthash.h>

typedef struct vertex_hash
{
	int id;			/* key */
	int index;		/* value */
	UT_hash_handle hh;	/* hash */
}
vertex_hash;

typedef struct face_list
{
	int indices[3];		/* value */
	struct face_list *next;
}
face_list;

typedef struct voxel_model
{
	char ***voxels;
	int size;

	int vertexIota;
	vertex_hash *vertices;
	face_list *faces;

	face_list *_facesTail;

	int _axisBits;
	int _axisMask;

	char*** _voxelsPP;
	char** _voxelsP;
	char* _voxels;
}
voxel_model;

typedef struct rect
{
	int x, y, z;
	int w, h;
}
rect;
