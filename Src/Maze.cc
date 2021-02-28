#include "Flux/ECS.hh"
#include "Flux/Input.hh"
#include "Flux/OpenGL/GLRenderer.hh"
#include "Flux/Renderer.hh"
#include "Flux/Resources.hh"
#include "glm/detail/func_exponential.hpp"
#include "glm/detail/func_trigonometric.hpp"
#include "glm/gtc/quaternion.hpp"
#include <cmath>
#include <iostream>
#include <vector>
#define GLM_FORCE_CTOR_INIT
#include "FluxArc/FluxArc.hh"
#include "Flux/Flux.hh"

#include "Maze.hh"

#include "Levels.ii"

Flux::ECSCtx ctx;
Flux::ECSCtx hud_ctx;
Flux::EntityRef camera;
Flux::EntityRef compass;
Flux::EntityRef death_screen;
Flux::EntityRef win_screen;
Flux::EntityRef splash_screen;
Flux::EntityRef terrain;

bool left_pressed = false;
bool right_pressed = false;

bool forwards_pressed = false;
bool backwards_pressed = false;

bool marker_pressed = false;

Direction direction = North;

float rotate_left = 0;
RotateType rotate_type = No;

const int speed = 8;

glm::vec2 position = glm::vec2(4, 4);

Level level1 {
    glm::vec2(3, 3),
    glm::vec2(14, 14),
    16,
    new int[16 * 16] {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 1, 1, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}
};

int current_level_id = 0;
const int total_levels = 3;

Level& current_level = level1;

std::vector<Monster*> monsters;
std::vector<CorpseIndicator> corpses;

std::vector<Flux::EntityRef> monster_entities;
std::vector<Flux::EntityRef> corpse_entities;
std::vector<Flux::EntityRef> marker_entities;

Flux::Resources::ResourceRef<Flux::Renderer::ShaderRes> basic_shaders;

std::vector<std::vector<RotateType> > ghost_trails;
std::vector<RotateType> current_trail;
std::vector<Flux::EntityRef> ghosts;

int time_step = 0;

int safe_moves = 0;
int deaths = 0;

enum States
{
    Playing, Dying, PostDeath, Win, Splash
};

States state = Playing;

double timer;

void startLevel(int id)
{
    current_level = levels[id];
    position = current_level.starting_pos;

    monsters = std::vector<Monster*>();
    monster_entities = std::vector<Flux::EntityRef>();
    terrain = generateTerrain(&ctx, current_level);

    // Add exit
    auto eloader = Flux::Resources::deserialize("Assets/Exit.farc");
    auto eens = eloader->addToECS(&ctx);
    Flux::Transform::translate(eens[0], glm::vec3(current_level.exit_position.x * 2 + 1, 0, current_level.exit_position.y * 2 + 1));
    monster_entities.insert(monster_entities.end(), eens.begin(), eens.end());

    // Create monster
    auto monloader = Flux::Resources::deserialize("Assets/Oozey/OozeyScaled.farc");

    for (int y = 0; y < current_level.size; y++)
    {
        for (int x = 0; x < current_level.size; x++)
        {
            if (current_level.level[y * current_level.size + x] == 2)
            {
                auto mens = monloader->addToECS(&ctx);
                Monster* mon = new Monster {mens[0], glm::vec2(x, y), South};
                Flux::Transform::translate(mon->entity, glm::vec3(mon->position.x * 2 + 1, 0, mon->position.y * 2 + 1));
                Flux::Transform::rotate(mon->entity, glm::vec3(0, 1, 0), glm::radians(180.0));
                monsters.push_back(mon);

                monster_entities.insert(monster_entities.end(), mens.begin(), mens.end());
            }
        }
    }
    rotate_left = 0;
    direction = North;
    camera.removeComponent<Flux::Transform::TransformCom>();
    Flux::Transform::giveTransform(camera);
    auto o = glm::vec3(position.x * 2 + 1, 0, position.y * 2 + 1);
    Flux::Transform::setTranslation(camera, o);

    compass.removeComponent<Flux::Transform::TransformCom>();
    Flux::Transform::giveTransform(compass);
    Flux::Transform::setParent(compass, camera);
    Flux::Transform::scale(compass, glm::vec3(0.25, 0.25, 0.25));

    auto gloader = Flux::Resources::deserialize("Assets/Ghost.farc");
    int c = 0;
    for (auto i : ghost_trails)
    {
        auto gens = gloader->addToECS(&ctx);
        monster_entities.insert(monster_entities.end(), gens.begin(), gens.end());
        ghosts.push_back(gens[0]);
        Flux::Transform::setTranslation(gens[0], glm::vec3(current_level.starting_pos.x * 2 + 1, 0, current_level.starting_pos.y * 2 + 1));

        switch (ghost_trails[c][0])
        {
        case Forwards: Flux::Transform::translate(gens[0], glm::vec3(0, 0, -2)); break;
        case Backwards: Flux::Transform::translate(gens[0], glm::vec3(0, 0, 2)); break;
        case Left: Flux::Transform::rotate(gens[0], glm::vec3(0, 1, 0), glm::radians(90.0)); break;
        case Right: Flux::Transform::rotate(gens[0], glm::vec3(0, -1, 0), glm::radians(90.0)); break;
        case No: break;
        }

        c++;
    }

    current_trail = std::vector<RotateType>();
    time_step = 0;

    state = Playing;
}

void destroyLevel()
{
    for (auto i : monsters)
    {
        // ctx.destroyEntity(i->entity);
        delete i;
    }

    for (auto i : monster_entities)
    {
        ctx.destroyEntity(i);
    }

    ctx.destroyEntity(terrain);
    ghosts = std::vector<Flux::EntityRef>();
}

void destroyCorpses()
{
    for (auto i : corpse_entities)
    {
        ctx.destroyEntity(i);
    }

    corpses = std::vector<CorpseIndicator>();
}

void addCorpse(const glm::vec2& position)
{
    auto corloader = Flux::Resources::deserialize("Assets/knightStanding.farc");
    auto ens = corloader->addToECS(&ctx);
    Flux::Transform::translate(ens[0], glm::vec3(position.x * 2 + 1, 0, position.y * 2 + 1));
    corpses.push_back(CorpseIndicator {ens[0]});

    corpse_entities.insert(corpse_entities.end(), ens.begin(), ens.end());

    // Rotate the corpse
    // Use 3d space coords for more accurate result
    float O = (current_level.exit_position.x * 2 + 1) - (position.x * 2 + 1);
    float H = glm::sqrt(glm::pow(O, 2) + glm::pow((current_level.exit_position.y * 2 + 1) - (position.y * 2 + 1), 2));
    float angle = glm::asin(O/H);
    std::cout << "Angle to exit: " << glm::degrees(angle) << "\n";
    Flux::Transform::rotate(ens[0], glm::vec3(0, 1, 0), glm::degrees(-90.0));
    Flux::Transform::rotate(ens[0], glm::vec3(0, 1, 0), angle);
}

void addMarker()
{
    auto corloader = Flux::Resources::deserialize("Assets/Marker.farc");
    auto ens = corloader->addToECS(&ctx);
    Flux::Transform::translate(ens[0], glm::vec3(position.x * 2 + 1, 0, position.y * 2 + 1));

    corpse_entities.insert(corpse_entities.end(), ens.begin(), ens.end());
}

void removeMarkers()
{
    for (auto i : marker_entities)
    {
        ctx.destroyEntity(i);
    }

    marker_entities = std::vector<Flux::EntityRef>();
    ghost_trails = std::vector< std::vector<RotateType>>();
}

void unshadify(const std::vector<Flux::EntityRef>& entities)
{
    for (auto i : entities)
    {
        if (i.hasComponent<Flux::Renderer::MeshCom>())
        {
            auto mc = i.getComponent<Flux::Renderer::MeshCom>();
            auto new_matres = Flux::Renderer::createMaterialResource(basic_shaders);
            Flux::Renderer::setUniform(new_matres, "color", *(glm::vec3*)mc->mat_resource->uniforms["color"].value);
            mc->mat_resource = new_matres;
        }
    }
}

void init(int argc, char **argv)
{
    ctx = Flux::ECSCtx();
    hud_ctx = Flux::ECSCtx();
    setupTerrain();

    basic_shaders = Flux::Renderer::createShaderResource("shaders/basic.vert", "shaders/basic.frag");

    auto loader = Flux::Resources::deserialize("Assets/Enclosure.farc");
    auto ens = loader->addToECS(&ctx);

    // auto cloader = Flux::Resources::deserialize("Assets/BasicCube.farc");
    // auto cens = cloader->addToECS(&ctx);
    // for (auto i : cens)
    // {
    //     if (i.hasComponent<Flux::Renderer::MeshCom>())
    //     {
    //         std::cout << "==================================" <<  "\n";
    //         auto c = i.getComponent<Flux::Renderer::MeshCom>();
    //         for (int v = 0; v < c->mesh_resource->vertices_length; v++)
    //         {
    //             auto a = c->mesh_resource->vertices[v];
    //             std::cout << "{" << a.x << ", " << a.y << ", " << a.z << ", " << a.nx << ", " << a.ny << ", " << a.nz << ", " << a.tx << ", " << a.ty << ", " << a.tanx << ", " << a.tany << ", " << a.tanz << ", " << a.btanx << ", " << a.btany << ", " << a.btanz << "}\n";
    //         }
    //         std::cout << "==================================" <<  "\n";
    //         for (int v = 0; v < c->mesh_resource->indices_length; v++)
    //         {
    //             std::cout << c->mesh_resource->indices[v] << std::endl;
    //         }
    //     }
    // }


    camera = ctx.createEntity();
    Flux::Transform::giveTransform(camera);
    Flux::Transform::setCamera(camera);
    Flux::Renderer::addSpotLight(camera, glm::radians(40.0), 9999, glm::vec3(1, 1, 1));

    // Add compass
    auto comloader = Flux::Resources::deserialize("Assets/Compass.farc");
    auto comens = comloader->addToECS(&ctx);
    Flux::Transform::setParent(comens[0], camera);
    Flux::Transform::scale(comens[0], glm::vec3(0.25, 0.25, 0.25));
    compass = comens[0];

    // unshadify(comens);

    // Add death screen
    auto deathloader = Flux::Resources::deserialize("Assets/DeathScreen.farc");
    auto deathens = deathloader->addToECS(&ctx);
    Flux::Transform::setParent(deathens[0], camera);
    Flux::Transform::rotate(deathens[0], glm::vec3(0, 1, 0), glm::radians(-90.0));
    Flux::Transform::translate(deathens[0], glm::vec3(-0.5, 0, 0));
    Flux::Transform::scale(deathens[0], glm::vec3(0.5, 0.5, 0.5));
    death_screen = deathens[0];
    Flux::Transform::setVisible(death_screen, false);

    unshadify(deathens);
    // Flux::Renderer::addPointLight(death_screen, 9999, glm::vec3(0, 0, 0));

    // Add Death Screen light
    auto hcam = ctx.createNamedEntity("DeathScreenLight");
    Flux::Transform::giveTransform(hcam);
    Flux::Transform::setParent(hcam, camera);
    Flux::Transform::translate(hcam, glm::vec3(0, 0, -1));
    Flux::Renderer::addPointLight(hcam, 9999, glm::vec3(0, 0, 0));

    // Add win screen
    auto winloader = Flux::Resources::deserialize("Assets/WinScreen.farc");
    auto winens = winloader->addToECS(&ctx);
    Flux::Transform::setParent(winens[0], camera);
    Flux::Transform::rotate(winens[0], glm::vec3(0, 1, 0), glm::radians(-90.0));
    Flux::Transform::translate(winens[0], glm::vec3(-0.5, 0, 0));
    Flux::Transform::scale(winens[0], glm::vec3(0.5, 0.5, 0.5));
    win_screen = winens[0];
    Flux::Transform::setVisible(win_screen, false);

    unshadify(winens);

    // Add Win Screen light
    auto wcam = ctx.createNamedEntity("WinScreenLight");
    Flux::Transform::giveTransform(wcam);
    Flux::Transform::setParent(wcam, camera);
    Flux::Transform::translate(wcam, glm::vec3(0, 0, -1));
    Flux::Renderer::addPointLight(wcam, 9999, glm::vec3(0, 0, 0));

    // Add janky "global illumination"
    auto gi = ctx.createEntity();
    Flux::Transform::giveTransform(gi);
    Flux::Renderer::addPointLight(gi, 9999, glm::vec3(0.002, 0.002, 0.002));
    Flux::Transform::setParent(gi, camera);

    Flux::GLRenderer::addGLRenderer(&ctx);
    LOG_INFO("Added GL Renderer");
    Flux::Transform::addTransformSystems(&ctx);
    LOG_INFO("Added transform systems");

    startLevel(0);

    // Add splash screen
    auto splashloader = Flux::Resources::deserialize("Assets/SplashScreen.farc");
    auto splashens = splashloader->addToECS(&ctx);
    Flux::Transform::setParent(splashens[0], camera);
    Flux::Transform::rotate(splashens[0], glm::vec3(0, 1, 0), glm::radians(-90.0));
    Flux::Transform::translate(splashens[0], glm::vec3(-0.5, 0, 0));
    Flux::Transform::scale(splashens[0], glm::vec3(0.5, 0.5, 0.5));
    splash_screen = splashens[0];
    Flux::Transform::setVisible(splash_screen, true);
    Flux::Transform::setVisible(compass, false);

    unshadify(splashens);

    // Add Win Screen light
    auto scam = ctx.createNamedEntity("SplashScreenLight");
    Flux::Transform::giveTransform(scam);
    Flux::Transform::setParent(scam, camera);
    Flux::Transform::translate(scam, glm::vec3(3, 0, 0));
    Flux::Renderer::addPointLight(scam, 9999, glm::vec3(1, 1, 1));
    state = Splash;
}

void smoothMove(float delta)
{
    // Smooth moving
    if (rotate_left > 0)
    {
        switch (rotate_type)
        {
            case No: break;
            case Left:
                rotate_left -= 6.265732 * delta;
                if (rotate_left < 0)
                {
                    Flux::Transform::rotate(camera, glm::vec3(0, 1, 0), rotate_left + 6.265732 * delta);
                    Flux::Transform::rotate(compass, glm::vec3(0, 1, 0), rotate_left + 6.265732 * delta * -1);
                }
                else
                {
                    Flux::Transform::rotate(camera, glm::vec3(0, 1, 0), 6.265732 * delta);
                    Flux::Transform::rotate(compass, glm::vec3(0, 1, 0), 6.265732 * delta * -1);
                }
                
                break;
            case Right: 
                rotate_left -= 6.265732 * delta;
                if (rotate_left < 0)
                {
                    Flux::Transform::rotate(camera, glm::vec3(0, -1, 0), rotate_left + 6.265732 * delta);
                    Flux::Transform::rotate(compass, glm::vec3(0, -1, 0), rotate_left + 6.265732 * delta * -1);
                }
                else
                {
                    Flux::Transform::rotate(camera, glm::vec3(0, -1, 0), 6.265732 * delta);
                    Flux::Transform::rotate(compass, glm::vec3(0, -1, 0), 6.265732 * delta * -1);
                }
                
                break;

            case Forwards:
                rotate_left -= speed * delta;
                if (rotate_left < 0)
                {
                    Flux::Transform::translate(camera, glm::vec3(0, 0, -(rotate_left + speed * delta)));
                }
                else
                {
                    Flux::Transform::translate(camera, glm::vec3(0, 0, speed * delta * -1));
                }
                
                break;
            case Backwards: 
                rotate_left -= speed * delta;
                if (rotate_left < 0)
                {
                    Flux::Transform::translate(camera, glm::vec3(0, 0, (rotate_left + speed * delta)));
                }
                else
                {
                    Flux::Transform::translate(camera, glm::vec3(0, 0, speed * delta));
                }
                
                break;
        }
    }
}

void loop(float delta)
{
    if (state == Playing)
    {
        bool player_moved = false;
        if (rotate_left <= 0)
        {
            if (Flux::Input::isKeyPressed(FLUX_KEY_A) && !left_pressed)
            {
                // Flux::Transform::rotate(camera, glm::vec3(0, 1, 0), glm::radians(90.0));
                rotate_type = Left;
                rotate_left = glm::radians(90.0);
                left_pressed = true;

                switch (direction)
                {
                    case North: direction = West; break;
                    case East: direction = North; break;
                    case South: direction = East; break;
                    case West: direction = South; break;
                }
                current_trail.push_back(Left);
                player_moved = true;
            }

            else if (Flux::Input::isKeyPressed(FLUX_KEY_D) && !right_pressed)
            {
                // Flux::Transform::rotate(camera, glm::vec3(0, 1, 0), glm::radians(90.0));
                rotate_type = Right;
                rotate_left = glm::radians(90.0);
                right_pressed = true;

                switch (direction)
                {
                    case North: direction = East; break;
                    case East: direction = South; break;
                    case South: direction = West; break;
                    case West: direction = North; break;
                }

                current_trail.push_back(Right);
                player_moved = true;
            }
            

            else if (Flux::Input::isKeyPressed(FLUX_KEY_W) && !forwards_pressed)
            {
                // Flux::Transform::rotate(camera, glm::vec3(0, 1, 0), glm::radians(90.0));
                rotate_type = Forwards;
                rotate_left = 2;
                forwards_pressed = true;

                // Change position
                auto old_position = position;
                switch (direction)
                {
                    case North: position.y -= 1; break;
                    case South: position.y += 1; break;
                    case East: position.x += 1; break;
                    case West: position.x -= 1; break;
                }

                current_trail.push_back(Forwards);

                // std::cout << "X: " << position.x << " Y: " << position.y << "\n";

                if (position.x < current_level.size && position.y < current_level.size && position.x > -1 && position.y > -1)
                {
                    if (current_level.level[(int)position.y * current_level.size + (int)position.x] == 1)
                    {
                        // Undo the move
                        rotate_left = 0;
                        position = old_position;
                    }
                }
                player_moved = true;
            }
            

            else if (Flux::Input::isKeyPressed(FLUX_KEY_S) && !backwards_pressed)
            {
                // Flux::Transform::rotate(camera, glm::vec3(0, 1, 0), glm::radians(90.0));
                rotate_type = Backwards;
                rotate_left = 2;
                backwards_pressed = true;

                // Change position
                auto old_position = position;
                switch (direction)
                {
                    case North: position.y -= -1; break;
                    case South: position.y += -1; break;
                    case East: position.x += -1; break;
                    case West: position.x -= -1; break;
                }

                current_trail.push_back(Backwards);

                if (position.x < current_level.size && position.y < current_level.size && position.x > -1 && position.y > -1)
                {
                    if (current_level.level[(int)position.y * current_level.size + (int)position.x] == 1)
                    {
                        // Undo the move
                        rotate_left = 0;
                        position = old_position;
                    }
                }
                player_moved = true;
            }

            else if (Flux::Input::isKeyPressed(FLUX_KEY_E) && !marker_pressed)
            {
                addMarker();
            }

            if (!Flux::Input::isKeyPressed(FLUX_KEY_S) && backwards_pressed)
            {
                backwards_pressed = false;
            }
            if (!Flux::Input::isKeyPressed(FLUX_KEY_W) && forwards_pressed)
            {
                forwards_pressed = false;
            }
            if (!Flux::Input::isKeyPressed(FLUX_KEY_D) && right_pressed)
            {
                right_pressed = false;
            }
            if (!Flux::Input::isKeyPressed(FLUX_KEY_A) && left_pressed)
            {
                left_pressed = false;
            }
            if (!Flux::Input::isKeyPressed(FLUX_KEY_E) && marker_pressed)
            {
                marker_pressed = false;
            }
        }

        smoothMove(delta);

        if (player_moved && safe_moves < 1)
        {
            // Now the Monsters also get to move!
            for (auto mon : monsters)
            {
                auto route = findPath(current_level, mon->direction, mon->position, position);
                int rnum = 2;
                if (route.size() < 2)
                {
                    rnum = 1;
                    if (glm::distance(mon->position, position) < 2)
                    {
                        std::cout << "No more route!!" << std::endl;
                        rotate_left = 0;
                        state = Dying;
                        // timer = Flux::Renderer::getTime() + 1;
                        timer = 0;
                    }
                }

                if (route.size() < 1)
                {
                    rnum = 0;
                }

                if (mon->direction != route[rnum].direction)
                {
                    // Rotate
                    if ((mon->direction == North && route[rnum].direction == East) || 
                        (mon->direction == North && route[rnum].direction == South) ||
                        (mon->direction == East && route[rnum].direction == South) || 
                        (mon->direction == East && route[rnum].direction == West) || 
                        (mon->direction == South && route[rnum].direction == West)  ||
                        (mon->direction == South && route[rnum].direction == North) ||
                        (mon->direction == West && route[rnum].direction == North) ||
                        (mon->direction == West && route[rnum].direction == East))
                    {
                        // Turn right
                        // _only_ turn right
                        switch (mon->direction)
                        {
                            case North: mon->direction = East; break;
                            case East: mon->direction = South; break;
                            case South: mon->direction = West; break;
                            case West: mon->direction = North; break;
                        }

                        Flux::Transform::rotate(mon->entity, glm::vec3(0, -1, 0), glm::radians(90.0));
                    }
                    else
                    {
                        // Turn left
                        switch (mon->direction)
                        {
                            case North: mon->direction = West; break;
                            case East: mon->direction = North; break;
                            case South: mon->direction = East; break;
                            case West: mon->direction = South; break;
                        }

                        Flux::Transform::rotate(mon->entity, glm::vec3(0, 1, 0), glm::radians(90.0));
                    }
                }
                else
                {
                    if (mon->next_move < 1)
                    {
                        // Translate!
                        // Move forward
                        switch (mon->direction)
                        {
                            case North: mon->position.y -= 1; break;
                            case South: mon->position.y += 1; break;
                            case East: mon->position.x += 1; break;
                            case West: mon->position.x -= 1; break;
                        }

                        // Make sure we're always at the right place
                        mon->position = route[1].destination;

                        // Flux::Transform::translate(mon->entity, glm::vec3(0, 0, 2));
                        Flux::Transform::setTranslation(mon->entity, glm::vec3(mon->position.x * 2 + 1, 0, mon->position.y * 2 + 1));

                        if (deaths == 0)
                        {
                            mon->next_move = 1;
                        }
                        else
                        {
                            mon->next_move = rand() % deaths + 1;
                        }
                    }
                    else
                    {
                        mon->next_move -= 1;
                    }
                    
                }

                if (mon->position == position)
                {
                    std::cout << "Player got eaten!\n";
                    rotate_left = 0;
                    state = Dying;
                    timer = Flux::Renderer::getTime() + 1;
                }
            }

            if (position == current_level.exit_position)
            {
                // Win
                state = Win;
                timer = Flux::Renderer::getTime() - 1;
            }
        }
        else
        {
            if (player_moved)
            {
                safe_moves -= 1;
            }
        }

        if (player_moved)
        {
            // Move the ghosts
            for (int i = 0; i < ghost_trails.size(); i++)
            {
                if (ghost_trails[i].size()-1 > time_step+1)
                {
                    switch (ghost_trails[i][time_step+1])
                    {
                    case Forwards: Flux::Transform::translate(ghosts[i], glm::vec3(0, 0, -2)); break;
                    case Backwards: Flux::Transform::translate(ghosts[i], glm::vec3(0, 0, 2)); break;
                    case Left: Flux::Transform::rotate(ghosts[i], glm::vec3(0, 1, 0), glm::radians(90.0)); break;
                    case Right: Flux::Transform::rotate(ghosts[i], glm::vec3(0, -1, 0), glm::radians(90.0)); break;
                    case No: break;
                    }
                }
            }
            time_step ++;
        }
    }
    else if (state == Dying)
    {
        if (timer == 0)
        {
            timer = Flux::Renderer::getTime() + 1;
        }
        else if (Flux::Renderer::getTime() > timer)
        {
            camera.getComponent<Flux::Renderer::LightCom>()->color = glm::vec3(0, 0, 0);
            timer = Flux::Renderer::getTime() + 1;
            state = PostDeath;
        }
    }
    else if (state == PostDeath)
    {
        if (Flux::Renderer::getTime() > timer && timer != 0)
        {
            camera.getComponent<Flux::Renderer::LightCom>()->color = glm::vec3(1, 1, 1);
            // death_screen.getComponent<Flux::Renderer::LightCom>()->color = glm::vec3(1, 1, 1);
            ctx.getNamedEntity("DeathScreenLight").getComponent<Flux::Renderer::LightCom>()->color = glm::vec3(1, 1, 1);
            if (direction == South)
            {
                Flux::Transform::translate(ctx.getNamedEntity("DeathScreenLight"), glm::vec3(-3, 0, 1));
            }
            else if (direction == North)
            {
                Flux::Transform::translate(ctx.getNamedEntity("DeathScreenLight"), glm::vec3(3, 0, 1));
            }
            else if (direction == East)
            {
                Flux::Transform::translate(ctx.getNamedEntity("DeathScreenLight"), glm::vec3(0, 0, -1));
            }
            else
            {
                Flux::Transform::translate(ctx.getNamedEntity("DeathScreenLight"), glm::vec3(0, 0, 1));
            }
            Flux::Transform::setVisible(death_screen, true);
            Flux::Transform::setVisible(compass, false);

            // Do farometer
            auto farometer = ctx.getNamedEntity("Farometer_Cone");
            // Far: -0.4
            // Close: 0.4
            auto distance = glm::distance(position, current_level.exit_position);
            Flux::Transform::setTranslation(farometer, glm::vec3(0, 0, ((distance/std::sqrt((float)current_level.size * (float)current_level.size * 2.0f)) * 0.8)));
            std::cout << 0.4 - ((distance/std::sqrt((float)current_level.size * (float)current_level.size * 2.0f)) * 0.8) << "\n";
            std::cout << distance << "\n" << std::sqrt((float)current_level.size * (float)current_level.size * 2.0f) << "\n";

            // Add corpse
            addCorpse(position);

            // Give the player some safe moves
            deaths ++;
            safe_moves = deaths * deaths + 10;

            // Add the players trail to the ghost trail
            ghost_trails.push_back(current_trail);

            timer = 0;
        }

        if (Flux::Input::isKeyPressed(FLUX_KEY_SPACE))
        {
            // TODO: Make easier
            destroyLevel();
            camera.getComponent<Flux::Renderer::LightCom>()->color = glm::vec3(1, 1, 1);
            // death_screen.getComponent<Flux::Renderer::LightCom>()->color = glm::vec3(1, 1, 1);
            ctx.getNamedEntity("DeathScreenLight").getComponent<Flux::Renderer::LightCom>()->color = glm::vec3(0, 0, 0);
            Flux::Transform::setVisible(death_screen, false);
            Flux::Transform::setVisible(compass, true);
            startLevel(current_level_id);
        }
    }
    else if (state == Win)
    {
        if (Flux::Renderer::getTime() > timer && timer != 0)
        {
            camera.getComponent<Flux::Renderer::LightCom>()->color = glm::vec3(1, 1, 1);
            // death_screen.getComponent<Flux::Renderer::LightCom>()->color = glm::vec3(1, 1, 1);
            ctx.getNamedEntity("WinScreenLight").getComponent<Flux::Renderer::LightCom>()->color = glm::vec3(1, 1, 1);
            if (direction == South)
            {
                Flux::Transform::translate(ctx.getNamedEntity("DeathScreenLight"), glm::vec3(-3, 0, 1));
            }
            else if (direction == North)
            {
                Flux::Transform::translate(ctx.getNamedEntity("DeathScreenLight"), glm::vec3(3, 0, 1));
            }
            else if (direction == East)
            {
                Flux::Transform::translate(ctx.getNamedEntity("DeathScreenLight"), glm::vec3(0, 0, -1));
            }
            else
            {
                Flux::Transform::translate(ctx.getNamedEntity("DeathScreenLight"), glm::vec3(0, 0, 1));
            }
            Flux::Transform::setVisible(win_screen, true);
            Flux::Transform::setVisible(compass, false);

            if (current_level_id+1 >= total_levels)
            {
                Flux::Transform::setVisible(ctx.getNamedEntity("NextLevel_Text.001"), false);
            }

            timer = 0;
        }

        if (Flux::Input::isKeyPressed(FLUX_KEY_SPACE))
        {
            if (current_level_id+1 < total_levels)
            {
                destroyCorpses();
                destroyLevel();
                removeMarkers();
                camera.getComponent<Flux::Renderer::LightCom>()->color = glm::vec3(1, 1, 1);
                // death_screen.getComponent<Flux::Renderer::LightCom>()->color = glm::vec3(1, 1, 1);
                ctx.getNamedEntity("WinScreenLight").getComponent<Flux::Renderer::LightCom>()->color = glm::vec3(0, 0, 0);
                Flux::Transform::setVisible(win_screen, false);
                Flux::Transform::setVisible(compass, true);
                safe_moves = 0;
                deaths = 0;
                startLevel(current_level_id + 1);
                current_level_id ++;
            }
        }
    }
    else if (state == Splash)
    {
        if (Flux::Input::isKeyPressed(FLUX_KEY_SPACE))
        {
            camera.getComponent<Flux::Renderer::LightCom>()->color = glm::vec3(1, 1, 1);
            // death_screen.getComponent<Flux::Renderer::LightCom>()->color = glm::vec3(1, 1, 1);
            ctx.getNamedEntity("SplashScreenLight").getComponent<Flux::Renderer::LightCom>()->color = glm::vec3(0, 0, 0);
            Flux::Transform::setVisible(splash_screen, false);
            Flux::Transform::setVisible(compass, true);
            state = Playing;
        }
    }

    ctx.runSystems(delta);
    hud_ctx.runSystems(delta);
}

void end()
{
    destroyLevel();
    destroyTerrain();
    ctx.destroyAllEntities();
    hud_ctx.destroyAllEntities();
    // Flux::Resources::destroyResources();
}