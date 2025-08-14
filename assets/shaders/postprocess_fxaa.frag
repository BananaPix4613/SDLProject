#version 330 core

in vec2 fragTexCoord;

uniform sampler2D inputTexture;

out vec4 fragOutput;

// FXAA settings
#define FXAA_EDGE_THRESHOLD_MIN 0.0312
#define FXAA_EDGE_THRESHOLD 0.125
#define FXAA_SUBPIX_TRIM 0.25
#define FXAA_SUBPIX_TRIM_SCALE (1.0/(1.0 - FXAA_SUBPIX_TRIM))
#define FXAA_SUBPIX_CAP 0.75
#define FXAA_SEARCH_STEPS 12
#define FXAA_SEARCH_ACCELERATION 1
#define FXAA_SEARCH_THRESHOLD 0.25

// Luminance calculation
float luma(vec3 color) {
    return dot(color, vec3(0.299, 0.587, 0.114));
}

vec3 fxaa(sampler2D tex, vec2 fragCoord, vec2 resolution) {
    vec2 inverseVP = 1.0 / resolution;
    vec3 rgbNW = texture(tex, fragCoord + vec2(-1.0, -1.0) * inverseVP).xyz;
    vec3 rgbNE = texture(tex, fragCoord + vec2(1.0, -1.0) * inverseVP).xyz;
    vec3 rgbSW = texture(tex, fragCoord + vec2(-1.0, 1.0) * inverseVP).xyz;
    vec3 rgbSE = texture(tex, fragCoord + vec2(1.0, 1.0) * inverseVP).xyz;
    vec3 rgbM  = texture(tex, fragCoord).xyz;

    float lumaNW = luma(rgbNW);
    float lumaNE = luma(rgbNE);
    float lumaSW = luma(rgbSW);
    float lumaSE = luma(rgbSE);
    float lumaM  = luma(rgbM);

    float lumaMin = min(lumaM, min(min(lumaNW, lumaNE), min(lumaSW, lumaSE)));
    float lumaMax = max(lumaM, max(max(lumaNW, lumaNE), max(lumaSW, lumaSE)));

    vec2 dir;
    dir.x = -((lumaNW + lumaNE) - (lumaSW + lumaSE));
    dir.y =  ((lumaNW + lumaSW) - (lumaNE + lumaSE));

    float dirReduce = max((lumaNW + lumaNE + lumaSW + lumaSE) * (0.25 * FXAA_SUBPIX_TRIM), FXAA_EDGE_THRESHOLD_MIN);
    float rcpDirMin = 1.0 / (min(abs(dir.x), abs(dir.y)) + dirReduce);

    dir = min(vec2(FXAA_SEARCH_STEPS, FXAA_SEARCH_STEPS),
          max(vec2(-FXAA_SEARCH_STEPS, -FXAA_SEARCH_STEPS),
          dir * rcpDirMin)) * inverseVP;

    vec3 rgbA = 0.5 * (
        texture(tex, fragCoord + dir * (1.0/3.0 - 0.5)).xyz +
        texture(tex, fragCoord + dir * (2.0/3.0 - 0.5)).xyz);
    vec3 rgbB = rgbA * 0.5 + 0.25 * (
        texture(tex, fragCoord + dir * -0.5).xyz +
        texture(tex, fragCoord + dir * 0.5).xyz);

    float lumaB = luma(rgbB);

    if ((lumaB < lumaMin) || (lumaB > lumaMax)) {
        return rgbA;
    } else {
        return rgbB;
    }
}

void main() {
    vec2 resolution = textureSize(inputTexture, 0);
    vec3 antialiasedColor = fxaa(inputTexture, fragTexCoord, resolution);
    
    fragOutput = vec4(antialiasedColor, 1.0);
}
