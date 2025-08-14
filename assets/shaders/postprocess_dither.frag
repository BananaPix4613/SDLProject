#version 330 core

in vec2 fragTexCoord;

uniform sampler2D inputTexture;
uniform float ditheringStrength;

out vec4 fragOutput;

// 8x8 Bayer dithering matrix for better quality
float bayerMatrix8x8(int x, int y) {
    int index = (y & 7) * 8 + (x & 7);
    
    float matrix[64] = float[](
         0.0/64.0, 32.0/64.0,  8.0/64.0, 40.0/64.0,  2.0/64.0, 34.0/64.0, 10.0/64.0, 42.0/64.0,
        48.0/64.0, 16.0/64.0, 56.0/64.0, 24.0/64.0, 50.0/64.0, 18.0/64.0, 58.0/64.0, 26.0/64.0,
        12.0/64.0, 44.0/64.0,  4.0/64.0, 36.0/64.0, 14.0/64.0, 46.0/64.0,  6.0/64.0, 38.0/64.0,
        60.0/64.0, 28.0/64.0, 52.0/64.0, 20.0/64.0, 62.0/64.0, 30.0/64.0, 54.0/64.0, 22.0/64.0,
         3.0/64.0, 35.0/64.0, 11.0/64.0, 43.0/64.0,  1.0/64.0, 33.0/64.0,  9.0/64.0, 41.0/64.0,
        51.0/64.0, 19.0/64.0, 59.0/64.0, 27.0/64.0, 49.0/64.0, 17.0/64.0, 57.0/64.0, 25.0/64.0,
        15.0/64.0, 47.0/64.0,  7.0/64.0, 39.0/64.0, 13.0/64.0, 45.0/64.0,  5.0/64.0, 37.0/64.0,
        63.0/64.0, 31.0/64.0, 55.0/64.0, 23.0/64.0, 61.0/64.0, 29.0/64.0, 53.0/64.0, 21.0/64.0
    );
    
    return matrix[index];
}

// Quantize color to simulate reduced bit depth
vec3 quantizeColor(vec3 color, float levels) {
    return floor(color * levels) / levels;
}

// Floyd-Steinberg dithering approximation
vec3 applyDithering(vec3 color, vec2 screenPos) {
    int x = int(screenPos.x);
    int y = int(screenPos.y);
    
    // Get dither threshold from Bayer matrix
    float ditherValue = bayerMatrix8x8(x, y);
    
    // Apply dithering with strength control
    vec3 ditheredColor = color + (ditherValue - 0.5) * ditheringStrength;
    
    return clamp(ditheredColor, 0.0, 1.0);
}

void main() {
    vec3 color = texture(inputTexture, fragTexCoord).rgb;
    
    // Apply dithering
    vec3 ditheredColor = applyDithering(color, gl_FragCoord.xy);
    
    // Optionally quantize to simulate lower bit depth
    // Uncomment for more aggressive retro look:
    // ditheredColor = quantizeColor(ditheredColor, 32.0);
    
    fragOutput = vec4(ditheredColor, 1.0);
}
