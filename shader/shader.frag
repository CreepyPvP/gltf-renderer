uniform vec4 mat_color;

in vec3 out_norm;

out vec4 out_Color;

#ifdef ATTRIB_UV
in vec2 out_uv;
#endif

#ifdef USE_DIFFUSE_TEXTURE
uniform sampler2D mat_diffuse;
#endif

#ifdef USE_NORMAL_TEXTURE
uniform sampler2D mat_normal;
uniform mat4 model;
#endif

vec3 light_dir = vec3(1, 2, 3);


void main() {
    vec3 l = normalize(light_dir);

#ifdef USE_NORMAL_TEXTURE
    vec3 n = normalize((model * texture(mat_normal, out_uv)).xyz);
#else
    vec3 n = normalize(out_norm);
#endif

#ifdef USE_DIFFUSE_TEXTURE
    vec4 diffuse_color = texture(mat_diffuse, out_uv);
#else
    vec4 diffuse_color = vec4(1, 1, 1, 1);
#endif

    out_Color = diffuse_color * mat_color * dot(l, n);
}
