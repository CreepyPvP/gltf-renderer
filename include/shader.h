#pragma once

#include "include/defines.h"
#include <glm/mat4x4.hpp>

struct MaterialShader
{
    u32 id;
    u32 u_proj_view;
    u32 u_model;
    u32 u_mat_color;
};

u32 create_shader(const char* vertex_file, const char* frag_file);
MaterialShader load_shader(const char* vert, const char* frag);
void set_mat4(u32 uniform, glm::mat4* mat);
void set_vec2(u32 uniform, glm::vec2* vec);
void set_vec4(u32 uniform, glm::vec4* vec);
