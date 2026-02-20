#version 420 core
out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2D srcTex;
uniform float threshold;   // user-defined threshold
uniform float knee;        // 0.1–0.5
uniform vec2 srcTexSize;   // size of the source texture
uniform int applyThreshold; // 1 = apply threshold (mip 0), 0 = skip

vec3 Prefilter(vec3 color)
{
    float brightness = max(max(color.r, color.g), color.b);

    float soft = brightness - threshold;
    soft = clamp(soft + knee, 0.0, 2.0 * knee);
    soft = soft * soft / (4.0 * knee + 0.00001);

    float contribution = max(soft, brightness - threshold);
    contribution /= max(brightness, 0.00001);

    return color * contribution;
}

void main()
{
    // Correct texel size for the source texture
    vec2 texelSize = 1.0 / srcTexSize;

    vec3 result = vec3(0.0);

    // 3x3 box blur to smooth the downsample
    for(int x = -1; x <= 1; ++x)
    for(int y = -1; y <= 1; ++y)
    {
        vec2 offset = vec2(x, y) * texelSize;
        result += texture(srcTex, TexCoords + offset).rgb;
    }

    result /= 9.0;

    // Apply threshold only on first mip
    if (applyThreshold == 1)
    {
        result = Prefilter(result);
    }

    FragColor = vec4(result, 1.0);
}