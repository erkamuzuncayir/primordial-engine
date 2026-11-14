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

layout(set = 1, binding = 0) uniform PerObjectBuffer {
    mat4 worldMatrix;
} object;

layout(set = 2, binding = 0) uniform PerMaterialBuffer {
    vec4 diffuseColor;
    vec3 specularColor;
    float specularPower;
    vec2 tiling;
    vec2 offset;
} material;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inTangent;
layout(location = 3) in vec2 inTexCoord;

layout(location = 0) out vec3 fragAmbient;
layout(location = 1) out vec3 fragDiffSpec;
layout(location = 2) out vec2 fragTexCoord;
layout(location = 3) out vec4 fragPosLightSpace;

void main() {
    vec4 worldPos = object.worldMatrix * vec4(inPosition, 1.0);
    gl_Position = global.projectionMatrix * global.viewMatrix * worldPos;

    fragPosLightSpace = global.lightSpaceMatrix * worldPos;

    fragTexCoord = inTexCoord * material.tiling + material.offset;

    vec3 N = normalize(mat3(object.worldMatrix) * inNormal);
    vec3 L = normalize(-global.lightDirection.xyz);
    vec3 V = normalize(global.inverseViewMatrix[3].xyz - worldPos.xyz);
    vec3 H = normalize(L + V);

    fragAmbient = global.ambientLightColor.rgb * global.ambientLightColor.a * material.diffuseColor.rgb;

    float diff = max(dot(N, L), 0.0);
    vec3 diffuse = diff * global.lightColor.rgb * global.lightColor.a  * material.diffuseColor.rgb;

    float spec = pow(max(dot(N, H), 0.0), material.specularPower);
    vec3 specular = spec * material.specularColor * global.lightColor.rgb * global.lightColor.a;

    fragDiffSpec = diffuse + specular;
}

#endif// VERTEX_SHADER

#if defined(FRAGMENT_SHADER)

layout(location = 0) in vec3 fragAmbient;
layout(location = 1) in vec3 fragDiffSpec;
layout(location = 2) in vec2 fragTexCoord;
layout(location = 3) in vec4 fragPosLightSpace;

layout(location = 0) out vec4 outColor;

layout(set = 2, binding = 1) uniform sampler2D albedoMap;

float CalculateShadow(vec4 posLightSpace) {
    vec3 projCoords = posLightSpace.xyz / posLightSpace.w;
    projCoords.xy = projCoords.xy * 0.5 + 0.5;

    if (projCoords.z > 1.0 || projCoords.x < 0.0 || projCoords.x > 1.0 || projCoords.y < 0.0 || projCoords.y > 1.0)
    return 1.0;

    float bias = 0.00005;
    float shadow = texture(shadowMap, vec3(projCoords.xy, projCoords.z - bias));

    return shadow;
}

void main() {
    vec4 texColor = texture(albedoMap, fragTexCoord);
    float shadow = CalculateShadow(fragPosLightSpace);

    vec3 finalLight = fragAmbient + (fragDiffSpec * shadow);
    vec3 finalColor = finalLight * texColor.rgb;

    outColor = vec4(clamp(finalColor, 0.0, 1.0), texColor.a);
}
#endif// FRAGMENT_SHADER