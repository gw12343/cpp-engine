#version 420 core
// Edge-aware (depth bilateral) blur — removes noise striations without
// smearing AO across depth discontinuities (walls / edges).

out float FragColor;
in vec2 TexCoords;

layout(binding = 0) uniform sampler2D ssaoInput;
layout(binding = 1) uniform sampler2D gDepth;

uniform vec2 screenSize;

const int   blurRadius   = 2;      // 5x5
const float depthSigma   = 0.0025; // relative depth difference weight
const float depthEps     = 1e-5;

void main()
{
    float centerDepth = texture(gDepth, TexCoords).r;
    if (centerDepth >= 0.9999)
    {
        FragColor = 1.0;
        return;
    }

    float result = 0.0;
    float total  = 0.0;

    for (int x = -blurRadius; x <= blurRadius; ++x)
    {
        for (int y = -blurRadius; y <= blurRadius; ++y)
        {
            vec2  uv    = TexCoords + vec2(float(x), float(y)) / screenSize;
            float depth = texture(gDepth, uv).r;
            float ao    = texture(ssaoInput, uv).r;

            // Bilateral weight: ignore samples on different surfaces
            float dz = abs(centerDepth - depth);
            // Nonlinear depth: scale sensitivity a bit by distance from camera plane
            float w  = exp(-dz * dz / (depthSigma * depthSigma + depthEps));

            // Softly reject sky samples
            if (depth >= 0.9999)
                w = 0.0;

            result += ao * w;
            total  += w;
        }
    }

    FragColor = (total > 1e-4) ? (result / total) : texture(ssaoInput, TexCoords).r;
}
