#version 430 core
in vec3 fragColor;
out vec4 FragColor;

void main()
{
    FragColor = vec4(fragColor, 0.3); // Semi-transparent for visualization
}