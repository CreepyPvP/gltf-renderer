layout (location = 0) out vec4 out_Color;

in vec2 pos;

uniform sampler2D current_frame;
uniform sampler2D prev_frame;
uniform sampler2D velocity;

uniform vec2 dimensions;
uniform int jitter_index;

float jitter_strength = 0.5;

vec2 jitter_offsets[] = {
    vec2(1.0, 1.0),
    vec2(-1.0, 1.0),
    vec2(1.0, -1.0),
    vec2(-1.0, -1.0),
};


void main() 
{
    vec2 pixel = vec2(1) / dimensions;

    vec2 uv = (pos + 1) / 2;
    vec2 uv_unjitterd = uv + pixel * jitter_strength * jitter_offsets[jitter_index];
    // vec2 uv_unjitterd = uv;

    vec3 min_color = vec3(1);
    vec3 max_color = vec3(0);
    float largest_vel_len = 0;
    vec2 vel = vec2(0);

    for (int x = -1; x <= 1; ++x) {
        for (int y = -1; y <= 1; ++y) {
            vec2 sample_uv = uv_unjitterd + vec2(x * pixel.x, y * pixel.y);
            sample_uv = clamp(sample_uv, 0, 1);

            vec3 sample_color = max(vec3(0), texture(current_frame, sample_uv).rgb); 
            float sample_dist = length(vec2(x, y));

            min_color = min(sample_color, min_color);
            max_color = max(sample_color, max_color);

            vec2 vel_sample = texture(velocity, sample_uv).xy;
            float vel_len = length(vel_sample);
            if (vel_len > largest_vel_len) {
                vel = vel_sample;
                largest_vel_len = vel_len;
            }
        }
    }

    vec2 prev_uv = uv - vel;
    vec3 source_sample = texture(current_frame, uv_unjitterd).rgb;

    if (prev_uv.x < 0 || prev_uv.x > 1 || prev_uv.y < 0 || prev_uv.y > 1) {
        out_Color = vec4(source_sample, 1);
        return;
    }

    vec3 prev_sample = texture(prev_frame, prev_uv).rgb;
    prev_sample = clamp(prev_sample, min_color, max_color);

    // TODO: Apply anti flickering
    out_Color = 0.05 * vec4(source_sample, 1) + 0.95 * vec4(prev_sample, 1);
    // out_Color = vec4(sigma, 1);
}
