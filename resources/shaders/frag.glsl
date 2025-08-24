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
layout (binding = 1) uniform sampler2DArray shadowMap;

// ✅ Unified texture scale for all material textures
uniform vec2 textureScale;

uniform vec3 lightDir;
uniform vec3 viewPos;
uniform float farPlane;

uniform int debugShadows;

uniform mat4 view;

// ✅ Material properties
uniform vec3 uDiffuseColor;
uniform vec3 uSpecularColor;
uniform vec3 uAmbientColor;
uniform vec3 uEmissiveColor;
uniform float uShininess;

layout (std140) uniform LightSpaceMatrices
{
    mat4 lightSpaceMatrices[16];
};
uniform float cascadePlaneDistances[16];
uniform int cascadeCount;// number of frusta - 1

int GetCascadeLayer(vec3 fragPosWorldSpace)
{
    vec4 fragPosViewSpace = view * vec4(fragPosWorldSpace, 1.0);
    float depthValue = abs(fragPosViewSpace.z);

    for (int i = 0; i < cascadeCount; ++i)
    {
        if (depthValue < cascadePlaneDistances[i])
        {
            return i;
        }
    }
    return cascadeCount;
}

float ShadowCalculation(vec3 fragPosWorldSpace, int layer)
{
    vec4 fragPosLightSpace = lightSpaceMatrices[layer] * vec4(fragPosWorldSpace, 1.0);
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;

    float currentDepth = projCoords.z;
    if (currentDepth > 1.0) return 0.0;

    vec3 normal = normalize(fs_in.Normal);
    float bias = max(0.05 * (1.0 - dot(normal, lightDir)), 0.005);
    const float biasModifier = 0.5f;
    bias *= 1.0 / ((layer == cascadeCount ? farPlane : cascadePlaneDistances[layer]) * biasModifier);

    float shadow = 0.0;
    vec2 texelSize = 1.0 / vec2(textureSize(shadowMap, 0));
    for (int x = -1; x <= 1; ++x)
    {
        for (int y = -1; y <= 1; ++y)
        {
            float pcfDepth = texture(shadowMap, vec3(projCoords.xy + vec2(x, y) * texelSize, layer)).r;
            shadow += (currentDepth - bias) > pcfDepth ? 1.0 : 0.0;
        }
    }
    shadow /= 9.0;
    return shadow;
}

void main()
{
    vec4 smp = texture(diffuseTexture, fs_in.TexCoords * textureScale);
    vec3 texColor = smp.rgb;
    float alpha = smp.a;
    if (alpha < 0.5f){
        discard;
    }
    vec3 normal = normalize(fs_in.Normal);

    // ✅ Material colors combined with texture
    vec3 baseColor = texColor * uDiffuseColor;

    // Ambient
    vec3 ambient = uAmbientColor * baseColor;

    // Diffuse
    float diff = max(dot(lightDir, normal), 0.0);
    vec3 diffuse = diff * uDiffuseColor;

    // Specular
    vec3 viewDir = normalize(viewPos - fs_in.FragPos);
    vec3 halfDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfDir), 0.0), uShininess);
    vec3 specular = spec * uSpecularColor;

    // Emissive
    vec3 emissive = uEmissiveColor;

    // ✅ Determine cascade layer
    int layer = GetCascadeLayer(fs_in.FragPos);

    // ✅ Shadow with layer
    float shadow = ShadowCalculation(fs_in.FragPos, layer);

    // ✅ Final lighting
    vec3 lighting = ambient + emissive + ((1.0 - shadow) * 0.5 + 0.5) * (diffuse + specular);
    lighting *= baseColor;

    // ✅ Debug: overlay cascade layer color
    vec3 cascadeColor;
    if (layer == 0) cascadeColor = vec3(1, 0, 0);// red
    else if (layer == 1) cascadeColor = vec3(0, 1, 0);// green
    else if (layer == 2) cascadeColor = vec3(0, 0, 1);// blue
    else if (layer == 3) cascadeColor = vec3(1, 1, 0);// yellow
    else cascadeColor = vec3(1.0);// white or fallback

    if (debugShadows == 1){
        FragColor = vec4(mix(lighting, cascadeColor, 0.35), 1.0);
    } else {
        FragColor = vec4(lighting, 1.0);
    }
}
