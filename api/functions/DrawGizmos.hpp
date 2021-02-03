#pragma once

#include "BaseFunction.hpp"

#include <glm/glm.hpp>
#include "imgui.h"

namespace functions {
	struct DrawGizmos : kengine::functions::BaseFunction<
		void(const glm::mat4 & proj, const glm::mat4 & view, const ImVec2 & windowSize, const ImVec2 & windowPos)
	>
	{};
}