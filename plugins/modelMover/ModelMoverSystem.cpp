#include <GLFW/glfw3.h>
#include <glm/gtc/type_ptr.hpp>
#include <filesystem>

#include "Export.hpp"
#include "EntityManager.hpp"

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

	putils_reflection_class_name(GizmoComponent);
	putils_reflection_attributes(
		putils_reflection_attribute(&GizmoComponent::type)
	);
};

static kengine::EntityManager * g_em;
static int g_gizmoId = 0;
static bool g_shouldOpenContextMenu = false;
static GizmoComponent * g_gizmoForContextMenu = nullptr;

#pragma region declarations
static void drawImGui();
#pragma endregion
EXPORT void loadKenginePlugin(kengine::EntityManager & em) {
	kengine::pluginHelper::initPlugin(em);

	g_em = &em;
	em += [&](kengine::Entity & e) {
		e += kengine::functions::Execute{ [](float deltaTime) { drawImGui(); } };
		e += kengine::InputComponent{
			.onMouseButton = [](kengine::Entity::ID window, int button, const putils::Point2f & screenCoordinates, bool pressed) {
				if (button != GLFW_MOUSE_BUTTON_RIGHT || !pressed)
					return;

				kengine::Entity::ID id = kengine::Entity::INVALID_ID;
				for (const auto & [e, getEntityInPixel] : g_em->getEntities<kengine::functions::GetEntityInPixel>())
					id = getEntityInPixel(window, screenCoordinates);

				if (id == kengine::Entity::INVALID_ID)
					return;

				auto e = g_em->getEntity(id);
				g_gizmoForContextMenu = &e.attach<GizmoComponent>();
				g_shouldOpenContextMenu = true;
			}
		};
	};
}

#pragma region drawImGui
#pragma region declarations
static void setupWindow(const kengine::ViewportComponent & viewport);
static void drawGizmos(kengine::CameraComponent & cam, const kengine::ViewportComponent & viewport);
#pragma endregion
static void drawImGui() {
	g_gizmoId = 0;

	ImGuizmo::BeginFrame();

	for (const auto & [_, cam, viewport] : g_em->getEntities<kengine::CameraComponent, kengine::ViewportComponent>()) {
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

static void setupWindow(const kengine::ViewportComponent & viewport) {
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

#pragma region helper
static void nextGizmo() { ImGuizmo::SetID(g_gizmoId++); }
#pragma endregion
static void drawGizmos(kengine::CameraComponent & cam, const kengine::ViewportComponent & viewport) {
	const auto proj = kengine::matrixHelper::getProjMatrix(cam, viewport, 0.001f, 1000.f);
	const auto view = kengine::matrixHelper::getViewMatrix(cam, viewport);

	for (auto & [e, transform, instance, selected] : g_em->getEntities<kengine::TransformComponent, kengine::InstanceComponent, kengine::SelectedComponent>()) {
		nextGizmo();

		auto & gizmo = e.attach<GizmoComponent>();

		const auto & modelComp = kengine::instanceHelper::getModel<kengine::ModelComponent>(*g_em, instance);

		auto model = kengine::matrixHelper::getModelMatrix(modelComp, transform);
		glm::mat4 deltaMatrix;

		switch (gizmo.type) {
		case GizmoComponent::Translate:
			ImGuizmo::Manipulate(glm::value_ptr(view), glm::value_ptr(proj), ImGuizmo::TRANSLATE, ImGuizmo::WORLD, glm::value_ptr(model), glm::value_ptr(deltaMatrix));
			transform.boundingBox.position += kengine::matrixHelper::getPosition(deltaMatrix);
			break;
		case GizmoComponent::Scale:
		{
			model = glm::transpose(model);
			ImGuizmo::Manipulate(glm::value_ptr(view), glm::value_ptr(proj), ImGuizmo::SCALE, ImGuizmo::WORLD, glm::value_ptr(model), glm::value_ptr(deltaMatrix));
			transform.boundingBox.size = kengine::matrixHelper::getScale(model);
			break;
		}
		case GizmoComponent::Rotate:
		{
			model = glm::transpose(model);
			ImGuizmo::Manipulate(glm::value_ptr(view), glm::value_ptr(proj), ImGuizmo::ROTATE, ImGuizmo::WORLD, glm::value_ptr(model), glm::value_ptr(deltaMatrix));
			const auto rotation = kengine::matrixHelper::getRotation(deltaMatrix);
			transform.yaw += rotation.y;
			transform.pitch += rotation.x;
			transform.roll += rotation.z;
			break;
		}
		default:
			kengine_assert_failed(*g_em, "Unknown gizmo type");
			static_assert(putils::magic_enum::enum_count<GizmoComponent::Type>() == 3);
			break;
		}
	}
}
#pragma endregion drawImGui