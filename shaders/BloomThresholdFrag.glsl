#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D inputTexture;
uniform float threshold;

void main()
{
    vec3 color = texture(inputTexture, TexCoords).rgb;

    // Calculate brightness using weighted RGB
    float brightness = dot(color, vec3(0.2126, 0.7152, 0.0722));

    // Apply threshold
    if (brightness > threshold)
    {
        FragColor = vec4(color, 1.0);
    }
    else
    {
        FragColor = vec4(0.0, 0.0, 0.0, 1.0);
    }
}