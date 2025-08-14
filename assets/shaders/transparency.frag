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
uniform float alphaDiscardThreshold;

// Camera and viewport
uniform vec3 cameraPosition;
uniform vec2 viewportSize;

// Transparency-specific uniforms
uniform vec3 ambientLight;
uniform int transparencyTechnique; // 0 = alpha blend, 1 = weighted blend, 2 = depth peel
uniform sampler2D depthTexture; // For soft particles
uniform sampler2D sceneDepthTexture; // Z-prepass depth for proper depth testing
uniform int softParticlesEnabled;
uniform bool enableDepthTesting;
uniform float depthBias;

// Legacy uniforms for fallback
uniform vec3 lightDirection;
uniform vec3 lightColor;
uniform float lightIntensity;

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

// Light data structure (matches C++ LightData - same as voxel shader)
struct LightData {
    vec4 position;       // w component: light type (0=directional, 1=point, 2=spot)
    vec4 direction;      // w component: inner cone angle (for spot lights)
    vec4 color;          // w component: intensity
    vec4 attenuation;    // x=constant, y=linear, z=quadratic, w=outer cone angle
    vec4 metadata;       // x=shadowIndex, y=castShadows, z=unused, w=unused
};

// Shader storage buffers for cluster data (same as voxel shader)
layout(std430, binding = 1) readonly buffer LightGridBuffer {
    uint lightGrid[];
};

layout(std430, binding = 2) readonly buffer LightIndicesBuffer {
    uint lightIndices[];
};

layout(std430, binding = 3) readonly buffer VisibleLightsBuffer {
    LightData visibleLights[];
};

// Calculate cluster index from view space position (using uniforms)
uint calculateClusterIndex(vec3 viewPos) {
    // Convert view position to cluster coordinates using uniforms
    vec3 clusterPos = (viewPos - clusterMinBounds) / clusterSize;
    
    // Clamp to valid range
    ivec3 clusterCoord = ivec3(clamp(clusterPos, vec3(0.0), vec3(clusterDimensions - 1)));
    
    // Convert 3D cluster coordinate to 1D index
    return uint(clusterCoord.z * (clusterDimensions.x * clusterDimensions.y) +
                clusterCoord.y * clusterDimensions.x +
                clusterCoord.x);
}

// Lighting calculation for transparent objects (simplified for performance)
vec3 calculateTransparencyLighting(LightData light, vec3 worldPos, vec3 normal, vec3 albedo, vec3 viewDir) {
    vec3 lightContrib = vec3(0.0);
    
    if (light.position.w == 0.0) {
        // Directional light
        vec3 lightDir = normalize(-light.direction.xyz);
        float NdotL = max(dot(normal, lightDir), 0.0);
        
        // Basic diffuse
        vec3 diffuse = light.color.rgb * light.color.w * NdotL;
        
        // Reduced specular for transparency using metallic and roughness
        vec3 reflectDir = reflect(-lightDir, normal);
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), (1.0 - roughness) * 64.0);
        vec3 specular = light.color.rgb * spec * metallic * light.color.w * 0.3; // Reduced for transparency
        
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
        
        // Basic diffuse
        vec3 diffuse = light.color.rgb * light.color.w * NdotL * attenuation;
        
        // Reduced specular for transparency using metallic and roughness
        vec3 reflectDir = reflect(-lightDir, normal);
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), (1.0 - roughness) * 64.0);
        vec3 specular = light.color.rgb * spec * metallic * light.color.w * attenuation * 0.3; // Reduced for transparency
        
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
        
        // Basic diffuse
        vec3 diffuse = light.color.rgb * light.color.w * NdotL * attenuation * intensity;
        
        // Reduced specular for transparency using metallic and roughness
        vec3 reflectDir = reflect(-lightDir, normal);
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), (1.0 - roughness) * 64.0);
        vec3 specular = light.color.rgb * spec * metallic * light.color.w * attenuation * intensity * 0.3; // Reduced for transparency
        
        lightContrib = diffuse + specular;
    }
    
    // Reduced lighting contribution for transparency (to avoid over-brightness)
    return lightContrib * albedo * 0.8;
}

// Forward+ lighting accumulation for transparent objects
vec3 calculateForwardPlusTransparencyLighting(vec3 worldPos, vec3 normal, vec3 albedo, vec3 viewDir) {
    // Calculate cluster index for this fragment
    uint clusterIndex = calculateClusterIndex(viewPosition);
    
    // Get light count and offset for this cluster
    uint lightCount = lightGrid[clusterIndex * 2u];
    uint lightOffset = lightGrid[clusterIndex * 2u + 1u];
    
    // Limit lights for transparency performance
    uint maxLights = uint(32); // Reduced for transparency
    #ifdef MAX_LIGHTS_PER_CLUSTER
    maxLights = min(uint(MAX_LIGHTS_PER_CLUSTER), uint(32));
    #endif
    
    // Accumulate lighting from lights in this cluster
    vec3 totalLighting = vec3(0.0);
    
    for (uint i = 0u; i < lightCount && i < maxLights; ++i) {
        uint lightIndex = lightIndices[lightOffset + i];
        
        if (lightIndex < uint(visibleLights.length())) {
            LightData light = visibleLights[lightIndex];
            totalLighting += calculateTransparencyLighting(light, worldPos, normal, albedo, viewDir);
        }
    }
    
    return totalLighting;
}

#endif // FORWARD_PLUS_ENABLED

// Fallback simple lighting for transparency
vec3 calculateSimpleTransparencyLighting(vec3 worldPos, vec3 normal, vec3 albedo) {
    // Simple directional light fallback
    vec3 lightDir = normalize(-lightDirection);
    float NdotL = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = lightColor * lightIntensity * NdotL;
    
    // Add simple specular using material properties
    vec3 viewDir = normalize(cameraPosition - worldPos);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), (1.0 - roughness) * 32.0);
    vec3 specular = lightColor * spec * metallic * lightIntensity * 0.3; // Reduced for transparency
    
    return albedo * (diffuse + specular) * 0.8; // Reduced for transparency
}

// Test fragment depth against Z-prepass buffer
bool testDepthBuffer() {
    if (!enableDepthTesting) {
        return true;
    }
    
    vec2 screenCoord = gl_FragCoord.xy / viewportSize;
    float sceneDepth = texture(sceneDepthTexture, screenCoord).r;
    float fragmentDepth = gl_FragCoord.z;
    
    // Apply small bias to avoid z-fighting with opaque geometry
    return fragmentDepth <= (sceneDepth + depthBias);
}

// Weight function for Order Independent Transparency
float calculateOITWeight(float alpha, float depth) {
    // Weight function from McGuire & Bavoil 2013
    return alpha * max(1e-2, 3e3 * pow(1.0 - depth, 3.0));
}

// Outputs for different transparency techniques
layout(location = 0) out vec4 accumulation;
layout(location = 1) out float revealage;
out vec4 fragOutput;

void main() {
    // Normalize normal
    vec3 normal = normalize(fragNormal);
    
    // Calculate view direction
    vec3 viewDir = normalize(cameraPosition - fragPosition);
    
    // Test against Z-prepass depth buffer first
    if (!testDepthBuffer()) {
        discard;
    }
    
    // Base material color
    vec3 albedo = baseColor * fragColor.rgb;
    
    // Calculate lighting
    vec3 finalColor;
    
#ifdef FORWARD_PLUS_ENABLED
    // Use forward+ clustered lighting
    finalColor = calculateForwardPlusTransparencyLighting(fragPosition, normal, albedo, viewDir);
    
    // Add ambient lighting
    vec3 ambient = ambientLight * 0.5; // Reduced ambient for transparency
    finalColor += albedo * ambient;
#else
    // Fallback to simple lighting
    finalColor = calculateSimpleTransparencyLighting(fragPosition, normal, albedo);
    
    // Add ambient
    vec3 ambient = ambientLight * 0.5;
    finalColor += albedo * ambient;
#endif
    
    // Add emissive contribution
    finalColor += emissive;
    
    // Get alpha from vertex color
    float alpha = fragColor.a;
    
    // Alpha discard test
    if (alpha < alphaDiscardThreshold) {
        discard;
    }
    
    // Soft particles depth fade
    if (softParticlesEnabled == 1) {
        vec2 screenCoord = gl_FragCoord.xy / viewportSize;
        float sceneDepth = texture(depthTexture, screenCoord).r;
        float particleDepth = gl_FragCoord.z;
        float fade = smoothstep(0.0, 0.1, sceneDepth - particleDepth);
        alpha *= fade;
    }
    
    // Output based on transparency technique
    if (transparencyTechnique == 1) {
        // Weighted Blended OIT
        float weight = calculateOITWeight(alpha, gl_FragCoord.z);
        
        accumulation = vec4(finalColor * alpha * weight, alpha * weight);
        revealage = alpha;
    } else {
        // Standard alpha blending
        fragOutput = vec4(finalColor, alpha);
    }
}