#version 330 core

layout (location = 0) in vec3 position;

uniform mat4 lightViewProj;
uniform mat4 modelMatrix;

void main() {
    gl_Position = lightViewProj * modelMatrix * vec4(position, 1.0);
}
