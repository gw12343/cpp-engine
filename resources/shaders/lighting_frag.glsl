#version 420 core
out vec4 FragColor;

in vec2 TexCoords;

layout (binding = 0) uniform sampler2D gDepth;
layout (binding = 1) uniform sampler2D gNormal;
layout (binding = 2) uniform sampler2D gAlbedo;
layout (binding = 3) uniform sampler2D gMaterial;
layout (binding = 4) uniform sampler2D skybox;
layout (binding = 5) uniform sampler2D ssaoBlurTex;
layout (binding = 6) uniform sampler2DArray shadowMap;

uniform vec3 lightDir;
uniform vec3 viewPos;

uniform mat4 view;
uniform mat4 invView;
uniform mat4 invProjection;
uniform float farPlane;

////$include resources/shaders/common

/* ---------- CSM ---------- */
layout (std140) uniform LightSpaceMatrices {
    mat4 lightSpaceMatrices[16];
};
uniform float cascadePlaneDistances[16];
uniform int cascadeCount;

/* ---------- Helpers ---------- */

vec3 ReconstructWorldPos(vec2 uv, float depth)
{
    float z = depth * 2.0 - 1.0;
    vec4 clip = vec4(uv * 2.0 - 1.0, z, 1.0);
    vec4 viewPos4 = invProjection * clip;
    viewPos4 /= viewPos4.w;
    return (invView * viewPos4).xyz;
}

vec3 ReconstructWorldRay(vec2 uv)
{
    vec4 clip = vec4(uv * 2.0 - 1.0, 1.0, 1.0);
    vec4 viewRay = invProjection * clip;
    viewRay /= viewRay.w;
    return normalize((invView * vec4(viewRay.xyz, 0.0)).xyz);
}

vec3 SampleSky(vec3 dir)
{
    const float PI = 3.14159265359;
    float phi   = atan(dir.z, dir.x);
    float theta = asin(dir.y);
    vec2 uv = vec2(phi / (2.0 * PI) + 0.5,
    theta / PI + 0.5);
    return texture(skybox, uv).rgb;
}

/* ---------- Shadows ---------- */

int GetCascadeLayer(vec3 fragPosWorldSpace)
{
    float depthValue = abs((view * vec4(fragPosWorldSpace, 1.0)).z);
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

    // Outside shadow map
    if (projCoords.z > 1.0) return 0.0;
    if (projCoords.x < 0.0 || projCoords.x > 1.0 ||
    projCoords.y < 0.0 || projCoords.y > 1.0)
    return 0.0;

    float ndotl = max(dot(normal, lightDir), 0.0);

    // Correct texel size (2D only!)
    vec2 texelSize = 1.0 / vec2(textureSize(shadowMap, 0).xy);

    // Texel-scaled slope bias (clamped)
    float bias = max(
        texelSize.x * 1.5 * (1.0 - ndotl),
        texelSize.x * 4.15
    );

    float shadow = 0.0;

    for (int x = -1; x <= 1; ++x)
    for (int y = -1; y <= 1; ++y)
    {
        float pcfDepth = texture(
            shadowMap,
            vec3(projCoords.xy + vec2(x, y) * texelSize, layer)
        ).r;

        shadow += (projCoords.z - bias) > pcfDepth ? 1.0 : 0.0;
    }

    shadow *= 1.0 / 9.0;

    // Fade out far cascade only
    float viewDepth = abs((view * vec4(fragPosWorldSpace, 1.0)).z);
    if (layer == cascadeCount - 1)
    {
        float fadeStart = cascadePlaneDistances[cascadeCount - 2];
        float fadeEnd   = cascadePlaneDistances[cascadeCount - 1];
        float fade = clamp((fadeEnd - viewDepth) / (fadeEnd - fadeStart), 0.0, 1.0);
        shadow *= fade;
    }

    return shadow;
}


/* ---------- Main ---------- */

void main()
{
    float depth = texture(gDepth, TexCoords).r;

/* SKY */
    if (depth >= 0.9999)
    {
        vec3 ray = ReconstructWorldRay(TexCoords);
        FragColor = vec4(SampleSky(ray), 1.0);
        return;
    }

    vec3 FragPos = ReconstructWorldPos(TexCoords, depth);
    vec3 N = normalize(texture(gNormal, TexCoords).rgb);
    vec3 V = normalize(viewPos - FragPos);
    vec3 L = normalize(lightDir);
    vec3 H = normalize(L + V);
    vec3 Albedo = texture(gAlbedo, TexCoords).rgb;

    vec2 material = texture(gMaterial, TexCoords).rg;
    float specStrength = material.r;
    float shininess = material.g * 256.0;

/* -------- Super Strong SSAO-like AO (testing) -------- */
    float ao = clamp(dot(N, V), 0.0, 1.0);
    ao = pow(ao, 6.0);
    ao = 0.2 + 0.8 * ao;
    ao = clamp(ao * 1.8, 0.0, 1.0);

/* -------- Ambient -------- */
    vec3 ambient = 0.05 * Albedo;

/* -------- Diffuse + wrap -------- */
    float wrap = 0.4;
    float diff = max((dot(N, L) + wrap) / (1.0 + wrap), 0.0);
    vec3 diffuse = diff * Albedo;

/* -------- Specular (AA + Fresnel) -------- */
    float specAA = shininess / (1.0 + fwidth(dot(N, H)) * shininess);
    float spec = pow(max(dot(N, H), 0.0), specAA);

    float F = pow(1.0 - max(dot(N, V), 0.0), 5.0);
    vec3 specular = spec * mix(vec3(0.04), vec3(1.0), F) * specStrength;

/* -------- Energy conservation -------- */
    diffuse *= (1.0 - specStrength);

/* -------- Sky IBL -------- */
    vec3 skyDiffuse = SampleSky(N) * 0.15;
    vec3 R = reflect(-V, N);
    vec3 skySpec = SampleSky(R) * specStrength * 0.2;

/* -------- Shadows -------- */
    float shadow = ShadowCalculation(FragPos, GetCascadeLayer(FragPos), N);

    vec3 lighting =
    ao * (ambient + skyDiffuse) +
    (1.0 - shadow) * (diffuse + specular + skySpec);

/* -------- Fog -------- */
    float dist = length(viewPos - FragPos);

    // Exponential fog (stable at distance)
    float fogDensity = 0.005;
    float fogFactor = 1.0 - exp(-dist * fogDensity);
    fogFactor = clamp(fogFactor, 0.0, 1.0);

    // Fade AO & shadows with fog
    ao = mix(1.0, ao, 1.0 - fogFactor);
    shadow *= (1.0 - fogFactor);

    // Re-evaluate lighting with faded terms
    lighting =
    ao * (ambient + skyDiffuse) +
    (1.0 - shadow) * (diffuse + specular + skySpec);

    // Prevent zero-light pixels before fog blend
    lighting = max(lighting, vec3(0.02));

    vec3 fogColor = vec3(0.6, 0.7, 0.8);
    lighting = mix(lighting, fogColor, fogFactor);

    FragColor = vec4(lighting, 1.0);
}
