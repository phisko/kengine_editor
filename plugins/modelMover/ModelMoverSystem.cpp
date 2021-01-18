#include <GLFW/glfw3.h>
#include <glm/gtc/type_ptr.hpp>
#include <filesystem>

#include "Export.hpp"
#include "kengine.hpp"

#include "data/AdjustableComponent.hpp"
#include "data/CameraComponent.hpp"
#include "data/DebugGraphicsComponent.hpp"
#include "data/InputComponent.hpp"
#include "data/InstanceComponent.hpp"
#include "data/TransformComponent.hpp"
#include "data/SelectedComponent.hpp"
#include "data/ViewportComponent.hpp"

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

static struct {
	float sphereAlpha = .5f;
} adjustables;

static int g_gizmoId = 0;
static bool g_shouldOpenContextMenu = false;
static GizmoComponent * g_gizmoForContextMenu = nullptr;

using namespace kengine;

EXPORT void loadKenginePlugin(void * state) noexcept {
	struct impl {
		static void init() noexcept {
			entities += [](Entity & e) {
				e += functions::Execute{ [](float deltaTime) noexcept { drawImGui(); } };
				e += InputComponent{ .onMouseButton = onClick };
				e += AdjustableComponent{ "Model mover", {
					{ "Identity sphere alpha", &adjustables.sphereAlpha }
				} };
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
				int i = 0;
				for (const auto name : putils::magic_enum::enum_names<GizmoComponent::Type>()) {
					if (ImGui::MenuItem(putils::string<64>(name)))
						g_gizmoForContextMenu->type = (GizmoComponent::Type)i;
					++i;
				}
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

			for (auto [e, transform, instance, selected] : entities.with<TransformComponent, InstanceComponent, SelectedComponent>()) {
				nextGizmo();

				auto & debugGraphics = e.attach<DebugGraphicsComponent>();
				if (debugGraphics.elements.empty())
					debugGraphics.elements.push_back(DebugGraphicsComponent::Element{
						DebugGraphicsComponent::Sphere{}, { 0.f, 0.f, 0.f }, putils::NormalizedColor{ 1.f, 1.f, 1.f, .5f }, DebugGraphicsComponent::ReferenceSpace::World
					});

				auto & gizmo = e.attach<GizmoComponent>();

				auto modelMatrix = matrixHelper::getModelMatrix(transform);

				glm::mat4 deltaMatrix;
				switch (gizmo.type) {
				case GizmoComponent::Translate:
					ImGuizmo::Manipulate(glm::value_ptr(view), glm::value_ptr(proj), ImGuizmo::TRANSLATE, ImGuizmo::WORLD, glm::value_ptr(modelMatrix), glm::value_ptr(deltaMatrix));
					transform.boundingBox.position += matrixHelper::getPosition(deltaMatrix);
					break;
				case GizmoComponent::Scale:
				{
					// modelMatrix = glm::transpose(modelMatrix);
					ImGuizmo::Manipulate(glm::value_ptr(view), glm::value_ptr(proj), ImGuizmo::SCALE, ImGuizmo::WORLD, glm::value_ptr(modelMatrix), glm::value_ptr(deltaMatrix));
					transform.boundingBox.size = matrixHelper::getScale(modelMatrix);
					break;
				}
				case GizmoComponent::Rotate:
				{
					// modelMatrix = glm::transpose(modelMatrix);
					ImGuizmo::Manipulate(glm::value_ptr(view), glm::value_ptr(proj), ImGuizmo::ROTATE, ImGuizmo::WORLD, glm::value_ptr(modelMatrix), glm::value_ptr(deltaMatrix));
					const auto rotation = matrixHelper::getRotation(deltaMatrix);
					transform.yaw += rotation.y;
					transform.pitch += rotation.x;
					transform.roll += rotation.z;
					break;
				}
				default:
					static_assert(putils::magic_enum::enum_count<GizmoComponent::Type>() == 3);
					break;
				}

				if (ImGuizmo::IsUsing())
					continue;

				auto model = entities[instance.model];
				auto & modelTransform = model.attach<TransformComponent>();

				modelTransform.boundingBox.position += transform.boundingBox.position;
				modelTransform.boundingBox.size *= transform.boundingBox.size;
				modelTransform.yaw += transform.yaw;
				modelTransform.pitch += transform.pitch;
				modelTransform.roll += transform.roll;
				transform = TransformComponent{};
			}
		}

		static void nextGizmo() {
			ImGuizmo::SetID(g_gizmoId++);
		}
	};

	pluginHelper::initPlugin(state);
	impl::init();
}