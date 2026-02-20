/* ---------- CSM ---------- */
layout (std140) uniform LightSpaceMatrices {
    mat4 lightSpaceMatrices[16];
};
uniform float cascadePlaneDistances[16];
uniform int cascadeCount;

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