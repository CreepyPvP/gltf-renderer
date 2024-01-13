#define PI 3.14

uniform vec4 mat_color;
// .x metallic, .y roughness
uniform vec2 mat_pbr;

in vec3 out_norm;
in vec3 out_pos;

in vec3 prev_screen_pos;
in vec3 screen_pos;

layout (location = 0) out vec3 out_Color;
layout (location = 1) out vec2 out_Velocity;

#ifdef ATTRIB_UV
in vec2 out_uv;
#endif

#ifdef ATTRIB_TANGENT
in vec3 out_tangent;
#endif

#ifdef USE_BASE_TEXTURE
uniform sampler2D mat_diffuse;
#endif

#ifdef USE_NORMAL_TEXTURE
uniform sampler2D mat_normal;
#endif

#ifdef USE_ROUGHNESS_TEXTURE
uniform sampler2D mat_roughness;
#endif

uniform vec3 camera_pos;

vec3 light_dirs[] = {
    vec3(1, 2, 3),
    vec3(1, 2, -3),
    vec3(1, -2, 0)
};

vec3 light_colors[] = {
    vec3(1.0, 1.0, 1.0),
    vec3(0.2, 0.2, 0.3),
    vec3(0.2, 0.2, 0.3)
};

int light_count = 3;

vec3 fresnel(float u, vec3 f0) {
    return f0 + (vec3(1.0) - f0) * pow(1.0 - u, 5.0);
}

// Use fast approximation?
float shadowing(float nov, float nol, float roughness) {
    float a2 = roughness * roughness;
    float ggxl = nov * sqrt((-nol * a2 + nol) * nol + a2);
    float ggxv = nol * sqrt((-nov * a2 + nov) * nov + a2);
    return 0.5 / (ggxv + ggxl);
}

float ndf(float noh, float roughness)
{
    float a2 = roughness * roughness;
    float f = (noh * a2 - noh) * noh + 1.0;
    return a2 / (PI * f * f);
}

vec3 brdf(vec3 v, vec3 l, vec3 n, vec3 diffuse_color, vec3 specular_color, float roughness)
{
    vec3 h = normalize(v + l);

    float nov = abs(dot(n, v)) + 1e-5;
    float nol = clamp(dot(n, l), 0.0, 1.0);
    float noh = clamp(dot(n, h), 0.0, 1.0);
    float loh = clamp(dot(l, h), 0.0, 1.0);

    float D = ndf(noh, roughness);
    vec3 F = fresnel(loh, specular_color);
    float V = shadowing(nov, nol, roughness);

    vec3 fr = (D * V) * F;
    return fr + diffuse_color / PI;
}

void main() {
    vec3 v = normalize(camera_pos - out_pos);
    vec3 n = normalize(out_norm);

#ifdef USE_NORMAL_TEXTURE
    vec3 tan = normalize(out_tangent);
    vec3 btan = cross(n, tan);
    vec3 tn = texture(mat_normal, out_uv).rgb;
    tn = tn * 2 - 1;
    n = normalize(tn.r * tan + tn.g * btan + tn.b * n);
#endif

#ifdef USE_BASE_TEXTURE
    vec4 base_color = texture(mat_diffuse, out_uv);
#else
    vec4 base_color = mat_color;
#endif
    if (base_color.a < 0.5) {
        discard;
    }

    float metallic = mat_pbr.x;
    float perceptual_roughness = mat_pbr.y;
    float reflectance = 0.5;

#ifdef USE_ROUGHNESS_TEXTURE
    vec2 metallic_roughness = texture(mat_roughness, out_uv).gb;
    perceptual_roughness *= metallic_roughness.x;
    metallic *= metallic_roughness.y;
#endif

    vec3 diffuse_color = (1.0 - metallic) * base_color.rgb;
    vec3 specular_color = 0.16 * reflectance * reflectance * (1.0 - metallic) + 
                            base_color.rgb * metallic;
    float roughness = perceptual_roughness * perceptual_roughness;
    roughness = clamp(roughness, 0.089, 1.0);

    vec3 color = vec3(0);
    for (int i = 0; i < light_count; ++i) {
        vec3 l = normalize(light_dirs[i]);
        float dir = clamp(dot(n, l), 0, 1);
        color += brdf(v, l, n, diffuse_color, specular_color, roughness) * dir * light_colors[i];
    }

    vec2 uv_prev = vec2(prev_screen_pos.x / prev_screen_pos.z, 
                        prev_screen_pos.y / prev_screen_pos.z) * 0.5 + 0.5;
    vec2 uv_current = vec2(screen_pos.x / screen_pos.z, 
                        screen_pos.y / screen_pos.z) * 0.5 + 0.5;

    // out_Velocity = uv_current - uv_prev;
    out_Velocity = uv_current - uv_prev;
    out_Color = color;
}
