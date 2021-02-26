#include "Flux/ECS.hh"
#include "Flux/Input.hh"
#include "Flux/OpenGL/GLRenderer.hh"
#include "Flux/Renderer.hh"
#include <iostream>
#define GLM_FORCE_CTOR_INIT
#include "FluxArc/FluxArc.hh"
#include "Flux/Flux.hh"

#include "Maze.hh"

Flux::ECSCtx ctx;
Flux::EntityRef camera;

bool left_pressed = false;
bool right_pressed = false;

bool forwards_pressed = false;
bool backwards_pressed = false;

enum RotateType
{
    No, Left, Right, Forwards, Backwards
};

enum Direction
{
    North, South, East, West
};

Direction direction = North;

float rotate_left = 0;
RotateType rotate_type = No;

const int speed = 8;

glm::vec2 position = glm::vec2(4, 4);

void init(int argc, char **argv)
{
    ctx = Flux::ECSCtx();

    auto loader = Flux::Resources::deserialize("Assets/Enclosure.farc");
    auto ens = loader->addToECS(&ctx);

    camera = ctx.createEntity();
    Flux::Transform::giveTransform(camera);
    Flux::Transform::setCamera(camera);
    auto o = glm::vec3(position.x * 2 + 1, 0, position.y * 2 + 1);
    Flux::Transform::translate(camera, o);
    Flux::Renderer::addSpotLight(camera, glm::radians(40.0), 9999, glm::vec3(1, 1, 1));

    // Add janky "global illumination"
    auto gi = ctx.createEntity();
    Flux::Transform::giveTransform(gi);
    Flux::Renderer::addPointLight(gi, 9999, glm::vec3(0.002, 0.002, 0.002));
    Flux::Transform::setParent(gi, camera);

    Flux::GLRenderer::addGLRenderer(&ctx);
    LOG_INFO("Added GL Renderer");
    Flux::Transform::addTransformSystems(&ctx);
    LOG_INFO("Added transform systems");

    generateTerrain(test_level, 8, &ctx);
}

void loop(float delta)
{
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
        }
        if (!Flux::Input::isKeyPressed(FLUX_KEY_A) && left_pressed)
        {
            left_pressed = false;
        }

        if (Flux::Input::isKeyPressed(FLUX_KEY_D) && !right_pressed)
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
        }
        if (!Flux::Input::isKeyPressed(FLUX_KEY_D) && right_pressed)
        {
            right_pressed = false;
        }

        if (Flux::Input::isKeyPressed(FLUX_KEY_W) && !forwards_pressed)
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

            std::cout << "X: " << position.x << " Y: " << position.y << "\n";

            if (position.x < 8 && position.y < 8)
            {
                if (test_level[(int)position.y * 8 + (int)position.x] == 1)
                {
                    // Undo the move
                    rotate_left = 0;
                    position = old_position;
                }
            }
        }
        if (!Flux::Input::isKeyPressed(FLUX_KEY_W) && forwards_pressed)
        {
            forwards_pressed = false;
        }

        if (Flux::Input::isKeyPressed(FLUX_KEY_S) && !backwards_pressed)
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

            if (test_level[(int)position.y * 8 + (int)position.x] == 1)
            {
                // Undo the move
                rotate_left = 0;
                position = old_position;
            }
        }
        if (!Flux::Input::isKeyPressed(FLUX_KEY_S) && backwards_pressed)
        {
            backwards_pressed = false;
        }
    }

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
                }
                else
                {
                    Flux::Transform::rotate(camera, glm::vec3(0, 1, 0), 6.265732 * delta);
                }
                
                break;
            case Right: 
                rotate_left -= 6.265732 * delta;
                if (rotate_left < 0)
                {
                    Flux::Transform::rotate(camera, glm::vec3(0, -1, 0), rotate_left + 6.265732 * delta);
                }
                else
                {
                    Flux::Transform::rotate(camera, glm::vec3(0, -1, 0), 6.265732 * delta);
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

    ctx.runSystems(delta);
}

void end()
{
    ctx.destroyAllEntities();
}