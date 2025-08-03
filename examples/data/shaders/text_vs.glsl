#version 330 core

layout (location = 0) in vec3 a_position;
layout (location = 1) in vec2 a_uv;
layout (location = 2) in vec4 a_color;

out vec3 Position;
out vec2 UV;
out vec4 Color;

struct Camera {
    mat4 VP;
    vec3 position;
    vec3 direction;
}; 
uniform Camera u_camera;

void main()
{
    vec4 vM = vec4(a_position.xyz, 1.0);

    gl_Position = vM * u_camera.VP;

    Position = gl_Position.xyz / gl_Position.w;
    Color = a_color;
    UV = a_uv;
}
