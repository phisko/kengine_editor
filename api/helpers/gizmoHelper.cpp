#include <GLFW/glfw3.h>
#include <glm/gtc/type_ptr.hpp>
#include "gizmoHelper.hpp"

#include "data/InstanceComponent.hpp"
#include "functions/OnClick.hpp"
#include "helpers/matrixHelper.hpp"

#include "magic_enum.hpp"
#include "epsilon.hpp"

#include "ImGuizmo.h"

namespace gizmoHelper {
	using namespace kengine;

	static bool g_shouldOpenContextMenu = false;
	static GizmoType g_gizmoType = GizmoType::Translate;

	static bool g_uniformScale = true;
	float putils::Point3f:: * g_currentScaleModifier = nullptr;

	static size_t g_gizmoId = 0;

	void newFrame(const ImVec2 & windowSize, const ImVec2 & windowPos) noexcept {
		g_gizmoId = 0;
		ImGuizmo::BeginFrame();
		ImGuizmo::SetDrawlist();
		ImGuizmo::SetRect(windowPos.x, windowPos.y, windowSize.x, windowSize.y);
	}

	void drawGizmo(kengine::TransformComponent & transform, const glm::mat4 & proj, const glm::mat4 & view, const glm::mat4 * parentMat, bool revertParentBeforeTransform) noexcept {
		ImGuizmo::SetID(g_gizmoId++);

		auto matrix = matrixHelper::getModelMatrix(transform);
		if (parentMat)
			matrix = *parentMat * matrix;
		glm::mat4 delta;

		switch (g_gizmoType) {
		case GizmoType::Translate:
			ImGuizmo::Manipulate(glm::value_ptr(view), glm::value_ptr(proj), ImGuizmo::TRANSLATE, ImGuizmo::WORLD, glm::value_ptr(matrix), glm::value_ptr(delta));
			if (parentMat && revertParentBeforeTransform)
				matrix = glm::inverse(*parentMat) * matrix;
			transform.boundingBox.position = matrixHelper::getPosition(matrix);
			break;
		case GizmoType::Scale:
		{
			ImGuizmo::Manipulate(glm::value_ptr(view), glm::value_ptr(proj), ImGuizmo::SCALE, ImGuizmo::WORLD, glm::value_ptr(matrix), glm::value_ptr(delta));
			if (parentMat && revertParentBeforeTransform)
				matrix = glm::inverse(*parentMat) * matrix;
			auto & size = transform.boundingBox.size;
			const auto previousSize = size;
			size = matrixHelper::getScale(matrix);
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
		case GizmoType::Rotate:
		{
			// modelMatrix = glm::transpose(modelMatrix);
			ImGuizmo::Manipulate(glm::value_ptr(view), glm::value_ptr(proj), ImGuizmo::ROTATE, ImGuizmo::WORLD, glm::value_ptr(matrix), glm::value_ptr(delta));
			if (parentMat && revertParentBeforeTransform)
				matrix = glm::inverse(*parentMat) * matrix;
			const auto rotation = matrixHelper::getRotation(matrix);
			transform.yaw = rotation.y;
			transform.pitch = rotation.x;
			transform.roll = rotation.z;
			break;
		}
		default:
			static_assert(putils::magic_enum::enum_count<GizmoType>() == 3);
			break;
		}

		if (!ImGuizmo::IsUsing())
			g_currentScaleModifier = nullptr;
	}

	void handleContextMenu(const std::function<void()> & displayContextMenu) noexcept {
		for (auto [e, instance] : entities.with<InstanceComponent>()) {
			e += kengine::functions::OnClick{ [](int button) {
				if (button == GLFW_MOUSE_BUTTON_RIGHT)
					g_shouldOpenContextMenu = true;
			} };
		}

		if (g_shouldOpenContextMenu) {
			ImGui::OpenPopup("Gizmo Popup");
			g_shouldOpenContextMenu = false;
		}

		if (ImGui::BeginPopup("Gizmo Popup")) {
			for (const auto [gizmoType, name] : putils::magic_enum::enum_entries<GizmoType>()) {
				bool ticked = g_gizmoType == gizmoType;
				if (ImGui::MenuItem(putils::string<64>(name), nullptr, &ticked))
					g_gizmoType = gizmoType;
			}
			ImGui::MenuItem("Uniform scale", nullptr, &g_uniformScale);

			if (displayContextMenu != nullptr)
				displayContextMenu();
			ImGui::EndPopup();
		}
	}
}