#version 330 core

// Shadow mapping fragment shader - outputs depth values only
// No color output needed since we're only interested in depth

void main() {
    // OpenGL automatically writes gl_FragCoord.z to the depth buffer
    // No explicit output needed for depth-only rendering
}
