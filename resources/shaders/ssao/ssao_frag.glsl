#version 420 core
// Hemisphere SSAO with depth-adaptive bias and range check.
// Tuned to reduce flat-wall banding / noise striations.

out float FragColor;
in vec2 TexCoords;

layout(binding = 0) uniform sampler2D gDepth;
layout(binding = 1) uniform sampler2D gNormal;
layout(binding = 2) uniform sampler2D noiseTex;

uniform vec3 samples[32];
uniform mat4 projection;
uniform mat4 invProjection;
uniform mat4 view;
uniform vec2 screenSize; // SSAO render target size (not OS window size)

const int   kernelSize = 32;
const float radius     = 0.5;
const float biasBase   = 0.035;
const float power      = 1.6; // softer than cube; less posterized gradients

vec3 ReconstructViewPos(vec2 uv)
{
    float depth = texture(gDepth, uv).r;
    // OpenGL NDC z from depth texture
    float z     = depth * 2.0 - 1.0;
    vec4  clip  = vec4(uv * 2.0 - 1.0, z, 1.0);
    vec4  viewP = invProjection * clip;
    return viewP.xyz / viewP.w;
}

void main()
{
    float depth = texture(gDepth, TexCoords).r;

    // Sky / far plane: fully unoccluded (1 = no AO)
    if (depth >= 0.9999)
    {
        FragColor = 1.0;
        return;
    }

    vec3 posVS = ReconstructViewPos(TexCoords);

    vec3 normalWS = texture(gNormal, TexCoords).xyz * 2.0 - 1.0;
    // Degenerate / unwritten normals
    if (dot(normalWS, normalWS) < 1e-4)
    {
        FragColor = 1.0;
        return;
    }
    normalWS = normalize(normalWS);

    // World → view normal (stable; avoid per-pixel inverse when possible)
    mat3 normalMat = mat3(view);
    vec3 normalVS  = normalize(normalMat * normalWS);
    // Re-orthonormalize against view-space geometric normal direction
    if (dot(normalVS, normalVS) < 1e-6)
    {
        FragColor = 1.0;
        return;
    }

    // 4x4 noise tile — scale must use SSAO buffer resolution
    vec2 noiseScale = screenSize / 4.0;
    vec3 randomVec  = texture(noiseTex, TexCoords * noiseScale).xyz;
    // Random vectors are in XY; rebuild orthonormal TBN around normal
    randomVec = normalize(randomVec + 1e-5);

    vec3 tangent   = normalize(randomVec - normalVS * dot(randomVec, normalVS));
    // If nearly parallel, pick a fixed axis
    if (dot(tangent, tangent) < 1e-4)
    {
        vec3 up = abs(normalVS.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(0.0, 1.0, 0.0);
        tangent = normalize(cross(up, normalVS));
    }
    vec3 bitangent = cross(normalVS, tangent);
    mat3 TBN       = mat3(tangent, bitangent, normalVS);

    // Depth-adaptive bias reduces self-occlusion banding on large flat walls
    float viewZ = abs(posVS.z);
    float bias  = biasBase * max(viewZ * 0.04, 1.0);

    float occlusion = 0.0;
    float contrib   = 0.0;

    for (int i = 0; i < kernelSize; ++i)
    {
        vec3 sampleOffset = TBN * samples[i];
        // Project samples slightly along the normal to reduce surface acne
        sampleOffset += normalVS * 0.02;
        vec3 samplePos = posVS + sampleOffset * radius;

        vec4 offset = projection * vec4(samplePos, 1.0);
        offset.xyz /= offset.w;
        offset.xyz  = offset.xyz * 0.5 + 0.5;

        // Discard off-screen samples (don't count as free space)
        if (offset.x < 0.0 || offset.x > 1.0 || offset.y < 0.0 || offset.y > 1.0)
            continue;

        float sampleDepthLinear = ReconstructViewPos(offset.xy).z;
        float sampleDepthRaw    = texture(gDepth, offset.xy).r;
        if (sampleDepthRaw >= 0.9999)
            continue; // sky sample

        // OpenGL view space: more negative z = farther.
        // Occluder is closer to the camera than the sample point → occludes.
        float rangeCheck = smoothstep(0.0, 1.0, radius / max(abs(posVS.z - sampleDepthLinear), 1e-4));
        // Tighter falloff at large depth deltas reduces long-range wall gradients
        rangeCheck *= 1.0 - smoothstep(radius * 0.5, radius * 1.5, abs(posVS.z - sampleDepthLinear));

        occlusion += (sampleDepthLinear >= samplePos.z + bias ? 1.0 : 0.0) * rangeCheck;
        contrib   += 1.0;
    }

    if (contrib < 1.0)
    {
        FragColor = 1.0;
        return;
    }

    occlusion = 1.0 - (occlusion / contrib);
    // Soft curve — avoid cubic which posterizes flat walls into bands
    occlusion = pow(clamp(occlusion, 0.0, 1.0), power);

    FragColor = occlusion;
}
