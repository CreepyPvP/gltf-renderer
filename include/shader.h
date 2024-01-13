#pragma once

#include "include/defines.h"
#include <glm/mat4x4.hpp>

#define MATERIAL_BASE_TEXTURE 1 << 0
#define MATERIAL_NORMAL_TEXTURE 1 << 1
#define MATERIAL_ROUGHNESS_TEXTURE 1 << 2

#define ATTRIB_POSITION 1 << 0
#define ATTRIB_NORMAL 1 << 1
#define ATTRIB_UV 1 << 2
#define ATTRIB_TANGENT 1 << 3

struct MaterialShader
{
    u32 id;
    u32 u_proj_view;
    u32 u_model;

    u32 u_mat_color;
    u32 u_mat_pbr;

    u32 u_prev_proj_view;
    u32 u_prev_model;

    u32 u_mat_diffuse;
    u32 u_mat_normal;
    u32 u_mat_roughness;

    u32 u_camera_pos;

    u32 u_jitter_index;
    u32 u_screen_dimensions;
};

struct PostProcessShader
{
    u32 id;
    u32 u_screen_dimensions;
};

struct TaaShader
{
    u32 id;
    u32 u_current_frame;
    u32 u_current_depth;
    u32 u_velocity;
    u32 u_prev_frame;
    u32 u_screen_dimensions;
    u32 u_jitter_index;
};

u32 create_shader(const char* vertex_file, const char* frag_file, u32 flags);
PostProcessShader load_post_shader(const char* vert, const char* frag);
TaaShader load_taa_shader(const char* vert, const char* frag);
MaterialShader load_shader(const char* vert, const char* frag, u32 flags);

void set_mat4(u32 uniform, glm::mat4* mat);
void set_vec2(u32 uniform, glm::vec2* vec);
void set_vec4(u32 uniform, glm::vec4* vec);
void set_vec3(u32 uniform, glm::vec3* vec);
void set_texture(u32 uniform, i32 slot);
