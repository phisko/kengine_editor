#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>

#include "kengine.hpp"
#include "Export.hpp"
#include "helpers/pluginHelper.hpp"

#include "data/AdjustableComponent.hpp"
#include "data/CameraComponent.hpp"
#include "data/ViewportComponent.hpp"
#include "data/InputComponent.hpp"

#include "functions/Execute.hpp"

#include "helpers/cameraHelper.hpp"
#include "helpers/matrixHelper.hpp"

#include "angle.hpp"
#include "imgui.h"
#include "ImGuizmo.h"

#pragma region Adjustables
static auto ZOOM_SPEED = .1f;
static auto GIZMO_LENGTH = 1.f;
static auto GIZMO_SCREEN_PERCENT = .1f;
#pragma endregion Adjustables

using namespace kengine;

EXPORT void loadKenginePlugin(void * state) {
	struct impl {
		static void init() noexcept {
			entities += [&](Entity & e) {
				e += CameraComponent{ { { 0.f, 0.f, -1.f }, { 1.f, 1.f, 1.f } } };
				e += ViewportComponent{};

				e += functions::Execute{ execute };
				e += InputComponent{
					.onScroll = processMouseScroll
				};

				e += AdjustableComponent{
					"Camera", {
						{ "Zoom speed", &ZOOM_SPEED },
						{ "Gizmo length", &GIZMO_LENGTH },
						{ "Gizmo screen percentage", &GIZMO_SCREEN_PERCENT }
					}
				};
			};
		}

		static void execute(float deltaTime) noexcept {
			drawImGui();

			if (ImGui::GetIO().WantCaptureMouse)
				return;

			for (auto [e, cam] : entities.with<CameraComponent>()) {
				const auto facings = cameraHelper::getFacings(cam);

				const auto dist = putils::getLength(cam.frustum.position);
				cam.frustum.position = -facings.front * dist;
			}
		}

		static void drawImGui() noexcept {
			ImGuizmo::BeginFrame();

			for (const auto & [_, cam, viewport] : entities.with<CameraComponent, ViewportComponent>()) {
				auto & io = ImGui::GetIO();

				const putils::Point2f scaledDisplaySize = { io.DisplaySize.x * viewport.boundingBox.size.x, io.DisplaySize.y * viewport.boundingBox.size.y };

				const ImVec2 size{ GIZMO_SCREEN_PERCENT * scaledDisplaySize.x, GIZMO_SCREEN_PERCENT * scaledDisplaySize.x };

				const auto imguiViewport = ImGui::GetMainViewport();
				const ImVec2 pos{
					imguiViewport->Pos.x + io.DisplaySize.x * viewport.boundingBox.position.x + scaledDisplaySize.x - size.x,
					imguiViewport->Pos.y + io.DisplaySize.y * viewport.boundingBox.position.y + scaledDisplaySize.y - size.y
				};

				auto view = matrixHelper::getViewMatrix(cam, viewport);
				ImGuizmo::ViewManipulate(glm::value_ptr(view), GIZMO_LENGTH, pos, size, 0);

				const auto quat = glm::conjugate(glm::toQuat(view));
				const auto rotation = matrixHelper::getRotation(glm::toMat4(quat));
				cam.yaw = rotation.y + putils::pi;
				cam.pitch = rotation.x;
				cam.roll = rotation.z;
			}
		}

		static void processMouseScroll(EntityID window, float xoffset, float yoffset, const putils::Point2f & coords) noexcept {
			const auto cameraId = cameraHelper::getViewportForPixel(window, coords).camera;
			if (cameraId == INVALID_ID)
				return;

			auto e = entities[cameraId];
			auto & cam = e.get<CameraComponent>();

			const auto facings = cameraHelper::getFacings(cam);
			cam.frustum.position += yoffset * ZOOM_SPEED * facings.front;
		}
	};

	pluginHelper::initPlugin(state);
	impl::init();
}

