#pragma once

#include <functional>
#include "kengine.hpp"
#include "data/TransformComponent.hpp"
#include "imgui.h"

namespace gizmoHelper {
	enum class GizmoType {
		Translate,
		Scale,
		Rotate
	};

	void newFrame(const ImVec2 & windowSize, const ImVec2 & windowPos) noexcept;
	void drawGizmo(kengine::TransformComponent & transform, const glm::mat4 & proj, const glm::mat4 & view, const glm::mat4 * parentMatrix = nullptr, bool revertParentMatrixBeforeTransform = false) noexcept;
	void handleContextMenu(const std::function<void()> & displayContextMenu = nullptr) noexcept;
}