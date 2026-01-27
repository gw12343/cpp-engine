#version 420 core

layout (location = 0) out vec4 gAlbedo;    // RGB = base color, A = alpha
layout (location = 1) out vec3 gNormal;    // world-space normal
layout (location = 2) out vec4 gMaterial;  // R = specular strength
layout (location = 3) out vec3 gEmissive;  // emissive color

in VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoords;
    mat3 TBN;
} fs_in;

layout (binding = 0) uniform sampler2D diffuseTexture;
layout (binding = 1) uniform sampler2D normalTexture;
layout (binding = 2) uniform sampler2D specularTexture;

uniform int hasDiffuseTexture;
uniform int hasNormalTexture;
uniform int hasSpecularTexture;

uniform vec2 textureScale;

uniform vec3 uDiffuseColor;
uniform vec3 uEmissiveColor;

void main()
{
    // -------------------------------
    // Albedo + alpha
    vec4 sampledDiffuse = hasDiffuseTexture == 1
    ? texture(diffuseTexture, fs_in.TexCoords * textureScale)
    : vec4(1.0);

    if (sampledDiffuse.a < 0.5)
    discard;

    gAlbedo = vec4(sampledDiffuse.rgb * uDiffuseColor, sampledDiffuse.a);


    // -------------------------------
    // Normal mapping (world space)
    vec3 normal;
    if (hasNormalTexture == 1) {
        vec3 tangentNormal = texture(normalTexture, fs_in.TexCoords * textureScale).rgb;
        tangentNormal = tangentNormal * 2.0 - 1.0;
        normal = normalize(fs_in.TBN * tangentNormal);
    } else {
        normal = normalize(fs_in.Normal);
    }

    gNormal = normal;

    // -------------------------------
    // Specular strength
    float specStrength = hasSpecularTexture == 1
    ? texture(specularTexture, fs_in.TexCoords * textureScale).r
    : 0.0;

    gMaterial = vec4(specStrength, 0.0, 0.0, 0.0);

    // -------------------------------
    // Emissive
    gEmissive = uEmissiveColor;
}
