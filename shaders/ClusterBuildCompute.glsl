#version 430 core
layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

// Uniforms
uniform mat4 viewProj;
uniform mat4 invViewProj;
uniform float nearClip;
uniform float farClip;
uniform int clusterDimX;
uniform int clusterDimY;
uniform int clusterDimZ;

// Buffers
layout(std430, binding = 0) buffer ClusterAABBs
{
    vec4 minPoints[];
    vec4 maxPoints[];
};

// Convert normalized device coordinates to view space
vec4 NDCToView(vec3 ndc)
{
    vec4 clipSpace = vec4(ndc, 1.0);
    vec4 viewSpace = invViewProj * clipSpace;
    return viewSpace / viewSpace.w;
}

void main()
{
    // Get the cluster coordinates
    uvec3 clusterID = gl_GlobalInvocationID;

    // Check if we're within bounds
    if (clusterID.x >= clusterDimX || clusterID.y >= clusterDimY || clusterID.z >= clusterDimZ)
    {
        return;
    }

    // Calculate cluster index
    uint clusterIndex = clusterID.x +
         clusterID.y + clusterDimX +
         clusterID.z * clusterDimX * clusterDimY;

    // Calculate cluster corners in NDC space
    // X and Y range from -1 to 1, Z ranges from 0 to 1
    float xStep = 2.0 / float(clusterDimX);
    float yStep = 2.0 / float(clusterDimY);
    float zStep = 1.0 / float(clusterDimZ);

    // In orthographic projection, we use linear depth distribution
    float near = nearClip;
    float far = farClip;
    float linearDepth = near + (far - near) * (float(clusterID.z) / float(clusterDimZ));
    float nextLinearDepth = near + (far - near) * (float(clusterID.z + 1) / float(clusterDimZ));

    vec3 minNDC, maxNDC;

    // For orthographic projection, we work directly in view space
    // Convert cluster coordinates to NDC space
    minNDC.x = -1.0 + float(clusterID.x) * xStep;
    minNDC.y = -1.0 + float(clusterID.y) * yStep;
    minNDC.z = float(clusterID.z) / float(clusterDimZ);  // 0 to 1 range

    maxNDC.x = -1.0 + float(clusterID.x + 1) * xStep;
    maxNDC.y = -1.0 + float(clusterID.y + 1) * yStep;
    maxNDC.z = float(clusterID.z + 1) / float(clusterDimZ);  // 0 to 1 range

    // Convert NDC corners to view space (8 corners of the frustum slice)
    vec4 corners[8];
    corners[0] = NDCToView(vec3(minNDC.x, minNDC.y, minNDC.z));
    corners[1] = NDCToView(vec3(maxNDC.x, minNDC.y, minNDC.z));
    corners[2] = NDCToView(vec3(minNDC.x, maxNDC.y, minNDC.z));
    corners[3] = NDCToView(vec3(maxNDC.x, maxNDC.y, minNDC.z));
    corners[4] = NDCToView(vec3(minNDC.x, minNDC.y, maxNDC.z));
    corners[5] = NDCToView(vec3(maxNDC.x, minNDC.y, maxNDC.z));
    corners[6] = NDCToView(vec3(minNDC.x, maxNDC.y, maxNDC.z));
    corners[7] = NDCToView(vec3(maxNDC.x, maxNDC.y, maxNDC.z));

    // Find min and max points of the cluster bounds
    vec3 minPoint = corners[0].xyz;
    vec3 maxPoint = corners[0].xyz;

    for (int i = 1; i < 8; i++)
    {
        minPoint = min(minPoint, corners[i].xyz);
        maxPoint = max(maxPoint, corners[i].xyz);
    }

    // Store the cluster AABB
    minPoints[clusterIndex] = vec4(minPoint, 1.0);
    maxPoints[clusterIndex] = vec4(maxPoint, 1.0);
}