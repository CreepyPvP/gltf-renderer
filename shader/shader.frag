uniform vec4 mat_color;
uniform sampler2D mat_diffuse;

in vec3 out_norm;

out vec4 out_Color;

#ifdef USE_DIFFUSE_TEXTURE
in vec2 out_uv;
#endif

vec3 light_dir = vec3(1, 2, 3);


void main() {
    vec3 l = normalize(light_dir);
    vec3 n = normalize(out_norm);

#ifdef USE_DIFFUSE_TEXTURE
    vec4 diffuse_color = texture(mat_diffuse, out_uv);
#else
    vec4 diffuse_color = vec4(1, 1, 1, 1);
#endif

    out_Color = diffuse_color * mat_color * dot(l, n);
}
