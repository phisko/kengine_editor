#pragma once

#include <string>
#include <functional>
#include "reflection.hpp"

struct ImGuiMainMenuBarItemComponent {
	std::string menu;
	std::string itemName;
	std::function<void()> draw;
};

#define refltype ImGuiMainMenuBarItemComponent
putils_reflection_info{
	putils_reflection_class_name;
	putils_reflection_attributes(
		putils_reflection_attribute(menu),
		putils_reflection_attribute(itemName),
		putils_reflection_attribute(draw)
	);
};
#undef refltype