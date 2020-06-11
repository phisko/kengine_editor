#pragma once

#include <string>
#include <functional>
#include "reflection.hpp"

struct ImGuiMainMenuBarItemComponent {
	std::string menu;
	std::string item;
	std::function<void()> onClick;

	putils_reflection_class_name(ImGuiMainMenuBarItemComponent);
	putils_reflection_attributes(
		putils_reflection_attribute(&ImGuiMainMenuBarItemComponent::menu),
		putils_reflection_attribute(&ImGuiMainMenuBarItemComponent::item),
		putils_reflection_attribute(&ImGuiMainMenuBarItemComponent::onClick)
	);
};