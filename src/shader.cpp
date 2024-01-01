#include "include/shader.h"
#include "include/defines.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/mat4x4.hpp>

#include "include/arena.h"
#include "include/utils.h"

u32 create_shader(const char* vertex_file, const char* frag_file)
{
    Arena arena;
    init_arena(&arena, &pool);

    char info_log[512];
    i32 status;

    char* vertex_code = read_file(vertex_file, NULL, &arena);
    if (!vertex_code) {
        exit(1);
    }
    u32 vertex_prog = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_prog, 1, &vertex_code, NULL);
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
    glShaderSource(frag_prog, 1, &frag_code, NULL);
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

MaterialShader load_shader(const char* vert, const char* frag)
{
    MaterialShader shader;
    shader.id = create_shader(vert, frag);
    shader.u_proj_view = glGetUniformLocation(shader.id, "proj_view");
    shader.u_model = glGetUniformLocation(shader.id, "model");
    shader.u_mat_color = glGetUniformLocation(shader.id, "mat_color");
    shader.u_mat_diffuse = glGetUniofrmLocation(shader.id, "mat_diffuse");
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
