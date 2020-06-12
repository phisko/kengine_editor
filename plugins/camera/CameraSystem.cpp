#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>

#include "Export.hpp"
#include "EntityManager.hpp"

#include "data/AdjustableComponent.hpp"
#include "data/CameraComponent.hpp"
#include "data/ViewportComponent.hpp"
#include "data/InputComponent.hpp"
#include "data/ImGuiComponent.hpp"

#include "functions/Execute.hpp"

#include "helpers/cameraHelper.hpp"
#include "helpers/matrixHelper.hpp"
#include "helpers/pluginHelper.hpp"

#include "angle.hpp"
#include "imgui.h"
#include "ImGuizmo.h"

static kengine::EntityManager * g_em;

#pragma region Adjustables
static auto ZOOM_SPEED = .1f;
static auto GIZMO_LENGTH = 1.f;
static auto GIZMO_SCREEN_PERCENT = .1f;
#pragma endregion Adjustables

#pragma region declarations
static void execute(float deltaTime);
static void drawImGui();
static void processMouseScroll(kengine::Entity::ID window, float xoffset, float yoffset, const putils::Point2f & coords);
#pragma endregion
EXPORT void loadKenginePlugin(kengine::EntityManager & em) {
	kengine::pluginHelper::initPlugin(em);

	g_em = &em;

	em += [&](kengine::Entity & e) {
		e += kengine::CameraComponent{ { { 0.f, 0.f, -1.f }, { 1.f, 1.f, 1.f } } };
		e += kengine::ViewportComponent{};

		e += kengine::functions::Execute{ execute };
		e += kengine::ImGuiComponent{ drawImGui };
		e += kengine::InputComponent{
			.onScroll = processMouseScroll
		};

		e += kengine::AdjustableComponent{
			"Camera", {
				{ "Zoom speed", &ZOOM_SPEED },
				{ "Gizmo length", &GIZMO_LENGTH },
				{ "Gizmo screen percentage", &GIZMO_SCREEN_PERCENT }
			}
		};
	};
}

static void execute(float deltaTime) {
	for (auto & [e, cam] : g_em->getEntities<kengine::CameraComponent>()) {
		const auto facings = kengine::cameraHelper::getFacings(cam);

		const auto dist = cam.frustum.position.getLength();
		cam.frustum.position = -facings.front * dist;
	}
}

static void drawImGui() {
	ImGuizmo::BeginFrame();

	for (const auto & [_, cam, viewport] : g_em->getEntities<kengine::CameraComponent, kengine::ViewportComponent>()) {
		auto & io = ImGui::GetIO();

		const putils::Point2f scaledDisplaySize = { io.DisplaySize.x * viewport.boundingBox.size.x, io.DisplaySize.y * viewport.boundingBox.size.y };

		const ImVec2 size{ GIZMO_SCREEN_PERCENT * scaledDisplaySize.x, GIZMO_SCREEN_PERCENT * scaledDisplaySize.x };

		const auto imguiViewport = ImGui::GetMainViewport();
		const ImVec2 pos{
			imguiViewport->Pos.x + io.DisplaySize.x * viewport.boundingBox.position.x + scaledDisplaySize.x - size.x,
			imguiViewport->Pos.y + io.DisplaySize.y * viewport.boundingBox.position.y + scaledDisplaySize.y - size.y
		};

		auto view = kengine::matrixHelper::getViewMatrix(cam, viewport);
		ImGuizmo::ViewManipulate(glm::value_ptr(view), GIZMO_LENGTH, pos, size, 0);

		const auto quat = glm::conjugate(glm::toQuat(view));
		const auto rotation = kengine::matrixHelper::getRotation(glm::toMat4(quat));
		cam.yaw = rotation.y + putils::pi;
		cam.pitch = rotation.x;
		cam.roll = rotation.z;
	}
}

static void processMouseScroll(kengine::Entity::ID window, float xoffset, float yoffset, const putils::Point2f & coords) {
	const auto cameraId = kengine::cameraHelper::getViewportForPixel(*g_em, window, coords).camera;
	if (cameraId == kengine::Entity::INVALID_ID)
		return;

	auto & e = g_em->getEntity(cameraId);
	auto & cam = e.get<kengine::CameraComponent>();

	const auto facings = kengine::cameraHelper::getFacings(cam);
	cam.frustum.position += yoffset * ZOOM_SPEED * facings.front;
}