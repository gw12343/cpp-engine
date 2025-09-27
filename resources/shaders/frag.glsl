#version 420 core
out vec4 FragColor;

in VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoords;
    vec3 Tangent;
    vec3 Bitangent;
    mat3 TBN;
} fs_in;

layout (binding = 0) uniform sampler2D diffuseTexture;
layout (binding = 1) uniform sampler2D normalTexture;
layout (binding = 2) uniform sampler2D specularTexture;
layout (binding = 3) uniform sampler2DArray shadowMap;

uniform int hasDiffuseTexture;
uniform int hasNormalTexture;
uniform int hasSpecularTexture;

uniform vec2 textureScale;

uniform vec3 lightDir;
uniform vec3 viewPos;
uniform float farPlane;
uniform int debugShadows;

uniform mat4 view;

uniform vec3 uDiffuseColor;
uniform vec3 uSpecularColor;
uniform vec3 uAmbientColor;
uniform vec3 uEmissiveColor;
uniform float uShininess;

layout (std140) uniform LightSpaceMatrices {
    mat4 lightSpaceMatrices[16];
};
uniform float cascadePlaneDistances[16];
uniform int cascadeCount;

int GetCascadeLayer(vec3 fragPosWorldSpace)
{
    vec4 fragPosViewSpace = view * vec4(fragPosWorldSpace, 1.0);
    float depthValue = abs(fragPosViewSpace.z);

    for (int i = 0; i < cascadeCount; ++i) {
        if (depthValue < cascadePlaneDistances[i]) {
            return i;
        }
    }
    return cascadeCount;
}

float ShadowCalculation(vec3 fragPosWorldSpace, int layer, vec3 normal)
{
    vec4 fragPosLightSpace = lightSpaceMatrices[layer] * vec4(fragPosWorldSpace, 1.0);
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;

    float currentDepth = projCoords.z;
    if (currentDepth > 1.0) return 0.0;

    float bias = max(0.05 * (1.0 - dot(normal, lightDir)), 0.005);
    const float biasModifier = 0.5f;
    bias *= 1.0 / ((layer == cascadeCount ? farPlane : cascadePlaneDistances[layer]) * biasModifier);

    float shadow = 0.0;
    vec2 texelSize = 1.0 / vec2(textureSize(shadowMap, 0));
    for (int x = -1; x <= 1; ++x) {
        for (int y = -1; y <= 1; ++y) {
            float pcfDepth = texture(shadowMap, vec3(projCoords.xy + vec2(x, y) * texelSize, layer)).r;
            shadow += (currentDepth - bias) > pcfDepth ? 1.0 : 0.0;
        }
    }
    shadow /= 9.0;
    return shadow;
}

void main()
{
    // -------------------------------
    // Diffuse color
    vec4 sampledDiffuse = hasDiffuseTexture == 1
    ? texture(diffuseTexture, fs_in.TexCoords * textureScale)
    : vec4(1.0);

    vec3 texColor = sampledDiffuse.rgb;
    float alpha = sampledDiffuse.a;
    if (alpha < 0.5f) discard;

    vec3 baseColor = texColor * uDiffuseColor;

    // -------------------------------
    // Normal mapping
    vec3 normal;
    if (hasNormalTexture == 1) {
        vec3 tangentNormal = texture(normalTexture, fs_in.TexCoords * textureScale).rgb;
        tangentNormal = tangentNormal * 2.0 - 1.0;// unpack
        normal = normalize(fs_in.TBN * tangentNormal);
    } else {
        normal = normalize(fs_in.Normal);
    }

    // -------------------------------
    // Ambient
    vec3 ambient = uAmbientColor * baseColor;

    // -------------------------------
    // Diffuse
    float diff = max(dot(lightDir, normal), 0.0);
    vec3 diffuse = diff * baseColor;

    // -------------------------------
    // Specular
    vec3 viewDir = normalize(viewPos - fs_in.FragPos);
    vec3 halfDir = normalize(lightDir + viewDir);

    float specStrength = 1.0;
    if (hasSpecularTexture == 1) {
        specStrength = texture(specularTexture, fs_in.TexCoords * textureScale).r;
    }

    float spec = pow(max(dot(normal, halfDir), 0.0), uShininess);
    vec3 specular = spec * uSpecularColor * specStrength;

    // -------------------------------
    // Emissive
    vec3 emissive = uEmissiveColor;

    // -------------------------------
    // Shadows
    int layer = GetCascadeLayer(fs_in.FragPos);
    float shadow = ShadowCalculation(fs_in.FragPos, layer, normal);

    vec3 lighting = ambient + emissive + ((1.0 - shadow) * 0.5 + 0.5) * (diffuse + specular);

    // -------------------------------
    // Debug cascade overlay
    if (debugShadows == 1) {
        vec3 cascadeColor;
        if (layer == 0) cascadeColor = vec3(1, 0, 0);
        else if (layer == 1) cascadeColor = vec3(0, 1, 0);
        else if (layer == 2) cascadeColor = vec3(0, 0, 1);
        else if (layer == 3) cascadeColor = vec3(1, 1, 0);
        else cascadeColor = vec3(1.0);
        FragColor = vec4(mix(lighting, cascadeColor, 0.35), 1.0);
    } else {
        FragColor = vec4(lighting, 1.0);
    }
}
