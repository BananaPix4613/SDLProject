#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D sceneTexture;
uniform sampler2D bloomMip0;
uniform sampler2D bloomMip1;
uniform sampler2D bloomMip2;
uniform sampler2D bloomMip3;
uniform sampler2D bloomMip4;
uniform int mipCount;
uniform float bloomIntensity;

void main()
{
    // Sample scene texture
    vec3 scene = texture(sceneTexture, TexCoords).rgb;

    // Sample all bloom mips and combine
    vec3 bloom = vec3(0.0);

    if (mipCount > 0) bloom += texture(bloomMip0, TexCoords).rgb;
    if (mipCount > 1) bloom += texture(bloomMip1, TexCoords).rgb * 0.8;
    if (mipCount > 2) bloom += texture(bloomMip2, TexCoords).rgb * 0.6;
    if (mipCount > 3) bloom += texture(bloomMip3, TexCoords).rgb * 0.4;
    if (mipCount > 4) bloom += texture(bloomMip4, TexCoords).rgb * 0.2;

    // Apply bloom intensity
    bloom *= bloomIntensity;

    // Add bloom to scene
    vec3 result = scene + bloom;

    FragColor = vec4(result, 1.0);
}