#version 420 core
out vec4 FragColor;

in VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoords;
    vec3 Tangent;
    vec3 Bitangent;
    mat3 TBN;
} fs_in;

layout (binding = 0) uniform sampler2D diffuseTexture;
layout (binding = 1) uniform sampler2D specularTexture;
layout (binding = 2) uniform sampler2D normalTexture;

uniform vec3 diffuseColor;
uniform vec3 specularColor;
uniform float shininess;

void main()
{
    // Sample textures
    vec4 diffuseSample = texture(diffuseTexture, fs_in.TexCoords);
    vec3 albedo = diffuseSample.rgb * diffuseColor;
    
    // Fallback for missing diffuse texture
    if (length(diffuseSample.rgb) < 0.1) {
        albedo = diffuseColor;
    }
    
    vec3 specularSample = texture(specularTexture, fs_in.TexCoords).rgb;
    vec3 specular = specularSample * specularColor;
    
    // Sample and apply normal map
    vec3 normal;
    vec3 normalSample = texture(normalTexture, fs_in.TexCoords).rgb;
    if (length(normalSample) > 0.1) {
        // Normal map exists, use it
        normal = normalSample * 2.0 - 1.0;
        normal = normalize(fs_in.TBN * normal);
    } else {
        // No normal map, use vertex normal
        normal = normalize(fs_in.Normal);
    }
    
    // Lighting setup - two lights for better material showcase
    vec3 lightColor1 = vec3(0.8);
    vec3 lightColor2 = vec3(0.4);
    vec3 lightDir1 = normalize(vec3(1.0, 1.0, 1.0));
    vec3 lightDir2 = normalize(vec3(-1.0, 0.5, -0.5));
    
    // Ambient
    vec3 ambient = 0.3 * albedo;
    
    // Diffuse lighting from both lights
    float diff1 = max(dot(lightDir1, normal), 0.0);
    float diff2 = max(dot(lightDir2, normal), 0.0);
    vec3 diffuse = (diff1 * lightColor1 + diff2 * lightColor2) * albedo;
    
    // Specular lighting (Blinn-Phong)
    vec3 viewDir = normalize(-fs_in.FragPos); // Camera at origin in view space
    
    vec3 halfwayDir1 = normalize(lightDir1 + viewDir);
    float spec1 = pow(max(dot(normal, halfwayDir1), 0.0), max(shininess, 1.0));
    
    vec3 halfwayDir2 = normalize(lightDir2 + viewDir);
    float spec2 = pow(max(dot(normal, halfwayDir2), 0.0), max(shininess, 1.0));
    
    vec3 specularLight = (spec1 * lightColor1 + spec2 * lightColor2) * specular;
    
    // Combine lighting
    vec3 lighting = ambient + diffuse + specularLight;
    
    FragColor = vec4(lighting, 1.0);
}
