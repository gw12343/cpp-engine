#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aUV;
layout (location = 2) in vec4 aColor;

uniform mat4 uViewProj;

out vec2 vUV;
out vec4 vColor;

void main()
{
    vUV = aUV;
    vColor = aColor;
    gl_Position = uViewProj * vec4(aPos, 1.0);
}
