#include <GLFW/glfw3.h>
#include <glm/gtc/type_ptr.hpp>
#include <filesystem>

#include "kengine.hpp"
#include "Export.hpp"
#include "helpers/pluginHelper.hpp"

#include "data/AdjustableComponent.hpp"
#include "data/CameraComponent.hpp"
#include "data/EditorComponent.hpp"
#include "data/InputComponent.hpp"
#include "data/InstanceComponent.hpp"
#include "data/TransformComponent.hpp"
#include "data/SelectedComponent.hpp"
#include "data/ViewportComponent.hpp"

#include "functions/Execute.hpp"
#include "functions/GetEntityInPixel.hpp"

#include "helpers/instanceHelper.hpp"
#include "helpers/matrixHelper.hpp"

#include "imgui.h"
#include "ImGuizmo.h"
#include "magic_enum.hpp"
#include "epsilon.hpp"

using namespace kengine;

struct GizmoComponent {
	enum Type {
		Translate,
		Scale,
		Rotate
	};

	Type type = Translate;
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
				e += functions::Execute{ [](float deltaTime) noexcept { drawImGui(); } };
				e += InputComponent{ .onMouseButton = onClick };
			};
		}

		static void onClick(EntityID window, int button, const putils::Point2f & screenCoordinates, bool pressed) noexcept {
			if (button != GLFW_MOUSE_BUTTON_RIGHT || !pressed)
				return;

			EntityID id = INVALID_ID;
			for (const auto & [e, getEntityInPixel] : entities.with<functions::GetEntityInPixel>())
				id = getEntityInPixel(window, screenCoordinates);

			if (id == INVALID_ID)
				return;

			auto e = entities[id];
			g_gizmoForContextMenu = &e.attach<GizmoComponent>();
			g_shouldOpenContextMenu = true;
		}

		static void drawImGui() noexcept {
			if (!g_active)
				return;

			g_gizmoId = 0;

			ImGuizmo::BeginFrame();

			for (const auto & [_, cam, viewport] : entities.with<CameraComponent, ViewportComponent>()) {
				setupWindow();
				drawGizmos(cam, viewport);
				ImGui::End(); // begin was called in setupWindow
			}

			if (g_shouldOpenContextMenu) {
				ImGui::OpenPopup("Gizmo type");
				g_shouldOpenContextMenu = false;
			}

			if (ImGui::BeginPopup("Gizmo type")) {
				for (const auto [gizmoType, name] : putils::magic_enum::enum_entries<GizmoComponent::Type>())
					if (ImGui::MenuItem(putils::string<64>(name)))
						g_gizmoForContextMenu->type = gizmoType;

				ImGui::MenuItem("Uniform scale", nullptr, &g_uniformScale);
				ImGui::EndPopup();
			}
		}

		static void setupWindow() noexcept {
			const auto imguiViewport = ImGui::GetMainViewport();
			ImGui::SetNextWindowViewport(imguiViewport->ID);
			ImGui::SetNextWindowPos(imguiViewport->Pos);
			ImGui::SetNextWindowSize(imguiViewport->Size);

			ImGui::Begin("Gizmos", nullptr, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoInputs);
			ImGuizmo::SetDrawlist();

			const auto windowSize = ImGui::GetWindowSize();
			const auto windowPos = ImGui::GetWindowPos();
			ImGuizmo::SetRect(windowPos.x, windowPos.y, windowSize.x, windowSize.y);
		}

		static void drawGizmos(CameraComponent & cam, const ViewportComponent & viewport) noexcept {
			const auto proj = matrixHelper::getProjMatrix(cam, viewport, 0.001f, 1000.f);
			const auto view = matrixHelper::getViewMatrix(cam, viewport);

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
				case GizmoComponent::Translate:
					ImGuizmo::Manipulate(glm::value_ptr(view), glm::value_ptr(proj), ImGuizmo::TRANSLATE, ImGuizmo::WORLD, glm::value_ptr(modelMatrix), glm::value_ptr(deltaMatrix));
					gizmo.transform.boundingBox.position += matrixHelper::getPosition(deltaMatrix);
					break;
				case GizmoComponent::Scale:
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
				case GizmoComponent::Rotate:
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

		static void nextGizmo() {
			ImGuizmo::SetID(g_gizmoId++);
		}
	};

	pluginHelper::initPlugin(state);
	impl::init();
}