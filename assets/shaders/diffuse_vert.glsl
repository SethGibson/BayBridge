#version 150
uniform mat4 ciModelViewProjection;
uniform mat4 ciModelMatrix;

in vec4 ciPosition;
in vec3 ciNormal;

out vec3 Normal;
out vec3 WorldPos;

void main()
{
	mat4 normalMatrix = mat4(mat3(transpose(inverse(ciModelMatrix))));
	Normal = vec3(normalMatrix * vec4(ciNormal,1.0));
	WorldPos = vec3(ciModelMatrix*ciPosition);
	gl_Position = ciModelViewProjection*ciPosition;
}