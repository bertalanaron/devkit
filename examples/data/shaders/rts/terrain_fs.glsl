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
	
	float diffuse = .15;
	if (mod(Position.x, .5) < .015 || mod(Position.z, .5) < .015)
		diffuse = .1;
    float value = diffuse + prod / 8;
    FragColor = vec4(value, value, value, 1.f);
}
