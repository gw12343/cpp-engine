#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in vec3 aTangent;
layout (location = 4) in vec3 aBitangent;

out VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoords;
    vec3 Tangent;
    vec3 Bitangent;

    mat3 TBN;
} vs_out;



uniform mat4 view;
uniform mat4 projection;
uniform mat4 model;

void main()
{
    vs_out.FragPos = vec3(model * vec4(aPos, 1.0));
    vs_out.Normal = transpose(inverse(mat3(model))) * aNormal;
    vs_out.TexCoords = aTexCoord;
    vs_out.Tangent = aTangent;
    vs_out.Bitangent = aBitangent;
    gl_Position = projection * view * model * vec4(aPos, 1.0);

    vec3 T = normalize(vec3(model * vec4(vs_out.Tangent, 0.0)));
    vec3 B = normalize(vec3(model * vec4(vs_out.Bitangent, 0.0)));
    vec3 N = normalize(vec3(model * vec4(vs_out.Normal, 0.0)));
    vs_out.TBN = mat3(T, B, N);
}