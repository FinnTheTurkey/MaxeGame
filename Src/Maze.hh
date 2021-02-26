#ifndef MAZE_HH
#define MAZE_HH

#include "Flux/ECS.hh"
extern int test_level[64];

void generateTerrain(int* terrain, int size, Flux::ECSCtx* ctx);

#endif