#version 430 core

in vec2 TexCoords;
out vec4 FragColor;

uniform sampler2D sourceTexture;
uniform int pixelSize;
uniform bool snapToGrid;

// Palette options
uniform bool usePalette;
uniform sampler1D palette;
uniform int paletteSize;

// Dithering options
uniform bool useDithering;
uniform sampler2D ditherPattern;
uniform float ditherStrength;

void main()
{
    // Calculate pixelated texture coordinates
    vec2 texSize = textureSize(sourceTexture, 0);
    vec2 pixelTexSize = texSize / pixelSize;

    vec2 pixelatedCoords;
    if (snapToGrid)
    {
        // Snap to pixel grid
        pixelatedCoords = floor(TexCoords * pixelTexSize) / pixelTexSize;
    }
    else
    {
        // Center sample in pixel
        pixelatedCoords = (floor(TexCoords * pixelTexSize) + 0.5) / pixelTexSize;
    }

    // Sample the texture at the quantized coordinates
    vec4 color = texture(sourceTexture, pixelatedCoords);

    // Apply palette mapping if enabled
    if (usePalette)
    {
        // Calculate luminance for mapping
        float luminance = dot(color.rgb, vec3(0.2126, 0.7152, 0.0722));

        // Apply dithering to luminance if enabled
        if (useDithering)
        {
            // Get screen position for dither pattern
            ivec2 screenPos = ivec2(gl_FragCoord.xy);
            vec2 patternSize = textureSize(ditherPattern, 0);

            // Sample dither pattern with wrap
            vec2 patternCoord = vec2(screenPos) / patternSize;
            float ditherValue = texture(ditherPattern, patternCoord).r;

            // Apply dither to luminance
            luminance += (ditherValue - 0.5) * ditherStrength;
            luminance = clamp(luminance, 0.0, 1.0);
        }

        // Map to palette based on luminance
        float paletteIndex = floor(luminance * (paletteSize - 1) + 0.5) / (paletteSize - 1);
        vec4 paletteColor = texture(oalette, paletteIndex);

        // Apply palette color while preserving alpha
        color.rgb = paletteColor.rgb;
    }

    FragColor = color;
}