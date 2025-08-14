#version 430 core

// Vertex attributes
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 texCoord;
layout (location = 3) in vec4 color;

// Transformation matrices
uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;
uniform mat4 viewProjectionMatrix;

// Output to fragment shader
out vec3 fragPosition;
out vec3 fragNormal;
out vec2 fragTexCoord;
out vec4 fragColor;
out vec3 viewPosition;

void main() {
    // Transform position to world space
    vec4 worldPos = modelMatrix * vec4(position, 1.0);
    fragPosition = worldPos.xyz;

    // Transform normal to world space
    fragNormal = normalize(mat3(transpose(inverse(modelMatrix))) * normal);

    // Transform to view space for cluster calculations
    vec4 viewPos = viewMatrix * worldPos;
    viewPosition = viewPos.xyz;

    // Pass through texture coordinates and color
    fragTexCoord = texCoord;
    fragColor = color;

    // Final position
    gl_Position = viewProjectionMatrix * worldPos;
}