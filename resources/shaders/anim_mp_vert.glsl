#version 330


uniform mat4 u_model;
uniform mat4 u_viewproj;

in vec3 a_position;
in vec3 a_normal;
in vec4 a_color;


void main() {
    vec4 vertex = vec4(a_position.xyz, 1.);
    gl_Position = u_viewproj * u_model * vertex;
    

}