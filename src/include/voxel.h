#pragma once

#include <voxel_types.h>

int voxel_init(voxel_model *vm, const int size);
void voxel_simple(voxel_model *vm);
void voxel_greedy(voxel_model *vm);
void voxel_free(voxel_model *vm);
