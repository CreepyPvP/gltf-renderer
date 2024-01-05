layout(location = 0) in vec2 aPos;

out vec2 pos;

void main() {
    pos = aPos;
    gl_Position = vec4(aPos, 0, 1);
}
