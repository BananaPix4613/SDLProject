#version 330 core

layout (location = 0) in vec3 position;

uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projMatrix;
uniform mat4 viewProjMatrix;
uniform vec3 cameraPosition;
uniform vec3 cameraDirection;
uniform vec2 viewport;
uniform vec2 renderSize;

void main() {
    gl_Position = viewProjMatrix * modelMatrix * vec4(position, 1.0);
}
