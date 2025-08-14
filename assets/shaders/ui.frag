#version 330 core

in vec2 fragTexCoord;

uniform sampler2D uiTexture;
uniform vec4 uiColor;
uniform int hasTexture;

out vec4 fragOutput;

void main() {
    vec4 finalColor = uiColor;
    
    if (hasTexture == 1) {
        vec4 texColor = texture(uiTexture, fragTexCoord);
        finalColor *= texColor;
    }
    
    fragOutput = finalColor;
}
