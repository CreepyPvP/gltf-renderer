layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNorm;

out vec3 out_norm;
out vec3 out_pos;
out vec3 prev_pos;

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


void main() {
    out_norm = normalize((model * vec4(aNorm, 1)).xyz);
    vec4 world_pos = model * vec4(aPos, 1);
    out_pos = world_pos.xyz;
    gl_Position = proj_view * world_pos;
    prev_pos = gl_Position.xyw;

#ifdef ATTRIB_UV
    out_uv = aUv;
#endif

#ifdef ATTRIB_TANGENT
    out_tangent = aTangent.w * normalize((model * aTangent).rgb);
#endif
}
