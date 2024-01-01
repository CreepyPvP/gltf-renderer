#version 440

uniform vec4 mat_color;
uniform sampler2D mat_diffuse;

in vec3 out_norm;
in vec2 out_uv;

out vec4 out_Color;

vec3 light_dir = vec3(1, 2, 3);

void main() {
    vec3 l = normalize(light_dir);
    vec3 n = normalize(out_norm);
    out_Color = texture(mat_diffuse, out_uv) * mat_color * dot(l, n);
}
