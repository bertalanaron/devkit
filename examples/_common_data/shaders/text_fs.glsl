#version 330 core

in vec3 Position;
in vec2 UV;
in vec4 Color;

out vec4 FragColor;

uniform sampler2D u_atlas;

void main()
{
    vec4 color = Color * texture(u_atlas, UV).r;

    if (color.a == 0.0)
        discard;

    FragColor = color;
}
