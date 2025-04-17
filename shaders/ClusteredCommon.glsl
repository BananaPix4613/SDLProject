#verison 430 core

// Light structure (16 floats = 64 bytes per light)
struct Light
{
    vec4 positionAndType;         // xyz = position, w = type
    vec4 colorAndIntensity;       // rgb = color, a = intensity
    vec4 directionAndRange;       // xyz = direction, w = range
    vec4 extraParams;             // Type-specific parameters
};

// Light types
#define LIGHT_DIRECTIONAL 0
#define LIGHT_POINT 1
#define LIGHT_SPOT 2
#define LIGHT_AREA 3
#define LIGHT_VOLUMETRIC 4

// Decal structure (28 floats = 112 bytes per decal)
struct Decal
{
    mat4 invTransform;            // Inverse transform matrix
    vec4 color;                   // RGBA color with alpha
    vec4 properties;              // x = blend mode, y = proj distance, z = fade start, w = fade end
    vec4 textureIndices;          // x = diffuse, y = normal, z = roughness, w = pixel snapping/size
};

// Decal blend modes
#define DECAL_BLEND_NORMAL 0
#define DECAL_BLEND_ADDITIVE 1
#define DECAL_BLEND_MULTIPLY 2

// Cluster grid structure
layout(std140, binding = 0) buffer LightGrid
{
    // For each cluster: x = offset into LightIndices, y = count
    uvec2 clusterLightGrid[];
};

layout(std140, binding = 1) buffer LightIndices
{
    // Indices into the light buffer
    uint lightIndices[];
};

layout(std140, binding = 2) buffer LightBuffer
{
    // Actual light data
    Light lights[];
};

layout(std140, binding = 3) buffer DecalGrid
{
    // For each cluster: x = offset into DecalIndices, y = count
    uvec2 clusterDecalGrid[];
};

layout(std140, binding = 4) buffer DecalIndices
{
    // Indices into the decal buffer
    uint decalIndices[];
};

layout(std140, binding = 5) buffer DecalBuffer
{
    // Actual decal data
    Decal decals[];
};

// Uniforms for cluster calculations
uniform vec4 zPlanes[16]; // Z-plane equations for clusters
uniform uvec3 clusterDims; // x, y, z dimensions of the cluster grid
uniform float nearClip;
uniform float farClip;