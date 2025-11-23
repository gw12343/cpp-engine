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




void main()
{
    float texScale = 1.0f;
    vec4 smp = texture(diffuseTexture, fs_in.TexCoords * texScale);
    vec3 color = smp.rgb;
    float alpha = smp.a;
    
    // Fallback for models without material data
    // If texture is very dark (likely missing/invalid), use default gray
    if (length(color) < 0.1) {
        color = vec3(0.7); // Light gray
        alpha = 1.0;
    }
    
    if (alpha < 0.5f){
        discard;
    }
    vec3 normal = normalize(fs_in.Normal);

    vec3 lightColor = vec3(0.3);
    vec3 ambient = 0.3 * color;

    vec3 lightDir = vec3(0.099, 0.990, 0.099);

    float diff = max(dot(lightDir, normal), 0.0);
    vec3 diffuse = diff * lightColor;


    vec3 viewPos = vec3(0, 0, 0);

    vec3 viewDir = normalize(viewPos - fs_in.FragPos);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(normal, normalize(lightDir + viewDir)), 0.0), 64.0);
    vec3 specular = spec * lightColor;



    vec3 lighting = (ambient + (diffuse + specular)) * color;


    FragColor = vec4(lighting, 1.0);
}