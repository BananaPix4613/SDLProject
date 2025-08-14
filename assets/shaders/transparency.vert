#version 430 core

// Vertex attributes (no layout qualifiers for compatibility)
in vec3 position;
in vec3 normal;
in vec2 texCoord;
in vec4 color;

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
    mat3 normalMatrix = mat3(transpose(inverse(modelMatrix)));
    fragNormal = normalize(normalMatrix * normal);

    // Pass through texture coordinates and color
    fragTexCoord = texCoord;
    fragColor = color;

    // Transform to view space for cluster calculations
    vec4 viewPos = viewMatrix * worldPos;
    viewPosition = viewPos.xyz;

    // Final position
    gl_Position = viewProjectionMatrix * worldPos;
}
