#define PI 3.14

uniform vec4 mat_color;

in vec3 out_norm;
in vec3 out_pos;

out vec4 out_Color;

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

uniform vec3 camera_pos;

vec3 light_dir = vec3(1, 2, 3);


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
    vec3 l = normalize(light_dir);
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

    float metallic = 0;
    float perceptual_roughness = 0.5;
    float reflectance = 0.5;

    vec3 diffuse_color = (1.0 - metallic) * base_color.rgb;
    vec3 specular_color = 0.16 * reflectance * reflectance * (1.0 - metallic) + 
                            base_color.rgb * metallic;
    float roughness = perceptual_roughness * perceptual_roughness;
    roughness = clamp(roughness, 0.089, 1.0);

    vec3 color = clamp(brdf(v, l, n, diffuse_color, specular_color, roughness), vec3(0), vec3(1)) 
        * clamp(dot(n, l), 0, 1);
    out_Color = vec4(color, base_color.a);
    // vec3 cold = vec3(0.02, 0.02, 0.025) + 0.55 * color;
    // vec3 warm = vec3(0.1, 0.05, 0) + color;
    // float light = dot(l, n);
    // out_Color = vec4(mix(cold, warm, light), 1);
}
