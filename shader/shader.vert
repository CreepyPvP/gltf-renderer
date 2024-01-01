#version 440

layout(location = 0) in vec3 aNorm;
layout(location = 1) in vec3 aPos;
layout(location = 2) in vec2 aUv;

out vec3 out_norm;
out vec2 out_uv;

uniform mat4 proj_view;
// uniform mat4 model;

void main() {
    out_norm = normalize(aNorm);
    out_uv = aUv;
    gl_Position = proj_view * vec4(aPos, 1);
}
