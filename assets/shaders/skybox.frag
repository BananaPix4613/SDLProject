#version 330 core

out vec4 FragColor;

in vec3 TexCoords;

uniform samplerCube skybox;
uniform float exposure;
uniform vec3 gradientTop;
uniform vec3 gradientBottom;

void main() {
    vec4 color = texture(skybox, TexCoords);
    
    // Apply exposure
    color.rgb *= exposure;
    
    // Use gradientTop and gradientBottom to prevent optimization (minimal impact)
    // In a real implementation, these would be used for procedural gradient skyboxes
    color.rgb += (gradientTop + gradientBottom) * 0.0001;
    
    FragColor = color;
}
