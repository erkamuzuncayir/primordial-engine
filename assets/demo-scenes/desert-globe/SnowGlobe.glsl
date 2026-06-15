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

#if defined(VERTEX_SHADER)

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inTangent;
layout(location = 3) in vec2 inTexCoord;

layout(location = 0) out vec3 fragWorldPos;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec3 fragLocalPos;

layout(set = 1, binding = 0) uniform PerObjectBuffer {
    mat4 worldMatrix;
} object;

void main() {
    vec4 worldPos = object.worldMatrix * vec4(inPosition, 1.0);
    fragWorldPos = worldPos.xyz;

    fragLocalPos = inPosition;

    fragNormal = normalize(mat3(transpose(inverse(object.worldMatrix))) * inNormal);

    gl_Position = global.projectionMatrix * global.viewMatrix * worldPos;
}

#endif // VERTEX_SHADER

#if defined(FRAGMENT_SHADER)

layout(location = 0) in vec3 fragWorldPos;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec3 fragLocalPos;

layout(location = 0) out vec4 outColor;

layout(set = 2, binding = 0) uniform PerMaterialBuffer {
    vec4 diffuseColor;
    vec3 specularColor;
    float specularPower;
    vec2 tiling;
    vec2 offset;
} material;

layout(set = 2, binding = 1) uniform samplerCube envMap;

vec3 DrawSunOnSkybox(vec3 viewDir, vec3 lightDir, vec3 skyColor) {
    float sunDot = dot(viewDir, lightDir);

    float sunSize = 0.999;

    float sunMask = smoothstep(sunSize, sunSize + 0.002, sunDot);

    vec3 sunColor = vec3(1.0, 0.9, 0.6) * 5.0;

    return mix(skyColor, sunColor, sunMask);
}

void main() {
    vec3 L = normalize(-global.lightDirection.xyz);
    if (!gl_FrontFacing) {
        vec3 viewDir = normalize(fragLocalPos);

        vec3 skyColor = texture(envMap, viewDir).rgb;
        skyColor *= 0.6;
        
        vec3 sunDir = L;

        vec3 finalSky = DrawSunOnSkybox(viewDir, sunDir, skyColor);

        outColor = vec4(finalSky, 1.0);
        return;
    }

    vec3 N = normalize(fragNormal);
    vec3 V = normalize(global.inverseViewMatrix[3].xyz - fragWorldPos);

    float F0 = 0.1;
    float fresnel = F0 + (1.0 - F0) * pow(1.0 - max(dot(N, V), 0.0), material.specularPower);

    vec3 R = reflect(-V, N);
    vec3 reflectionColor = texture(envMap, R).rgb * material.specularColor;

    vec3 finalColor = mix(material.diffuseColor.rgb, reflectionColor, fresnel);
    float finalAlpha = material.diffuseColor.a + (fresnel * 0.5);

    outColor = vec4(finalColor, clamp(finalAlpha, 0.0, 1.0));
}
#endif // FRAGMENT_SHADER
