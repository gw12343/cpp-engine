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

uniform vec2 texScale;
uniform vec3 lightDir;
uniform vec3 viewPos;
uniform float farPlane;

uniform int debugShadows;

uniform mat4 view;

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
    vec4 smp = texture(diffuseTexture, fs_in.TexCoords * texScale);
    vec3 color = smp.rgb;
    float alpha = smp.a;
    if (alpha < 0.5f){
        discard;
    }
    vec3 normal = normalize(fs_in.Normal);

    vec3 lightColor = vec3(0.3);
    vec3 ambient = 0.3 * color;
    float diff = max(dot(lightDir, normal), 0.0);
    vec3 diffuse = diff * lightColor;

    vec3 viewDir = normalize(viewPos - fs_in.FragPos);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(normal, normalize(lightDir + viewDir)), 0.0), 64.0);
    vec3 specular = spec * lightColor;

    // ✅ Determine cascade layer
    int layer = GetCascadeLayer(fs_in.FragPos);

    // ✅ Shadow with layer
    float shadow = ShadowCalculation(fs_in.FragPos, layer);

    vec3 lighting = (ambient + (1.0 - shadow) * (diffuse + specular)) * color;

    // ✅ Debug: overlay cascade layer color
    vec3 cascadeColor;
    if (layer == 0) cascadeColor = vec3(1, 0, 0);// red
    else if (layer == 1) cascadeColor = vec3(0, 1, 0);// green
    else if (layer == 2) cascadeColor = vec3(0, 0, 1);// blue
    else if (layer == 3) cascadeColor = vec3(1, 1, 0);// yellow
    else cascadeColor = vec3(1.0);// white or fallback

    // ✅ Blend with lighting (or set directly)
    if (debugShadows == 1){
        FragColor = vec4(mix(lighting, cascadeColor, 0.35), 1.0);
    } else {
        FragColor = vec4(lighting, 1.0);
    }

    //FragColor = vec4(alpha, alpha, alpha, 1.0);

    // Alternatively, to show only debug color:
    //FragColor = vec4(cascadeColor, 1.0);
}