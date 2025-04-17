#version 430 core
layout(location = 0) in vec3 aPos;

uniform mat4 view;
uniform mat4 projection;

// Cluster AABBs
layout(std430, binding = 0) readonly buffer ClusterAABBs
{
    vec4 minPoints[];
    vec4 maxPoints[];
};

out vec3 fragColor;

void main()
{
    // Get the cluster index from gl_InstanceID
    uint clusterIndex = gl_InstanceID;

    // Get cluster bounds
    vec3 minPoint = minPoints[clusterIndex].xyz;
    vec3 maxPoint = maxPoints[clusterIndex].xyz;

    // Scale and translate the unit cube to match the cluster bounds
    vec3 scale = maxPoint - minPoint;
    vec3 position = aPos * scale + minPoint;

    // Calculate cluster color based on index for visualization
    vec3 color = vec3(
        float(clusterIndex % 64) / 64.0,
        float((clusterIndex / 64) % 64) / 64.0,
        float(clusterIndex / 4096) / 16.0
    );

    fragColor = color;

    gl_Position = projection * view * vec4(position, 1.0);
}