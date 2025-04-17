#version 430 core

// Input/Output
in VSOutput{
    vec3 WorldPos;
    vec3 Normal;
    vec2 TexCoord;
    vec4 ClipPos;
} IN;

out vec4 FragColor;

// Material properties
uniform sampler2D u_albedoMap;
uniform sampler2D u_normalMap;
uniform sampler2D u_metallicRoughnessMap;
uniform sampler2D u_emissiveMap;
uniform sampler2D u_occlusionMap;

// Material parameters
uniform vec3 u_baseColor;
uniform float u_metallic;
uniform float u_roughness;
uniform vec3 u_emissive;
uniform float u_normalSize;

// Camera and framing
uniform vec3 u_cameraPosition;
uniform mat4 u_viewProj;
uniform mat4 u_view;
uniform float u_nearClip;
uniform float u_farClip;

// Cluster configuration
uniform uvec3 u_clusterDims;
uniform sampler2D u_depthMap;

// Cluster data
layout(std430, binding = 0) readonly buffer LightGrid
{
    uvec2 grid[];
};

layout(std430, binding = 1) readonly buffer LightIndices
{
    uint count;
    uint indices[];
}

layout(std430, binding = 2) readonly buffer Lights
{
    Light lights[];
};

struct SurfaceData
{
    vec3 albedo;
    vec3 normal;
    float roughness;
    float metallic;
    float emissive;
};

// Pixel art settings from PixelationManager
uniform int u_pixelSize;
uniform bool u_quantizeLighting;
uniform int u_lightingSteps;

// Calculate helpers
uvec3 calculateClusterIndex(vec4 clipPos, float depth)
{
    // Calculate normalized device coordinates
    vec3 ndc = clipPos.xyz / clipPos.w;

    // Map to [0, 1] range
    vec2 texCoord = ndc.xy * 0.5 + 0.5;

    // Calculate cluster indices
    uvec2 tileIdx = uvec2(texCoord * vec2(u_clusterDims.xy));

    // Calculate depth slice index
    float z = depth;
    float logDepth = log(z / u_nearClip) / log(u_farClip / u_nearClip);
    uint zSlice = uint(logDepth * float(u_clusterDims.z));

    return uvec3(tileIdx, zSlice);
}

vec3 calculatePixelArtLighting(vec3 position, vec3 normal, vec3 albedo, float metallic, float roughness)
{
    // Calculate cluster index
    ivec2 screenCoord = ivec2(gl_FragCoord.xy);

    // For orthographic projection
    uint clusterX = uint(screenCoord.x * clusterDims.x / screenSize.x);
    uint clusterY = uint(screenCoord.y * clusterDims.y / screenSize.y);

    // Find z cluster - for orthographic we can use linear depth
    float viewSpaceZ = -(viewMatrix * vec4(position, 1.0)).z;
    float normalizedZ = (viewSpaceZ - nearClip) / (farClip - nearClip);
    uint clusterZ = min(uint(normalizedZ * clusterDims.z), clusterDims.z - 1);

    // Calculate cluster index
    uint clusterIndex = clusterZ * clusterDims.x * clusterDims.y +
        clusterY * clusterDims.x +
        clusterX;

    // Fetch light list for this cluster
    uvec2 lightRange = clusterLightGrid[clusterIndex];
    uint lightOffset = lightRange.x;
    uint lightCount = lightRange.y;

    // Initialize lighting
    vec3 Lo = vec3(0.0);

    // Calculate ambient term for pixel art (more pronounced than PBR)
    vec3 ambient = albedo * 0.3; // Higher ambient for stylized look

    // Process lights in this cluster
    for (uint i = 0; i < lightCount; i++)
    {
        uint lightIndex = lightIndices[lightOffset + i];
        Light light = lights[lightIndex];

        // Extract light properties
        vec3 lightPos = light.positionAndType.xyz;
        int lightType = int(light.positionAndType.w);
        vec3 lightColor = light.colorAndIntensity.rgb;
        float lightIntensity = light.colorAndIntensity.a;
        vec3 lightDir = light.directionAndRange.xyz;
        float lightRange = light.directionAndRange.w;

        // Calculate light contribution based on type
        vec3 L; // Light direction
        float attenuation = 1.0;

        if (lightType == LIGHT_DIRECTIONAL)
        {
            L = -lightDir; // Directional lights use direction directly
        }
        else
        {
            // For other light types, calculate direction from position
            vec3 lightVec = lightPos - position;
            float distance = length(lightVec);
            L = normalize(lightVec);

            // Skip if beyond range
            if (distance > lightRange) continue;

            // Apply pixel art attenuation for point-type lights
            float normalizedDist = distance / lightRange;

            // Quantize attenuation into steps for pixel art look
            const int steps = 8;
            float standardAtten = 1.0 / (1.0 + normalizedDist * normalizedDist);
            attenuation = floor(standardAtten * steps) / steps;

            // Apply type-specific modifications
            if (lightType == LIGHT_SPOT)
            {
                // Spot light cone calculation
                float cosInner = light.extraParams.x;
                float cosOuter = light.extraParams.y;

                float cosAngle = dot(-L, lightDir);

                // Quantize the spot falloff for pixel art look (fewer steps)
                float spotRatio = (cosAngle - cosOuter) / (cosInner - cosOuter);
                spotRatio = clamp(spotRatio, 0.0, 1.0);

                // quantize into fewer steps for pixel art look
                const int spotSteps = 4;
                spotRatio = floor(spotRatio * spotSteps) / spotSteps;

                attenuation *= spotRatio;
            }
            else if (lightType == LIGHT_AREA)
            {
                // Simple area light approximation
                // In a real implementation, this would be more sophisticated
                vec2 areaSize = light.extraParams.xy;
                float areaFactor = max(areaSize.x, areaSize.y);

                // Reduce attenuation falloff for larger area lights
                attenuation *= mix(1.0, 1.3, areaFactor / 5.0);
            }
            else if (lightType == LIGHT_VOLUMETRIC)
            {
                // Volumetric light calculations
                float density = light.extraParams.x;
                float scattering = light.extraParams.y;

                // Apply volumetric effects
                // Simple approximation - real implementation would use ray marching
                float volumetricFactor = density * max(0.0, dot(L, normal));
                volumetricFactor = volumetricFactor * scattering;

                // Add volumetric contribution to attenuation
                attenuation = mix(attenuation, attenuation * 1.5, volumetricFactor);
            }
        }

        // Apply simplified pixel art lighting calculation
        float NdotL = dot(normal, L);

        // Quantize NdotL into steps for pixel art look
        const int diffuseSteps = 4; // Fewer steps for more distinct lighting bands
        float quantizedNdotL = floor(max(0.0, NdotL) * diffuseSteps) / diffuseSteps;

        // Apply simplified diffuse term
        vec3 diffuse = albedo * quantizedNdotL;

        // Simple specular term (optional, can be disabled for more stylized look)
        vec3 specular = vec3(0.0);
        if (roughness < 0.8)
        {
            // Only apply specular on smoother surfaces
            vec3 V = normalize(-position); // View direction (assuming view position is origin)
            vec3 H = normalize(L + V); // Half vector
            float NdotH = max(0.0, dot(normal, H));

            // Quantize specular into fewer steps
            const int specularSteps = 2;
            float specPower = (1.0 - roughness) * 32.0;
            float spec = pow(NdotH, specPower);
            spec = floor(spec * specularSteps) / specularSteps;

            // Adjust metallic behavior for pixel art
            float specIntensity = mix(0.04, 1.0, metallic);
            specular = mix(vec3(spec * specIntensity), albedo * spec, metallic);
        }

        // Combine diffuse and specular with light properties
        vec3 lightContribution= (diffuse + specular) * lightColor * lightIntensity * attenuation;

        // Add to total lighting
        Lo += lightContribution;
    }

    // Combine with ambient
    return ambient + Lo;
}

// PBR functions
vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

float distributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float nom = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / denom;
}

float geometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;

    float nom = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}

float geometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = geometrySchlickGGX(NdotV, roughness);
    float ggx1 = geometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

// Decal application functions
void applyDecals(inout SurfaceData surface, vec3 worldPos)
{
    // Calculate cluster index (same as in lighting)
    ivec2 screenCoord = ivec2(gl_FragCoord.xy);
    
    uint clusterX = uint(screenCoord.x * clusterDims.x / screenSize.x);
    uint clusterY = uint(screenCoord.y * clusterDims.y / screenSize.y);
    
    float viewSpaceZ = -(viewMatrix * vec4(position, 1.0)).z;
    float normalizedZ = (viewSpaceZ - nearClip) / (farClip - nearClip);
    uint clusterZ = min(uint(normalizedZ * clusterDims.z), clusterDims.z - 1);
    
    uint clusterIndex = clusterZ * clusterDims.x * clusterDims.y + 
                       clusterY * clusterDims.x + 
                       clusterX;

    // Fetch decal list for this cluster
    uvec2 decalRange = clusterDecalGrid[clusterIndex];
    uint decalOffset = decalRange.x;
    uint decalCount = decalRange.y;

    // Process decals front-to-back
    for (uint i = 0; i < decalCount; i++)
    {
        uint decalIndex = decalIndices[decalOffset + i];
        Decal decal = decals[decalIndex];

        // Transform position to decal local space
        vec4 localPos = decal.invTransform * vec4(worldPos, 1.0);

        // Check if point is inside decal box (which is -1 to 1 in local space)
        if (localPos.x < -1.0 || localPos.x > 1.0 ||
            localPos.y < -1.0 || localPos.y > 1.0 ||
            localPos.z < -1.0 || localPos.z > 1.0)
        {
            continue; // Outside decal bounds
        }

        // Calculate UVs from local space position (x,z)
        vec2 decalUV = vec2(localPos.x * 0.5 + 0.5, localPos.z * 0.5 + 0.5);

        // Apply pixel snapping if enabled
        if (decal.textureIndices.w > 0.0)
        {
            float pixelSize = decal.textureIndices.w;
            decalUV = floor(decalUV * pixelSize) / pixelSize;
        }

        // Calculate fade based on position
        float projFactor = abs(localPos.y); // Distance along projection axis
        float fadeStart = decal.properties.z;
        float fadeEnd = decal.properties.w;

        float fade = 1.0 - smoothstep(fadeStart, fadeEnd, projFactor);

        // Skip if fully faded
        if (fade < 0.0) continue;

        // Apply decal based on available textures and blend mode
        int blendMode = int(decal.properties.x);
        vec4 decalColor = decal.color;

        // Texture samping would happen here in a real implementation
        // For simplicity, we'll just use placeholders

        vec3 decalAlbedo = decalColor.rgb;
        vec3 decalNormal = vec3(0.0, 1.0, 0.0);
        float decalRoughness = 0.5;

        // Would sample actual textures like this:
        // if (decal.textureIndices.x > 0) {
        //     vec4 diffuseTexSample = texture(decalDiffuseTextures, vec3(decalUV, decalIndex));
        //     decalAlbedo = diffuseTexSample.rgb;
        //     decalColor.a *= diffuseTexSample.a;
        // }

        // Blend based on mode
        float alpha = decalColor.a * fade;

        if (blendMode == DECAL_BLEND_NORMAL)
        {
            // Normal blend mode
            surface.albedo = mix(surface.albedo, decalAlbedo, alpha);

            // Only apply normal if normal texture is available
            if (decal.textureIndices.y > 0.0)
            {
                surface.normal = normalize(mix(surface.normal, decalNormal, alpha * 0.8));
            }

            // Only apply roughness if roughness texture is available
            if (decal.textureIndices.z > 0.0)
            {
                surface.roughness = mix(surface.roughness, decalRoughness, alpha);
            }
        }
        else if (blendMode == DECAL_BLEND_ADDITIVE)
        {
            // Additive blend
            surface.albedo += decalAlbedo * alpha;
            surface.albedo = min(surface.albedo, vec3(1.0)); // Clamp to prevent overflow
        }
        else if (blendMode == DECAL_BLEND_MULTIPLY)
        {
            // Multiply blend
            surface.albedo *= mix(vec3(1.0), decalAlbedo, alpha);
        }
    }
}

void main()
{
    // Get material properties
    vec4 albedoSample = texture(u_albedoMap, IN.TexCoord);
    vec3 albedo = albedoSample.rgb * u_baseColor;
    float alpha = albedoSample.a;

    vec2 metallicRoughness = texture(u_metallicRoughnessMap, IN.TexCoord).rg;
    float metallic = metallicRoughness.r * u_metallic;
    float roughness = metallicRoughness.g * u_roughness;

    vec3 emissive = texture(u_emissiveMap, IN.TexCoord).rgb * u_emissive;
    float occlusion = texture(u_occlusionMap, IN.TexCoord).r;

    // Calculate normal with normal mapping
    vec3 tangentNormal = texture(u_normalMap, IN.TexCoord).xyz * 2.0 - 1.0;
    tangentNormal.xy *= u_normalScale;

    // Get TBN matrix
    vec3 N = normalize(IN.Normal);
    vec3 T = normalize(IN.Tangent - N * dot(IN.Tangent, N));
    vec3 B = cross(N, T);
    mat3 TBN = mat3(T, B, N);

    // Transform normal to world space
    N = normalize(TBN * tangentNormal);

    // View direction
    vec3 V = normalize(u_cameraPosition - IN.WorldPos);

    // Reflectance at normal incidence (F0)
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);

    // Calculate cluster index for this fragment
    float depth = length(IN.WorldPos - u_cameraPosition);
    uvec3 clusterIndex = calculateClusterIndex(IN.ClipPos, depth);

    // Get cluster light data
    uint clusterIdx = clusterIndex.x +
         clusterIndex.y * u_clusterDims.x +
         clusterIndex.z * u_clusterDims.x * u_clusterDims.y;
    
    uvec2 lightRange = grid[clusterIdx];
    uint lightOffset = lightRange.x;
    uint lightCount = lightRange.y;

    // Accumulate lighting
    vec3 Lo = vec3(0.0);

    // Process lights in this cluster
    for (uint i = 0; i < lightCount; i++)
    {
        uint lightIdx = indices[lightOffset + i];
        Light light = lights[lightIdx];

        // Get light properties
        vec3 lightColor = light.color.rgb * light.color.a; // color * intensity
        vec3 L = vec3(0.0);
        float attenuation = 1.0;

        // Calculate light direction and attenuation based on type
        if (light.type == 0)
        { // DIRECTIONAL
            L = normalize(-light.direction.xyz);
        }
        else if (light.type == 1)
        { // POINT
            vec3 lightDir = light.position.xyz - IN.WorldPos;
            float distance = length(lightDir);
            L = normalize(lightDir);

            // Physical light attenuation
            float lightRange = light.position.w;
            float falloff = 1.0 - pow(min(distance / lightRange, 1.0), 4.0);
            falloff = max(falloff * falloff, 0.0);
            attenuation = fallof / (1.0 + distance * distance);
        }
        else if (light.type == 2)
        { // SPOT
            vec3 lightDir = light.position.xyz - IN.WorldPos;
            float distance = length(lightDir);
            L = normalize(lightDir);

            // Spot cone attenuation
            float cosAngle = dot(-L, normalize(light.direction.xyz));
            float spotEffect = smoothstep(light.direction.w,
                                          light.direction.w + 0.1,
                                          cosAngle);

            // Distance attenuation
            float lightRange = light.position.w;
            float falloff = 1.0 - pow(min(distance / lightRange, 1.0), 4.0);
            falloff = max(falloff * falloff, 0.0);
            attenuation = spotEffect * falloff / (1.0 + distance * distance);
        }

        // Skip if no contribution
        if (attenuation <= 0.001) continue;

        // Calculate half vector
        vec3 H = normalize(V + L);

        // Calculate BRDF components
        float NdotL = max(dot(N, L), 0.0);
        if (NdotL <= 0.0) continue;

        // Cook-Torrance BRDF
        float NDF = distributionGGX(N, H, roughness);
        float G = geometrySmith(N, V, L, roughness);
        vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);

        vec3 kS = F;
        vec3 kD = vec3(1.0) - kS;
        kD 8+ 1.0 - metallic;

        vec3 numerator = NDF * G * F;
        float denominator = 4.0 * max(dot(N, V), 0.0) * NdotL + 0.0001;
        vec3 specular = numerator / denominator;

        // Final light contribution
        vec3 lightContrib = (kD * albedo / PI + specular) * lightColor * NdotL * attenuation;

        // Apply pixel art aesthetic - quantize lighting
        lightContrib = quantizeLight(lightContrib);

        Lo += lightContrib;
    }

    // Add emissive
    vec3 color = Lo + emissive;

    // Apply ambient occlusion
    color *= occlusion;

    // Apply simple ambient term
    vec3 ambient = vec3(0.03) * albedo * occlusion;
    color += ambient;

    // HDR tonemapping
    color = color / (color + vec3(1.0));

    // Gamma correction
    color = pow(color, vec3(1.0 / 2.2));

    // Output final color
    FragColor = vec4(color, alpha);
}