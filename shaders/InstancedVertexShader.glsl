#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
// Instance attributes (model matrix - 4 vec4s)
layout (location = 3) in vec4 aModel1;
layout (location = 4) in vec4 aModel2;
layout (location = 5) in vec4 aModel3;
layout (location = 6) in vec4 aModel4;
// Instance color
layout (location = 7) in vec3 aColor;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoords;
out vec4 FragPosLightSpace;
out vec3 InstanceColor;

uniform mat4 view;
uniform mat4 projection;
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

    // Calculate fragment position in world space
    FragPos = vec3(model * vec4(aPos, 1.0));

    // Calculate normal with normal matrix
    mat3 normalMatrix = mat3(transpose(inverse(model)));
    Normal = normalMatrix * aNormal;

    // Pass texture coordinates
    TexCoords = aTexCoords;

    // Pass instance color to fragment shader
    InstanceColor = aColor;

    // Calculate fragment position in light space for shadow mapping
    FragPosLightSpace = lightSpaceMatrix * vec4(FragPos, 1.0);

    // Calculate final vertex position
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}