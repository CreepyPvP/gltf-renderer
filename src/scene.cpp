#include "include/scene.h"
#include "include/defines.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "include/arena.h"

void init_scene(Scene* scene)
{
    scene->object_count = 0;
    scene->object_capacity = 8;
    scene->object_page = get_page(&pool, scene->object_capacity * sizeof(Object));

    if (scene->object_page < 0) {
        printf("Failed to allocate scene\n");
        exit(1);
    }

    scene->objects = (Object*) pool.pages[scene->object_page].memory;
}

void push_object(Scene* scene, Object object)
{
    if (scene->object_count >= scene->object_capacity) {
        scene->object_capacity = scene->object_capacity << 1;
        i32 new_page = get_page(&pool, scene->object_capacity * sizeof(Object));

        if (new_page < 0) {
            printf("Failed to allocate scene\n");
            exit(1);
        }

        Object* ptr = (Object*) pool.pages[scene->object_page].memory;
        memcpy(ptr, scene->objects, scene->object_count * sizeof(Renderable));
        scene->objects = ptr;
        free_page(&pool, scene->object_page);
        scene->object_page = new_page;
    }

    scene->objects[scene->object_count] = object;
    scene->object_count++;
}

void scene_update(Scene* scene)
{
}
