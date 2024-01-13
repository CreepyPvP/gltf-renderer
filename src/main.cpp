#include "include/defines.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#define STB_IMAGE_IMPLEMENTATION
#include "include/stb_image.h"

#include <math.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <vector>
#include <variant>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <fastgltf/parser.hpp>
#include <fastgltf/types.hpp>

#include "include/shader.h"
#include "include/arena.h"
#include "include/scene.h"
#include "include/camera.h"

struct Material
{
    glm::vec4 color;
    u16 flags;

    u32 diffuse_texture;
    u32 normal_texture;
    u32 roughness_texture;

    float roughness_factor;
    float metallic_factor;
};

struct Primitive 
{
    u32 vao;
    u32 index_count;
    u32 index_type;
    u32 index_offset;
    u32 material;

    u16 attrib_flags;
};

struct Mesh
{
    Primitive* primitives;
    u32 primitive_count;
};

u32 width = 1280;
u32 height = 720;

GLFWwindow *window;
float last_mouse_pos_x;
float last_mouse_pos_y;

Arena asset_arena;

Mesh* meshes;
Material* materials;
u32* textures;

u32 square_vao;

u32 fbos[3];
u32 fbo_textures[4];
u32 depth_buffer;


void setup_framebuffer(bool);

void resize_callback(GLFWwindow *window, i32 new_width, i32 new_height) 
{
    width = new_width;
    height = new_height;
    glViewport(0, 0, width, height);
    setup_framebuffer(true);
}

void mouse_callback(GLFWwindow* window, double pos_x, double pos_y) 
{
    float x_offset = pos_x - last_mouse_pos_x;
    float y_offset = pos_y - last_mouse_pos_y;
    last_mouse_pos_x = pos_x;
    last_mouse_pos_y = pos_y;
    const float sensitivity = 0.1f;
    x_offset *= sensitivity;
    y_offset *= sensitivity;
    camera.process_mouse_input(x_offset, y_offset);
}

void setup_square_vao() {
    float vertex_data[] = {
        -1,  1,
         1,  1,
        -1, -1,

         1,  1,
         1, -1,
        -1, -1
    };

    glGenVertexArrays(1, &square_vao);
    glBindVertexArray(square_vao);
    u32 vertex_buffer;
    glGenBuffers(1, &vertex_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 2 * 6, vertex_data, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, 0);
}

void load_node(Scene* scene, fastgltf::Asset* asset, u32 node_index)
{
    fastgltf::Node* node = asset->nodes.data() + node_index;

    if (node->meshIndex.has_value()) {
        u32 mesh_index = node->meshIndex.value();

        fastgltf::Node::TRS* transform = std::get_if<fastgltf::Node::TRS>(&node->transform);

        // TODO: Fix this
        Object object;
        object.render.mesh = mesh_index;
        object.transform.pos = glm::vec3(transform->translation[0], 
                                         transform->translation[1], 
                                         transform->translation[2]);
        object.transform.rot = glm::vec3(transform->rotation[0],
                                         transform->rotation[1],
                                         transform->rotation[2]);
        object.transform.scale = glm::vec3(transform->scale[0],
                                           transform->scale[1],
                                           transform->scale[2]);

        push_object(scene, object);
    }

    for (u32 i = 0; i < node->children.size(); ++i) {
        load_node(scene, asset, node->children[i]);
    }
}

void load_scene(Scene* scene, const char* file)
{
    std::filesystem::path path = file;

    fastgltf::Parser parser;
    fastgltf::GltfDataBuffer data;
    data.loadFromFile(path);

    fastgltf::Expected<fastgltf::Asset> result = parser.loadGLTF(&data, 
                                                    path.parent_path(), 
                                                    fastgltf::Options::LoadExternalBuffers |
                                                    fastgltf::Options::LoadExternalImages |
                                                    fastgltf::Options::DecomposeNodeMatrices);
    auto error = result.error();
    if (error !=  fastgltf::Error::None) {
        printf("Failed to load %s\n", file);
        exit(1);
    }

    fastgltf::Asset* asset = &result.get();
    if (asset->scenes.size() == 0) {
        printf("No scenes to load\n");
        return;
    }

    Arena arena;
    init_arena(&arena, &pool);

    u32 scene_to_load = asset->defaultScene.has_value()? asset->defaultScene.value() : 0;
    fastgltf::Scene* scn = asset->scenes.data() + scene_to_load;
    printf("Loading scene: %u\n", scene_to_load);

    // load materials
    u32 material_count = asset->materials.size();
    materials = (Material*) push_size(&asset_arena, sizeof(Material) * material_count);
    for (u32 i = 0; i < material_count; ++i) {
        fastgltf::Material* material = asset->materials.data() + i;
        materials[i].flags = 0;
        materials[i].color = glm::vec4(material->pbrData.baseColorFactor[0],
                                       material->pbrData.baseColorFactor[1],
                                       material->pbrData.baseColorFactor[2],
                                       material->pbrData.baseColorFactor[3]);
        materials[i].roughness_factor = material->pbrData.roughnessFactor;
        materials[i].metallic_factor = material->pbrData.metallicFactor;
        if (material->pbrData.baseColorTexture.has_value()) {
            materials[i].diffuse_texture = material->pbrData.baseColorTexture.value().textureIndex;
            materials[i].flags |= MATERIAL_BASE_TEXTURE;
        }
        if (material->pbrData.metallicRoughnessTexture.has_value()) {
            materials[i].roughness_texture = 
                material->pbrData.metallicRoughnessTexture.value().textureIndex;
            materials[i].flags |= MATERIAL_ROUGHNESS_TEXTURE;
        }
        if (material->normalTexture.has_value()) {
            materials[i].normal_texture = material->normalTexture.value().textureIndex;
            materials[i].flags |= MATERIAL_NORMAL_TEXTURE;
        }
    }

    // textures
    u32 texture_count = asset->textures.size();
    textures = (u32*) push_size(&asset_arena, sizeof(u32) * texture_count);
    glGenTextures(texture_count, textures);
    for (u32 i = 0; i < texture_count; ++i) {
        fastgltf::Texture* texture = asset->textures.data() + i;

        if (!texture->imageIndex.has_value()) {
            printf("Texture has no image index\n");
            continue;
        }
        u32 image_index = texture->imageIndex.value();

        fastgltf::Image* image = asset->images.data() + image_index;
        fastgltf::sources::Vector* ptr = std::get_if<fastgltf::sources::Vector>(&image->data);
        if (!ptr) {
            printf("Image uses not implemented data source\n");
            continue;
        }
        i32 width;
        i32 height;
        i32 nu_channels;
        u8* data = stbi_load_from_memory(ptr->bytes.data(), ptr->bytes.size(), 
                                         &width, &height, &nu_channels, 0);

        glBindTexture(GL_TEXTURE_2D, textures[i]);
        // TODO: read sampler info here
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        i32 format;
        if (nu_channels == 4) {
            format = GL_RGBA;
        } else if (nu_channels == 3) {
            format = GL_RGB;
        } else {
            printf("Unknown texture format\n");
            exit(1);
        }

        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        stbi_image_free(data);
    }

    // retrieve buffer pointer
    u32 buffer_count = asset->buffers.size();
    u8** buffer_ptr = (u8**) push_size(&arena, sizeof(u8*) * buffer_count);
    for (u32 i = 0; i < buffer_count; ++i) {
        fastgltf::Buffer* buffer = asset->buffers.data() + i;

        fastgltf::sources::Vector* ptr = std::get_if<fastgltf::sources::Vector>(&buffer->data);
        if (ptr) {
            buffer_ptr[i] = ptr->bytes.data();
            continue;
        }
        
        printf("Buffer uses not implemented data source\n");
        exit(1);
    }

    // load gpu buffers
    u32 buffer_view_count = asset->bufferViews.size();
    u32* gpu_buffers = (u32*) push_size(&arena, sizeof(u32) * buffer_view_count);
    glGenBuffers(buffer_view_count, gpu_buffers);
    for (u32 i = 0; i < buffer_view_count; ++i) {
        fastgltf::BufferView* view = asset->bufferViews.data() + i;

        u32 target;
        if (!view->target.has_value()) {
            printf("No target defined for buffer view, guessing.\n");
            
            // TODO: implement usage guessing
            if (i == 3) {
                target = GL_ELEMENT_ARRAY_BUFFER;
            }
            else {
                target = GL_ARRAY_BUFFER;
            }
        } else {
            target = (u32) view->target.value();
        }

        glBindBuffer(target, gpu_buffers[i]);
        glBufferData(target, view->byteLength, 
                     buffer_ptr[view->bufferIndex] + view->byteOffset, 
                     GL_STATIC_DRAW);
    }

    u32 mesh_count = asset->meshes.size();
    meshes = (Mesh*) push_size(&asset_arena, sizeof(Mesh) * mesh_count);
    for (u32 i = 0; i < mesh_count; ++i) {
        fastgltf::Mesh* mesh = asset->meshes.data() + i;

        u32 primitive_count = mesh->primitives.size();
        meshes[i].primitive_count = primitive_count;
        meshes[i].primitives = (Primitive*) push_size(&asset_arena, 
                                                      sizeof(Primitive) * primitive_count);
        for (u32 j = 0; j < mesh->primitives.size(); ++j) {
            fastgltf::Primitive* prim = mesh->primitives.data() + j;

            u32 attrib_count = prim->attributes.size();

            if (!prim->indicesAccessor.has_value()) {
                printf("No index accessor for primitive\n");
                continue;
            }
            u32 index_acc_id = prim->indicesAccessor.value();

            u32 vao;
            glGenVertexArrays(1, &vao);
            glBindVertexArray(vao);

            fastgltf::Accessor* index_accessor = asset->accessors.data() + index_acc_id;
            if (!index_accessor->bufferViewIndex.has_value()) {
                continue;
            }
            u32 index_view_index = index_accessor->bufferViewIndex.value();
            fastgltf::BufferView* index_view =  asset->bufferViews.data() + index_view_index;

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gpu_buffers[index_view_index]);
            
            Primitive* prim_ptr = meshes[i].primitives + j;
            prim_ptr->index_type = (u32) fastgltf::getGLComponentType(index_accessor->componentType);
            prim_ptr->material = prim->materialIndex.has_value()? prim->materialIndex.value() : 0;
            prim_ptr->index_count = index_accessor->count;
            prim_ptr->index_offset = index_accessor->byteOffset;
            prim_ptr->vao = vao;
            prim_ptr->attrib_flags = 0;

            for (u32 k = 0; k < attrib_count; ++k) {
                const char* attr_type = prim->attributes[k].first.c_str();

                u32 attr_id;
                if (strcmp(attr_type, "POSITION") == 0) {
                    attr_id = 0;
                } else if (strcmp(attr_type, "NORMAL") == 0) {
                    attr_id = 1;
                } else if (strcmp(attr_type, "TEXCOORD_0") == 0) {
                    attr_id = 2;
                } else if (strcmp(attr_type, "TANGENT") == 0) {
                    attr_id = 3;
                } else {
                    printf("Unknown attr type: %s. Skipping\n", attr_type);
                    continue;
                }
                prim_ptr->attrib_flags |= (1 << attr_id);

                u32 accessor_index = prim->attributes[k].second;
                fastgltf::Accessor* accessor = asset->accessors.data() + accessor_index;

                u32 byte_offset = accessor->byteOffset;
                u8 num_components = getNumComponents(accessor->type);

                if (!accessor->bufferViewIndex.has_value()) {
                    printf("No buffer view specified for vertex attr\n");
                    continue;
                }
                u32 buffer_view_index = accessor->bufferViewIndex.value();
                fastgltf::BufferView* buffer_view = asset->bufferViews.data() + buffer_view_index;

                u32 stride = 0;
                if (buffer_view->byteStride.has_value()) {
                    stride = buffer_view->byteStride.value();
                }

                glBindBuffer(GL_ARRAY_BUFFER, gpu_buffers[buffer_view_index]);
                glEnableVertexAttribArray(attr_id);
                glVertexAttribPointer(attr_id, num_components, 
                                      GL_FLOAT, 
                                      GL_FALSE, 
                                      stride,
                                      (void*) byte_offset);
            }

            if (materials[prim_ptr->material].flags & MATERIAL_NORMAL_TEXTURE) {
                u32 normal_uv_mask = ATTRIB_NORMAL | ATTRIB_UV;
                if (prim_ptr->attrib_flags & normal_uv_mask != normal_uv_mask) {
                    printf("Normal and UVs required for normal mapping\n");
                }
            
                if (!(prim_ptr->attrib_flags & ATTRIB_TANGENT)) {
                    // TODO: calculate tangents here
                    printf("No tangents provided for normal mapping\n");
                }
            }
        }

    }

    for (u32 i = 0; i < scn->nodeIndices.size(); ++i) {
        u32 node_index = scn->nodeIndices[i];
        load_node(scene, asset, node_index);
    }

    dispose(&arena);
}

void setup_framebuffer(bool do_cleanup) 
{
    if (do_cleanup) {
        glDeleteFramebuffers(3, fbos);
        glDeleteTextures(4, fbo_textures);
        glDeleteRenderbuffers(1, &depth_buffer);
    }

    glGenFramebuffers(3, fbos);
    glGenTextures(4, fbo_textures);
    glGenRenderbuffers(1, &depth_buffer);

    // History buffer 0
    glBindFramebuffer(GL_FRAMEBUFFER, fbos[0]);
    glBindTexture(GL_TEXTURE_2D, fbo_textures[0]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fbo_textures[0], 0);
    u32 hbuffers[] = { GL_COLOR_ATTACHMENT0 };
    glDrawBuffers(1, hbuffers);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // History buffer 1
    glBindFramebuffer(GL_FRAMEBUFFER, fbos[1]);
    glBindTexture(GL_TEXTURE_2D, fbo_textures[1]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fbo_textures[1], 0);
    glDrawBuffers(1, hbuffers);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Main pass buffer
    glBindFramebuffer(GL_FRAMEBUFFER, fbos[2]);
    glBindTexture(GL_TEXTURE_2D, fbo_textures[2]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fbo_textures[2], 0);
    glBindTexture(GL_TEXTURE_2D, fbo_textures[3]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RG32F, width, height, 0, GL_RG, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, fbo_textures[3], 0);
    glBindRenderbuffer(GL_RENDERBUFFER, depth_buffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth_buffer);
    u32 buffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
    glDrawBuffers(2, buffers);

}

void init_window() 
{
    glfwInit();

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwSwapInterval(1);

    window = glfwCreateWindow(width, height, "YAGE", NULL, NULL);
    glfwSetFramebufferSizeCallback(window, resize_callback);

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwMakeContextCurrent(window);
}

i32 main(i32 argc, char** argv) 
{
    init_window();
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        printf("failed to load required extensions\n");
        return 1;
    }

    init_pool(&pool);
    init_arena(&asset_arena, &pool);

    u32 current_frame = 0;

    setup_framebuffer(false);
    setup_square_vao();

    camera.init();
    Scene scene;
    init_scene(&scene);

    const char* scene_file;
    if (argc > 1) {
        scene_file = argv[1];
    } else {
        scene_file = "../assets/2.0/Sponza/glTF/Sponza.gltf";
    }
    printf("Loading file: %s\n", scene_file);
    load_scene(&scene, scene_file);

    u16 attrib_flags = ATTRIB_UV | ATTRIB_NORMAL | ATTRIB_TANGENT;
    u16 mat_flags = MATERIAL_BASE_TEXTURE | MATERIAL_NORMAL_TEXTURE | MATERIAL_ROUGHNESS_TEXTURE;

    u32 shader_features = (u32) mat_flags | (u32) attrib_flags << 16;
    MaterialShader shader = load_shader("shader/shader.vert", 
                                        "shader/shader.frag", 
                                        shader_features);
    PostProcessShader post_shader = load_post_shader("shader/postprocess.vert",
                                                     "shader/postprocess.frag");
    TaaShader taa_shader = load_taa_shader("shader/taa.vert",
                                           "shader/taa.frag");

    glm::mat4 projection = glm::perspective(glm::radians(55.0f), 
                                            (float) width / (float) height, 
                                            0.01f, 1000.0f);

    glFrontFace(GL_CCW);
    glClearColor(0, 0, 0, 1);

    float time_last_frame = glfwGetTime();
    float delta = 0;
    u32 jitter_index = 0;
    glm::vec2 dimensions = glm::vec2(width, height);
    glm::mat4 prev_proj_view = glm::mat4(1.0f);
    glm::mat4 prev_model = glm::mat4(1.0f);

    while (!glfwWindowShouldClose(window)) {
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(window, true);
        }

        float current_time = glfwGetTime();
        delta = current_time - time_last_frame;
        time_last_frame = current_time;
        u32 next_frame = (current_frame + 1) % 2;

        glBindFramebuffer(GL_FRAMEBUFFER, fbos[2]);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glUseProgram(shader.id);

        set_texture(shader.u_jitter_index, jitter_index);
        set_vec2(shader.u_screen_dimensions, &dimensions);

        scene_update(&scene);

        camera.process_key_input(window, delta);
        glm::mat4 view = glm::lookAt(camera.pos,
                                     camera.pos + camera.front,
                                     glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 proj_view = projection * view;
        set_mat4(shader.u_proj_view, &proj_view);
        set_mat4(shader.u_prev_proj_view, &prev_proj_view);
        prev_proj_view = proj_view;

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        set_vec3(shader.u_camera_pos, &camera.pos);

        for (u32 i = 0; i < scene.object_count; ++i) {
            Object* obj = scene.objects + i;
            u32 mesh_id = obj->render.mesh;

            glm::mat4 res;
            res = glm::mat4(1.0);
            res = glm::translate(res, obj->transform.pos);
            res = glm::rotate(res, glm::radians(obj->transform.rot.x), glm::vec3(1.0f, 0.0f, 0.0f));
            res = glm::rotate(res, glm::radians(obj->transform.rot.y), glm::vec3(0.0f, 1.0f, 0.0f));
            res = glm::rotate(res, glm::radians(obj->transform.rot.z), glm::vec3(0.0f, 0.0f, 1.0f));
            res = glm::scale(res, obj->transform.scale);
            set_mat4(shader.u_model, &res);
            set_mat4(shader.u_prev_model, &res);

            for (u32 j = 0; j < meshes[mesh_id].primitive_count; ++j) {
                Primitive* prim = meshes[mesh_id].primitives + j;
                Material* mat = materials + prim->material;

                glm::vec2 pbr_data = glm::vec2(mat->metallic_factor, mat->roughness_factor);

                set_vec4(shader.u_mat_color, &mat->color);
                set_vec2(shader.u_mat_pbr, &pbr_data);
                if (mat->flags & MATERIAL_BASE_TEXTURE) {
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, textures[mat->diffuse_texture]);
                    set_texture(shader.u_mat_diffuse, 0);
                }
                if (mat->flags & MATERIAL_NORMAL_TEXTURE) {
                    glActiveTexture(GL_TEXTURE1);
                    glBindTexture(GL_TEXTURE_2D, textures[mat->normal_texture]);
                    set_texture(shader.u_mat_normal, 1);
                }
                if (mat->flags & MATERIAL_ROUGHNESS_TEXTURE) {
                    glActiveTexture(GL_TEXTURE2);
                    glBindTexture(GL_TEXTURE_2D, textures[mat->roughness_texture]);
                    set_texture(shader.u_mat_roughness, 2);
                }

                glBindVertexArray(prim->vao);
                glDrawElements(GL_TRIANGLES, 
                               prim->index_count, 
                               prim->index_type, 
                               (void*) prim->index_offset);
            }

        }

        glDisable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);
        glBindVertexArray(square_vao);

        glBindFramebuffer(GL_FRAMEBUFFER, fbos[current_frame]);
        // glClear(GL_COLOR_BUFFER_BIT);
        glUseProgram(taa_shader.id);
        set_texture(taa_shader.u_jitter_index, jitter_index);
        set_vec2(taa_shader.u_screen_dimensions, &dimensions);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, fbo_textures[2]);
        set_texture(taa_shader.u_current_frame, 0);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, fbo_textures[3]);
        set_texture(taa_shader.u_velocity, 1);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, fbo_textures[next_frame]);
        set_texture(taa_shader.u_prev_frame, 2);
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, depth_buffer);
        set_texture(taa_shader.u_current_depth, 3);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        // glClear(GL_COLOR_BUFFER_BIT);
        glUseProgram(post_shader.id);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, fbo_textures[current_frame]);
        set_vec2(post_shader.u_screen_dimensions, &dimensions);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        current_frame = next_frame;
        jitter_index = (jitter_index + 1) % 4;

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
}
