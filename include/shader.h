#pragma once

#include "include/defines.h"
#include <glm/mat4x4.hpp>

#define MATERIAL_DIFFUSE_TEXTURE 1 << 0
#define MATERIAL_NORMAL_TEXTURE 1 << 1
#define MATERIAL_ROUGHNESS_TEXTURE 1 << 2

struct MaterialShader
{
    u32 id;
    u32 u_proj_view;
    u32 u_model;
    u32 u_mat_color;

    u32 u_mat_diffuse;
    u32 u_mat_normal;
};

u32 create_shader(const char* vertex_file, const char* frag_file, u32 flags);
MaterialShader load_shader(const char* vert, const char* frag, u32 flags);
void set_mat4(u32 uniform, glm::mat4* mat);
void set_vec2(u32 uniform, glm::vec2* vec);
void set_vec4(u32 uniform, glm::vec4* vec);
void set_texture(u32 uniform, i32 slot);
