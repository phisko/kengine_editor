#pragma once

#include <string>
#include "reflection.hpp"

struct EditorComponent {
	std::string name;
	bool * active = nullptr;
};

#define refltype EditorComponent
putils_reflection_info{
	putils_reflection_class_name;
	putils_reflection_attributes(
		putils_reflection_attribute(name),
		putils_reflection_attribute(active)
	);
};
#undef refltype
