#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D hdrTexture;
uniform float exposure;
uniform float gamma;

void main()
{
    // Sample HDR texture
    vec3 hdrColor = texture(hdrTexture, TexCoords).rgb;

    // Tone mapping (Reinhard)
    vec3 mapped = vec3(1.0) - exp(-hdrColor * exposure);

    // Gamma correction
    mapped = pow(mapped, vec3(1.0 / gamma));

    FragColor = vec4(mapped, 1.0);
}