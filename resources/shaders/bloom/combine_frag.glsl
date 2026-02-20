#version 420 core
out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2D sceneTex;
uniform sampler2D bloomTex;
uniform float bloomStrength;

void main()
{
    vec3 scene = texture(sceneTex, TexCoords).rgb;
    vec3 bloom = texture(bloomTex, TexCoords).rgb;

    vec3 color = scene + bloom * bloomStrength;

    FragColor = vec4(color, 1.0);
}