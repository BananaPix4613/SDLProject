#version 430 core

layout(local_size_x = 16, local_size_y = 1, local_size_z = 1) in;

// Shared structures from previous section
struct Light
{
    vec4 positionAndType;
    vec4 colorAndIntensity;
    vec4 directionAndRange;
    vec4 extraParams;
};

// Shared memory for light indices within a workgroup
shared uint s_lightCount;
shared uint s_lightList[256]; // Maximum lights per workgroup

// Cluster data
layout(std430, binding = 0) readonly buffer ClusterAABBs
{
    vec4 clusterMinPoint[];
    vec4 clusterMaxPoint[];
};

// Output light grid
layout(std430, binding = 1) buffer LightGrid
{
    uint lightCount; // Global counter for light indices
    uvec2 lightGridData[]; // x = offset, y = count for each cluster
};

// Output light indices
layout(std430, binding = 2) writeonly buffer LightIndices
{
    uint lightIndexList[];
};

// Light data
layout(std430, binding = 3) readonly buffer LightBuffer
{
    Light lights[];
};

// Input uniforms
uniform uint lightCount;
uniform uint clusterCount;
uniform uint maxLightsPerCluster;

// Forward declarations
bool sphereIntersectsAABB(vec3 center, float radius, vec3 aabbMin, vec3 aabbMax);
bool coneIntersectsAABB(vec3 coneOrigin, vec3 coneDir, float coneAngle, float coneRange, vec3 aabbMin, vec3 aabbMax);

void main()
{
    // Get global invocation index
    uint clusterIndex = gl_GlobalInvocationID.x;

    // Skip if beyond cluster count
    if (clusterIndex >= clusterCount)
    {
        return;
    }

    // Initialize shared light count
    if (gl_LocalInvocationIndex == 0)
    {
        s_lightCount = 0;
    }

    // Ensure initialization is complete
    barrier();

    // Get cluster bounds
    vec3 minPoint = clusterMinPoint[clusterIndex].xyz;
    vec3 maxPoint = clusterMaxPoint[clusterIndex].xyz;

    // Check each light for intersection with this cluster
    for (uint lightIndex = 0; lightIndex < lightCount; lightIndex++)
    {
        Light light = lights[lightIndex];

        // Get light properties
        vec3 lightPos = light.positionAndType.xyz;
        int lightType = int(light.positionAndType.w);
        float lightRange = light.directionAndRange.w;
        vec3 lightDir = light.directionAndRange.xyz;

        bool intersects = false;

        // Check intersection based on light type
        if (lightType == 0)
        { // Directional
            // Directional lights affect all clusters
            intersects = true;
        }
        else if (lightType == 2)
        { // Spot
            // Get spot angle from extraParams
            float cosOuter = light.extraParams.y;
            float spotAngle = acos(cosOuter);

            // Check cone intersection
            intersects = coneIntersectsAABB(lightPos, lightDir, spotAngle, lightRange, minPoint, maxPoint);
        }
        else
        { // Point, Area, Volumetric (simplified as spheres)
            // For area lights, adjust radius based on size
            float adjustedRange = lightRange;
            if (lightType == 3)
            { // Area light
                vec2 areaSize = light.extraParams.xy;
                float maxSize = max(areaSize.x, areaSize.y);
                adjustedRange += maxSize * 0.5;
            }

            // Check sphere intersection
            intersects = sphereIntersectsAABB(lightPos, adjustedRange, minPoint, maxPoint);
        }

        // If light intersects this cluster, add to shared list
        if (intersects)
        {
            uint index = atomicAdd(s_lightCount, 1);

            // Only add if we have space
            if (index < 256)
            {
                s_lightList[index] = lightIndex;
            }
        }
    }

    // Ensure all threads have finished adding lights
    barrier();

    // First thread writes the results to global memory
    if (gl_LocalInvocationIndex == 0 && s_lightCount > 0)
    {
        // Clamp light count to maximum
        uint count = min(s_lightCount, maxLightsPerCluster);

        // Atomic add to get our portion of the global light index list
        uint offset = atomicAdd(lightCount, count);

        // Write offset and count to grid
        lightGridData[clusterIndex] = uvec2(offset, count);

        // Write indices to global list
        for (uint i = 0; i < count; i++)
        {
            lightIndexList[offset + i] = s_lightList[i];
        }
    }
    else if (gl_LocalInvocationIndex == 0)
    {
        // No lights, initialize to zero
        lightGridData[clusterIndex] = uvec2(0, 0);
    }
}

// Utility functions for intersection tests

bool sphereAABBIntersect(vec3 center, float radius, vec3 aabbMin, vec3 aabbMax)
{
    // Calculate squared distance from sphere center to AABB
    float squaredDistance = 0.0;

    // For each axis, calculate the closest point on the AABB to the sphere center
    for (int i = 0; i < 3; i++)
    {
        float v = center[i];
        if (v < aabbMin[i]) squaredDistance += (aabbMin[i] - v) * (aabbMin[i] - v);
        if (v > aabbMax[i]) squaredDistance += (v - aabbMax[i]) * (v - aabbMax[i]);
    }

    // Sphere intersects if the squared distance is less than squared radius
    return squaredDistance <= (radius * radius);
}

bool coneIntersectsAABB(vec3 coneOrigin, vec3 coneDir, float coneAngle, float coneRange, vec3 aabbMin, vec3 aabbMax)
{
    // First quick rejection: sphere check at cone origin
    if (!sphereIntersectsAABB(coneOrigin, coneRange, aabbMin, aabbMax))
    {
        return false;
    }

    // Second quick acceptance: if the AABB contains the cone origin
    if (coneOrigin.x >= aabbMin.x && coneOrigin.x <= aabbMax.x &&
        coneOrigin.y >= aabbMin.y && coneOrigin.y <= aabbMax.y &&
        coneOrigin.z >= aabbMin.z && coneOrigin.z <= aabbMax.z)
    {
        return true;
    }

    // Simplified cone check - for each AABB corner, check if it's inside the cone
    vec3 corners[8];
    corners[0] = vec3(aabbMin.x, aabbMin.y, aabbMin.z);
    corners[1] = vec3(aabbMax.x, aabbMin.y, aabbMin.z);
    corners[2] = vec3(aabbMin.x, aabbMax.y, aabbMin.z);
    corners[3] = vec3(aabbMax.x, aabbMax.y, aabbMin.z);
    corners[4] = vec3(aabbMin.x, aabbMin.y, aabbMax.z);
    corners[5] = vec3(aabbMax.x, aabbMin.y, aabbMax.z);
    corners[6] = vec3(aabbMin.x, aabbMax.y, aabbMax.z);
    corners[7] = vec3(aabbMax.x, aabbMax.y, aabbMax.z);

    float cosAngle = cos(coneAngle);

    for (int i = 0; i < 8; i++)
    {
        vec3 cornerVec = corners[i] - coneOrigin;
        float distance = length(cornerVec);

        // Skip if beyond range
        if (distance > coneRange)
        {
            continue;
        }

        // Check if inside cone angle
        float cosCornerAngle = dot(normalize(cornerVec), coneDir);
        if (cosCornerAngle > cosAngle)
        {
            return true; // Corner is inside cone
        }
    }

    // More accurate test would check for cone-AABB edge intersections,
    // but this simplified version works for most cases
    return false;
}