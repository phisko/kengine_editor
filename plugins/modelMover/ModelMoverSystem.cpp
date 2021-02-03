#include <GLFW/glfw3.h>
#include <glm/gtc/type_ptr.hpp>
#include <filesystem>

#include "kengine.hpp"
#include "Export.hpp"
#include "helpers/pluginHelper.hpp"

#include "data/AdjustableComponent.hpp"
#include "data/CameraComponent.hpp"
#include "data/EditorComponent.hpp"
#include "data/InstanceComponent.hpp"
#include "data/TransformComponent.hpp"
#include "data/SelectedComponent.hpp"
#include "data/ViewportComponent.hpp"

#include "functions/DrawGizmos.hpp"
#include "functions/OnClick.hpp"

#include "meta/ToSave.hpp"

#include "helpers/gizmoHelper.hpp"
#include "helpers/instanceHelper.hpp"
#include "helpers/typeHelper.hpp"

#include "imgui.h"
#include "ImGuizmo.h"
#include "magic_enum.hpp"
#include "epsilon.hpp"

using namespace kengine;

struct GizmoComponent {
	TransformComponent transform;
	TransformComponent modelTransformSave;
};

static bool g_active = true;

EXPORT void loadKenginePlugin(void * state) noexcept {
	struct impl {
		static void init() noexcept {
			typeHelper::getTypeEntity<TransformComponent>() += meta::ToSave{};
			entities += [](Entity & e) {
				e += EditorComponent{ "Transform", &g_active };
				e += ::functions::DrawGizmos{ drawGizmos };
			};
		}

		static void drawGizmos(const glm::mat4 & proj, const glm::mat4 & view, const ImVec2 & windowSize, const ImVec2 & windowPos) noexcept {
			if (!g_active)
				return;

			processGizmos(proj, view, windowSize, windowPos);
			gizmoHelper::handleContextMenu();
		}
		
		static void processGizmos(const glm::mat4 & proj, const glm::mat4 & view, const ImVec2 & windowSize, const ImVec2 & windowPos) noexcept {
			gizmoHelper::newFrame(windowSize, windowPos);

			for (auto [e, instance, selected] : entities.with<InstanceComponent, SelectedComponent>()) {
				auto model = entities[instance.model];
				auto & modelTransform = model.attach<TransformComponent>();

				const bool gizmoExists = e.has<GizmoComponent>();
				auto & gizmo = e.attach<GizmoComponent>();
				if (!gizmoExists)
					gizmo.modelTransformSave = modelTransform;

				gizmoHelper::drawGizmo(gizmo.transform, proj, view);

				if (ImGuizmo::IsUsing()) {
					modelTransform.boundingBox.position = gizmo.transform.boundingBox.position + gizmo.modelTransformSave.boundingBox.position;
					modelTransform.boundingBox.size = gizmo.transform.boundingBox.size * gizmo.modelTransformSave.boundingBox.size;
					modelTransform.yaw = gizmo.transform.yaw + gizmo.modelTransformSave.yaw;
					modelTransform.pitch = gizmo.transform.pitch + gizmo.modelTransformSave.pitch;
					modelTransform.roll = gizmo.transform.roll + gizmo.modelTransformSave.roll;
				}
				else {
					gizmo.transform = {};
					gizmo.modelTransformSave = modelTransform;
				}
			}
		}
	};

	pluginHelper::initPlugin(state);
	impl::init();
}