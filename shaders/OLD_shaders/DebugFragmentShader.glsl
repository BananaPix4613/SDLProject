#version 330 core
out vec4 FragColor;

in vec2 TexCoord;

uniform sampler2D debugTexture;
uniform bool isDepthTexture;

void main()
{
	if (isDepthTexture)
	{
		float depthValue = texture(debugTexture, TexCoord).r;
		// Convert depth to visible range (closer = darker for visualization)
		FragColor = vec4(vec3(depthValue), 1.0);
	}
	else
	{
		FragColor = texture(debugTexture, TexCoord);
	}
}