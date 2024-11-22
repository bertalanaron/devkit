#version 330 core

layout (location = 0) in vec3 vertex;
layout (location = 1) in vec2 uv;

struct Camera {
    mat4 VP;
    vec3 position;
    vec3 direction;
}; uniform Camera u_camera;

out vec3 Position;
out vec2 UV;

void main()
{
    // Transform the vertex to world space and then to clip space
    vec4 worldPosition = vec4(vertex, 1.0);
    vec4 transformed = u_camera.VP * worldPosition;
    gl_Position = u_camera.VP * worldPosition;

    // Transform UV coordinates based on screen-space position
    vec4 screenSpace = transformed / transformed.w; // NDC [-1, 1]
    UV = 0.5 * screenSpace.xy + 0.5;                // Map to [0, 1]

    // vec4 worldPosition = vec4(vertex, 1.0);
    // gl_Position = worldPosition;
    // UV = uv; 
}
