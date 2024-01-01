#pragma once

#include "include/defines.h"

#include <glm/glm.hpp>

struct Renderable
{
    u32 mesh;
    u32 material;
};

struct Transform
{
    glm::vec3 pos;
    glm::vec3 rot;
    glm::vec3 scale;
};

struct Object
{
    Renderable render;
    Transform transform;
};

struct Scene
{
    Object* objects;
    u32 object_capacity;
    u32 object_count;
    i32 object_page;
};

void init_scene(Scene* scene);
void push_object(Scene* scene, Object object);
void scene_update(Scene* scene);
