#include <GLFW/glfw3.h>
#include <glm/gtc/type_ptr.hpp>
#include <filesystem>

#include "Export.hpp"
#include "kengine.hpp"

#include "data/CameraComponent.hpp"
#include "data/InputComponent.hpp"
#include "data/InstanceComponent.hpp"
#include "data/ModelComponent.hpp"
#include "data/TransformComponent.hpp"
#include "data/SelectedComponent.hpp"
#include "data/ViewportComponent.hpp"
#include "data/WindowComponent.hpp"

#include "functions/Execute.hpp"
#include "functions/GetEntityInPixel.hpp"

#include "helpers/instanceHelper.hpp"
#include "helpers/pluginHelper.hpp"
#include "helpers/matrixHelper.hpp"

#include "imgui.h"
#include "ImGuizmo.h"
#include "magic_enum.hpp"

struct GizmoComponent {
	enum Type {
		Translate,
		Scale,
		Rotate
	};

	Type type = Translate;
};

#define refltype GizmoComponent 
putils_reflection_info{
	putils_reflection_class_name;
	putils_reflection_attributes(
		putils_reflection_attribute(type)
	);
};
#undef refltype

static int g_gizmoId = 0;
static bool g_shouldOpenContextMenu = false;
static GizmoComponent * g_gizmoForContextMenu = nullptr;

using namespace kengine;

EXPORT void loadKenginePlugin(void * state) noexcept {
	struct impl {
		static void init() noexcept {
			entities += [](Entity & e) {
				e += functions::Execute{ [](float deltaTime) noexcept { drawImGui(); } };
				e += InputComponent{
					.onMouseButton = [](EntityID window, int button, const putils::Point2f & screenCoordinates, bool pressed) noexcept {
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
				};
			};
		}

		static void drawImGui() noexcept {
			g_gizmoId = 0;

			ImGuizmo::BeginFrame();

			for (const auto & [_, cam, viewport] : entities.with<CameraComponent, ViewportComponent>()) {
				setupWindow(viewport);
				drawGizmos(cam, viewport);
				ImGui::End(); // begin was called in setupWindow
			}

			if (g_shouldOpenContextMenu) {
				ImGui::OpenPopup("Gizmo type");
				g_shouldOpenContextMenu = false;
			}

			if (ImGui::BeginPopup("Gizmo type")) {
				int i = 0;
				for (const auto name : putils::magic_enum::enum_names<GizmoComponent::Type>()) {
					if (ImGui::MenuItem(putils::string<64>(name)))
						g_gizmoForContextMenu->type = (GizmoComponent::Type)i;
					++i;
				}
				ImGui::EndPopup();
			}
		}

		static void setupWindow(const ViewportComponent & viewport) noexcept {
			auto & io = ImGui::GetIO();
			const putils::Rect2f displayRegion = {
				{
					viewport.boundingBox.position.x * io.DisplaySize.x,
					viewport.boundingBox.position.y * io.DisplaySize.y
				},
				{
					viewport.boundingBox.size.x * io.DisplaySize.x,
					viewport.boundingBox.size.y * io.DisplaySize.y
				}
			};

			const auto imguiViewport = ImGui::GetMainViewport();
			ImGui::SetNextWindowViewport(imguiViewport->ID);
			ImGui::SetNextWindowPos(imguiViewport->Pos);
			ImGui::SetNextWindowSize(imguiViewport->Size);

			ImGui::Begin("Gizmos", nullptr, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoInputs);
			ImGuizmo::SetDrawlist();

			ImGuizmo::SetRect(
				displayRegion.position.x, displayRegion.position.y,
				displayRegion.size.x, displayRegion.size.y
			);
		}

		static void drawGizmos(CameraComponent & cam, const ViewportComponent & viewport) noexcept {
			const auto proj = matrixHelper::getProjMatrix(cam, viewport, 0.001f, 1000.f);
			const auto view = matrixHelper::getViewMatrix(cam, viewport);


			for (auto [e, transform, instance, selected] : entities.with<TransformComponent, InstanceComponent, SelectedComponent>()) {
				nextGizmo();

				auto & gizmo = e.attach<GizmoComponent>();

				const auto * modelTransform = instanceHelper::tryGetModel<TransformComponent>(instance);
				auto model = matrixHelper::getModelMatrix(transform, modelTransform);
				glm::mat4 deltaMatrix;

				switch (gizmo.type) {
				case GizmoComponent::Translate:
					ImGuizmo::Manipulate(glm::value_ptr(view), glm::value_ptr(proj), ImGuizmo::TRANSLATE, ImGuizmo::WORLD, glm::value_ptr(model), glm::value_ptr(deltaMatrix));
					transform.boundingBox.position += matrixHelper::getPosition(deltaMatrix);
					break;
				case GizmoComponent::Scale:
				{
					model = glm::transpose(model);
					ImGuizmo::Manipulate(glm::value_ptr(view), glm::value_ptr(proj), ImGuizmo::SCALE, ImGuizmo::WORLD, glm::value_ptr(model), glm::value_ptr(deltaMatrix));
					transform.boundingBox.size = matrixHelper::getScale(model);
					break;
				}
				case GizmoComponent::Rotate:
				{
					model = glm::transpose(model);
					ImGuizmo::Manipulate(glm::value_ptr(view), glm::value_ptr(proj), ImGuizmo::ROTATE, ImGuizmo::WORLD, glm::value_ptr(model), glm::value_ptr(deltaMatrix));
					const auto rotation = matrixHelper::getRotation(deltaMatrix);
					transform.yaw += rotation.y;
					transform.pitch += rotation.x;
					transform.roll += rotation.z;
					break;
				}
				default:
					kengine_assert_failed("Unknown gizmo type");
					static_assert(putils::magic_enum::enum_count<GizmoComponent::Type>() == 3);
					break;
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