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

using namespace kengine;

static struct {
	float lookAtY = .5f;
	float zoomSpeed = .1f;
	float gizmoLength = 1.f;
	float gizmoScreenPercent = .1f;
} adjustables;

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
						{ "Look at Y", &adjustables.lookAtY },
						{ "Zoom speed", &adjustables.zoomSpeed },
						{ "Gizmo length", &adjustables.gizmoLength },
						{ "Gizmo screen percentage", &adjustables.gizmoScreenPercent }
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

				const putils::Point3f lookAt{ 0.f, adjustables.lookAtY, 0.f };
				const auto dist = putils::getLength(cam.frustum.position - lookAt);
				cam.frustum.position = lookAt - facings.front * dist;
			}
		}

		static void drawImGui() noexcept {
			ImGuizmo::BeginFrame();

			for (const auto & [_, cam, viewport] : entities.with<CameraComponent, ViewportComponent>()) {
				auto & io = ImGui::GetIO();

				const putils::Point2f scaledDisplaySize = { io.DisplaySize.x * viewport.boundingBox.size.x, io.DisplaySize.y * viewport.boundingBox.size.y };

				const ImVec2 size{ adjustables.gizmoScreenPercent * scaledDisplaySize.x, adjustables.gizmoScreenPercent * scaledDisplaySize.x };

				const auto imguiViewport = ImGui::GetMainViewport();
				const ImVec2 pos{
					imguiViewport->Pos.x + io.DisplaySize.x * viewport.boundingBox.position.x + scaledDisplaySize.x - size.x,
					imguiViewport->Pos.y + io.DisplaySize.y * viewport.boundingBox.position.y + scaledDisplaySize.y - size.y
				};

				auto view = matrixHelper::getViewMatrix(cam, viewport);
				ImGuizmo::ViewManipulate(glm::value_ptr(view), adjustables.gizmoLength, pos, size, 0);

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
			cam.frustum.position += yoffset * adjustables.zoomSpeed * facings.front;
		}
	};

	pluginHelper::initPlugin(state);
	impl::init();
}

