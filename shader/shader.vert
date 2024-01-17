layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNorm;

out vec3 out_norm;
out vec3 out_pos;
out vec3 prev_screen_pos;
out vec3 screen_pos;

#ifdef ATTRIB_UV
layout(location = 2) in vec2 aUv;
out vec2 out_uv;
#endif

#ifdef ATTRIB_TANGENT
layout(location = 3) in vec4 aTangent;
out vec3 out_tangent;
#endif

uniform mat4 proj_view;
uniform mat4 model;

uniform mat4 prev_proj_view;
uniform mat4 prev_model;

uniform int jitter_index;
uniform vec2 screen_dimensions;

float jitter_strength = 0.4;

layout (std140, binding = 2) uniform Halton
{
    vec2 halton[128];
};

void main() {
    out_norm = normalize((model * vec4(aNorm, 1)).xyz);
    vec4 world_pos = model * vec4(aPos, 1);
    out_pos = world_pos.xyz;

    // mat4 jitter_mat = mat4(1);
    // jitter_mat[2][0] = halton[jitter_index].x * jitter_strength / screen_dimensions.x * 2;
    // jitter_mat[2][1] = halton[jitter_index].y * jitter_strength / screen_dimensions.y * 2;
    // gl_Position = jitter_mat * proj_view * world_pos;
    gl_Position = proj_view * world_pos;
    gl_Position.rg -= halton[jitter_index] * jitter_strength / screen_dimensions * 2 * gl_Position.w;

    prev_screen_pos = (prev_proj_view * prev_model * vec4(aPos, 1)).xyw;
    screen_pos = (proj_view * world_pos).xyw;

#ifdef ATTRIB_UV
    out_uv = aUv;
#endif

#ifdef ATTRIB_TANGENT
    out_tangent = aTangent.w * normalize((model * aTangent).rgb);
#endif
}
