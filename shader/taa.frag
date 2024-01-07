layout (location = 0) out vec4 out_Color;

in vec2 pos;

uniform sampler2D current_frame;
uniform sampler2D prev_frame;
uniform sampler2D velocity;

uniform vec2 dimensions;

void main() 
{
    vec2 uv = (pos + 1) / 2;
    vec2 vel = texture(velocity, uv).xy;
    vec2 uv_prev = uv - (vel / 100);

    vec2 pixel = vec2(1) / dimensions;

    vec2 offsets[] = {
        vec2(0, pixel.y),
        vec2(-pixel.x, 0),
        vec2(pixel.x, 0),
        vec2(0, 0),
        vec2(0, -pixel.y)
    };

    vec3 min_color = vec3(1);
    vec3 max_color = vec3(0);

    for (int i = 0; i < 5; ++i) {
        vec3 color_sample = texture(current_frame, uv + offsets[i]).rgb;
        min_color = min(color_sample, min_color);
        max_color = max(color_sample, max_color);
    }

    vec3 prev_sample = texture(prev_frame, uv_prev).rgb;
    prev_sample = max(prev_sample, min_color);
    prev_sample = min(prev_sample, max_color);

    out_Color = 0.5 * texture(current_frame, uv) + 0.5 * vec4(prev_sample, 1);
}
