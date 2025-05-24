#version 330 core
in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;
in vec3 Tangent;
in vec3 Bitangent;

out vec4 FragColor;

uniform sampler2D diffuseTexture;
uniform sampler2D specularTexture;
uniform sampler2D normalTexture;

// Hardcoded directional light (sun)
const vec3 lightDir = normalize(vec3(-0.7, -0.7, -0.25));
const vec3 lightColor = vec3(1.0, 1.0, 1.0);
const float lightIntensity = 1.0;

// Material properties
const vec3 ambientColor = vec3(0.1, 0.1, 0.1);
const float shininess = 32.0;

void main()
{
    // Sample textures
    vec4 diffuseSample = texture(diffuseTexture, TexCoord);
    vec3 diffuseColor = diffuseSample.rgb;
    if (diffuseSample.a < 0.5) {
        discard;
    }
    vec3 specularColor = texture(specularTexture, TexCoord).rgb;

    // Calculate normal in view space
    vec3 normal = normalize(Normal);

    // Ambient
    vec3 ambient = ambientColor * diffuseColor;

    // Diffuse
    float diff = max(dot(normal, -lightDir), 0.0);
    vec3 diffuse = lightColor * diff * diffuseColor * lightIntensity;

    // Specular (Blinn-Phong)
    vec3 viewDir = normalize(-FragPos);
    vec3 halfwayDir = normalize(-lightDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), shininess);
    vec3 specular = lightColor * spec * specularColor * lightIntensity;

    // Combine all lighting components
    vec3 result = ambient + diffuse + specular;
    FragColor = vec4(result, 1.0);
} 