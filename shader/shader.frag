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
#endif

vec3 light_dir = vec3(1, 2, 3);


void main() {
    vec3 l = normalize(light_dir);

#ifdef USE_NORMAL_TEXTURE
    vec3 n = normalize(out_norm);
#else
    vec3 n = normalize(out_norm);
#endif

#ifdef USE_DIFFUSE_TEXTURE
    vec4 diffuse_color = texture(mat_diffuse, out_uv);
#else
    vec4 diffuse_color = vec4(1, 1, 1, 1);
#endif

    vec3 overall = diffuse_color.xyz * mat_color.xyz;
    vec3 cold = vec3(0.0, 0.0, 0.04) + 0.6 * overall;
    vec3 warm = vec3(0.1, 0.05, 0) + overall;
    float light = dot(l, n);
    out_Color = vec4(mix(cold, warm, light), 1);
}
