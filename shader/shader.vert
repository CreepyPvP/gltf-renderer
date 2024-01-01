layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNorm;

out vec3 out_norm;

#ifdef USE_DIFFUSE_TEXTURE
layout(location = 2) in vec2 aUv;
out vec2 out_uv;
#endif

uniform mat4 proj_view;
uniform mat4 model;


void main() {
    out_norm = normalize((model * vec4(aNorm, 1)).xyz);
    gl_Position = proj_view * model * vec4(aPos, 1);

#ifdef USE_DIFFUSE_TEXTURE
    out_uv = aUv;
#endif
}
