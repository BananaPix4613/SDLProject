#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;
in vec4 FragPosLightSpace;

uniform vec3 lightPos;
uniform vec3 viewPos;
uniform vec3 lightColor;
uniform vec3 color;

uniform float ambientStrength;
uniform float specularStrength;
uniform float shininess;
uniform float shadowBias;
uniform bool usePCF;

uniform sampler2D shadowMap;

float ShadowCalculation(vec4 fragPosLightSpace)
{
    // Perform perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;

    // Transform to [0,1] range
    projCoords = projCoords * 0.5 + 0.5;

    // Get closest depth value from light's perspective
    float closestDepth = texture(shadowMap, projCoords.xy).r;

    // Get current depth value
    float currentDepth = projCoords.z;

    // Calculate dynamic bias based on surface normal and light direction
    vec3 normal = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float bias = max(shadowBias * (1.0 - dot(normal, lightDir)), shadowBias * 0.05);

    // PCF (percentage-closer filtering)
    float shadow = 0.0;
    if (usePCF)
    {
        vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
        for (int x = -2; x <= 2; x++)
        {
            for (int y = -2; y <= 2; y++)
            {
                float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
                shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
            }
        }
        shadow /= 25.0; // 5x5 kernel
    }
    else
    {
        // Standard shadow mapping
        shadow = currentDepth - bias > closestDepth ? 1.0 : 0.0;
    }

    // No shadow outside of the light's projection
    if (projCoords.z > 1.0)
        shadow = 0.0;

    return shadow;
}

void main()
{
    // Ambient
    vec3 ambient = ambientStrength * lightColor;
    
    // Diffuse
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;
    
    // Specular
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(Normal, halfwayDir), 0.0), shininess);
    vec3 specular = specularStrength * spec * lightColor;
    
    // Calculate shadow
    float shadow = ShadowCalculation(FragPosLightSpace);

    // Edge highlighting (optional)
    float edgeStrength = 0.0;
    float edge = 1.0 - max(dot(viewDir, Normal), 0.0);
    edge = pow(edge, 2.0) * edgeStrength;
    
    // Simple attenuation
    float constant = 1.0;
    float linear = 0.09;
    float quadratic = 0.032;
    float distance = length(lightPos - FragPos);
    float attenuation = 1.0 / (constant + linear * distance + quadratic * distance * distance);

    // Apply shadow and attenuation
    ambient *= attenuation;
    // Only diffuse and specular are affected by shadows
    diffuse *= (1.0 - shadow) * attenuation;
    specular *= (1.0 - shadow) * attenuation;

    // Combine lighting
    vec3 result = (ambient + diffuse + specular) * color;
    result = pow(result, vec3(1.0/2.2)); // Gamma correction

    FragColor = vec4(result, 1.0);
}