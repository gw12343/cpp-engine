#version 330 core

layout (location = 0) out vec3 gPosition;
layout (location = 1) out vec3 gNormal;
layout (location = 2) out vec4 gAlbedoSpec;

in VS_OUT {
    vec3 FragPos;
    vec2 TexCoords;
    mat3 TBN;
} fs_in;

// Material textures
uniform sampler2D diffuseTexture;
uniform sampler2D normalTexture;
uniform sampler2D specularTexture;

uniform int hasDiffuseTexture;
uniform int hasNormalTexture;
uniform int hasSpecularTexture;

// Material constants (used if texture missing)
uniform vec3 uDiffuseColor;
uniform vec3 uSpecularColor;
uniform float uShininess;

// Sentinel values for "empty" pixels
const vec3 EMPTY_POS = vec3(0.0);
const vec3 EMPTY_NORMAL = vec3(0.0);
const vec4 EMPTY_ALBEDO_SPEC = vec4(0.0);

void main()
{
    // Optional: check if this fragment should exist
    // For models, it always exists. If you have alpha-tested objects, you can discard them:
    // if (texture(diffuseTexture, fs_in.TexCoords).a < 0.01) {
    //     gPosition = EMPTY_POS;
    //     gNormal   = EMPTY_NORMAL;
    //     gAlbedoSpec = EMPTY_ALBEDO_SPEC;
    //     return;
    // }

    // Store fragment world position
    gPosition = fs_in.FragPos;

    // Normal: either from normal map or interpolated
    vec3 N;
    if (hasNormalTexture == 1) {
        vec3 normalSample = texture(normalTexture, fs_in.TexCoords).rgb;
        normalSample = normalize(normalSample * 2.0 - 1.0);// tangent-space normal
        N = normalize(fs_in.TBN * normalSample);
    } else {
        N = normalize(fs_in.TBN[2]);// fallback: vertex normal
    }
    gNormal = N;

    // Albedo
    vec3 albedo = (hasDiffuseTexture == 1)
    ? texture(diffuseTexture, fs_in.TexCoords).rgb
    : uDiffuseColor;

    // Specular intensity (store in alpha)
    float spec = (hasSpecularTexture == 1)
    ? texture(specularTexture, fs_in.TexCoords).r
    : uShininess;

    gAlbedoSpec.rgb = albedo;
    gAlbedoSpec.a   = spec;
}
