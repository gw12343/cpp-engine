-- Builds two entities that store each other's EntityHandle in `other`.
-- Called from the editor "Prefab Pair Test" window.

local CUBE = "assets/models/cube.obj"

local function addPairMember(name, worldPos)
    local e = createEntity(name)
    local tr = e:AddTransform()
    tr.position = worldPos
    tr.scale = vec3(0.8, 0.8, 0.8)

    local mr = e:AddModelRenderer()
    mr:setModel(CUBE)

    local label = e:AddText3DComponent()
    label.text = name
    label.size = 0.18
    label.billboard = true

    local script = e:AddLuaScript()
    script:setScript(e, "scripts/prefab_pair.lua")
    return e
end

function BuildPrefabPair()
    local root = createEntity("PrefabPair")
    local rootTr = root:AddTransform()
    rootTr.position = vec3(0, 1.5, 0)

    local a = addPairMember("PairA", vec3(-1.2, 1.5, 0))
    local b = addPairMember("PairB", vec3(1.2, 1.5, 0))

    local rootHandle = root:getHandle()
    a:setParent(rootHandle)
    b:setParent(rootHandle)

    -- These ehandles are what prefab instantiate must remap.
    a:GetLuaScript():setVariable("other", b:getHandle())
    b:GetLuaScript():setVariable("other", a:getHandle())

    print("[prefab_pair_setup] PairA.other = " .. b:getHandle():getGuid() .. " (" .. b:getName() .. ")")
    print("[prefab_pair_setup] PairB.other = " .. a:getHandle():getGuid() .. " (" .. a:getName() .. ")")
    print("  Play and read the 3D labels / Script `other` field.")
    print("  Save PrefabPair as a prefab, instantiate it, then compare GUIDs.")
    print("  New PairA.other must equal the NEW PairB GUID, not the original.")

    local spawner = createEntity("PrefabSpawner")
    spawner:AddTransform()
    local ss = spawner:AddLuaScript()
    ss:setScript(spawner, "scripts/prefab_spawner.lua")
    return root
end
