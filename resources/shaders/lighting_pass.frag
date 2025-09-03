#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gAlbedoSpec;

uniform vec3 viewPos;

// Directional light
uniform vec3 lightDir;// direction *from light to fragment*
uniform vec3 lightColor;// radiant intensity

// If roughness isn’t in the G-buffer yet, pass as a uniform for testing
const float uRoughness = 0.5;// 0.05 (smooth) → 1.0 (rough)

const float PI = 3.14159265359;

// -------------------------------------------------
// Helpers
// -------------------------------------------------

// Normal distribution function (GGX/Trowbridge-Reitz)
float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a      = roughness * roughness;
    float a2     = a * a;
    float NdotH  = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    return a2 / (PI * denom * denom + 1e-6);
}

// Geometry function (Schlick-GGX)
float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;// direct lighting

    return NdotV / (NdotV * (1.0 - k) + k + 1e-6);
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2  = GeometrySchlickGGX(NdotV, roughness);
    float ggx1  = GeometrySchlickGGX(NdotL, roughness);
    return ggx1 * ggx2;
}

// Fresnel (Schlick approx)
vec3 FresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

// -------------------------------------------------
// Main
// -------------------------------------------------
void main()
{
    vec3 FragPos = texture(gPosition, TexCoords).rgb;
    vec3 N       = normalize(texture(gNormal, TexCoords).rgb);
    vec3 albedo  = pow(texture(gAlbedoSpec, TexCoords).rgb, vec3(2.2));// assume sRGB in G-buffer
    float metallic = texture(gAlbedoSpec, TexCoords).a;

    if (length(albedo) < 0.001)
    discard;

    vec3 V = normalize(viewPos - FragPos);
    vec3 L = normalize(-lightDir);// directional light
    vec3 H = normalize(V + L);

    // Base reflectivity at normal incidence
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);

    // Cook–Torrance BRDF
    float NDF = DistributionGGX(N, H, uRoughness);
    float G   = GeometrySmith(N, V, L, uRoughness);
    vec3  F   = FresnelSchlick(max(dot(H, V), 0.0), F0);

    vec3 numerator    = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 1e-6;
    vec3 specular     = numerator / denominator;

    // kS = Fresnel, kD = diffuse part (energy conservation)
    vec3 kS = F;
    vec3 kD = (1.0 - kS) * (1.0 - metallic);

    float NdotL = max(dot(N, L), 0.0);
    vec3 Lo = (kD * albedo / PI + specular) * lightColor * NdotL;

    // Add ambient (IBL/skybox later – stub as low constant)
    vec3 ambient = 0.03 * albedo;

    vec3 color = ambient + Lo;

    // HDR tonemap + gamma
    color = color / (color + vec3(1.0));// Reinhard
    color = pow(color, vec3(1.0/2.2));

    FragColor = vec4(color, 1.0);
}
