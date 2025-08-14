#version 330 core

in vec4 currentPos;
in vec4 previousPos;

uniform float u_velocityScale;
uniform float u_maxVelocity;
uniform int u_ndcEncoding;

out vec2 fragOutput;

void main() {
    // Convert to NDC
    vec2 currentNDC = currentPos.xy / currentPos.w;
    vec2 previousNDC = previousPos.xy / previousPos.w;
    
    // Calculate velocity in screen space
    vec2 velocity = (currentNDC - previousNDC) * u_velocityScale;
    
    // Clamp velocity to maximum
    float velocityLength = length(velocity);
    if (velocityLength > u_maxVelocity) {
        velocity = normalize(velocity) * u_maxVelocity;
    }
    
    // Encode velocity based on encoding type
    if (u_ndcEncoding == 1) {
        // NDC encoding: [-1, 1] -> [0, 1]
        velocity = velocity * 0.5 + 0.5;
    }
    
    fragOutput = velocity;
}
