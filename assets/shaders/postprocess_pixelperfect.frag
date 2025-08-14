#version 330 core

in vec2 fragTexCoord;

uniform sampler2D inputTexture;
uniform int pixelScale;

out vec4 fragOutput;

// Pixel perfect nearest neighbor sampling
vec3 pixelPerfectSample(sampler2D tex, vec2 uv, int scale) {
    vec2 texSize = textureSize(tex, 0);
    
    // Calculate the virtual pixel grid
    vec2 scaledSize = texSize / float(scale);
    
    // Get the pixel coordinates
    vec2 pixelCoord = floor(uv * scaledSize);
    
    // Convert back to UV coordinates with pixel center sampling
    vec2 pixelUV = (pixelCoord + 0.5) / scaledSize;
    
    return texture(tex, pixelUV).rgb;
}

// Alternative sharp scaling with optional smoothing at edges
vec3 sharpScale(sampler2D tex, vec2 uv, int scale) {
    vec2 texSize = textureSize(tex, 0);
    vec2 scaledTexSize = texSize / float(scale);
    
    // Calculate pixel positions
    vec2 pixelPos = uv * scaledTexSize;
    vec2 pixelFloor = floor(pixelPos);
    vec2 pixelFract = fract(pixelPos);
    
    // Sample the four nearest pixels
    vec2 texelSize = 1.0 / scaledTexSize;
    vec2 centerUV = (pixelFloor + 0.5) * texelSize;
    
    // For pixel perfect, we just sample the center
    return texture(tex, centerUV).rgb;
}

// Subpixel positioning for crisp scaling
vec3 crispScale(sampler2D tex, vec2 uv, int scale) {
    vec2 texSize = textureSize(tex, 0);
    
    // Calculate the effective resolution after scaling
    vec2 effectiveSize = texSize / float(scale);
    
    // Map UV to pixel grid
    vec2 pixelCoord = uv * effectiveSize;
    
    // Round to nearest pixel center
    vec2 nearestPixel = floor(pixelCoord + 0.5);
    
    // Convert back to texture coordinates
    vec2 samplingUV = nearestPixel / effectiveSize;
    
    // Clamp to valid texture bounds
    samplingUV = clamp(samplingUV, vec2(0.0), vec2(1.0));
    
    return texture(tex, samplingUV).rgb;
}

void main() {
    vec3 color;
    
    if (pixelScale <= 1) {
        // No scaling, just pass through
        color = texture(inputTexture, fragTexCoord).rgb;
    } else {
        // Apply pixel perfect scaling
        color = crispScale(inputTexture, fragTexCoord, pixelScale);
    }
    
    fragOutput = vec4(color, 1.0);
}
