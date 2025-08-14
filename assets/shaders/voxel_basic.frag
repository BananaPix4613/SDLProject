#version 430 core

// Input from vertex shader
in vec3 fragPosition;
in vec3 fragNormal;
in vec2 fragTexCoord;
in vec4 fragColor;
in vec3 viewPosition;

// Material properties
uniform vec3 baseColor;
uniform float metallic;
uniform float roughness;
uniform vec3 emissive;

// Camera and viewport
uniform vec3 cameraPosition;
uniform vec2 viewportSize;

// ============================================================================
// FORWARD+ CLUSTER UNIFORMS AND DATA STRUCTURES
// Note: These defines will be added by ForwardPlusShaderManager:
// #define FORWARD_PLUS_ENABLED
// #define CLUSTER_COUNT_X 16
// #define CLUSTER_COUNT_Y 9  
// #define CLUSTER_COUNT_Z 24
// #define MAX_LIGHTS_PER_CLUSTER 64
// #define CLUSTER_DATA_BINDING 0
// #define LIGHT_GRID_BINDING 1
// #define LIGHT_INDICES_BINDING 2
// #define VISIBLE_LIGHTS_BINDING 3
// ============================================================================

#ifdef FORWARD_PLUS_ENABLED

// Cluster uniforms (set by updateClusterUniforms)
uniform ivec3 clusterDimensions;
uniform vec3 clusterMinBounds;
uniform vec3 clusterMaxBounds;
uniform vec3 clusterSize;
uniform int frameCounter;

// Light data structure (matches C++ LightData)
struct LightData {
    vec4 position;       // w component: light type (0=directional, 1=point, 2=spot)
    vec4 direction;      // w component: inner cone angle (for spot lights)
    vec4 color;          // w component: intensity
    vec4 attenuation;    // x=constant, y=linear, z=quadratic, w=outer cone angle
    vec4 metadata;       // x=shadowIndex, y=castShadows, z=unused, w=unused
};

// Cluster data structure
struct ClusterData {
    vec3 minBounds;
    vec3 maxBounds;
    vec3 clusterSize;
    ivec3 dimensions;
};

// Shader storage buffers for cluster data (conservative binding)
layout(std430, binding = 0) readonly buffer ClusterDataBuffer {
    ClusterData clusterData;
};

layout(std430, binding = 1) readonly buffer LightGridBuffer {
    uint lightGrid[];
};

layout(std430, binding = 2) readonly buffer LightIndicesBuffer {
    uint lightIndices[];
};

layout(std430, binding = 3) readonly buffer VisibleLightsBuffer {
    LightData visibleLights[];
};

// Calculate cluster index from view space position
uint calculateClusterIndex(vec3 viewPos) {
    // Convert view position to cluster coordinates
    vec3 clusterPos = (viewPos - clusterData.minBounds) / clusterData.clusterSize;
    
    // Clamp to valid range
    ivec3 clusterCoord = ivec3(clamp(clusterPos, vec3(0.0), vec3(clusterData.dimensions - 1)));
    
    // Convert 3D cluster coordinate to 1D index
    return uint(clusterCoord.z * (clusterData.dimensions.x * clusterData.dimensions.y) +
                clusterCoord.y * clusterData.dimensions.x +
                clusterCoord.x);
}

// Simple PBR lighting calculation for a single light
vec3 calculateLighting(LightData light, vec3 worldPos, vec3 normal, vec3 albedo, vec3 viewDir) {
    vec3 lightContrib = vec3(0.0);
    
    if (light.position.w == 0.0) {
        // Directional light
        vec3 lightDir = normalize(-light.direction.xyz);
        float NdotL = max(dot(normal, lightDir), 0.0);
        
        // Basic PBR diffuse
        vec3 diffuse = light.color.rgb * light.color.w * NdotL;
        
        // Simple specular using metallic and roughness
        vec3 reflectDir = reflect(-lightDir, normal);
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), (1.0 - roughness) * 128.0);
        vec3 specular = light.color.rgb * spec * metallic * light.color.w;
        
        lightContrib = diffuse + specular;
    }
    else if (light.position.w == 1.0) {
        // Point light
        vec3 lightDir = light.position.xyz - worldPos;
        float distance = length(lightDir);
        lightDir = normalize(lightDir);
        
        float NdotL = max(dot(normal, lightDir), 0.0);
        
        // Attenuation
        float attenuation = 1.0 / (light.attenuation.x + 
                                  light.attenuation.y * distance + 
                                  light.attenuation.z * distance * distance);
        
        // Basic PBR diffuse
        vec3 diffuse = light.color.rgb * light.color.w * NdotL * attenuation;
        
        // Simple specular using metallic and roughness  
        vec3 reflectDir = reflect(-lightDir, normal);
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), (1.0 - roughness) * 128.0);
        vec3 specular = light.color.rgb * spec * metallic * light.color.w * attenuation;
        
        lightContrib = diffuse + specular;
    }
    else if (light.position.w == 2.0) {
        // Spot light
        vec3 lightDir = light.position.xyz - worldPos;
        float distance = length(lightDir);
        lightDir = normalize(lightDir);
        
        float NdotL = max(dot(normal, lightDir), 0.0);
        
        // Spot light cone
        float theta = dot(lightDir, normalize(-light.direction.xyz));
        float epsilon = light.direction.w - light.attenuation.w;
        float intensity = clamp((theta - light.attenuation.w) / epsilon, 0.0, 1.0);
        
        // Attenuation
        float attenuation = 1.0 / (light.attenuation.x + 
                                  light.attenuation.y * distance + 
                                  light.attenuation.z * distance * distance);
        
        // Basic PBR diffuse
        vec3 diffuse = light.color.rgb * light.color.w * NdotL * attenuation * intensity;
        
        // Simple specular using metallic and roughness
        vec3 reflectDir = reflect(-lightDir, normal);
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), (1.0 - roughness) * 128.0);
        vec3 specular = light.color.rgb * spec * metallic * light.color.w * attenuation * intensity;
        
        lightContrib = diffuse + specular;
    }
    
    return lightContrib * albedo;
}

// Forward+ lighting accumulation
vec3 calculateForwardPlusLighting(vec3 worldPos, vec3 normal, vec3 albedo, vec3 viewDir) {
    // Calculate cluster index for this fragment
    uint clusterIndex = calculateClusterIndex(viewPosition);
    
    // Get light count and offset for this cluster
    uint lightCount = lightGrid[clusterIndex * 2u];
    uint lightOffset = lightGrid[clusterIndex * 2u + 1u];
    
    // Accumulate lighting from all lights in this cluster
    vec3 totalLighting = vec3(0.0);
    
    uint maxLights = uint(64); // MAX_LIGHTS_PER_CLUSTER fallback
    #ifdef MAX_LIGHTS_PER_CLUSTER
    maxLights = uint(MAX_LIGHTS_PER_CLUSTER);
    #endif
    
    for (uint i = 0u; i < lightCount && i < maxLights; ++i) {
        uint lightIndex = lightIndices[lightOffset + i];
        
        if (lightIndex < uint(visibleLights.length())) {
            LightData light = visibleLights[lightIndex];
            totalLighting += calculateLighting(light, worldPos, normal, albedo, viewDir);
        }
    }
    
    return totalLighting;
}

#endif // FORWARD_PLUS_ENABLED

// Fallback simple lighting (when forward+ is disabled)
vec3 calculateSimpleLighting(vec3 worldPos, vec3 normal, vec3 albedo) {
    // Simple directional light fallback
    vec3 lightDirection = normalize(vec3(0.5, -1.0, 0.3));
    vec3 lightColor = vec3(1.0, 0.95, 0.8);
    float lightIntensity = 1.0;
    
    vec3 lightDir = normalize(-lightDirection);
    float NdotL = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = lightColor * lightIntensity * NdotL;
    
    // Add simple specular using material properties
    vec3 viewDir = normalize(cameraPosition - worldPos);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), (1.0 - roughness) * 64.0);
    vec3 specular = lightColor * spec * metallic * lightIntensity;
    
    return albedo * (diffuse + specular);
}

out vec4 fragOutput;

void main() {
    // Normalize normal
    vec3 normal = normalize(fragNormal);
    
    // Calculate view direction
    vec3 viewDir = normalize(cameraPosition - fragPosition);
    
    // Base material color
    vec3 albedo = baseColor * fragColor.rgb;
    
    // Calculate lighting
    vec3 finalColor;
    
#ifdef FORWARD_PLUS_ENABLED
    // Use forward+ clustered lighting
    finalColor = calculateForwardPlusLighting(fragPosition, normal, albedo, viewDir);
    
    // Add ambient lighting
    vec3 ambient = vec3(0.1, 0.1, 0.15) * 0.2;
    finalColor += albedo * ambient;
#else
    // Fallback to simple lighting
    finalColor = calculateSimpleLighting(fragPosition, normal, albedo);
    
    // Add ambient
    vec3 ambient = vec3(0.2, 0.2, 0.3) * 0.3;
    finalColor += albedo * ambient;
#endif
    
    // Add emissive contribution
    finalColor += emissive;
    
    // Output final color
    fragOutput = vec4(finalColor, 1.0);
}