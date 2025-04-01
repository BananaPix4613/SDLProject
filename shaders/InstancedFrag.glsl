#version 330 core
out vec4 FragColor;

// Fragment inputs
in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

// Camera and view settings
uniform vec3 viewPos;

// Ambient light settings
struct AmbientLight
{
    vec3 color;
    float intensity;
};
uniform vec4 FragPosLightSpace;

// Light structure definitions
struct Light
{
    int type;            // 0 = directional, 1 = point, 2 = spot
    bool enabled;

    // Common parameters
    vec3 color;
    float intensity;
    bool castShadows;
    mat4 lightSpaceMatrix;
    sampler2D shadowMap;

    // Directional light parameters
    vec3 direction;      // Used by directional and spot lights

    // Point and spot light parameters
    vec3 position;       // Used by point and spot lights
    float range;         // Used by point and spot lights
    vec3 attenuation;    // constant, linear, quadratic components

    // Spot light parameters
    float cutoff;        // Inner cutoff (cosine of angle)
    float outerCutoff;   // Outer cutoff (cosine of angle)
}

// Light array and count
#define MAX_LIGHTS 8
uniform Light lights[MAX_LIGHTS];
uniform int lightCount;

// Output
out vec4 FragColor;

float calculateShadow(vec4 fragPosLightSpace, sampler2D shadowMap, vec3 normal, vec3 lightDir)
{
    // Perform perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    
    // Transform to [0,1] range
    projCoords = projCoords * 0.5 + 0.5;
    
    // Check if position is outside of shadow frustum
    if (projCoords.x < 0.0 || projCoords.x > 1.0 ||
        projCoords.y < 0.0 || projCoords.y > 1.0 ||
        projCoords.z < 0.0 || projCoords.z > 1.0)
    {
        return 0.0; // No shadow outside frustum
    }
    
    // Get depth of current fragment from light's perspective
    float currentDepth = projCoords.z;
    
    // Apply bias to avoid shadow acne
    float bias = max(0.05 * (1.0 - dot(normal, lightDir)), 0.005);
    
    // PCF (Percentage Closer Filtering)
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);

    for(int x = -1; x <= 1; x++) {
        for(int y = -1; y <= 1; y++) {
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
        }
    }
    shadow /= 9.0; // Average of the 9 samples
    
    return shadow;
}

vec3 calculateDirectionalLight(Light light, vec3 normal, vec3 viewDir)
{
    vec3 lightDir = normalize(-light.direction);

    // Diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);

    // Specular shading
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), 32.0);

    // Combine results
    vec3 diffuse = light.color * diff * light.intensity;
    vec3 specular = light.color * spec * light.intensity * 0.3; // Reduced specular for voxels

    // Calculate shadow if enabled
    float shadow = 0.0;
    if (light.castShadows)
    {
        vec4 fragPosLightSpace
    }
}

void main()
{
    // Ambient lighting
    vec3 ambient = ambientStrength * lightColor;
    
    // Diffuse lighting
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;
    
    // Specular lighting
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
    vec3 specular = specularStrength * spec * lightColor;
    
    // Calculate shadow
    float shadow = enableShadows ? ShadowCalculation(FragPosLightSpace) : 0.0;
    vec3 lighting = (ambient + (1.0 - shadow) * (diffuse + specular));
    
    // Use instance color
    vec3 objectColor = Color;
    
    FragColor = vec4(lighting * objectColor, 1.0);
}