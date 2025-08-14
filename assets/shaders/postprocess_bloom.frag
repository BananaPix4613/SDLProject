#version 330 core

in vec2 fragTexCoord;

uniform sampler2D inputTexture;
uniform float threshold;
uniform float intensity;

out vec4 fragOutput;

// Luminance calculation
float luminance(vec3 color) {
    return dot(color, vec3(0.299, 0.587, 0.114));
}

// Simple box blur for bloom effect
vec3 blur(sampler2D tex, vec2 texCoord, vec2 texelSize) {
    vec3 result = vec3(0.0);
    float weight = 0.0;
    
    // 5x5 kernel
    for (int x = -2; x <= 2; x++) {
        for (int y = -2; y <= 2; y++) {
            vec2 offset = vec2(float(x), float(y)) * texelSize;
            vec3 sample = texture(tex, texCoord + offset).rgb;
            
            // Gaussian-like weight
            float w = exp(-0.5 * (x*x + y*y));
            result += sample * w;
            weight += w;
        }
    }
    
    return result / weight;
}

void main() {
    vec3 color = texture(inputTexture, fragTexCoord).rgb;
    
    // Extract bright areas above threshold
    float lum = luminance(color);
    vec3 brightColor = vec3(0.0);
    
    if (lum > threshold) {
        // Soft threshold to avoid hard cutoff
        float softThreshold = threshold + 0.1;
        float bloomAmount = smoothstep(threshold, softThreshold, lum);
        brightColor = color * bloomAmount;
    }
    
    // Apply blur to bright areas
    vec2 texelSize = 1.0 / textureSize(inputTexture, 0);
    vec3 blurredBright = blur(inputTexture, fragTexCoord, texelSize * 2.0);
    
    // Only use the bright parts for bloom
    if (luminance(blurredBright) > threshold) {
        brightColor = blurredBright * intensity;
    }
    
    // Output only the bloom contribution
    fragOutput = vec4(brightColor, 1.0);
}
