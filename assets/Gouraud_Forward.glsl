#version 450

// =========================================================================
// SHARED SETS
// =========================================================================

// SET 0: Global Buffer
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

    mat4 lightSpaceMatrix; // Shadow Matrix
} global;

// SET 0, Binding 1: Shadow Map (Gouraud olsa bile buna ihtiyacimiz var)
layout(set = 0, binding = 1) uniform sampler2DShadow shadowMap;

// =========================================================================
// VERTEX SHADER
// =========================================================================
#if defined(VERTEX_SHADER)

// SET 1: Object Buffer
layout(set = 1, binding = 0) uniform PerObjectBuffer {
    mat4 worldMatrix;
} object;

// SET 2: Material Properties
layout(set = 2, binding = 0) uniform PerMaterialBuffer {
    vec4 diffuseColor;
    vec3 specularColor;
    float specularPower;
    vec2 tiling;
    vec2 offset;
} material;

// Inputs
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inTangent;
layout(location = 3) in vec2 inTexCoord;

// Outputs
// NOT: Gouraud'da golgeyi duzgun yapmak icin Ambient ve Direct Isigi ayri gonderiyoruz.
// Eger hepsini birlestirip gonderirsek, golgeye giren yerler simsiyah (zifiri) olur.
layout(location = 0) out vec3 fragAmbient;      // Ortam Isigi (Golgeden etkilenmez)
layout(location = 1) out vec3 fragDiffSpec;     // Diffuse + Specular (Golgeden etkilenir)
layout(location = 2) out vec2 fragTexCoord;     // Texture UV
layout(location = 3) out vec4 fragPosLightSpace; // Golge Haritasi Koordinati

void main() {
    // 1. World Position
    vec4 worldPos = object.worldMatrix * vec4(inPosition, 1.0);
    gl_Position = global.projectionMatrix * global.viewMatrix * worldPos;

    // 2. Light Space Position (Fragment shader'da golge kontrolu icin)
    fragPosLightSpace = global.lightSpaceMatrix * worldPos;

    // 3. Texture Coordinates
    fragTexCoord = inTexCoord * material.tiling + material.offset;

    // --- GOURAUD ISIK HESABI (Per-Vertex) ---

    vec3 N = normalize(mat3(object.worldMatrix) * inNormal);
    vec3 L = normalize(-global.lightDirection.xyz);
    vec3 V = normalize(global.inverseViewMatrix[3].xyz - worldPos.xyz);
    vec3 H = normalize(L + V);

    // Ambient (Golge dusse bile burasi aydinlik kalmali)
    fragAmbient = global.ambientLightColor.rgb * global.ambientLightColor.a * material.diffuseColor.rgb;

    // Diffuse
    float diff = max(dot(N, L), 0.0);
    vec3 diffuse = diff * global.lightColor.rgb * global.lightColor.a  * material.diffuseColor.rgb;

    // Specular
    float spec = pow(max(dot(N, H), 0.0), material.specularPower);
    vec3 specular = spec * material.specularColor * global.lightColor.rgb * global.lightColor.a;

    // Diffuse ve Specular'i topluyoruz (Bunlar golgelenecek)
    fragDiffSpec = diffuse + specular;
}

#endif // VERTEX_SHADER

// =========================================================================
// FRAGMENT SHADER
// =========================================================================
#if defined(FRAGMENT_SHADER)

// Inputs
layout(location = 0) in vec3 fragAmbient;
layout(location = 1) in vec3 fragDiffSpec;
layout(location = 2) in vec2 fragTexCoord;
layout(location = 3) in vec4 fragPosLightSpace;

layout(location = 0) out vec4 outColor;

// Texture Sampler
layout(set = 2, binding = 1) uniform sampler2D albedoMap;
// Shadow Map Sampler (Set 0 Binding 1 - Yukarida tanimli)

float CalculateShadow(vec4 posLightSpace) {
    // 1. Perspective divide
    vec3 projCoords = posLightSpace.xyz / posLightSpace.w;

    // 2. Transform ONLY X and Y from [-1,1] to [0,1] for UV sampling
    projCoords.xy = projCoords.xy * 0.5 + 0.5;

    // Optional: Bounds check to prevent artifacts outside the map
    if (projCoords.z > 1.0 || projCoords.x < 0.0 || projCoords.x > 1.0 || projCoords.y < 0.0 || projCoords.y > 1.0) {
        return 1.0;
    }

    // 3. Bias
    // Note: Since we use Hardware Comparison, we subtract bias from our Z
    float bias = 0.00005;

    // 4. Sample
    // ERROR FIX: 'sampler2DShadow' takes a vec3(u, v, ref_z)
    // It automatically performs the comparison: (projCoords.z - bias) < storedDepth
    // Returns 1.0 if visible (lit), 0.0 if occluded (shadow)
    float shadow = texture(shadowMap, vec3(projCoords.xy, projCoords.z - bias));

    return shadow;
}

void main() {
    // 1. Texture Rengini Oku
    vec4 texColor = texture(albedoMap, fragTexCoord);

    // 2. Golge Faktorunu Hesapla (Per-Pixel olmak ZORUNDA)
    float shadow = CalculateShadow(fragPosLightSpace);

    // 3. Birlestirme: Ambient + (DiffuseSpecular * Shadow)
    // Ambient isiga golge uygulanmaz!
    vec3 finalLight = fragAmbient + (fragDiffSpec * shadow);

    // 4. Texture ile carp
    vec3 finalColor = finalLight * texColor.rgb;

    outColor = vec4(clamp(finalColor, 0.0, 1.0), texColor.a);
}
#endif // FRAGMENT_SHADER