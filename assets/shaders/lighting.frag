#version 430 core

in vec3 fragPosition;
in vec3 fragNormal;
in vec2 fragTexCoord;
in vec4 fragColor;
in vec3 viewPosition;

uniform vec3 baseColor;
uniform float metallic;
uniform float roughness;
uniform vec3 emissive;

uniform vec3 cameraPosition;
uniform vec3 cameraDirection;
uniform vec3 ambientLight;
uniform mat4 projMatrix;
uniform mat4 viewProjMatrix;
uniform vec2 viewport;
uniform vec2 renderSize;

// Forward+ cluster uniforms (injected by ForwardPlusShaderManager)
uniform ivec3 clusterDimensions;
uniform vec3 clusterMinBounds;
uniform vec3 clusterMaxBounds;
uniform vec3 clusterSize;
uniform vec2 viewportSize;
uniform float nearPlane;
uniform float farPlane;
uniform int frameCounter;

// Legacy uniforms for fallback
uniform vec3 lightDirection;
uniform vec3 lightColor;
uniform float lightIntensity;

out vec4 fragOutput;

// Forward+ cluster data structures (binding points injected by preprocessor)
#ifdef FORWARD_PLUS_ENABLED
layout(std430, binding = CLUSTER_DATA_BINDING) readonly buffer ClusterDataBuffer {
    uint clusterOffsets[];
};

layout(std430, binding = LIGHT_GRID_BINDING) readonly buffer LightGridBuffer {
    uint lightGrid[];
};

layout(std430, binding = LIGHT_INDICES_BINDING) readonly buffer LightIndicesBuffer {
    uint lightIndices[];
};

layout(std430, binding = VISIBLE_LIGHTS_BINDING) readonly buffer VisibleLightsBuffer {
    vec4 lightPositions[];
    vec4 lightColors[];
    vec4 lightDirections[];
    vec4 lightProperties[]; // x: intensity, y: range, z: type, w: unused
};
#endif

// Light types
const int LIGHT_TYPE_DIRECTIONAL = 0;
const int LIGHT_TYPE_POINT = 1;
const int LIGHT_TYPE_SPOT = 2;

// Calculate cluster index for current fragment
int calculateClusterIndex(vec3 viewPos) {
#ifdef FORWARD_PLUS_ENABLED
    // Transform to cluster space
    vec3 clusterPos = (viewPos - clusterMinBounds) / clusterSize;
    
    ivec3 clusterCoord = ivec3(floor(clusterPos));
    
    // Clamp to cluster bounds
    clusterCoord = clamp(clusterCoord, ivec3(0), clusterDimensions - 1);
    
    // Calculate flat index
    return clusterCoord.z * (clusterDimensions.x * clusterDimensions.y) + 
           clusterCoord.y * clusterDimensions.x + clusterCoord.x;
#else
    return -1;
#endif
}

// PBR lighting calculation for pixel art aesthetics
vec3 calculatePBRLighting(vec3 normal, vec3 viewDir, vec3 lightDir, vec3 lightCol, float intensity, float attenuation) {
    // Lambertian diffuse
    float NdotL = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = lightCol * intensity * NdotL * attenuation;

    // Simple specular for pixel art (reduced intensity)
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0 * (1.0 - roughness));
    vec3 specular = lightCol * spec * metallic * 0.3 * attenuation; // Reduced specular for pixel art

    return diffuse + specular;
}

// Calculate attenuation for point/spot lights
float calculateAttenuation(float distance, float range) {
    if (range <= 0.0) return 1.0; // No attenuation for directional lights
    
    float attenuation = 1.0 / (1.0 + 0.09 * distance + 0.032 * distance * distance);
    
    // Smooth cutoff at range
    float cutoff = 1.0 - smoothstep(range * 0.8, range, distance);
    return attenuation * cutoff;
}

// Forward+ clustered lighting calculation
vec3 calculateClusteredLighting(vec3 normal, vec3 viewDir, vec3 worldPos, vec3 viewPos) {
#ifdef FORWARD_PLUS_ENABLED
    vec3 totalLighting = vec3(0.0);
    
    int clusterIndex = calculateClusterIndex(viewPos);
    if (clusterIndex < 0 || clusterIndex >= clusterDimensions.x * clusterDimensions.y * clusterDimensions.z) {
        return totalLighting; // Outside cluster bounds
    }
    
    // Get light count and offset for this cluster
    uint lightCount = lightGrid[clusterIndex * 2];
    uint lightOffset = lightGrid[clusterIndex * 2 + 1];
    
    // Limit lights per cluster for performance
    lightCount = min(lightCount, MAX_LIGHTS_PER_CLUSTER);
    
    // Process each light in this cluster
    for (uint i = 0; i < lightCount; ++i) {
        uint lightIndex = lightIndices[lightOffset + i];
        
        vec3 lightPos = lightPositions[lightIndex].xyz;
        vec3 lightCol = lightColors[lightIndex].rgb;
        vec3 lightDir = lightDirections[lightIndex].xyz;
        float lightIntensity = lightProperties[lightIndex].x;
        float lightRange = lightProperties[lightIndex].y;
        int lightType = int(lightProperties[lightIndex].z);
        
        vec3 lightContribution = vec3(0.0);
        
        if (lightType == LIGHT_TYPE_DIRECTIONAL) {
            // Directional light
            vec3 L = normalize(-lightDir);
            lightContribution = calculatePBRLighting(normal, viewDir, L, lightCol, lightIntensity, 1.0);
        }
        else if (lightType == LIGHT_TYPE_POINT) {
            // Point light
            vec3 L = lightPos - worldPos;
            float distance = length(L);
            L = normalize(L);
            
            float attenuation = calculateAttenuation(distance, lightRange);
            lightContribution = calculatePBRLighting(normal, viewDir, L, lightCol, lightIntensity, attenuation);
        }
        else if (lightType == LIGHT_TYPE_SPOT) {
            // Spot light (simplified implementation)
            vec3 L = lightPos - worldPos;
            float distance = length(L);
            L = normalize(L);
            
            float attenuation = calculateAttenuation(distance, lightRange);
            
            // Simple spot cone calculation
            float spotEffect = dot(L, normalize(-lightDir));
            float spotCutoff = 0.7; // 45 degree cone
            float spotSmoothness = 0.1;
            spotEffect = smoothstep(spotCutoff - spotSmoothness, spotCutoff + spotSmoothness, spotEffect);
            
            attenuation *= spotEffect;
            lightContribution = calculatePBRLighting(normal, viewDir, L, lightCol, lightIntensity, attenuation);
        }
        
        totalLighting += lightContribution;
    }
    
    return totalLighting;
#else
    return vec3(0.0);
#endif
}

void main() {
    vec3 normal = normalize(fragNormal);
    vec3 viewDir = normalize(cameraPosition - fragPosition);

    // Start with ambient lighting
    vec3 finalColor = ambientLight * baseColor;

#ifdef FORWARD_PLUS_ENABLED
    // Use forward+ clustered lighting
    vec3 viewPos = (mat4(mat3(viewProjMatrix)) * vec4(fragPosition - cameraPosition, 1.0)).xyz;
    finalColor += calculateClusteredLighting(normal, viewDir, fragPosition, viewPos) * baseColor;
#else
    // Fallback to simple directional light
    vec3 lightDir = normalize(-lightDirection);
    finalColor += calculatePBRLighting(normal, viewDir, lightDir, lightColor, lightIntensity, 1.0) * baseColor;
#endif

    // Apply vertex color modulation for voxel variety
    finalColor *= fragColor.rgb;

    // Add emissive contribution
    finalColor += emissive;

    // Tone down for pixel art aesthetic
    finalColor = pow(finalColor, vec3(1.0/2.2)); // Simple gamma correction

    // Output with full opacity
    fragOutput = vec4(finalColor, 1.0);
}
