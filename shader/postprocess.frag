out vec4 out_Color;

in vec2 pos;

uniform sampler2D color_buffer;

void main() 
{
    vec2 uv = (pos + 1) / 2;
    out_Color = texture(color_buffer, uv);
}
