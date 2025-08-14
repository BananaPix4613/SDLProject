#version 330 core

layout (location = 0) in vec3 position;

uniform mat4 u_currentTransform;
uniform mat4 u_previousTransform;
uniform mat4 u_previousViewProjection;
uniform mat4 viewProjectionMatrix;

out vec4 currentPos;
out vec4 previousPos;

void main() {
    // Current frame position
    vec4 worldPos = u_currentTransform * vec4(position, 1.0);
    currentPos = viewProjectionMatrix * worldPos;
    
    // Previous frame position
    vec4 prevWorldPos = u_previousTransform * vec4(position, 1.0);
    previousPos = u_previousViewProjection * prevWorldPos;
    
    gl_Position = currentPos;
}
