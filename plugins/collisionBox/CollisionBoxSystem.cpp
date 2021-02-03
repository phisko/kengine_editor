#include <GLFW/glfw3.h>
#include <glm/gtc/type_ptr.hpp>
#include "kengine.hpp"
#include "Export.hpp"
#include "helpers/pluginHelper.hpp"

#include "data/EditorComponent.hpp"
#include "data/InstanceComponent.hpp"
#include "data/ModelColliderComponent.hpp"
#include "data/ModelSkeletonComponent.hpp"
#include "data/PhysicsComponent.hpp"
#include "data/SkeletonComponent.hpp"
#include "data/TransformComponent.hpp"

#include "functions/DrawGizmos.hpp"
#include "functions/Execute.hpp"
#include "functions/OnClick.hpp"

#include "meta/ToSave.hpp"

#include "helpers/gizmoHelper.hpp"
#include "helpers/ImGuizmo.h"
#include "helpers/matrixHelper.hpp"
#include "helpers/skeletonHelper.hpp"
#include "helpers/typeHelper.hpp"

#include "imgui.h"
#include "magic_enum.hpp"

namespace kengine::bullet {
	struct BulletPhysicsComponent;
}

using namespace kengine;

enum class GizmoType {
	Translate,
	Scale,
	Rotate
};

static GizmoType g_gizmoType = GizmoType::Translate;

static bool g_active = true;
static int g_gizmoId = 0;
static bool g_shouldOpenContextMenu = false;
static std::optional<ModelColliderComponent::Collider::Shape> g_shapeToAdd = std::nullopt;

EXPORT void loadKenginePlugin(void * state) noexcept {
	struct impl {
		static void init() noexcept {
			typeHelper::getTypeEntity<ModelColliderComponent>() += meta::ToSave{};
			entities += [](Entity & e) noexcept {
				e += EditorComponent{ "Collisions", &g_active };
				e += ::functions::DrawGizmos{ drawGizmos };
			};
		}

		static void drawGizmos(const glm::mat4 & proj, const glm::mat4 & view, const ImVec2 & windowSize, const ImVec2 & windowPos) noexcept {
			if (!g_active)
				return;

			processGizmos(proj, view, windowSize, windowPos);
			gizmoHelper::handleContextMenu([] {
				if (ImGui::BeginMenu("Add collider")) {
					for (const auto [shape, name] : putils::magic_enum::enum_entries<ModelColliderComponent::Collider::Shape>())
						if (ImGui::MenuItem(putils::string<64>(name)))
							g_shapeToAdd = shape;
					ImGui::EndMenu();
				}
			});
		}

		static void processGizmos(const glm::mat4 & proj, const glm::mat4 & view, const ImVec2 & windowSize, const ImVec2 & windowPos) noexcept {
			g_gizmoId = 0;

			gizmoHelper::newFrame(windowSize, windowPos);

			for (auto [e, instance] : entities.with<InstanceComponent>()) {
				auto model = entities[instance.model];
				auto & modelColliders = model.attach<ModelColliderComponent>();

				if (g_shapeToAdd) {
					modelColliders.colliders.push_back({ *g_shapeToAdd });
					modelColliders.colliders.back().transform.boundingBox.position = { 0.f, .5f, 0.f };
					g_shapeToAdd = std::nullopt;
				}

				for (auto & collider : modelColliders.colliders) {
					glm::mat4 parentMat(1.f);
					if (!collider.boneName.empty()) {
						kengine_assert(e.has<SkeletonComponent>() && model.has<ModelSkeletonComponent>());

						const auto worldSpaceBone = skeletonHelper::getBoneMatrix(collider.boneName.c_str(), e.get<SkeletonComponent>(), model.get<ModelSkeletonComponent>());
						const auto pos = matrixHelper::getPosition(worldSpaceBone);

						const auto modelTransform = model.tryGet<TransformComponent>();
						auto parentScale = pos;
						if (modelTransform != nullptr)
							parentScale *= modelTransform->boundingBox.size;
						parentMat = glm::translate(parentMat, matrixHelper::toVec(parentScale));
						parentMat = glm::translate(parentMat, -matrixHelper::toVec(pos));
						parentMat *= worldSpaceBone;
					}

					gizmoHelper::drawGizmo(collider.transform, proj, view, &parentMat, true);
				}

				e += PhysicsComponent{ 0.f };
			}
		}
	};

	pluginHelper::initPlugin(state);
	impl::init();
}
