#version 330 core
out vec4 FragColor;

in vec3 TexCoords;

uniform sampler2D skybox;

void main()
{    
    // Convert cube coordinates to spherical coordinates for EXR mapping
    vec3 dir = normalize(TexCoords);
    float phi = atan(dir.z, dir.x);
    float theta = asin(dir.y);
    
    // Map to UV coordinates
    vec2 uv = vec2(phi / (2.0 * 3.14159) + 0.5, theta / 3.14159 + 0.5);
    
    FragColor = texture(skybox, uv);
}