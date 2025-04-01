#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoords;
out vec4 FragPosLightSpace;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 lightSpaceMatrix;

void main()
{
    // Calculate fragment position in world space
    FragPos = vec3(model * vec4(aPos, 1.0));

    // Calculate normal with normal matrix (transpose of inverse of model matrix)
    // This handles non-uniform scaling correctly
    Normal = mat3(transpose(inverse(model))) * aNormal;

    // Pass texture coordinates
    TexCoords = aTexCoords;

    // Calculate fragment position in light space for shadow mapping
    FragPosLightSpace = lightSpaceMatrix * vec4(FragPos, 1.0);

    // Calculate final vertex position
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}