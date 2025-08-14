#version 330 core

in vec2 fragTexCoord;

uniform sampler2D inputTexture;
uniform sampler2D paletteTexture;
uniform int paletteSize;
uniform float ditherStrength;

out vec4 fragOutput;

// Find closest color in palette
vec3 findNearestPaletteColor(vec3 color) {
    float minDistance = 100.0;
    vec3 nearestColor = color;
    
    // Sample through the palette texture
    for (int i = 0; i < paletteSize; i++) {
        float u = (float(i) + 0.5) / float(paletteSize);
        vec3 paletteColor = texture(paletteTexture, vec2(u, 0.5)).rgb;
        
        // Calculate distance in RGB space
        float distance = length(color - paletteColor);
        
        if (distance < minDistance) {
            minDistance = distance;
            nearestColor = paletteColor;
        }
    }
    
    return nearestColor;
}

// Simple dithering pattern
float dither(vec2 coord) {
    // 4x4 Bayer matrix
    int x = int(coord.x) % 4;
    int y = int(coord.y) % 4;
    
    float matrix[16] = float[](
        0.0/16.0,  8.0/16.0,  2.0/16.0, 10.0/16.0,
       12.0/16.0,  4.0/16.0, 14.0/16.0,  6.0/16.0,
        3.0/16.0, 11.0/16.0,  1.0/16.0,  9.0/16.0,
       15.0/16.0,  7.0/16.0, 13.0/16.0,  5.0/16.0
    );
    
    return matrix[y * 4 + x];
}

void main() {
    vec3 color = texture(inputTexture, fragTexCoord).rgb;
    
    // Add dithering noise before palette mapping
    float ditherNoise = dither(gl_FragCoord.xy) * ditherStrength;
    color += vec3(ditherNoise);
    
    // Map to nearest palette color
    vec3 paletteColor = findNearestPaletteColor(color);
    
    fragOutput = vec4(paletteColor, 1.0);
}
