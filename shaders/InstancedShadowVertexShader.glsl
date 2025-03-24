#version 330 core
layout (location = 0) in vec3 aPos;
// Instance attributes (model matrix - 4 vec4s)
layout (location = 3) in vec4 aModel1;
layout (location = 4) in vec4 aModel2;
layout (location = 5) in vec4 aModel3;
layout (location = 6) in vec4 aModel4;

uniform mat4 lightSpaceMatrix;

void main()
{
	// Reconstruct model matrix from instance attributes
	mat4 model = mat4(
		aModel1,
		aModel2,
		aModel3,
		aModel4
	);

	gl_Position = lightSpaceMatrix * model * vec4(aPos, 1.0);
}