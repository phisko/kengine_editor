#include "kengine.hpp"
#include "Export.hpp"
#include "helpers/pluginHelper.hpp"

#include "data/InstanceComponent.hpp"
#include "functions/Execute.hpp"
#include "helpers/imguiHelper.hpp"

#include "imgui.h"

using namespace kengine;

EXPORT void loadKenginePlugin(void * state) noexcept {
	struct impl {
		static void init() noexcept {
			entities += [](Entity & e) noexcept {
				e += functions::Execute{ execute };
			};
		}

		static void execute(float deltaTime) noexcept {
			for (const auto & [e, instance] : entities.with<InstanceComponent>()) {
				auto model = entities[instance.model];
				if (ImGui::Begin("Model"))
					imguiHelper::editEntity(model);
				ImGui::End();
			}
		}
	};

	pluginHelper::initPlugin(state);
	impl::init();
}
