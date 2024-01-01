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

struct Material
{
    glm::vec4 color;
    u32 texture;
};

struct Primitive 
{
    u32 vao;
    u32 index_count;
    u32 index_type;
    u32 material;
};

struct Mesh
{
    Primitive* primitives;
    u32 primitive_count;
};

const u32 width = 1280;
const u32 height = 720;

GLFWwindow *window;

Arena asset_arena;

Mesh* meshes;
Material* materials;

void resize_callback(GLFWwindow *window, i32 width, i32 height) 
{
}

void mouse_callback(GLFWwindow* window, double pos_x, double pos_y) 
{
}

void load_node(Scene* scene, fastgltf::Asset* asset, u32 node_index)
{
    printf("Loading node\n");
    fastgltf::Node* node = asset->nodes.data() + node_index;

    if (node->meshIndex.has_value()) {
        u32 mesh_index = node->meshIndex.value();
        fastgltf::Mesh* mesh = asset->meshes.data() + mesh_index;

        for (u32 i = 0; i < mesh->primitives.size(); ++i) {
            fastgltf::Primitive* prim = mesh->primitives.data() + i;

            // TODO: Fix this
            Object object;
            object.render.mesh = 0;
            object.transform.pos = glm::vec3(0.0f, 0.0f, 0.0f);
            object.transform.rot = glm::vec3(0.0f, 0.0f, 0.0f);
            object.transform.scale = glm::vec3(1.0f, 1.0f, 1.0f);

            push_object(scene, object);
        }

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
                                                    fastgltf::Options::LoadExternalImages);
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
    printf("Loading Scene: %u\n", scene_to_load);

    // load materials
    u32 material_count = asset->materials.size();
    materials = (Material*) push_size(&asset_arena, sizeof(Material) * material_count);
    for (u32 i = 0; i < material_count; ++i) {
        fastgltf::Material* material = asset->materials.data() + i;
        materials[i].color = glm::vec4(material->pbrData.baseColorFactor[0],
                                       material->pbrData.baseColorFactor[1],
                                       material->pbrData.baseColorFactor[2],
                                       material->pbrData.baseColorFactor[3]);
    }

    // textures
    u32 texture_count = asset->textures.size();
    u32* textures = (u32*) push_size(&arena, sizeof(u32) * texture_count);
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
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        i32 format = GL_RGBA;
        if (nu_channels == 3) {
            format = GL_RGB;
        }
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        stbi_image_free(data);

        materials[0].texture = textures[i];
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
        if (!view->target.has_value()) {
            continue;
        }
        u32 target = (u32) view->target.value();

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

            if (!index_view->target.has_value()) {
                continue;
            }

            glBindBuffer((u32) index_view->target.value(), gpu_buffers[index_view_index]);
            
            meshes[i].primitives[j].index_type = 
                (u32) fastgltf::getGLComponentType(index_accessor->componentType);
            meshes[i].primitives[j].index_count = index_accessor->count;
            meshes[i].primitives[j].vao = vao;

            meshes[i].primitives[j].material = prim->materialIndex.has_value()? 
                prim->materialIndex.value() : 0;

            for (u32 k = 0; k < attrib_count; ++k) {
                u32 accessor_index = prim->attributes[k].second;
                fastgltf::Accessor* accessor = asset->accessors.data() + accessor_index;

                u32 byte_offset = accessor->byteOffset;
                u8 num_components = getNumComponents(accessor->type);

                if (!accessor->bufferViewIndex.has_value()) {
                    continue;
                }
                u32 buffer_view_index = accessor->bufferViewIndex.value();
                fastgltf::BufferView* buffer_view = asset->bufferViews.data() + buffer_view_index;

                if (!buffer_view->target.has_value()) {
                    continue;
                }
                u32 stride = 0;
                std::optional byte_stride_opt = buffer_view->byteStride;
                if (byte_stride_opt.has_value()) {
                    stride = buffer_view->byteStride.value();
                }

                glBindBuffer((GLenum) buffer_view->target.value(), gpu_buffers[buffer_view_index]);
                glEnableVertexAttribArray(k);
                glVertexAttribPointer(k, num_components, 
                                      GL_FLOAT, 
                                      GL_FALSE, 
                                      stride,
                                      (void*) accessor->byteOffset);
            }
        }

    }

    for (u32 i = 0; i < scn->nodeIndices.size(); ++i) {
        u32 node_index = scn->nodeIndices[i];
        load_node(scene, asset, node_index);
    }

    dispose(&arena);
}

void init_window() 
{
    glfwInit();

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwSwapInterval(1);

    window = glfwCreateWindow(width, height, "YAGE", NULL, NULL);
    glfwSetFramebufferSizeCallback(window, resize_callback);

    // glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
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

    Scene scene;
    init_scene(&scene);

    MaterialShader shader = load_shader("shader/shader.vert", 
                                        "shader/shader.frag", 
                                        SHADER_DIFFUSE_TEXTURE);

    const char* scene_file;
    if (argc > 1) {
        scene_file = argv[1];
    } else {
        scene_file = "../assets/2.0/BoxTextured/glTF/BoxTextured.gltf";
    }
    printf("Loading scene: %s\n", scene_file);
    load_scene(&scene, scene_file);


    glm::mat4 projection = glm::perspective(glm::radians(55.0f), 
                                            (float) width / (float) height, 
                                            1.0f, 1000.0f);
    glm::mat4 view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), 
                                 glm::vec3(0.0f, 0.0f, 0.0f),
                                 glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 proj_view = projection * view;

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glClearColor(0, 0, 0, 1);

    glUseProgram(shader.id);
    set_mat4(shader.u_proj_view, &proj_view);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, materials[0].texture);
    set_vec4(shader.u_mat_color, &materials[0].color);

    while (!glfwWindowShouldClose(window)) {
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(window, true);
        }

        float time = glfwGetTime();

        scene_update(&scene);

        glm::mat4 model = glm::rotate(glm::mat4(1.0f), 
                                      glm::radians(90.0f * time), 
                                      glm::vec3(0.0f, 1.0f, 0.0f));

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        set_mat4(shader.u_model, &model);

        for (u32 i = 0; i < meshes[0].primitive_count; ++i) {
            Primitive* prim = meshes[0].primitives + i;

            glBindVertexArray(prim->vao);
            glDrawElements(GL_TRIANGLES, prim->index_count, prim->index_type, 0);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
}
