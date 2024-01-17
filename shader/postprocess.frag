out vec4 out_Color;

in vec2 pos;

uniform vec2 dimensions;
uniform sampler2D color_buffer;

uniform float sharpness;
uniform float contrast;
uniform float brightness;
uniform float saturation;
uniform float gamma;

void main() 
{
    vec2 uv = (pos + 1) * 0.5;
    vec2 pixel = vec2(1) / dimensions;

    vec3 top = texture(color_buffer, uv + vec2(0, pixel.y)).rgb;
    vec3 bot = texture(color_buffer, uv + vec2(0, -pixel.y)).rgb;
    vec3 left = texture(color_buffer, uv + vec2(-pixel.x, 0)).rgb;
    vec3 right = texture(color_buffer, uv + vec2(pixel.x, 0)).rgb;
    vec3 middle = texture(color_buffer, uv).rgb;

    vec3 color = middle * (sharpness * 4 + 1) 
        - sharpness * top
        - sharpness * bot
        - sharpness * left
        - sharpness * right;
    color = clamp(color, 0, 1);

    color = contrast * (color - 0.5) + 0.5 + brightness;
    color = clamp(color, 0, 1);

    float luminance = dot(color, vec3(0.299, 0.587, 0.114));
    color = saturation * color + (1 - saturation) * vec3(luminance);
    color = clamp(color, 0, 1);

    color = pow(color, vec3(gamma));
    color = clamp(color, 0, 1);

    out_Color = vec4(color, 1);
}
