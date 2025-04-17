#version 430 core

layout(local_size_x = 16, local_size_y = 1, local_size_z = 1) in;

// Shared structures from previous section
struct Decal
{
    mat4 invTransform;
    vec4 color;
    vec4 properties;
    vec4 textureIndices;
};

// Shared memory for decal indices within a workgroup
shared uint s_decalCount;
shared uint s_decalList[64]; // Maximum decals per workgroup

// Cluster data
layout(std430, binding = 0) readonly buffer ClusterAABBs
{
    vec4 clusterMinPoint[];
    vec4 clusterMaxPoint[];
};

// Output decal grid
layout(std430, binding = 1) buffer DecalGrid
{
    uint decalCount; // Global counter for decal indices
    uvec2 decalGridData[]; // x = offset, y = count for each cluster
};

// Output decal indices
layout(std430, binding = 2) writeonly buffer DecalIndices
{
    uint decalIndexList[];
};

// Decal data
layout(std430, binding = 3) readonly buffer DecalBuffer
{
    Decal decals[];
};

// Input uniforms
uniform uint decalCount;
uniform uint clusterCount;
uniform uint maxDecalsPerCluster;

// Forward declarations
bool boxIntersectsBox(vec3 boxAMin, vec3 boxAMax, vec3 boxBMin, vec3 boxBMax);

void main()
{
    // Get global invocation index
    uint clusterIndex = gl_GlobalInvocationID.x;

    // Skip if beyond cluster count
    if (clusterIndex >= clusterCount)
    {
        return;
    }

    // Initialize shared decal count
    if (gl_LocalInvocationIndex == 0)
    {
        s_decalCount = 0;
    }

    // Ensure initialization is complete
    barrier();

    // Get cluster bounds
    vec3 minPoint = clusterMinPoint[clusterIndex].xyz;
    vec3 maxPoint = clusterMaxPoint[clusterIndex].xyz;

    // Check each decal for intersection with this cluster
    for (uint decalIndex = 0; decalIndex < decalCount; decalIndex++)
    {
        Decal decal = decals[decalIndex];

        // Decal bounds are -1 to 1 in local space
        // Transform cluster bounds to decal space to check intersection

        // Simple approximation: create decal world-space bounds
        // In a real implementation, this would use the full transformation

        // Get transform from inverse transform
        mat4 decalTransform = inverse(decal.invTransform);

        // Get corners of the decal box
        vec3 decalCorners[8];
        decalCorners[0] = (decalTransform * vec4(-1.0, -1.0, -1.0, 1.0)).xyz;
        decalCorners[1] = (decalTransform * vec4(1.0, -1.0, -1.0, 1.0)).xyz;
        decalCorners[2] = (decalTransform * vec4(-1.0, 1.0, -1.0, 1.0)).xyz;
        decalCorners[3] = (decalTransform * vec4(1.0, 1.0, -1.0, 1.0)).xyz;
        decalCorners[4] = (decalTransform * vec4(-1.0, -1.0, 1.0, 1.0)).xyz;
        decalCorners[5] = (decalTransform * vec4(1.0, -1.0, 1.0, 1.0)).xyz;
        decalCorners[6] = (decalTransform * vec4(-1.0, 1.0, 1.0, 1.0)).xyz;
        decalCorners[7] = (decalTransform * vec4(1.0, 1.0, 1.0, 1.0)).xyz;

        // Calculate AABB of decal
        vec3 decalMin = decalCorners[0];
        vec3 decalMax = decalCorners[0];

        for (int i = 1; i < 8; i++)
        {
            decalMin = min(decalMin, decalCorners[i]);
            decalMax = max(decalMax, decalCorners[i]);
        }

        // Check if decal box intersects cluster box
        if (boxIntersectsBox(minPoint, maxPoint, decalMin, decalMax))
        {
            uint index = atomicAdd(s_decalCount, 1);

            // Only add if we have space
            if (index < 64)
            {
                s_decalList[index] = decalIndex;
            }
        }
    }

    // Ensure all threads have finished adding decals
    barrier();

    // First thread writes the results to global memory
    if (gl_LocalInvocationIndex == 0 && s_decalCount > 0)
    {
        // Clamp decal count to maximum
        uint count = min(s_decalCount, maxDecalsPerCluster);

        // Atomic add to get our portion of the global decal index list
        uint offset = atomicAdd(decalCount, count);

        // Write offset and count to grid
        decalGridData[clusterIndex] = uvec2(offset, count);

        // Write indices to global list
        for (uint i = 0; i < count; i++)
        {
            decalIndexList[offset + i] = s_decalList[i];
        }
    }
    else if (gl_LocalInvocationIndex == 0)
    {
        // No decals, initialize to zero
        decalGridData[clusterIndex] = uvec2(0, 0);
    }
}

// Utility functions

bool boxIntersectsBox(vec3 boxAMin, vec3 boxAMax, vec3 boxBMin, vec3 boxBMax)
{
    // Check if boxes overlap on all axes
    return (boxAMin.x <= boxBMax.x && boxAMax.x >= boxBMin.x) &&
           (boxAMin.y <= boxBMax.y && boxAMax.y >= boxBMin.y) &&
           (boxAMin.z <= boxBMax.z && boxAMax.z >= boxBMin.z);
}