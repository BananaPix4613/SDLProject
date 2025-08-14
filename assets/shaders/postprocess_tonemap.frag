#version 330 core

in vec2 fragTexCoord;

uniform sampler2D inputTexture;
uniform float exposure;

out vec4 fragOutput;

// ACES tone mapping function for filmic look
vec3 acesToneMapping(vec3 color) {
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    
    return clamp((color * (a * color + b)) / (color * (c * color + d) + e), 0.0, 1.0);
}

// Reinhard tone mapping
vec3 reinhardTonemapping(vec3 color) {
    return color / (color + vec3(1.0));
}

// Simple gamma correction
vec3 gammaCorrection(vec3 color, float gamma) {
    return pow(color, vec3(1.0 / gamma));
}

void main() {
    vec3 hdrColor = texture(inputTexture, fragTexCoord).rgb;
    
    // Apply exposure
    hdrColor *= exposure;
    
    // Apply tone mapping (using ACES for better pixel art preservation)
    vec3 mapped = acesToneMapping(hdrColor);
    
    // Apply gamma correction
    mapped = gammaCorrection(mapped, 2.2);
    
    fragOutput = vec4(mapped, 1.0);
}
