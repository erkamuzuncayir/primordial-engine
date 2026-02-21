#version 450

// =========================================================================
// SHARED SETS
// =========================================================================

// SET 0: Global Buffer
// Unlit shader bu verilerin çogunu kullanmaz ama
// C++ tarafindaki Pipeline Layout ile uyumlu olmasi icin
// tanimlamalari aynen tutuyoruz.
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

// SET 0, Binding 1: Shadow Map
// Unlit shader golge okumaz ama Layout uyumu icin tanimliyoruz.
// Kullanmadigimiz icin Vulkan "unused" uyarisi verebilir, sorun degil.
layout(set = 0, binding = 1) uniform sampler2DShadow shadowMap;

// =========================================================================
// VERTEX SHADER
// =========================================================================
#if defined(VERTEX_SHADER)

// Inputs
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;   // Unlit icin gereksiz ama format geregi var
layout(location = 2) in vec3 inTangent;  // Unlit icin gereksiz
layout(location = 3) in vec2 inTexCoord;

// Outputs
layout(location = 0) out vec2 fragTexCoord;

// SET 1: Object Buffer
layout(set = 1, binding = 0) uniform PerObjectBuffer {
    mat4 worldMatrix;
} object;

void main() {
    // 1. World Position
    vec4 worldPos = object.worldMatrix * vec4(inPosition, 1.0);

    // 2. Clip Space (Ekrana Basma)
    gl_Position = global.projectionMatrix * global.viewMatrix * worldPos;

    // 3. Texture Coordinates
    fragTexCoord = inTexCoord;
}

#endif // VERTEX_SHADER

// =========================================================================
// FRAGMENT SHADER
// =========================================================================
#if defined(FRAGMENT_SHADER)

// Inputs
layout(location = 0) in vec2 fragTexCoord;

// Outputs
layout(location = 0) out vec4 outColor;

// SET 2: Material Properties
// Unlit sadece Diffuse Color ve Tiling kullanir.
// Specular, Roughness vb. yoksayilir.
layout(set = 2, binding = 0) uniform PerMaterialBuffer {
    vec4 diffuseColor;
    vec2 tiling;
    vec2 offset;
} material;

// SET 2: Textures
layout(set = 2, binding = 1) uniform sampler2D albedoMap;
// Normal map unlit shader'da kullanilmaz.

void main() {
    // 1. Texture Rengini Oku
    vec4 texColor = texture(albedoMap, fragTexCoord * material.tiling + material.offset);

    // 2. Final Renk (Işık hesaplaması YOK)
    // Sadece Texture Rengi * Materyal Rengi
    vec3 finalColor = texColor.rgb * material.diffuseColor.rgb;

    // Alpha kanalini koru
    outColor = vec4(finalColor, texColor.a * material.diffuseColor.a);
}
#endif // FRAGMENT_SHADER