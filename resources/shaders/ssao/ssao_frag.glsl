#version 420 core
out float FragColor;
in vec2 TexCoords;

layout(binding = 0) uniform sampler2D gDepth;
layout(binding = 1) uniform sampler2D gNormal;
layout(binding = 2) uniform sampler2D noiseTex;

uniform vec3 samples[32];
uniform mat4 projection;
uniform mat4 invProjection;
uniform mat4 view;
uniform vec2 screenSize;

float radius = 0.4;
float bias   = 0.025;

vec3 ReconstructViewPos(vec2 uv)
{
    float depth = texture(gDepth, uv).r;
    float z = depth * 2.0 - 1.0;
    vec4 clip = vec4(uv * 2.0 - 1.0, z, 1.0);
    vec4 viewPos = invProjection * clip;
    return viewPos.xyz / viewPos.w;
}

void main()
{
    vec3 posVS = ReconstructViewPos(TexCoords);

    vec3 normalWS = texture(gNormal, TexCoords).xyz * 2.0 - 1.0;
    normalWS = normalize(normalWS);

    mat3 normalMat = transpose(inverse(mat3(view)));
    vec3 normalVS = normalize(normalMat * normalWS);

    vec2 noiseScale = screenSize / 4.0;
    vec3 randomVec = texture(noiseTex, TexCoords * noiseScale).xyz;

    vec3 tangent = normalize(randomVec - normalVS * dot(randomVec, normalVS) + 1e-5);
    vec3 bitangent = cross(normalVS, tangent);
    mat3 TBN = mat3(tangent, bitangent, normalVS);

    float occlusion = 0.0;   // <-- THIS WAS MISSING

    for (int i = 0; i < 32; i++)
    {
        vec3 samplePos = posVS + TBN * samples[i] * radius;

        vec4 offset = projection * vec4(samplePos, 1.0);
        offset.xyz /= offset.w;
        offset.xyz = offset.xyz * 0.5 + 0.5;

        if (offset.x < 0.0 || offset.x > 1.0 ||
        offset.y < 0.0 || offset.y > 1.0)
        continue;

        float sampleDepth = ReconstructViewPos(offset.xy).z;

        float rangeCheck = smoothstep(0.0, 1.0, radius / abs(posVS.z - sampleDepth));
        occlusion += (sampleDepth >= samplePos.z + bias ? 1.0 : 0.0) * rangeCheck;
    }

    occlusion = 1.0 - (occlusion / 32.0);
    FragColor = (occlusion * occlusion * occlusion);


}



//    FragColor = clamp((-posVS.z) / 50.0, 0.0, 1.0);
    //FragColor = normalWS.x;