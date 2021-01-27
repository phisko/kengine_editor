#include "kengine.hpp"
#include "Export.hpp"
#include "helpers/pluginHelper.hpp"

#include "data/CameraComponent.hpp"
#include "data/EditorComponent.hpp"
#include "data/InstanceComponent.hpp"
#include "data/ViewportComponent.hpp"

#include "functions/Execute.hpp"
#include "functions/DrawGizmos.hpp"

#include "helpers/matrixHelper.hpp"

#include "imgui.h"

using namespace kengine;

static bool g_active = true;

EXPORT void loadKenginePlugin(void * state) noexcept {
	struct impl {
		static void init() noexcept {
			entities += [](Entity & e) noexcept {
				e += kengine::functions::Execute{ execute };
			};
		}

		static void execute(float deltaTime) noexcept {
			for (const auto & [e, cam, viewport] : entities.with<CameraComponent, ViewportComponent>()) {
				const auto proj = matrixHelper::getProjMatrix(cam, viewport, 0.001f, 1000.f);
				const auto view = matrixHelper::getViewMatrix(cam, viewport);

				const auto imguiViewport = ImGui::GetMainViewport();
				ImGui::SetNextWindowViewport(imguiViewport->ID);
				ImGui::SetNextWindowPos(imguiViewport->Pos);
				ImGui::SetNextWindowSize(imguiViewport->Size);

				ImGui::Begin("Gizmos", nullptr, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoInputs);

				const auto windowSize = ImGui::GetWindowSize();
				const auto windowPos = ImGui::GetWindowPos();

				for (const auto & [e, drawGizmos] : entities.with<::functions::DrawGizmos>())
					drawGizmos(proj, view, windowSize, windowPos);

				ImGui::End();
			}
		}
	};

	pluginHelper::initPlugin(state);
	impl::init();
}
