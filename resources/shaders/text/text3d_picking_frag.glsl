#version 330 core

in vec2 vUV;

uniform sampler2D uAtlas;
uniform vec3 entityIDColor;
// Include outline / soft edge slightly outside the hard fill.
uniform float uPickThreshold;

out vec4 FragColor;

void main()
{
    // FreeType SDF: 0 outside, 0.5 edge, 1 inside.
    float sd = texture(uAtlas, vUV).r;
    if (sd < uPickThreshold)
        discard;

    FragColor = vec4(entityIDColor, 1.0);
}
