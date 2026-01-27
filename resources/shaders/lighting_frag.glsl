#version 420 core
out vec4 FragColor;

in vec2 TexCoords;

layout (binding = 0) uniform sampler2D gDepth;
layout (binding = 1) uniform sampler2D gNormal;
layout (binding = 2) uniform sampler2D gAlbedo;
layout (binding = 3) uniform sampler2D gMaterial;
layout (binding = 4) uniform sampler2D skybox;
layout (binding = 5) uniform sampler2DArray shadowMap;

uniform vec3 lightDir;
uniform vec3 viewPos;

uniform mat4 view;
uniform mat4 invView;
uniform mat4 invProjection;
uniform float farPlane;

/* ---------- CSM ---------- */
layout (std140) uniform LightSpaceMatrices {
    mat4 lightSpaceMatrices[16];
};
uniform float cascadePlaneDistances[16];
uniform int cascadeCount;

/* ---------- Position reconstruction ---------- */

vec3 ReconstructWorldPos(vec2 uv, float depth)
{
    float z = depth * 2.0 - 1.0;

    vec4 clip = vec4(uv * 2.0 - 1.0, z, 1.0);
    vec4 viewPos4 = invProjection * clip;
    viewPos4 /= viewPos4.w;

    vec4 worldPos4 = invView * viewPos4;
    return worldPos4.xyz;
}

vec3 ReconstructWorldRay(vec2 uv)
{
    // Clip space (far plane)
    vec4 clip = vec4(uv * 2.0 - 1.0, 1.0, 1.0);

    // View space
    vec4 viewRay = invProjection * clip;
    viewRay /= viewRay.w;

    // World space (direction → w = 0)
    vec3 worldRay = (invView * vec4(viewRay.xyz, 0.0)).xyz;
    return normalize(worldRay);
}


/* ---------- Shadows ---------- */

int GetCascadeLayer(vec3 fragPosWorldSpace)
{
    vec4 fragPosViewSpace = view * vec4(fragPosWorldSpace, 1.0);
    float depthValue = abs(fragPosViewSpace.z);

    for (int i = 0; i < cascadeCount; ++i)
    if (depthValue < cascadePlaneDistances[i])
    return i;

    return cascadeCount - 1;
}

float ShadowCalculation(vec3 fragPosWorldSpace, int layer, vec3 normal)
{
    vec4 fragPosLightSpace = lightSpaceMatrices[layer] * vec4(fragPosWorldSpace, 1.0);
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;

    if (projCoords.z > 1.0)
    return 0.0;

    float bias = max(0.05 * (1.0 - dot(normal, lightDir)), 0.005);
    bias *= 1.0 / ((layer == cascadeCount ? farPlane : cascadePlaneDistances[layer]) * 0.5);

    float shadow = 0.0;
    vec2 texelSize = 1.0 / vec2(textureSize(shadowMap, 0));

    for (int x = -1; x <= 1; ++x)
    for (int y = -1; y <= 1; ++y)
    {
        float pcfDepth = texture(shadowMap,
                                 vec3(projCoords.xy + vec2(x, y) * texelSize, layer)).r;

        shadow += (projCoords.z - bias) > pcfDepth ? 1.0 : 0.0;
    }

    return shadow / 9.0;
}

/* ---------- Skybox ---------- */

vec3 SampleSky(vec3 dir)
{
    const float PI = 3.14159265359;

    float phi   = atan(dir.z, dir.x);
    float theta = asin(dir.y);

    vec2 uv = vec2(
    phi / (2.0 * PI) + 0.5,
    theta / PI + 0.5
    );

    return texture(skybox, uv).rgb;
}



/* ---------- Main ---------- */

void main()
{

    float depth = texture(gDepth, TexCoords).r;

    if (depth >= 0.9999)
    {
        vec3 ray = ReconstructWorldRay(TexCoords);
        vec3 sky = SampleSky(ray);
        FragColor = vec4(sky, 1.0);
        return;
    }

    vec3 FragPos = ReconstructWorldPos(TexCoords, depth);
    vec3 Normal  = normalize(texture(gNormal, TexCoords).rgb);
    vec3 Albedo  = texture(gAlbedo, TexCoords).rgb;

    vec2 material = texture(gMaterial, TexCoords).rg;
    float specStrength = material.r;
    float shininess = material.g * 256.0;

    // Ambient
    vec3 ambient = 0.1 * Albedo;

    // Diffuse
    float diff = max(dot(Normal, lightDir), 0.0);
    vec3 diffuse = diff * Albedo;

    // Specular
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 halfDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(Normal, halfDir), 0.0), shininess);
    vec3 specular = vec3(spec * specStrength);

    // Shadows
    int layer = GetCascadeLayer(FragPos);
    float shadow = ShadowCalculation(FragPos, layer, Normal);

    vec3 lighting = ambient + (1.0 - shadow) * (diffuse + specular);

    FragColor = vec4(lighting, 1.0);



}
