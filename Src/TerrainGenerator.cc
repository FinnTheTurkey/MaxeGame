
#include "Flux/ECS.hh"
#include "Flux/Renderer.hh"
#include "Flux/Resources.hh"
#include <vector>
int test_level[64] {
    0, 0, 1, 1, 1, 1, 0, 0,
    0, 0, 1, 0, 0, 1, 0, 0,
    1, 1, 1, 0, 0, 1, 1, 1,
    0, 0, 0, 0, 0, 0, 0, 0,
    1, 1, 1, 0, 0, 1, 1, 1,
    0, 0, 1, 0, 0, 1, 0, 0,
    0, 0, 1, 1, 0, 1, 0, 0,
    0, 0, 1, 1, 1, 1, 0, 0
};

void generateTerrain(int* terrain, int size, Flux::ECSCtx* ctx)
{
    // Load cube base
    auto cube = Flux::Resources::deserialize("Assets/Cube.farc");

    for (int y = 0; y < size; y++)
    {
        for (int x = 0; x < size; x++)
        {
            if (terrain[y * 8 + x] == 1)
            {
                // Add a 2x2 cube
                auto ens = cube->addToECS(ctx);
                Flux::Transform::translate(ens[0], glm::vec3(x * 2 + 1, 0, y * 2 + 1));
            }
        }
    }
}