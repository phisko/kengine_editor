#include "kengine.hpp"
#include "Export.hpp"
#include "helpers/pluginHelper.hpp"

#include "data/AdjustableComponent.hpp"
#include "data/AnimationComponent.hpp"
#include "data/AnimationFilesComponent.hpp"
#include "data/EditorComponent.hpp"
#include "data/ModelAnimationComponent.hpp"

#include "functions/Execute.hpp"

#include "meta/ToSave.hpp"

#include "helpers/instanceHelper.hpp"
#include "helpers/typeHelper.hpp"

#include "imgui.h"
#include "helpers/imfilebrowser.h"

using namespace kengine;

static bool g_active = true;
static ImGui::FileBrowser g_dialog;

EXPORT void loadKenginePlugin(void * state) noexcept {
	struct impl {
		static void init() noexcept {
			g_dialog.SetTitle("Add an animation file");

			typeHelper::getTypeEntity<AnimationFilesComponent>() += meta::ToSave{};

			entities += [](Entity & e) noexcept {
				e += EditorComponent{ "Animations", &g_active };
				e += functions::Execute{ execute };
			};
		}

		static void execute(float deltaTime) noexcept {
			if (!g_active)
				return;

			if (ImGui::Begin("Animations")) {
				for (auto [e, instance] : entities.with<InstanceComponent>()) {
					auto model = entities[instance.model];
					const auto modelAnim = model.tryGet<ModelAnimationComponent>();
					if (!modelAnim)
						continue;

					auto & animFiles = model.attach<AnimationFilesComponent>();
					displayAnimFileLoader(animFiles);

					auto & anim = e.attach<AnimationComponent>();
					displayAnimPicker(anim, *modelAnim);
					displaySpeedAndTimeEditor(anim, *modelAnim);
				}
			}
			ImGui::End();
		}

		static void displayAnimFileLoader(AnimationFilesComponent & animFiles) noexcept {
			if (ImGui::Button("Add animation file", { -1.f, 0.f }))
				g_dialog.Open();

			g_dialog.Display();
			if (!g_dialog.HasSelected())
				return;

			const auto selected = g_dialog.GetSelected().string();
			g_dialog.ClearSelected();

			for (const auto & f : animFiles.files)
				if (f == selected)
					return;
			
			animFiles.files.push_back(std::move(selected));
		}

		static void displayAnimPicker(AnimationComponent & anim, const ModelAnimationComponent & modelAnim) noexcept {
			int currentAnim = anim.currentAnim;
			ImGui::PushItemWidth(-1.f);
			ImGui::Combo("##Playing", &currentAnim, comboItemGetter, (void *)&modelAnim, (int)modelAnim.animations.size());
			ImGui::PopItemWidth();
			anim.currentAnim = currentAnim;
		}

		static bool comboItemGetter(void * data, int idx, const char ** out_text) noexcept {
			const auto modelAnim = (const ModelAnimationComponent *)data;
			const auto & fileName = modelAnim->animations[idx].name;

			// Only show "filename/animName"
			int lastSlash = (int)fileName.size() - 1; {
				size_t skipped = 0;
				for (; lastSlash >= 0; --lastSlash) {
					if (fileName[lastSlash] != '/' && fileName[lastSlash] != '\\')
						continue;
					++skipped;
					if (skipped == 2)
						break;
				}
			}
			++lastSlash;

			*out_text = fileName.c_str() + lastSlash;
			return true;
		}

		static void displaySpeedAndTimeEditor(AnimationComponent & anim, const ModelAnimationComponent & modelAnim) noexcept {
			const auto & currentAnim = modelAnim.animations[anim.currentAnim];
			ImGui::SliderFloat("Time", &anim.currentTime, 0.f, currentAnim.totalTime);
			ImGui::InputFloat("Speed", &anim.speed);
		}
	};

	pluginHelper::initPlugin(state);
	impl::init();
}
