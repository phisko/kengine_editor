#include "helpers/registerTypeHelper.hpp"
#include "data/LuaComponent.hpp"
#include "data/LuaTableComponent.hpp"

void registerLuaComponent() noexcept {
	kengine::registerComponents<
		kengine::LuaComponent,
		kengine::LuaTableComponent
	>();
}
