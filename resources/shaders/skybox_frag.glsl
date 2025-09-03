#version 330 core
out vec4 FragColor;

in vec3 TexCoords;

uniform sampler2D skybox;// <-- equirectangular HDR map

vec2 SampleSphericalMap(vec3 v)
{
    vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
    uv *= vec2(0.1591, 0.3183);// (1/2PI, 1/PI)
    uv += 0.5;
    return uv;
}

void main()
{
    vec3 dir = normalize(TexCoords);
    vec2 uv = SampleSphericalMap(dir);

    vec3 color = texture(skybox, uv).rgb;

    // Optional tonemap + gamma
    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0/2.2));

    FragColor = vec4(color, 1.0);
}
