#version 330 core
layout(location = 0) in vec3 aPos;

out vec3 TexCoords;

uniform mat4 view;
uniform mat4 projection;

void main()
{
    TexCoords = aPos;

    // remove translation from view so the sky stays infinitely far
    mat4 viewNoTranslation = mat4(mat3(view));

    vec4 pos = projection * viewNoTranslation * vec4(aPos, 1.0);

    // Force depth to the far plane
    gl_Position = pos.xyww;
}
