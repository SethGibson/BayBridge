#version 150
uniform vec3 uLightPos = vec3(100,100,-100);
uniform vec3 uColor;
uniform float uAmbient;

in vec3 Normal;
in vec3 WorldPos;

out vec4 FragColor;

void main()
{
	vec3 normal = normalize(Normal);
	vec3 lightDir = normalize(uLightPos-WorldPos);

	float diffContrib = max(dot(normal,lightDir),0.0);
	vec4 diffColor = vec4(uColor,1.0)*diffContrib;
	vec4 ambColor = vec4(0.2, 0.4, 0.9,1.0)*0.15;

	FragColor = diffColor+ambColor;
}