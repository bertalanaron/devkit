#version 330 core

struct Camera {
    mat4 VP;
    vec3 position;
    vec3 direction;
}; uniform Camera u_camera;

uniform sampler2D u_grassTexture;
uniform sampler2D u_rockTexture;

in vec3 Normal;
in vec3 Position;

out vec4 FragColor;

void main()
{
    float prod = 0.0;
	
	float isUp = dot(Normal, vec3(0, 1, 0));
	vec4 textureColor;
	if (isUp > 0.8)
		textureColor = texture(u_grassTexture, Position.xz /4);
	else
		textureColor = texture(u_rockTexture, Position.xy);

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
	
	float diffuse = .9;
	if (mod(Position.x, .5) < .015 || mod(Position.z, .5) < .015)
		diffuse = .4;
    vec4 value = diffuse * (prod / 3 + .66) * textureColor;
    FragColor = vec4(value.xyz, 1.f);
}
