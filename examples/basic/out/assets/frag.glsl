#version 330 core

uniform sampler2D u_texture;

in vec3 Position;
in vec2 UV;

out vec4 FragColor;

void main()
{
    FragColor = texture(u_texture, UV);
}
