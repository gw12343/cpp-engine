#version 330 core

in vec2 vUV;
in vec4 vColor;

uniform sampler2D uAtlas;
// Approximate atlas-pixel range of the FreeType SDF spread (for AA floor).
uniform float uPxRange;
// Outline thickness in SDF units outside the edge (0 = none). Edge is 0.5.
uniform float uOutlineWidth;
uniform vec3  uOutlineColor;

out vec4 FragColor;

void main()
{
    // FreeType SDF: 0 = far outside, 0.5 = edge, 1.0 = far inside.
    float sd = texture(uAtlas, vUV).r;
    float dist = sd - 0.5;

    // Screen-space AA. Cap fwidth so large screen derivatives on thin
    // joins don't hollow out the interior and mix in outline color.
    float w = fwidth(dist);
    w = clamp(w, 0.0, 0.075);
    // Tiny floor so very small labels still soft-edge a little.
    w = max(w, 0.5 / max(uPxRange * 4.0, 1.0));

    // Coverage of the fill body (solid inside, soft edge only).
    float fill = smoothstep(-w, w, dist);

    vec3  rgb   = vColor.rgb;
    float alpha = fill;

    if (uOutlineWidth > 0.001) {
        // Soft outer edge of the outline ring (still outside the glyph).
        float outer = smoothstep(-uOutlineWidth - w, -uOutlineWidth + w, dist);
        // Outline only where we are outside the solid fill.
        // Use fill as a hard gate so interior joins never pick up outline black.
        float outlineMask = clamp(outer - fill, 0.0, 1.0);
        // Premultiplied-style composite: fill sits on top of outline.
        rgb   = vColor.rgb * fill + uOutlineColor * outlineMask;
        // Normalize for straight alpha output.
        alpha = max(fill, outer);
        if (alpha > 1e-4) {
            rgb /= alpha;
        }
    }

    alpha *= vColor.a;
    if (alpha < 0.01)
        discard;

    FragColor = vec4(rgb, alpha);
}
