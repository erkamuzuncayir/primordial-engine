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

layout(set = 0, binding = 1) uniform sampler2DShadow shadowMap;

#if defined(VERTEX_SHADER)

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inTangent;
layout(location = 3) in vec2 inTexCoord;

layout(location = 0) out vec2 fragTexCoord;

layout(set = 1, binding = 0) uniform PerObjectBuffer {
    mat4 worldMatrix;
} object;

void main() {
    vec4 worldPos = object.worldMatrix * vec4(inPosition, 1.0);
    gl_Position = global.projectionMatrix * global.viewMatrix * worldPos;
    fragTexCoord = inTexCoord;
}

#endif// VERTEX_SHADER

#if defined(FRAGMENT_SHADER)

layout(location = 0) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

layout(set = 2, binding = 0) uniform PerMaterialBuffer {
    vec4 diffuseColor;
    vec2 tiling;
    vec2 offset;
} material;

layout(set = 2, binding = 1) uniform sampler2D albedoMap;

void main() {
    vec4 texColor = texture(albedoMap, fragTexCoord * material.tiling + material.offset);
    vec3 finalColor = texColor.rgb * material.diffuseColor.rgb;
    outColor = vec4(finalColor, texColor.a * material.diffuseColor.a);
}
#endif// FRAGMENT_SHADER
