#include "Flux/ECS.hh"
#include "Flux/Renderer.hh"
#include "Flux/Resources.hh"
#include "Maze.hh"
#include <algorithm>
#include <cmath>
#include <unordered_map>
#include <vector>
#include <queue>

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

using namespace Flux::Renderer;

Vertex cube_vertices[24] {
    {1, 1, -1, 0, 1, 0, 0.625, 0.5, -1, 0, 0, 0, 0, -1},
    {-1, 1, -1, 0, 1, 0, 0.875, 0.5, -1, 0, 0, 0, 0, -1},
    {-1, 1, 1, 0, 1, 0, 0.875, 0.75, -1, 0, 0, 0, 0, -1},
    {1, 1, 1, 0, 1, 0, 0.625, 0.75, -1, 0, 0, 0, 0, -1},
    {1, -1, 1, 0, 0, 1, 0.375, 0.75, 0, 1, 0, 1, 0, 0},
    {1, 1, 1, 0, 0, 1, 0.625, 0.75, 0, 1, 0, 1, 0, 0},
    {-1, 1, 1, 0, 0, 1, 0.625, 1, 0, 1, 0, 1, 0, 0},
    {-1, -1, 1, 0, 0, 1, 0.375, 1, 0, 1, 0, 1, 0, 0},
    {-1, -1, 1, -1, 0, 0, 0.375, 0, 0, 1, 0, 0, 0, 1},
    {-1, 1, 1, -1, 0, 0, 0.625, 0, 0, 1, 0, 0, 0, 1},
    {-1, 1, -1, -1, 0, 0, 0.625, 0.25, 0, 1, 0, 0, 0, 1},
    {-1, -1, -1, -1, 0, 0, 0.375, 0.25, 0, 1, 0, 0, 0, 1},
    {-1, -1, -1, 0, -1, 0, 0.125, 0.5, 1, 0, 0, 0, 0, -1},
    {1, -1, -1, 0, -1, 0, 0.375, 0.5, 1, 0, 0, 0, 0, -1},
    {1, -1, 1, 0, -1, 0, 0.375, 0.75, 1, 0, 0, 0, 0, -1},
    {-1, -1, 1, 0, -1, 0, 0.125, 0.75, 1, 0, 0, 0, 0, -1},
    {1, -1, -1, 1, 0, 0, 0.375, 0.5, 0, 1, 0, 0, 0, -1},
    {1, 1, -1, 1, 0, 0, 0.625, 0.5, 0, 1, 0, 0, 0, -1},
    {1, 1, 1, 1, 0, 0, 0.625, 0.75, 0, 1, 0, 0, 0, -1},
    {1, -1, 1, 1, 0, 0, 0.375, 0.75, 0, 1, 0, 0, 0, -1},
    {-1, -1, -1, 0, 0, -1, 0.375, 0.25, 0, 1, 0, -1, 0, 0},
    {-1, 1, -1, 0, 0, -1, 0.625, 0.25, 0, 1, 0, -1, 0, 0},
    {1, 1, -1, 0, 0, -1, 0.625, 0.5, 0, 1, 0, -1, 0, 0},
    {1, -1, -1, 0, 0, -1, 0.375, 0.5, 0, 1, 0, -1, 0, 0}
};

uint32_t cube_indices[36] {
    0,
    1,
    2,
    0,
    2,
    3,
    4,
    5,
    6,
    4,
    6,
    7,
    8,
    9,
    10,
    8,
    10,
    11,
    12,
    13,
    14,
    12,
    14,
    15,
    16,
    17,
    18,
    16,
    18,
    19,
    20,
    21,
    22,
    20,
    22,
    23
};

Flux::Resources::ResourceRef<Flux::Renderer::ShaderRes> shader_res;
Flux::Resources::ResourceRef<Flux::Renderer::MaterialRes> mat_res;

void setupTerrain()
{
    auto diffuse = Flux::Resources::createResource(new TextureRes);
    // diffuse->loadImage("Assets/Rock023_1K-JPG/Rock023_1K_Color.jpg");
    diffuse->loadImage("Assets/textures/TexturesCom_Pavement_CobblestoneMedieval01_4x4_1K_albedo.png");

    auto normal = Flux::Resources::createResource(new TextureRes);
    // normal->loadImage("Assets/Rock023_1K-JPG/Rock023_1K_Normal.jpg");
    normal->loadImage("Assets/textures/TexturesCom_Pavement_CobblestoneMedieval01_4x4_1K_normal.png");

    auto roughness = Flux::Resources::createResource(new TextureRes);
    // roughness->loadImage("Assets/Rock023_1K-JPG/Rock023_1K_Roughness.jpg");
    roughness->loadImage("Assets/textures/TexturesCom_Pavement_CobblestoneMedieval01_4x4_1K_roughness.png");

    shader_res = createShaderResource("shaders/vertex.vert", "shaders/fragment.frag");
    mat_res = createMaterialResource(shader_res);
    setUniform(mat_res, "color", glm::vec3(0.6, 0.6, 0.6));
    setUniform(mat_res, "has_diffuse", true);
    setUniform(mat_res, "diffuse_texture", diffuse);
    setUniform(mat_res, "has_metal_map", false);
    setUniform(mat_res, "has_normal_map", true);
    setUniform(mat_res, "normal_map", normal);
    setUniform(mat_res, "has_roughness_map", false);
    setUniform(mat_res, "roughness_map", roughness);
    setUniform(mat_res, "metallic", 0.0f);
    setUniform(mat_res, "roughness", 0.6f);
}

void destroyTerrain()
{
    auto x = Flux::Resources::ResourceRef<Flux::Renderer::ShaderRes>();
    shader_res = x;
    auto y = Flux::Resources::ResourceRef<Flux::Renderer::MaterialRes>();
    mat_res = y;
}

Flux::EntityRef generateTerrain(Flux::ECSCtx* ctx, const Level& level)
{
    // Load cube base
    // auto cube = Flux::Resources::deserialize("Assets/Cube.farc");

    std::vector<Flux::Renderer::Vertex> vertices;
    std::vector<uint32_t> indices;

    for (int y = 0; y < level.size; y++)
    {
        for (int x = 0; x < level.size; x++)
        {
            if (level.level[y * level.size + x] == 1)
            {
                // Add a 2x2 cube
                // auto ens = cube->addToECS(ctx);
                // Flux::Transform::translate(ens[0], glm::vec3(x * 2 + 1, 0, y * 2 + 1));

                // vertices.push_back(Vertex {x * 2 + 1.0f, y * 2 - 1.0f, 1,  /* Normals */ 0, 0, -1, 0, 0, /* Tan stuff */ 1, 0, 0, 0, 0, -1});
                // vertices.push_back(Vertex {x * 2 + 1.0f, y * 2 - 1.0f, 1,  /* Normals */ 0, 0, -1, 0, 0, /* Tan stuff */ 1, 0, 0, 0, 0, -1});
                int before_size = vertices.size();

                for (int i = 0; i < 24; i++)
                {
                    auto to_add = cube_vertices[i];
                    to_add.x = to_add.x + (x * 2 + 1);
                    // to_add.y += y * 2;
                    to_add.z = to_add.z + (y * 2 + 1);
                    // std::cout << to_add.x << " " << to_add.z << "\n";
                    vertices.push_back(to_add);
                }

                for (int i = 0; i < 36; i++)
                {
                    // Make sure the indices are for the right cube
                    indices.push_back(before_size + cube_indices[i]);
                }
            }
        }
    }

    // Create Mesh Resource
    auto mesh_res = new MeshRes;
    mesh_res->draw_mode = DrawMode::Triangles;
    mesh_res->indices_length = indices.size();
    mesh_res->indices = new uint32_t[indices.size()];
    for (int i = 0; i < indices.size(); i++)
    {
        auto in = indices[i];
        mesh_res->indices[i] = in;
    }

    mesh_res->vertices_length = vertices.size();
    mesh_res->vertices = new Vertex[vertices.size()];
    for (int i = 0; i < vertices.size(); i++)
    {
        auto v = vertices[i];
        mesh_res->vertices[i] = v;
        // std::cout << mesh_res->vertices[i].x << " " << mesh_res->vertices[i].z << "\n";
    }

    auto mres = Flux::Resources::createResource(mesh_res);

    auto entity = ctx->createEntity();
    addMesh(entity, mres, mat_res);

    return entity;
}

template<typename T, typename priority_t>
struct PriorityQueue {
  typedef std::pair<priority_t, T> PQElement;
  std::priority_queue<PQElement, std::vector<PQElement>,
                 std::greater<PQElement>> elements;

  inline bool empty() const {
     return elements.empty();
  }

  inline void put(T item, priority_t priority) {
    elements.emplace(priority, item);
  }

  T get() {
    T best_item = elements.top().second;
    elements.pop();
    return best_item;
  }
};

struct GridPosition
{
    GridPosition()
    {
        x = -1;
        y = -1;
    }
    GridPosition(float x, float y)
    {
        this->x = x;
        this->y = y;
    }
    GridPosition(const glm::vec2& v)
    {
        x = v.x;
        y = v.y;
    }

    float x;
    float y;

    friend bool operator==(const GridPosition& a, const GridPosition& b)
    {
        return a.x == b.x && a.y == b.y;
    }

    friend bool operator!=(const GridPosition& a, const GridPosition& b)
    {
        return a.x != b.x || a.y != b.y;
    }

    friend bool operator>(const GridPosition& a, const GridPosition& b)
    {
        return a.x + a.y > b.x + b.y;
    }

    friend bool operator<(const GridPosition& a, const GridPosition& b)
    {
        return a.x + a.y > b.x + b.y;
    }

    friend bool operator>=(const GridPosition& a, const GridPosition& b)
    {
        return a.x + a.y >= b.x + b.y;
    }

    friend bool operator<=(const GridPosition& a, const GridPosition& b)
    {
        return a.x + a.y >= b.x + b.y;
    }
};

inline double heuristic(const GridPosition& a, const GridPosition& b) {
  return std::abs(a.x - b.x) + std::abs(a.y - b.y);
}

struct GridNode
{
    GridPosition position;
    GridPosition came_from;
    double cost;
    Direction direction_from;
};

void processNode(const GridPosition& current, const GridPosition& next, PriorityQueue<GridPosition, double>& frontier,
                const Level& level, Direction direction, GridNode* grid, const GridPosition& end)
{
    double new_cost = grid[(int)current.y * level.size + (int)current.x].cost + 
                    (level.level[(int)next.y * level.size + (int)next.x] == 1 ? 1 : 0);
    
    int c = grid[(int)next.y * level.size + (int)next.x].cost;
    if (c == -1 || new_cost < c)
    {
        grid[(int)next.y * level.size + (int)next.x].cost = new_cost;
        double priority = new_cost;// + heuristic(next, end);
        frontier.put(next, priority);
        grid[(int)next.y * level.size + (int)next.x].came_from = current;
        grid[(int)next.y * level.size + (int)next.x].direction_from = direction;
    }
}

std::vector<RouteNode> findPath(const Level& level, Direction direction, const glm::vec2& start, const glm::vec2& end)
{
    // Create pathfinding grid
    GridNode nodes[level.size * level.size];
    
    // Initialize the nodes
    for (int y = 0; y < level.size; y++)
    {
        for (int x = 0; x < level.size; x++)
        {
            nodes[y * level.size + x] = GridNode {GridPosition(x, y), GridPosition(-1, -1), -1, North};
        }
    }

    PriorityQueue<GridPosition, double> frontier;
    frontier.put(start, 0);
    nodes[(int)start.y * level.size + (int)start.x].cost = 0;

    while (!frontier.empty())
    {
        GridPosition current = frontier.get();

        if (current == end)
        {
            break;
        }

        // Get all the 4 directions
        // The route is reversed, so the directions are too
        if (current.x-1 >= 0)
        {
            // West aka East
            processNode(current, GridPosition(current.x-1, current.y), frontier, level, East, nodes, end);
        }

        if (current.x+1 < level.size)
        {
            // East aka West
            processNode(current, GridPosition(current.x+1, current.y), frontier, level, West, nodes, end);
        }

        if (current.y+1 < level.size)
        {
            // South aka North
            processNode(current, GridPosition(current.x, current.y+1), frontier, level, North, nodes, end);
        }

        if (current.y-1 >= 0)
        {
            // North aka South
            processNode(current, GridPosition(current.x, current.y-1), frontier, level, South, nodes, end);
        }
    }

    std::vector<RouteNode> route;
    GridPosition pos = end;
    while (pos != GridPosition(start))
    {
        // Follow the bread crumb trail
        RouteNode node;
        node.destination = glm::vec2(nodes[(int)pos.y * level.size + (int)pos.x].came_from.x,
                            nodes[(int)pos.y * level.size + (int)pos.x].came_from.y);
        node.direction = nodes[(int)pos.y * level.size + (int)pos.x].direction_from;
        route.push_back(node);

        pos = node.destination;
    }

    std::reverse(route.begin(), route.end());

    // Print out route
    // std::cout << "==========================\n";
    // for (auto i : route)
    // {
    //     std::cout << i.destination.x << " " << i.destination.y << " " << i.direction << "\n";
    // }

    return route;
}
