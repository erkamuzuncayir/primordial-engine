#include "Assets/Model.h"

#include <algorithm>
#include <fstream>
#include <sstream>
#include <vector>

#include "Assets/AssetManager.h"
#include "Assets/Texture.h"
#include "Utilities/Logger.h"

namespace PE::Assets::Model::Loader {
struct ObjIndex {
	int	 p = -1, t = -1, n = -1;
	bool operator==(const ObjIndex &o) const { return p == o.p && t == o.t && n == o.n; }
};

struct ObjIndexHash {
	size_t operator()(const ObjIndex &k) const {
		return ((std::hash<int>()(k.p) ^ (std::hash<int>()(k.t) << 1)) >> 1) ^ (std::hash<int>()(k.n) << 1);
	}
};

ObjIndex ParseFaceIndex(const std::string &token) {
	ObjIndex		  idx;
	std::stringstream ss(token);
	ss.imbue(std::locale::classic());
	std::string				 segment;
	std::vector<std::string> parts;
	while (std::getline(ss, segment, '/')) parts.push_back(segment);

	if (!parts.empty() && !parts[0].empty()) idx.p = std::stoi(parts[0]);
	if (parts.size() > 1 && !parts[1].empty()) idx.t = std::stoi(parts[1]);
	if (parts.size() > 2 && !parts[2].empty()) idx.n = std::stoi(parts[2]);
	return idx;
}

void CalculateTangents(std::vector<Graphics::Vertex> &vertices, const std::vector<uint32_t> &indices) {
	std::vector<Math::Vec3> tangents(vertices.size(), Math::Vec3(0.0f, 0.0f, 0.0f));

	for (size_t i = 0; i < indices.size(); i += 3) {
		uint32_t i0 = indices[i];
		uint32_t i1 = indices[i + 1];
		uint32_t i2 = indices[i + 2];

		const Graphics::Vertex &v0 = vertices[i0];
		const Graphics::Vertex &v1 = vertices[i1];
		const Graphics::Vertex &v2 = vertices[i2];

		Math::Vec3 edge1 = v1.Position - v0.Position;
		Math::Vec3 edge2 = v2.Position - v0.Position;

		Math::Vec2 deltaUV1 = v1.TexC - v0.TexC;
		Math::Vec2 deltaUV2 = v2.TexC - v0.TexC;

		float r = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

		if (std::isinf(r) || std::isnan(r)) r = 1.0f;

		Math::Vec3 tangent = (edge1 * deltaUV2.y - edge2 * deltaUV1.y) * r;

		tangents[i0] = tangents[i0] + tangent;
		tangents[i1] = tangents[i1] + tangent;
		tangents[i2] = tangents[i2] + tangent;
	}

	for (size_t i = 0; i < vertices.size(); ++i) {
		const Math::Vec3 &n = vertices[i].Normal;
		const Math::Vec3 &t = tangents[i];

		float	   dotNT	   = (n.x * t.x) + (n.y * t.y) + (n.z * t.z);
		Math::Vec3 reprojected = t - (n * dotNT);

		float lengthSq =
			(reprojected.x * reprojected.x) + (reprojected.y * reprojected.y) + (reprojected.z * reprojected.z);

		if (lengthSq < 0.000001f) {
			Math::Vec3 c1 = Math::Cross(n, Math::Vec3(0.0f, 0.0f, 1.0f));
			Math::Vec3 c2 = Math::Cross(n, Math::Vec3(0.0f, 1.0f, 0.0f));

			if (Math::Length(c1) > Math::Length(c2)) {
				vertices[i].Tangent = Math::Normalize(c1);
			} else {
				vertices[i].Tangent = Math::Normalize(c2);
			}
		} else {
			vertices[i].Tangent = Math::Normalize(reprojected);
		}
	}
}

ModelLoadResult LoadOBJ(const std::filesystem::path &path) {
	ModelLoadResult result;
	std::ifstream	file(path);
	if (!file.is_open()) {
		PE_LOG_ERROR("Failed to open OBJ: " + path.string());
		return result;
	}

	std::filesystem::path objPath(path);
	std::filesystem::path rootDir	= objPath.parent_path();
	std::string			  modelName = objPath.stem().string();

	result.modelAssetInfo.name = modelName;
	result.modelAssetInfo.sourcePaths.push_back(objPath);

	std::vector<Math::Vec3> rawPos;
	std::vector<Math::Vec2> rawUV;
	std::vector<Math::Vec3> rawNor;

	std::string											 currentRawMatName = "Default";
	std::vector<Graphics::Vertex>						 currentVertices;
	std::vector<uint32_t>								 currentIndices;
	std::unordered_map<ObjIndex, uint32_t, ObjIndexHash> uniqueMap;

	auto FlushSubmesh = [&](const std::string &nextRawMatName) {
		if (!currentIndices.empty()) {
			ProcessedMesh pm;

			CalculateTangents(currentVertices, currentIndices);

			pm.meshData.Vertices = std::move(currentVertices);
			pm.meshData.Indices	 = std::move(currentIndices);

			pm.assetInfo.name		 = modelName + "_Mesh_" + std::to_string(result.meshes.size());
			pm.assetInfo.vertexCount = static_cast<uint32_t>(pm.meshData.Vertices.size());
			pm.assetInfo.indexCount	 = static_cast<uint32_t>(pm.meshData.Indices.size());

			ModelAssetInfo::SubMeshEntry entry;
			entry.meshAssetName = pm.assetInfo.name;

			entry.materialAssetName = modelName + "_Mat_" + currentRawMatName;

			result.modelAssetInfo.subMeshes.push_back(entry);
			result.meshes.push_back(std::move(pm));

			currentVertices.clear();
			currentIndices.clear();
			uniqueMap.clear();
		}
		currentRawMatName = nextRawMatName;
	};

	auto FixIndex = [](int index, size_t size) -> int {
		if (index == 0) return -1;
		if (index > 0) return index - 1;
		return static_cast<int>(size) + index;
	};

	std::string line;
	while (std::getline(file, line)) {
		if (line.empty() || line[0] == '#') continue;
		std::stringstream ss(line);
		std::string		  type;
		ss >> type;

		if (type == "mtllib") {
			std::string mtlFilename;
			ss >> mtlFilename;

			std::filesystem::path mtlPath = rootDir / mtlFilename;
			if (!LoadMTL(mtlPath, modelName, result)) {
				MaterialAssetInfo newMat;
				newMat.name			   = AssetManager::ErrorMaterialName;
				newMat.shaderAssetName = AssetManager::DefaultShaderName;
				newMat.sourcePaths.push_back(mtlPath);
				newMat.type = AssetType::Material;
				result.materials.push_back(newMat);
				PE_LOG_WARN("mtllib file can't found at " + mtlPath.string());
			}
		} else if (type == "usemtl") {
			std::string matName;
			ss >> matName;
			FlushSubmesh(matName);
		} else if (type == "v") {
			float x, y, z;
			ss >> x >> y >> z;

			rawPos.emplace_back(x, y, -z);
		} else if (type == "vt") {
			float u, v;
			ss >> u >> v;

			rawUV.emplace_back(u, 1.0f - v);
		} else if (type == "vn") {
			float x, y, z;
			ss >> x >> y >> z;

			rawNor.emplace_back(x, y, -z);
		} else if (type == "f") {
			std::string v1, v2, v3, v4;
			ss >> v1 >> v2 >> v3;
			std::vector<ObjIndex> faceIndices = {ParseFaceIndex(v1), ParseFaceIndex(v2), ParseFaceIndex(v3)};
			if (ss >> v4) faceIndices.push_back(ParseFaceIndex(v4));

			for (auto &idx : faceIndices) {
				if (idx.p != -1) idx.p = FixIndex(idx.p, rawPos.size());
				if (idx.t != -1) idx.t = FixIndex(idx.t, rawUV.size());
				if (idx.n != -1) idx.n = FixIndex(idx.n, rawNor.size());
			}

			for (size_t i = 1; i < faceIndices.size() - 1; ++i) {
				for (const ObjIndex tri[3] = {faceIndices[0], faceIndices[i + 1], faceIndices[i]}; auto &idx : tri) {
					if (!uniqueMap.contains(idx)) {
						Graphics::Vertex v;
						if (idx.p >= 0) v.Position = rawPos[idx.p];
						if (idx.t >= 0) v.TexC = rawUV[idx.t];
						if (idx.n >= 0) v.Normal = rawNor[idx.n];

						uniqueMap[idx] = static_cast<uint32_t>(currentVertices.size());
						currentVertices.push_back(v);
					}
					currentIndices.push_back(uniqueMap[idx]);
				}
			}
		}
	}

	FlushSubmesh("");
	result.success = true;
	return result;
}

bool LoadMTL(const std::filesystem::path &mtlPath, const std::string &modelNamePrefix, ModelLoadResult &outResult) {
	std::ifstream file(mtlPath);
	if (!file.is_open()) {
		PE_LOG_WARN(R"(MTL file not found: )" + mtlPath.string());
		return false;
	}

	std::string		   line;
	MaterialAssetInfo *currentMat = nullptr;

	while (std::getline(file, line)) {
		if (line.empty() || line[0] == '#' || line[0] == '\r') continue;

		std::stringstream ss(line);
		ss.imbue(std::locale::classic());
		std::string type;
		ss >> type;

		if (type == "newmtl") {
			std::string matNameRaw;
			ss >> matNameRaw;

			MaterialAssetInfo newMat;

			newMat.name			   = modelNamePrefix + "_Mat_" + matNameRaw;
			newMat.shaderAssetName = AssetManager::DefaultShaderName;
			newMat.sourcePaths.push_back(mtlPath);
			newMat.type = AssetType::Material;

			newMat.properties[Graphics::MaterialProperty::Metallic]			 = 0.0f;
			newMat.properties[Graphics::MaterialProperty::Roughness]		 = 0.5f;
			newMat.properties[Graphics::MaterialProperty::Opacity]			 = 1.0f;
			newMat.properties[Graphics::MaterialProperty::Tiling]			 = Math::Vec2(1.0f, 1.0f);
			newMat.properties[Graphics::MaterialProperty::Offset]			 = Math::Vec2(0.0f, 0.0f);
			newMat.properties[Graphics::MaterialProperty::EmissiveColor]	 = Math::Vec3(0.0f, 0.0f, 0.0f);
			newMat.properties[Graphics::MaterialProperty::EmissiveIntensity] = 0.0f;
			outResult.materials.push_back(newMat);
			currentMat = &outResult.materials.back();
		}

		if (!currentMat) continue;

		if (type == "Kd") {
			float r, g, b;
			ss >> r >> g >> b;
			currentMat->properties[Graphics::MaterialProperty::Color] = Math::Vec4(r, g, b, 1.0f);
		} else if (type == "Ks") {
			float r, g, b;
			ss >> r >> g >> b;
			currentMat->properties[Graphics::MaterialProperty::SpecularColor] = Math::Vec3(r, g, b);
		} else if (type == "Pr") {
			float pr;
			ss >> pr;
			currentMat->properties[Graphics::MaterialProperty::Roughness] = pr;
		} else if (type == "Ke") {
			float r, g, b;
			ss >> r >> g >> b;
			currentMat->properties[Graphics::MaterialProperty::EmissiveColor] = Math::Vec3(r, g, b);

			float maxChannel													  = std::max({r, g, b});
			float intensity														  = (maxChannel > 0.001f) ? 1.0f : 0.0f;
			currentMat->properties[Graphics::MaterialProperty::EmissiveIntensity] = intensity;
		} else if (type == "Ns") {
			float ns;
			ss >> ns;
			currentMat->properties[Graphics::MaterialProperty::SpecularPower] = ns;
		} else if (type == "d") {
			float d;
			ss >> d;
			currentMat->properties[Graphics::MaterialProperty::Opacity] = d;
		} else if (type == "Tr") {
			float tr;
			ss >> tr;
			currentMat->properties[Graphics::MaterialProperty::Opacity] = 1.0f - tr;
		}

		auto ParseTexture = [&](const Graphics::TextureType texType, const std::string &name) {
			std::string texPathRaw;
			ss >> texPathRaw;

			const std::filesystem::path fullTexPath = mtlPath.parent_path() / texPathRaw;

			TextureAssetInfo texInfo;
			texInfo.name = currentMat->name + name;
			texInfo.sourcePaths.push_back(fullTexPath);

			Graphics::Texture tempTex;
			if (const bool loaded = Texture::Loader::Load(fullTexPath, tempTex); !loaded) {
				PE_LOG_WARN("Failed to load texture: " + fullTexPath.string());
				texInfo.name = AssetManager::ErrorTextureName;
			}
			texInfo.params = tempTex.GetTextureParameters();

			outResult.textures.push_back(texInfo);

			currentMat->textureBindings[texType] = {texInfo.name, texInfo.sourcePaths};
		};

		if (type == "map_Kd") {
			ParseTexture(Graphics::TextureType::Albedo, "_Diffuse");
		} else if (type == "map_Bump" || type == "bump" || type == "norm") {
			ParseTexture(Graphics::TextureType::Normal, "_Normal");
		} else if (type == "map_d") {
		} else if (type == "map_Ke") {
			ParseTexture(Graphics::TextureType::Emissive, "_Emissive");
		}
	}

	return true;
}
}  // namespace PE::Assets::Model::Loader