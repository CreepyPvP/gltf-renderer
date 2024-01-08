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
    vec2 prev_uv = uv - (vel / 100);

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

    vec3 prev_sample = texture(prev_frame, prev_uv).rgb;
    prev_sample = max(prev_sample, min_color);
    prev_sample = min(prev_sample, max_color);

    float blend = 0.1;
    if (prev_uv.x < 0 || prev_uv.x > 1 || prev_uv.y < 0 || prev_uv.y > 1) {
        blend = 1;
    }

    out_Color = blend * texture(current_frame, uv) + (1 - blend) * vec4(prev_sample, 1);
}
