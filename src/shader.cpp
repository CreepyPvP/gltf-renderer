#include "include/shader.h"
#include "include/defines.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/mat4x4.hpp>

#include "include/arena.h"
#include "include/utils.h"

void append_str(char* dst, char* str) 
{
    char* ptr = dst;
    while (*ptr) {
        ptr++;
    }
    while (*str) {
        *ptr = *str;
        ptr++;
        str++;
    } 
}

u32 create_shader(const char* vertex_file, const char* frag_file, u32 flags)
{
    Arena arena;
    init_arena(&arena, &pool);

    char shader_header[1024] = {};
    append_str(shader_header, "#version 440\n");
    append_str(shader_header, "#define ATTRIB_UV\n");

    if (flags & MATERIAL_DIFFUSE_TEXTURE) {
        append_str(shader_header, "#define USE_DIFFUSE_TEXTURE\n");
    }
    if (flags & MATERIAL_NORMAL_TEXTURE) {
        append_str(shader_header, "#define USE_NORMAL_TEXTURE\n");
    }
    if (flags >> 16 & ATTRIB_NORMAL) {
        append_str(shader_header, "#define ATTRIB_NORMAL\n");
    }
    if (flags >> 16 & ATTRIB_UV) {
        append_str(shader_header, "#define ATTRIB_UV\n");
    }
    if (flags >> 16 & ATTRIB_TANGENT) {
        append_str(shader_header, "#define ATTRIB_TANGENT\n");
    }

    char* sources[2] = {shader_header};

    char info_log[512];
    i32 status;

    char* vertex_code = read_file(vertex_file, NULL, &arena);
    if (!vertex_code) {
        exit(1);
    }
    u32 vertex_prog = glCreateShader(GL_VERTEX_SHADER);
    sources[1] = vertex_code;
    glShaderSource(vertex_prog, 2, sources, NULL);
    glCompileShader(vertex_prog);
    glGetShaderiv(vertex_prog, GL_COMPILE_STATUS, &status);
    if (!status) {
        glGetShaderInfoLog(vertex_prog, 512, NULL, info_log);
        printf("Error compiling vertex shader: %s\n", info_log);
        exit(1);
    }

    char* frag_code = read_file(frag_file, NULL, &arena);
    if (!frag_code) {
        exit(1);
    }
    u32 frag_prog = glCreateShader(GL_FRAGMENT_SHADER);
    sources[1] = frag_code;
    glShaderSource(frag_prog, 2, sources, NULL);
    glCompileShader(frag_prog);
    glGetShaderiv(frag_prog, GL_COMPILE_STATUS, &status);
    if(!status) {
        glGetShaderInfoLog(frag_prog, 512, NULL, info_log);
        printf("Error compiling fragment shader: %s\n", info_log);
        exit(1);
    }

    u32 shader = glCreateProgram();
    glAttachShader(shader, vertex_prog);
    glAttachShader(shader, frag_prog);
    glLinkProgram(shader);
    glGetProgramiv(shader, GL_LINK_STATUS, &status);
    if(!status) {
        glGetProgramInfoLog(shader, 512, NULL, info_log);
        printf("Error linking shader: %s\n", info_log);
        exit(1);
    }

    glDeleteShader(vertex_prog);
    glDeleteShader(frag_prog);
    dispose(&arena);
    return shader;
}

MaterialShader load_shader(const char* vert, const char* frag, u32 flags)
{
    MaterialShader shader;
    shader.id = create_shader(vert, frag, flags);
    shader.u_proj_view = glGetUniformLocation(shader.id, "proj_view");
    shader.u_model = glGetUniformLocation(shader.id, "model");
    shader.u_mat_color = glGetUniformLocation(shader.id, "mat_color");
    shader.u_mat_diffuse = glGetUniformLocation(shader.id, "mat_diffuse");
    shader.u_mat_normal = glGetUniformLocation(shader.id, "mat_normal");
    return shader;
}

void set_mat4(u32 uniform, glm::mat4* mat) 
{
    glUniformMatrix4fv(uniform, 1, GL_FALSE, &(*mat)[0][0]);
}

void set_vec2(u32 uniform, glm::vec2* vec)
{
    glUniform2fv(uniform, 1, &(*vec)[0]);
}

void set_vec4(u32 uniform, glm::vec4* vec)
{
    glUniform4fv(uniform, 1, &(*vec)[0]);
}

void set_texture(u32 uniform, i32 slot)
{
    glUniform1i(uniform, slot);
}
