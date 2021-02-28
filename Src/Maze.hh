#ifndef MAZE_HH
#define MAZE_HH

#include "Flux/ECS.hh"
#include "glm/glm.hpp"

enum RotateType
{
    No, Left, Right, Forwards, Backwards
};

enum Direction
{
    North, South, East, West
};

struct Level
{
    glm::vec2 starting_pos;
    glm::vec2 exit_position;
    uint32_t size;
    int* level;
};

struct RouteNode
{
    glm::vec2 destination;
    Direction direction;
};

struct Monster
{
    Flux::EntityRef entity;
    glm::vec2 position;
    Direction direction;
    int next_move;
};

struct CorpseIndicator
{
    Flux::EntityRef entity;

};

extern int test_level[64];

void setupTerrain();
void destroyTerrain();

Flux::EntityRef generateTerrain(Flux::ECSCtx* ctx, const Level& level);

std::vector<RouteNode> findPath(const Level& level, Direction direction, const glm::vec2& start, const glm::vec2& end);

#endif