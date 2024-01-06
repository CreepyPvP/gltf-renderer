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

vec2 jitter_offsets[] = {
    vec2(1.0, 1.0),
    vec2(-1.0, 1.0),
    vec2(0, 0),
    vec2(1.0, -1.0),
    vec2(-1.0, -1.0),
};



void main() {
    out_norm = normalize((model * vec4(aNorm, 1)).xyz);
    vec4 world_pos = model * vec4(aPos, 1);
    out_pos = world_pos.xyz;

    mat4 jitter_mat = mat4(1.0);
    jitter_mat[3][0] += jitter_offsets[jitter_index].x * jitter_strength / screen_dimensions.x;
    jitter_mat[3][1] += jitter_offsets[jitter_index].y * jitter_strength / screen_dimensions.y;
    gl_Position = jitter_mat * proj_view * world_pos;

    prev_screen_pos = (jitter_mat * prev_proj_view * prev_model * vec4(aPos, 1)).xyw;
    screen_pos = gl_Position.xyw;

#ifdef ATTRIB_UV
    out_uv = aUv;
#endif

#ifdef ATTRIB_TANGENT
    out_tangent = aTangent.w * normalize((model * aTangent).rgb);
#endif
}
