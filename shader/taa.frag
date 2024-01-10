layout (location = 0) out vec4 out_Color;

in vec2 pos;

uniform sampler2D current_frame;
uniform sampler2D prev_frame;
uniform sampler2D velocity;

uniform vec2 dimensions;

vec3 clip_aabb(vec3 aabb_min, vec3 aabb_max, vec3 p, vec3 q)
{
    vec3 r = q - p;
    vec3 rmax = aabb_max - p.xyz;
    vec3 rmin = aabb_min - p.xyz;

    const float eps = 0.0;

    if (r.x > rmax.x + eps)
        r *= (rmax.x / r.x);
    if (r.y > rmax.y + eps)
        r *= (rmax.y / r.y);
    if (r.z > rmax.z + eps)
        r *= (rmax.z / r.z);

    if (r.x < rmin.x - eps)
        r *= (rmin.x / r.x);
    if (r.y < rmin.y - eps)
        r *= (rmin.y / r.y);
    if (r.z < rmin.z - eps)
        r *= (rmin.z / r.z);

    return p + r;
}

float mitchell(float d) 
{
    float b = 0.5;
    float c = 0.5;
    d = abs(d);
    if (d < 1) {
        return (12 - 9 * b - 6 * c) * d * d * d + 
               (-18 + 12 * b + 6 * c) * d * d + 6 - 2 * b;
    }
    if (d >= 1 && d < 2) {
        return (-b - 6 * c) * d * d * d +
                (6 * b * 30 * c) * d * d +
                (-12 * b - 48 * c) * d +
                (8 * b  + 24 * c);
    }
    return 0;
}

void main() 
{
    vec2 uv = (pos + 1) / 2;

    vec2 pixel = vec2(1) / dimensions;

    // vec2 test_uv = texture(velocity, uv).xy / 256;
    // out_Color = vec4(abs(uv - test_uv) * dimensions, 0, 1);
    // return;

    vec3 min_color = vec3(1);
    vec3 max_color = vec3(0);
    vec3 sample_total = vec3(0);
    float sample_total_weight = 0;
    vec3 m1 = vec3(0);
    vec3 m2 = vec3(0);
    float largest_vel_len = 0;
    vec2 vel = vec2(0);

    for (int x = -1; x <= 1; ++x) {
        for (int y = -1; y <= 1; ++y) {
            vec2 sample_uv = uv + vec2(x * pixel.x, y * pixel.y);
            sample_uv = clamp(sample_uv, 0, 1);

            vec3 sample_color = max(vec3(0), texture(current_frame, sample_uv).rgb); 
            float sample_dist = length(vec2(x, y));
            float sample_weight = mitchell(sample_dist);

            sample_total += sample_color * sample_weight;
            sample_total_weight += sample_weight;

            min_color = min(sample_color, min_color);
            max_color = max(sample_color, max_color);

            m1 += sample_color;
            m2 += sample_color * sample_color;

            // TODO: Use nearest depth instead
            vec2 vel_sample = texture(velocity, sample_uv).xy;
            float vel_len = length(vel_sample);
            if (vel_len > largest_vel_len) {
                vel = vel_sample;
                largest_vel_len = vel_len;
            }
        }
    }

    vec2 prev_uv = uv - vel;
    vec3 source_sample = sample_total / sample_total_weight;

    if (prev_uv.x < 0 || prev_uv.x > 1 || prev_uv.y < 0 || prev_uv.y > 1) {
        out_Color = vec4(source_sample, 1);
        return;
    }

    vec3 prev_sample_acc = vec3(0);
    float prev_sample_weight = 0;
    for (int x = -1; x <= 1; ++x) {
        for (int y = -1; y <= 1; ++y) {
            vec2 sample_uv = prev_uv + vec2(x * pixel.x, y * pixel.y);
            sample_uv = clamp(sample_uv, 0, 1);

            vec3 sample_color = texture(prev_frame, sample_uv).xyz;
            float weight = mitchell(length(vec2(x, y)));

            prev_sample_acc += weight * sample_color;
            prev_sample_weight += weight;
        }
    }

    vec3 prev_sample = prev_sample_acc / prev_sample_weight;

    float one_by_samples = 1.0f / 9.0f;
    float gamma = 1.0f;
    vec3 mu = m1 * one_by_samples;
    vec3 sigma = sqrt(abs((m2 * one_by_samples) - (mu * mu)));
    vec3 minc = mu - gamma * sigma;
    vec3 maxc = mu + gamma * sigma;

    // prev_sample = clip_aabb(minc, maxc, clamp(prev_sample, min_color, max_color), mu);
    prev_sample = clamp(prev_sample, min_color, max_color);

    // TODO: Apply anti flickering
    out_Color = 0.2 * vec4(source_sample, 1) + 0.8 * vec4(prev_sample, 1);
    // out_Color = vec4(sigma, 1);
}
