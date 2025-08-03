#version 330 core

struct Camera {
    mat4 VP;
    vec3 position;
    vec3 direction;
}; uniform Camera u_camera;

in vec3 Normal;
in vec3 Position;

out vec4 FragColor;

void main()
{
    float prod = 0.0;

    // PERSPECTIVE
    if (u_camera.VP[3][3] == 1.0) {
        prod = dot(normalize(-u_camera.direction), normalize(Normal));
    }
    // ORTHOGRAPHIC
    else {
        prod = dot(normalize(u_camera.position - Position), normalize(Normal));
    }
    
    if (prod < 0)
        prod *= -1;
    //float diffuse = prod;
    float diffuse = .45 + prod / 3;

    //FragColor = vec4(Position.x - round(Position.x), Position.y - round(Position.y), Position.z - round(Position.z), 1.f);
    FragColor = vec4(diffuse, diffuse, diffuse, 1.f);
}
