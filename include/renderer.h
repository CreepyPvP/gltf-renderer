#pragma once

#include "include/defines.h"
#include "include/scene.h"

void init_renderer();
void cleanup_renderer();

void start_frame();
void end_frame();

void draw_object(u32 mesh, u32 material, Transform transform);
