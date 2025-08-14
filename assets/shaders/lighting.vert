#version 330 core

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 texCoord;
layout (location = 3) in vec4 color;

uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projMatrix;
uniform mat4 viewProjMatrix;
uniform mat3 normalMatrix;
uniform vec3 cameraPosition;
uniform vec3 cameraDirection;
uniform vec2 viewport;
uniform vec2 renderSize;
uniform ivec3 clusterDimensions;
uniform int shadowMappingEnabled;
uniform int clusteredLightingEnabled;

out vec3 fragPosition;
out vec3 fragNormal;
out vec2 fragTexCoord;
out vec4 fragColor;
out vec3 viewPosition;

void main() {
    // Transform position to world space
    vec4 worldPos = modelMatrix * vec4(position, 1.0);
    fragPosition = worldPos.xyz;

    // Transform normal to world space using normal matrix
    fragNormal = normalize(normalMatrix * normal);

    // Pass through texture coordinates and color
    fragTexCoord = texCoord;
    fragColor = color;

    // View space position for lighting calculations
    viewPosition = (viewMatrix * worldPos).xyz;

    // Final position
    gl_Position = viewProjMatrix * worldPos;
}
