out vec4 out_Color;

in vec2 pos;

uniform vec2 dimensions;
uniform sampler2D color_buffer;

void main() 
{
    vec2 uv = (pos + 1) / 2;
    vec2 pixel = vec2(1) / dimensions;
    vec2 offsets[9] = vec2[](
        vec2(-pixel.x, pixel.y),
        vec2(0.0f, pixel.y),
        vec2(pixel.x,  pixel.y),
        vec2(-pixel.x, 0.0f),
        vec2(0.0f, 0.0f),
        vec2(pixel.x, 0.0f),
        vec2(-pixel.x, -pixel.y),
        vec2(0.0f, -pixel.y),
        vec2(pixel.x, -pixel.y)
    );

    float kernel[9] = {
        -1, -1, -1,
        -1,  9, -1,
        -1, -1, -1
    };
    
    vec3 col = vec3(0.0);
    for(int i = 0; i < 9; i++) {
        col += texture(color_buffer, uv + offsets[i]).rgb * kernel[i];
    }

    out_Color = vec4(col, 1);
}
