#version 420

layout (binding = 0) uniform sampler2D diffuseTexture;

in vec2 UV;

void main()
{
    if (texture(diffuseTexture, UV).a < 0.1f){
        //discard;
    }
}