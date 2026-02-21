#version 450

// =========================================================================
// VERTEX SHADER
// =========================================================================
#if defined(VERTEX_SHADER)
layout(location = 0) in vec3 inPosition; // Quad Vertex Position
layout(location = 1) in vec2 inTexCoord; // Quad UV

// Instance Attributes (Binding 1)
layout(location = 2) in vec3 inInstancePos;
layout(location = 3) in vec4 inInstanceColor;
layout(location = 4) in float inInstanceSize;

layout(location = 0) out vec2 fragTexCoord;
layout(location = 1) out vec4 fragColor;

// Reuse standard Global Buffer for Camera Matrices
layout(set = 0, binding = 0) uniform PerPassBuffer {
    mat4 viewMatrix;
    mat4 projectionMatrix;
    mat4 inverseViewMatrix;
    mat4 inverseProjectionMatrix;

    float time;
    float deltaTime;
    vec2 resolution;
    vec2 inverseResolution;
    vec2 _pad0;

    vec4 lightColor;
    vec4 lightDirection;
    vec4 ambientLightColor;

    mat4 lightSpaceMatrix;
} global;

void main() {
    // Billboarding: Construct rotation from View Matrix
    // In a View Matrix (Left Handed), Column 0 is Right, Column 1 is Up
    vec3 cameraRight = vec3(global.viewMatrix[0][0], global.viewMatrix[1][0], global.viewMatrix[2][0]);
    vec3 cameraUp    = vec3(global.viewMatrix[0][1], global.viewMatrix[1][1], global.viewMatrix[2][1]);

    // Calculate vertex world position
    vec3 worldPos = inInstancePos
    + (cameraRight * inPosition.x * inInstanceSize)
    + (cameraUp * inPosition.y * inInstanceSize);

    gl_Position = global.projectionMatrix * global.viewMatrix * vec4(worldPos, 1.0);

    fragTexCoord = inTexCoord;
    fragColor = inInstanceColor;
}
#endif // VERTEX_SHADER

// =========================================================================
// FRAGMENT SHADER
// =========================================================================
#if defined(FRAGMENT_SHADER)
layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) in vec4 fragColor;

layout(location = 0) out vec4 outColor;

// Separate Set for Particle Texture to keep it simple
layout(set = 1, binding = 0) uniform sampler2D texSampler;

void main() {
    vec4 texColor = texture(texSampler, fragTexCoord);
    outColor = texColor * fragColor;

    // Simple Alpha Test
    if(outColor.a < 0.05) discard;
}
#endif // FRAGMENT_SHADER