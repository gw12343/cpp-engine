#version 420 core
out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2D lowResTex;

void main()
{
    vec3 color = texture(lowResTex, TexCoords).rgb;
    FragColor = vec4(color, 1.0);
}