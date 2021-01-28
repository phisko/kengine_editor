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

#include "helpers/instanceHelper.hpp"
#include "helpers/matrixHelper.hpp"

#include "imgui.h"
#include "ImGuizmo.h"
#include "magic_enum.hpp"
#include "epsilon.hpp"

using namespace kengine;

struct GizmoComponent {
	enum class Type {
		Translate,
		Scale,
		Rotate
	};

	Type type = Type::Translate;
	TransformComponent transform;
	TransformComponent modelTransformSave;
};

#define refltype GizmoComponent 
putils_reflection_info{
	putils_reflection_class_name;
	putils_reflection_attributes(
		putils_reflection_attribute(type)
	);
};
#undef refltype

static bool g_active = true;
static int g_gizmoId = 0;
static bool g_shouldOpenContextMenu = false;
static GizmoComponent * g_gizmoForContextMenu = nullptr;

static bool g_uniformScale = true;
float putils::Point3f:: * g_currentScaleModifier = nullptr;

EXPORT void loadKenginePlugin(void * state) noexcept {
	struct impl {
		static void init() noexcept {
			entities += [](Entity & e) {
				e += EditorComponent{ "Transform", &g_active };
				e += ::functions::DrawGizmos{ drawGizmos };
			};
		}

		static void drawGizmos(const glm::mat4 & proj, const glm::mat4 & view, const ImVec2 & windowSize, const ImVec2 & windowPos) noexcept {
			if (!g_active)
				return;

			processGizmos(proj, view, windowSize, windowPos);
			processContextMenu();
		}
		
		static void processGizmos(const glm::mat4 & proj, const glm::mat4 & view, const ImVec2 & windowSize, const ImVec2 & windowPos) noexcept {
			g_gizmoId = 0;

			ImGuizmo::BeginFrame();
			ImGuizmo::SetDrawlist();
			ImGuizmo::SetRect(windowPos.x, windowPos.y, windowSize.x, windowSize.y);

			for (auto [e, instance, selected] : entities.with<InstanceComponent, SelectedComponent>()) {
				nextGizmo();

				auto model = entities[instance.model];
				auto & modelTransform = model.attach<TransformComponent>();

				const bool gizmoExists = e.has<GizmoComponent>();
				auto & gizmo = e.attach<GizmoComponent>();
				if (!gizmoExists)
					gizmo.modelTransformSave = modelTransform;

				auto modelMatrix = matrixHelper::getModelMatrix(gizmo.transform);
				glm::mat4 deltaMatrix;
				switch (gizmo.type) {
				case GizmoComponent::Type::Translate:
					ImGuizmo::Manipulate(glm::value_ptr(view), glm::value_ptr(proj), ImGuizmo::TRANSLATE, ImGuizmo::WORLD, glm::value_ptr(modelMatrix), glm::value_ptr(deltaMatrix));
					gizmo.transform.boundingBox.position += matrixHelper::getPosition(deltaMatrix);
					break;
				case GizmoComponent::Type::Scale:
				{
					// modelMatrix = glm::transpose(modelMatrix);
					ImGuizmo::Manipulate(glm::value_ptr(view), glm::value_ptr(proj), ImGuizmo::SCALE, ImGuizmo::WORLD, glm::value_ptr(modelMatrix), glm::value_ptr(deltaMatrix));
					const auto previousSize = gizmo.transform.boundingBox.size;
					auto & size = gizmo.transform.boundingBox.size;
					size = matrixHelper::getScale(modelMatrix);
					if (g_uniformScale) {
						putils::reflection::for_each_attribute<putils::Point3f>([&](const auto name, const auto member) noexcept {
							if (g_currentScaleModifier != nullptr && member != g_currentScaleModifier)
								return;

							if (g_currentScaleModifier == nullptr && putils::epsilonEquals(previousSize.*member, size.*member))
								return;

							g_currentScaleModifier = member;
							putils::reflection::for_each_attribute(size, [&](const auto name, auto & otherMember) {
								otherMember = size.*member;
								});
							});
					}
					break;
				}
				case GizmoComponent::Type::Rotate:
				{
					// modelMatrix = glm::transpose(modelMatrix);
					ImGuizmo::Manipulate(glm::value_ptr(view), glm::value_ptr(proj), ImGuizmo::ROTATE, ImGuizmo::WORLD, glm::value_ptr(modelMatrix), glm::value_ptr(deltaMatrix));
					const auto rotation = matrixHelper::getRotation(deltaMatrix);
					gizmo.transform.yaw += rotation.y;
					gizmo.transform.pitch += rotation.x;
					gizmo.transform.roll += rotation.z;
					break;
				}
				default:
					static_assert(putils::magic_enum::enum_count<GizmoComponent::Type>() == 3);
					break;
				}

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
					g_currentScaleModifier = nullptr;
				}
			}
		}

		static void nextGizmo() noexcept {
			ImGuizmo::SetID(g_gizmoId++);
		}

		static void processContextMenu() noexcept {
			for (auto [e, instance] : entities.with<InstanceComponent>()) {
				auto & gizmo = e.attach<GizmoComponent>();
				e += kengine::functions::OnClick{ [&gizmo](int button) {
					onClick(button, gizmo);
				} };
			}

			if (g_shouldOpenContextMenu) {
				ImGui::OpenPopup("Gizmo type#mover");
				g_shouldOpenContextMenu = false;
			}

			if (ImGui::BeginPopup("Gizmo type#mover")) {
				for (const auto [gizmoType, name] : putils::magic_enum::enum_entries<GizmoComponent::Type>())
					if (ImGui::MenuItem(putils::string<64>(name)))
						g_gizmoForContextMenu->type = gizmoType;

				ImGui::MenuItem("Uniform scale", nullptr, &g_uniformScale);
				ImGui::EndPopup();
			}
		}

		static void onClick(int button, GizmoComponent & gizmo) noexcept {
			if (button != GLFW_MOUSE_BUTTON_RIGHT)
				return;

			g_gizmoForContextMenu = &gizmo;
			g_shouldOpenContextMenu = true;
		}
	};

	pluginHelper::initPlugin(state);
	impl::init();
}