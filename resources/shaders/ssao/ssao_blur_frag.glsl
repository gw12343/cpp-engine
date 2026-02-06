#version 420 core
out float FragColor;
in vec2 TexCoords;

layout(binding = 0) uniform sampler2D ssaoInput;
uniform vec2 screenSize;

void main()
{
    float result = 0.0;

    for (int x = -2; x <= 2; x++)
    for (int y = -2; y <= 2; y++)
    {
        vec2 offset = vec2(x, y) / screenSize;
        result += texture(ssaoInput, TexCoords + offset).r;
    }

    FragColor = result / 25.0;
}
