#pragma once
#include <cstdint>

#include "ECS/Entity.h"
#include "Math/Math.h"
#include "Utilities/EnumReflection.h"
#include "Vertex.h"

#ifdef PE_D3D11
#include <d3d11.h>
#else
#include <vulkan/vulkan_core.h>
#endif

namespace PE::Graphics {
using TextureID		 = uint32_t;
using ShaderID		 = uint32_t;
using MaterialID	 = uint32_t;
using MeshID		 = uint32_t;
using RenderTargetID = uint32_t;
using SamplerID		 = uint32_t;

constexpr uint32_t INVALID_HANDLE = UINT32_MAX;

enum class RenderPass : uint8_t { GBuffer = 0, Shadow = 1, Lighting = 2, Forward = 3, UI = 4 };

struct RenderKey {
	uint64_t value;

	static uint64_t Create(const uint8_t layer, const ShaderID shaderID, const MaterialID matID, const uint32_t depth) {
		return (static_cast<uint64_t>(layer) << 60) | ((static_cast<uint64_t>(shaderID) & 0xFFF) << 48) |
			   ((static_cast<uint64_t>(matID) & 0xFFFF) << 32) | (depth & 0xFFFFFFFF);
	}
};

// TODO: Make enum class
enum RenderFlags : uint8_t {
	RenderFlag_None				= 0,
	RenderFlag_Visible			= 1 << 0,
	RenderFlag_CastShadows		= 1 << 1,
	RenderFlag_ReceiveShadows	= 1 << 2,
	RenderFlag_ForceTransparent = 1 << 3
};

struct RenderCommand {
	uint64_t	key		   = UINT64_MAX;
	MeshID		meshID	   = INVALID_HANDLE;
	MaterialID	materialID = INVALID_HANDLE;
	Math::Mat44 worldMatrix{};
	uint32_t	ownerEntityID = ECS::INVALID_ENTITY_ID;
	uint8_t		flags		  = RenderFlag_None;
};

enum class PrimitiveType { Box, Sphere, Geosphere, Cylinder, Grid, Quad, FullscreenQuad, DesertMesh };

struct MeshData {
	std::vector<Vertex>	  Vertices;
	std::vector<uint32_t> Indices;
};

enum class TextureType : uint8_t {
	Albedo,
	Normal,
	Mask,
	Emissive,
	Displacement,

	Terrain_Splat,
	Terrain_Layer_0,
	Terrain_Layer_1,
	Terrain_Layer_2,
	Terrain_Layer_3,

	Generic_0,
	Generic_1,

	Count = 12
};

static inline constexpr std::array<Utilities::EnumEntry<TextureType>, static_cast<int>(TextureType::Count)>
	TEX_TYPE_MAP{{{"Albedo", TextureType::Albedo},
				  {"Normal", TextureType::Normal},
				  {"Mask", TextureType::Mask},
				  {"Emissive", TextureType::Emissive},
				  {"Displacement", TextureType::Displacement},
				  {"Terrain_Splat", TextureType::Terrain_Splat},
				  {"Terrain_Layer_0", TextureType::Terrain_Layer_0},
				  {"Terrain_Layer_1", TextureType::Terrain_Layer_1},
				  {"Terrain_Layer_2", TextureType::Terrain_Layer_2},
				  {"Terrain_Layer_3", TextureType::Terrain_Layer_3},
				  {"Generic_0", TextureType::Generic_0},
				  {"Generic_1", TextureType::Generic_1}}};

struct TextureParameters {
	TextureType type		= TextureType::Albedo;
	uint16_t	width		= 1;
	uint16_t	height		= 1;
	uint8_t		depth		= 32;
	uint8_t		mipLevels	= 1;
	uint8_t		arrayLayers = 1;
	uint8_t		samples		= 1;
	bool		isCubemap	= false;

	union {
#ifdef PE_D3D11
		DXGI_FORMAT dxgiFormat = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
#else
		VkFormat vulkanFormat = VK_FORMAT_R8G8B8A8_SRGB;
#endif
	} format;

	union {
#ifdef PE_D3D11
		D3D11_USAGE d3d11Usage = D3D11_USAGE_DEFAULT;
#else
		VkImageUsageFlags vulkanUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
#endif
	} usage;
};

enum class SamplerType : uint8_t {
	LinearRepeat = 0,
	LinearClamp,
	PointRepeat,
	PointClamp,
	Anisotropic,
	ShadowPCF,
	Count
};

static constexpr std::array<Utilities::EnumEntry<SamplerType>, static_cast<int>(SamplerType::Count)> SAMPLER_TYPE_MAP{
	{{"LinearRepeat", SamplerType::LinearRepeat},
	 {"LinearClamp", SamplerType::LinearClamp},
	 {"PointRepeat", SamplerType::PointRepeat},
	 {"PointClamp", SamplerType::PointClamp},
	 {"Anisotropic", SamplerType::Anisotropic},
	 {"ShadowPCF", SamplerType::ShadowPCF}}};

enum class MaterialProperty : uint8_t {
	// Color / Texture
	Color  = 0,
	Tiling = 1,
	Offset = 2,

	// Lit / PBR
	SpecularColor	  = 3,
	SpecularPower	  = 4,
	Roughness		  = 5,
	Metallic		  = 6,
	NormalStrength	  = 7,
	OcclusionStrength = 8,
	EmissiveColor	  = 9,
	EmissiveIntensity = 10,

	// Terrain
	LayerTiling	  = 11,
	BlendDistance = 12,
	BlendFalloff  = 13,

	// UI
	Opacity			= 14,
	BorderColor		= 15,
	BorderThickness = 16,
	Softness		= 17,
	Padding			= 18,

	Count = 19
};

static constexpr std::array<Utilities::EnumEntry<MaterialProperty>, static_cast<int>(MaterialProperty::Count)>
	MAT_PROP_MAP{{{"Color", MaterialProperty::Color},
				  {"Tiling", MaterialProperty::Tiling},
				  {"Offset", MaterialProperty::Offset},
				  {"SpecularColor", MaterialProperty::SpecularColor},
				  {"SpecularPower", MaterialProperty::SpecularPower},
				  {"Roughness", MaterialProperty::Roughness},
				  {"Metallic", MaterialProperty::Metallic},
				  {"NormalStrength", MaterialProperty::NormalStrength},
				  {"OcclusionStrength", MaterialProperty::OcclusionStrength},
				  {"EmissiveColor", MaterialProperty::EmissiveColor},
				  {"EmissiveIntensity", MaterialProperty::EmissiveIntensity},
				  {"LayerTiling", MaterialProperty::LayerTiling},
				  {"BlendDistance", MaterialProperty::BlendDistance},
				  {"BlendFalloff", MaterialProperty::BlendFalloff},
				  {"Opacity", MaterialProperty::Opacity},
				  {"BorderColor", MaterialProperty::BorderColor},
				  {"BorderThickness", MaterialProperty::BorderThickness},
				  {"Softness", MaterialProperty::Softness},
				  {"Padding", MaterialProperty::Padding}}};

struct MaterialPropertyLayout {
	uint32_t offset = 0;
	uint32_t size	= 0;
};

enum class ShaderType : uint8_t { Unlit, Lit, PBR, Shadow, Terrain, UI, Particle, SnowGlobe, Count };

static constexpr std::array<Utilities::EnumEntry<ShaderType>, static_cast<int>(ShaderType::Count)> SHADER_TYPE_MAP{{
	{"Unlit", ShaderType::Unlit},
	{"Lit", ShaderType::Lit},
	{"PBR", ShaderType::PBR},
	{"Shadow", ShaderType::Shadow},
	{"Terrain", ShaderType::Terrain},
	{"UI", ShaderType::UI},
	{"Particle", ShaderType::Particle},
	{"SnowGlobe", ShaderType::SnowGlobe},
}};

struct alignas(16) CBPerPass {
	Math::Mat44 view;
	Math::Mat44 projection;
	Math::Mat44 inverseView;
	Math::Mat44 inverseProjection;

	float	   time;
	float	   deltaTime;
	Math::Vec2 resolution;
	Math::Vec2 inverseResolution;
	float	   _pad0[2];

	Math::Vec4 lightColor;
	Math::Vec4 lightDirection;
	Math::Vec4 ambientLightColor;

	Math::Mat44 lightSpaceMatrix;
};

struct alignas(16) CBPerObject {
	Math::Mat44 world;
};

struct alignas(16) CBMaterial_Lit {
	Math::Vec4 diffuseColor;

	Math::Vec3 specularColor;
	float	   specularPower;

	Math::Vec2 tiling;
	Math::Vec2 offset;
};

struct alignas(16) CBMaterial_Unlit {
	Math::Vec4 color;

	Math::Vec2 tiling;
	Math::Vec2 offset;
};

struct alignas(16) CBMaterial_PBR {
	Math::Vec4 albedoColor;

	float normalStrength;
	float roughness;
	float metallic;
	float occlusionStrength;

	Math::Vec3 emissiveColor;
	float	   emissiveIntensity;

	Math::Vec2 tiling;
	Math::Vec2 offset;
};

struct alignas(16) CBMaterial_Terrain {
	Math::Vec4 layerTiling;

	float blendDistance;
	float blendFalloff;
	float padding[2];
};

struct alignas(16) CBMaterial_UI {
	Math::Vec4 color;

	float opacity;
	float borderThickness;
	float softness;
	float padding;

	Math::Vec4 borderColor;
};

enum class ParticleType { Fire, Rain, Snow, Dust, Custom, Count };

static constexpr std::array<Utilities::EnumEntry<ParticleType>, static_cast<int>(ParticleType::Count)>
	PARTICLE_TYPE_MAP{{{"Fire", ParticleType::Fire},
					   {"Rain", ParticleType::Rain},
					   {"Snow", ParticleType::Snow},
					   {"Dust", ParticleType::Dust},
					   {"Custom", ParticleType::Custom}}};

struct GPUInstanceData {
	Math::Vec3 Position;
	Math::Vec4 Color;
	float	   Size;

	static VkVertexInputBindingDescription GetBindingDescription() {
		constexpr VkVertexInputBindingDescription bindingDescription{
			.binding   = 1,
			.stride	   = sizeof(GPUInstanceData),
			.inputRate = VK_VERTEX_INPUT_RATE_INSTANCE,
		};
		return bindingDescription;
	}

	static std::array<VkVertexInputAttributeDescription, 3> GetAttributeDescription() {
		std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};

		attributeDescriptions[0].binding  = 1;
		attributeDescriptions[0].location = 2;
		attributeDescriptions[0].format	  = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[0].offset	  = offsetof(GPUInstanceData, Position);

		attributeDescriptions[1].binding  = 1;
		attributeDescriptions[1].location = 3;
		attributeDescriptions[1].format	  = VK_FORMAT_R32G32B32A32_SFLOAT;
		attributeDescriptions[1].offset	  = offsetof(GPUInstanceData, Color);

		attributeDescriptions[2].binding  = 1;
		attributeDescriptions[2].location = 4;
		attributeDescriptions[2].format	  = VK_FORMAT_R32_SFLOAT;
		attributeDescriptions[2].offset	  = offsetof(GPUInstanceData, Size);

		return attributeDescriptions;
	}
};

struct ParticleBatch {
	TextureID					 textureID;
	std::vector<GPUInstanceData> instances;
};
}  // namespace PE::Graphics