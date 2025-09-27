#version 330


out vec4 o_color;

uniform vec3 entityIDColor;

void main() {
    o_color = vec4(entityIDColor, 1.0);
}