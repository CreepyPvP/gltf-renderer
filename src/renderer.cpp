#include "include/renderer.h"

#include "include/defines.h"

u32 model(

u32 generateMesh(u8* vertex_data, u32 vertex_count, u32* indices, u32 index_count) {
    u32 vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    u32 buffers[2];
    glGenBuffers(2, buffers);
    unsigned int vertexBuffer = buffers[0];
    unsigned int indexBuffer = buffers[1];

    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * vertices.size(), &vertices[0], GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * indices.size(), &indices[0], GL_STATIC_DRAW);

    if (features && FEATURE_POS) {
        // position
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*) offsetof(Vertex, x));
    }
    if (features && FEATURE_NORMAL) {
        // normal
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*) offsetof(Vertex, normalX));
    }
    if (features && FEATURE_UV) {
        // uv
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*) offsetof(Vertex, uvX));
    }

    glBindVertexArray(0);

    return vao;
}

