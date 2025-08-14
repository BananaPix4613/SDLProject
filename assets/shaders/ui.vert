#version 330 core

layout (location = 0) in vec2 position;
layout (location = 1) in vec2 texCoord;

uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;

out vec2 fragTexCoord;

void main() {
    fragTexCoord = texCoord;
    
    // Transform position through model, view, and projection matrices
    vec4 worldPos = modelMatrix * vec4(position, 0.0, 1.0);
    gl_Position = projectionMatrix * viewMatrix * worldPos;
}
