#version 450

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

layout(set = 1, binding = 0) uniform PerObjectBuffer {
    mat4 worldMatrix;
} object;

#if defined(VERTEX_SHADER)

layout(location = 0) in vec3 inPosition;

void main() {
    vec4 worldPos = object.worldMatrix * vec4(inPosition, 1.0);

    gl_Position = global.lightSpaceMatrix * worldPos;
}

#endif// VERTEX_SHADER

#if defined(FRAGMENT_SHADER)

void main() {
}

#endif// FRAGMENT_SHADER
