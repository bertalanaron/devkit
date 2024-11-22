#version 330 core

uniform sampler2D u_texture;
struct Camera {
    mat4 VP;
    vec3 position;
    vec3 direction;
}; uniform Camera u_camera;

in vec3 Position;
in vec2 UV;

out vec4 FragColor;

void main()
{
    // vec4 eUV = vec4(UV.x, 0.0, UV.y, 1.0) * u_camera.VP;
    FragColor = texture(u_texture, UV);
}
