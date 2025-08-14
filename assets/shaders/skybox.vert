#version 330 core

layout (location = 0) in vec3 aPos;

out vec3 TexCoords;

uniform mat4 viewMatrix;
uniform mat4 projMatrix;

void main() {
    TexCoords = aPos;
    vec4 pos = projMatrix * viewMatrix * vec4(aPos, 1.0);
    // Ensure the skybox is always at minimum depth
    gl_Position = pos.xyzw;
}
