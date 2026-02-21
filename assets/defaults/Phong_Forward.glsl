#version 450

// =========================================================================
// SHARED SETS (Available to both Vertex and Fragment stages)
// =========================================================================

// SET 0: Global Buffer (PerPass)
// In VulkanShader.cpp, Set 0 is bound for both Vertex and Fragment stages.
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

// SET 0, Binding 1: Shadow Map Sampler (YENİ!)
// sampler2DShadow, derinlik karşılaştırmasını (d < z) otomatik yapar.
layout(set = 0, binding = 1) uniform sampler2DShadow shadowMap;

// =========================================================================
// VERTEX SHADER
// =========================================================================
#if defined(VERTEX_SHADER)

// Attributes (Inputs)
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inTangent;
layout(location = 3) in vec2 inTexCoord;

// Varyings (Outputs to Fragment Shader)
layout(location = 0) out vec3 fragWorldPos;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out mat3 fragTBN;
layout(location = 5) out vec4 fragPosLightSpace; // Shadow Coordinate

// SET 1: Object Buffer
layout(set = 1, binding = 0) uniform PerObjectBuffer {
    mat4 worldMatrix;
} object;

void main() {
    // World Space
    vec4 worldPos = object.worldMatrix * vec4(inPosition, 1.0);
    fragWorldPos = worldPos.xyz;

    // Clip Space
    gl_Position = global.projectionMatrix * global.viewMatrix * worldPos;

    // 3. Light Space Position (Gölge Haritası Koordinatı)
    // Dünya pozisyonunu Işığın bakış açısına çeviriyoruz.
    fragPosLightSpace = global.lightSpaceMatrix * worldPos;

    // 4. Texture Coordinates
    fragTexCoord = inTexCoord;

    vec3 T = normalize(mat3(transpose(inverse(object.worldMatrix))) * inTangent);
    vec3 N = normalize(mat3(transpose(inverse(object.worldMatrix))) * inNormal);

    // Gram-Schmidt process to re-orthogonalize T
    T = normalize(T - dot(T, N) * N);

    // Calculate Bitangent
    vec3 B = -cross(N, T);

    // Pass data to Fragment Shader
    fragTBN = mat3(T, B, N);
}

#endif // VERTEX_SHADER

// =========================================================================
// FRAGMENT SHADER
// =========================================================================
#if defined(FRAGMENT_SHADER)

// Inputs (From Vertex Shader)
layout(location = 0) in vec3 fragWorldPos;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in mat3 fragTBN;
layout(location = 5) in vec4 fragPosLightSpace; // Shadow Coordinate

// Outputs
layout(location = 0) out vec4 outColor;

// SET 2: Material & Textures
// VulkanShader.cpp -> Set 2, Binding 0: Properties
layout(set = 2, binding = 0) uniform PerMaterialBuffer {
    vec4 diffuseColor;
    vec3 specularColor;
    float specularPower;
    vec2 tiling;
    vec2 offset;
} material;

// VulkanShader.cpp -> Set 2, Binding 1: Texture (Combined Image Sampler)
layout(set = 2, binding = 1) uniform sampler2D albedoMap;
layout(set = 2, binding = 2) uniform sampler2D normalMap;

// --- GÖLGE HESAPLAMA ---
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
    // 1. Texture & Normal
    vec4 texColor = texture(albedoMap, fragTexCoord * material.tiling + material.offset);

    if (texColor.a < 0.5) {
        discard;
    }

    // 2. Sample Normal Map and Unpack
    // Normal maps are stored as [0, 1], we need [-1, 1]
    vec3 normalSample = texture(normalMap, fragTexCoord * material.tiling + material.offset).rgb;
    normalSample = normalSample * 2.0 - 1.0;

    // 3. Transform to World Space using TBN
    vec3 N = normalize(fragTBN * normalSample);

    // 4. Lighting Calculation (Blinn-Phong)
    vec3 L = normalize(-global.lightDirection.xyz);
    vec3 V = normalize(global.inverseViewMatrix[3].xyz - fragWorldPos);
    vec3 H = normalize(L + V); // Half vector

    // Ambient
    vec3 ambient = global.ambientLightColor.rgb * global.ambientLightColor.w;

    // Diffuse
    float diff = max(dot(N, L), 0.0);
    vec3 diffuse = diff * global.lightColor.rgb * global.lightColor.w;

    // Specular
    float spec = pow(max(dot(N, H), 0.0), material.specularPower);
    vec3 specular = spec * material.specularColor * global.lightColor.rgb * global.lightColor.w;

    // 4. GÖLGE HESABI
    // Gölge sadece Diffuse ve Specular'ı etkiler, Ambient hep vardır.
    float shadow = CalculateShadow(fragPosLightSpace);

    // 5. Birleştirme
    vec3 lighting = (ambient + (diffuse + specular) * shadow) * texColor.rgb * material.diffuseColor.rgb;

    outColor = vec4(lighting, texColor.a * material.diffuseColor.a);
}
#endif // FRAGMENT_SHADER