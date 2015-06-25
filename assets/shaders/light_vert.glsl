#version 150
uniform mat4 ciModelViewProjection;
in vec4 ciPosition;

in vec3 iPosition;
in vec2	iTexCoord;
in vec3 iColor;

out vec2 UV;
out vec3 Color;
void main()
{
	Color = iColor;
	UV = iTexCoord;
	gl_Position = ciModelViewProjection*(ciPosition+vec4(iPosition,1.0));
}