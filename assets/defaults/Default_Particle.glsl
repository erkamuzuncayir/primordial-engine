#version 450

#if defined(VERTEX_SHADER)

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;

layout(location = 2) in vec3 inInstancePos;
layout(location = 3) in vec4 inInstanceColor;
layout(location = 4) in float inInstanceSize;

layout(location = 0) out vec2 fragTexCoord;
layout(location = 1) out vec4 fragColor;

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
    vec3 cameraRight = vec3(global.viewMatrix[0][0], global.viewMatrix[1][0], global.viewMatrix[2][0]);
    vec3 cameraUp    = vec3(global.viewMatrix[0][1], global.viewMatrix[1][1], global.viewMatrix[2][1]);

    vec3 worldPos = inInstancePos
    + (cameraRight * inPosition.x * inInstanceSize)
    + (cameraUp * inPosition.y * inInstanceSize);

    gl_Position = global.projectionMatrix * global.viewMatrix * vec4(worldPos, 1.0);

    fragTexCoord = inTexCoord;
    fragColor = inInstanceColor;
}
#endif// VERTEX_SHADER

#if defined(FRAGMENT_SHADER)

layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) in vec4 fragColor;

layout(location = 0) out vec4 outColor;

layout(set = 1, binding = 0) uniform sampler2D texSampler;

void main() {
    vec4 texColor = texture(texSampler, fragTexCoord);
    outColor = texColor * fragColor;
    if (outColor.a < 0.05) discard;
}
#endif// FRAGMENT_SHADER
