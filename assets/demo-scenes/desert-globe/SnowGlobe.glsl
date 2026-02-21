#version 450

// =========================================================================
// SET 0: Global Buffer
// =========================================================================
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

// =========================================================================
// VERTEX SHADER
// =========================================================================
#if defined(VERTEX_SHADER)

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inTangent;
layout(location = 3) in vec2 inTexCoord;

layout(location = 0) out vec3 fragWorldPos;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec3 fragLocalPos; // Skybox örneklemesi için yerel pozisyon

layout(set = 1, binding = 0) uniform PerObjectBuffer {
    mat4 worldMatrix;
} object;

void main() {
    vec4 worldPos = object.worldMatrix * vec4(inPosition, 1.0);
    fragWorldPos = worldPos.xyz;

    // Yerel pozisyonu fragmente taşı (Kürenin merkezi 0,0,0 kabul edilir)
    fragLocalPos = inPosition;

    // Normali dünya uzayına çevir
    fragNormal = normalize(mat3(transpose(inverse(object.worldMatrix))) * inNormal);

    gl_Position = global.projectionMatrix * global.viewMatrix * worldPos;
}

#endif // VERTEX_SHADER

// =========================================================================
// FRAGMENT SHADER
// =========================================================================
#if defined(FRAGMENT_SHADER)

layout(location = 0) in vec3 fragWorldPos;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec3 fragLocalPos;

layout(location = 0) out vec4 outColor;

// SET 2: Material (CBMaterial_Lit Yapısının Birebir Kopyası)
layout(set = 2, binding = 0) uniform PerMaterialBuffer {
    vec4 diffuseColor;       // C++: diffuseColor (Cam Rengi + Opaklık)

    vec3 specularColor;  // C++: specularColor (Yansıma Rengi)
    float specularPower;   // C++: specularPower (Kenar Parlaklığı)

    vec2 tiling;          // C++: tiling (Kullanılmıyor ama padding için şart)
    vec2 offset;          // C++: offset (Kullanılmıyor ama padding için şart)
} material;

// Cubemap Texture (Binding 1)
layout(set = 2, binding = 1) uniform samplerCube envMap;

vec3 DrawSunOnSkybox(vec3 viewDir, vec3 lightDir, vec3 skyColor) {
    // Bakış yönümüz ile Işık yönü çakışıyor mu?
    float sunDot = dot(viewDir, lightDir);

    // Güneş Boyutu (1.0'a ne kadar yakınsa o kadar küçük)
    float sunSize = 0.999;

    // Kenar yumuşatma (Anti-aliasing)
    float sunMask = smoothstep(sunSize, sunSize + 0.002, sunDot);

    // Güneş Rengi (HDR gibi parlak sarı)
    vec3 sunColor = vec3(1.0, 0.9, 0.6) * 5.0;

    // Gökyüzü rengi ile Güneşi karıştır (Maske 1 ise Güneş, 0 ise Gökyüzü)
    return mix(skyColor, sunColor, sunMask);
}

void main() {
    vec3 L = normalize(-global.lightDirection.xyz);
    // =====================================================================
    // DURUM 1: İÇERİDEN BAKIŞ (Back Faces)
    // =====================================================================
    // Eğer yüzeyin ters tarafındaysak, kürenin içindeyiz demektir.
    // Vulkan koordinat sistemine ve modelin sarım yönüne (Winding) göre
    // gl_FrontFacing değeri değişebilir. Eğer ters çalışırsa buraya ! koyun.
    if (!gl_FrontFacing) {
        // 1. Skybox Koordinatını al
        vec3 viewDir = normalize(fragLocalPos);
  //      viewDir.y *= -1.0; // Vulkan Flip

        // 2. Dokudan rengi oku
        vec3 skyColor = texture(envMap, viewDir).rgb;
        skyColor *= 0.6; // İçeriyi biraz karart (Dışarısı patlasın diye)

        // 3. ENVIRONMENT MAPPING ÜZERİNE GÜNEŞ ÇİZ (İstediğin Kısım)
        // Işık yönünü de flip etmemiz gerekebilir çünkü Skybox flip edildi.
        vec3 sunDir = L;
//        sunDir.y *= -1.0;

        vec3 finalSky = DrawSunOnSkybox(viewDir, sunDir, skyColor);

        outColor = vec4(finalSky, 1.0);
        return;
    }

    // =====================================================================
    // DURUM 2: DIŞARIDAN BAKIŞ (Front Faces - Cam Efekti)
    // =====================================================================

    vec3 N = normalize(fragNormal);
    vec3 V = normalize(global.inverseViewMatrix[3].xyz - fragWorldPos);

    // 1. Fresnel Hesaplama (Kenarlar daha yansıtıcı)
    float F0 = 0.1;
    float fresnel = F0 + (1.0 - F0) * pow(1.0 - max(dot(N, V), 0.0), material.specularPower);

    // 2. Yansıma (Reflection)
    vec3 R = reflect(-V, N);
    // Yansıma rengini örnekle ve materyaldeki yansıma rengiyle çarp
    vec3 reflectionColor = texture(envMap, R).rgb * material.specularColor;

    // 3. Karıştırma (Cam Rengi ile Yansıma)
    // Fresnel arttıkça yansıma artar, azaldıkça camın kendi rengi görünür
    vec3 finalColor = mix(material.diffuseColor.rgb, reflectionColor, fresnel);

    // 4. Alpha (Kenarlar daha opak, orta daha şeffaf)
    float finalAlpha = material.diffuseColor.a + (fresnel * 0.5);

    outColor = vec4(finalColor, clamp(finalAlpha, 0.0, 1.0));
}
#endif // FRAGMENT_SHADER