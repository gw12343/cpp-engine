#version 420 core
out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2D sceneTex;
uniform sampler2D bloomTex;
uniform float bloomStrength;

// Vignette constants
const vec2 vignetteCenter = vec2(0.5, 0.5); // screen center
const float vignetteRadius = 0.75;          // radius at which darkening starts
const float vignetteSoftness = 0.45;        // softness of the fade
const float vignetteIntensity = 0.6;        // max darkening

void main()
{
    vec3 scene = texture(sceneTex, TexCoords).rgb;
    vec3 bloom = texture(bloomTex, TexCoords).rgb;

    vec3 color = scene + bloom * bloomStrength;

    // Compute vignette factor
    float dist = distance(TexCoords, vignetteCenter);
    float vignette = smoothstep(vignetteRadius, vignetteRadius - vignetteSoftness, dist);
    vignette = mix(1.0, vignette, vignetteIntensity);

    color *= vignette;

    FragColor = vec4(color, 1.0);
}