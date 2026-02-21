#version 450

// =========================================================================
// SHARED SETS
// =========================================================================

// SET 0: Global Buffer (PerPass)
// Shadow Pass sirasinda C++ tarafinda Set 0 bind edilir.
// Buradaki 'lightSpaceMatrix' bizim icin kritik olandir.
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

    mat4 lightSpaceMatrix; // <--- KRITIK VERI
} global;

// SET 1: Object Buffer
// Her objenin kendi Dunya matrisi
layout(set = 1, binding = 0) uniform PerObjectBuffer {
    mat4 worldMatrix;
} object;

// NOT: Set 2 (Material) burada tanimlanmaz cunku Shadow Pass
// sirasinda texture veya materyal baglamiyoruz (C++ tarafinda).

// =========================================================================
// VERTEX SHADER
// =========================================================================
#if defined(VERTEX_SHADER)

// Attributes (Sadece Pozisyon yeterli)
layout(location = 0) in vec3 inPosition;
// Normal, Tangent ve UV'ye golge haritasi cikarirken ihtiyac yok.

void main() {
    // 1. Model Space -> World Space
    vec4 worldPos = object.worldMatrix * vec4(inPosition, 1.0);

    // 2. World Space -> Light Clip Space
    // Standart kameranin view/proj matrisi yerine, isigin matrisini kullaniyoruz.
    gl_Position = global.lightSpaceMatrix * worldPos;
}

#endif // VERTEX_SHADER

// =========================================================================
// FRAGMENT SHADER
// =========================================================================
#if defined(FRAGMENT_SHADER)

// Shadow Map Pass'inde Fragment Shader bostur.
// Cunku renk ciktisi (outColor) yoktur.
// Vulkan (ve OpenGL), derinlik (Depth) buffer'ina yazma islemini
// Fragment Shader bittikten sonra otomatik yapar.
// Eger "Alpha Testing" (Yapraklar vb.) yapmayacaksak burasi bos kalir.

void main() {
    // Explicitly do nothing.
    // Depth write is implicit.
}

#endif // FRAGMENT_SHADER