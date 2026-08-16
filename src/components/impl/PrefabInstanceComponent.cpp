#include "PrefabInstanceComponent.h"

#include "assets/Prefab.h"
#include "core/Entity.h"
#include "rendering/ui/InspectorUI.h"
#include "scripting/ScriptManager.h"

#include <stdexcept>
#include <vector>

namespace Engine::Components {
	void PrefabInstance::OnAdded(Entity& entity)
	{
		(void) entity;
	}

	void PrefabInstance::OnRemoved(Entity& entity)
	{
		(void) entity;
	}

	void PrefabInstance::RenderInspector(Entity& entity)
	{
		(void) entity;
		LeftLabelAssetPrefab("Prefab", &prefab);
	}

	void PrefabInstance::AddBindings()
	{
		auto& lua = GetScriptManager().lua;

		lua.new_usertype<PrefabHandle>(
		    "PrefabHandle",
		    sol::constructors<PrefabHandle(), PrefabHandle(const std::string&)>(),
		    "getGuid",
		    &PrefabHandle::GetID,
		    "isValid",
		    &PrefabHandle::IsValid,
		    "clear",
		    [](PrefabHandle& self) { self = PrefabHandle(); },
		    "instantiate",
		    sol::overload(
		        [](const PrefabHandle& self) { return InstantiatePrefab(self); },
		        [](const PrefabHandle& self, const EntityHandle& parent) { return InstantiatePrefab(self, parent); }));

		lua.set_function("prefab", sol::overload([]() { return PrefabHandle(); }, [](const std::string& g) { return PrefabHandle(g); }));
		lua.set_function(
		    "instantiatePrefab",
		    sol::overload([](const PrefabHandle& h) { return InstantiatePrefab(h); }, [](const PrefabHandle& h, const EntityHandle& parent) { return InstantiatePrefab(h, parent); }));

		using PrefabVector = std::vector<PrefabHandle>;
		lua.new_usertype<PrefabVector>(
		    "PrefabVector",
		    sol::constructors<PrefabVector()>(),
		    "push_back",
		    static_cast<void (PrefabVector::*)(const PrefabHandle&)>(&PrefabVector::push_back),
		    "size",
		    &PrefabVector::size,
		    sol::meta_function::index,
		    [](PrefabVector& self, std::size_t i) -> PrefabHandle& {
			    if (i == 0 || i > self.size()) throw std::out_of_range("Index out of range");
			    return self[i - 1];
		    },
		    sol::meta_function::new_index,
		    [](PrefabVector& self, std::size_t i, const PrefabHandle& value) {
			    if (i == 0 || i > self.size()) throw std::out_of_range("Index out of range");
			    self[i - 1] = value;
		    });
	}
} // namespace Engine::Components
