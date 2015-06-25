#version 150
uniform sampler2D	uTextureSampler;
uniform float		uWhiteMax = 1.0;
uniform float		uWhiteMid = 0.5;
uniform float		uWhiteThreshold = 0.25;

in vec2 UV;
out vec4 FragColor;

void main()
{
	vec4 sample = texture(uTextureSampler, UV);
	float a = sample.a;
	sample.rgb *= (uWhiteMid / uWhiteMax );
	sample.rgb *= 1.0 + (sample.rgb / (uWhiteThreshold * uWhiteThreshold) );
	sample.rgb -= 0.5;
	sample.rgb /=(1.0 + sample.rgb);

	FragColor = vec4(sample.rgb, a);
}
