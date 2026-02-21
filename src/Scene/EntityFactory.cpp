#include "Scene/EntityFactory.h"

#include "Graphics/Components/MeshRenderer.h"
#include "Scene/Components/Transform.h"

namespace PE::Scene {
ERROR_CODE EntityFactory::Initialize(ECS::EntityManager *entityManager) {
	PE_CHECK_ALREADY_INIT(m_isInitialized, "Entity factory is already initialized!");
	ref_eM			= entityManager;
	m_isInitialized = true;

	return ERROR_CODE::OK;
}

void EntityFactory::Shutdown() {
	if (!m_isInitialized) return;

	m_isInitialized = false;
}

ECS::EntityID EntityFactory::CreatePrimitive(const Graphics::PrimitiveType type) {
	const ECS::EntityID entityID = ref_eM->CreateEntity();

	Graphics::Components::MeshRenderer meshRenderer;
	meshRenderer.subMeshes.emplace_back(Assets::AssetManager::RequestPrimitiveMesh(type),
										Assets::AssetManager::RequestDefaultMaterial());

	ref_eM->AddComponent<Components::Transform>(entityID, Components::Transform());
	ref_eM->AddComponent<Graphics::Components::MeshRenderer>(entityID, meshRenderer);

	return entityID;
}
}  // namespace PE::Scene