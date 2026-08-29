-- Assign the saved PrefabPair asset, then press I while playing.

variables = {
    pairPrefab = prefab(),
    spacing = 4.0,
}

local spawnCount = 0

local function dumpHandles(entity, indent)
    indent = indent or ""
    local line = indent .. entity:getName() .. "  me=" .. entity:getHandle():getGuid()
    if entity:HasLuaScript() then
        local other = entity:GetLuaScript():getVariable("other")
        if other then
            local resolved = getEntityFromHandle(other)
            local name = "(unresolved)"
            if resolved and resolved:isValid() then
                name = resolved:getName()
            end
            line = line .. "  other=" .. other:getGuid() .. " -> " .. name
        end
    end
    print(line)

    local kids = entity:getChildren()
    for i = 1, kids:size() do
        local child = getEntityFromHandle(kids[i])
        if child and child:isValid() then
            dumpHandles(child, indent .. "  ")
        end
    end
end

local function spawnOne()
    if not variables.pairPrefab or not variables.pairPrefab:isValid() then
        print("[prefab_spawner] Assign pairPrefab in the inspector first")
        return
    end

    local root = instantiatePrefab(variables.pairPrefab)
    if not root or not root:isValid() then
        print("[prefab_spawner] instantiate failed")
        return
    end

    spawnCount = spawnCount + 1
    if root:HasTransform() then
        local tr = root:GetTransform()
        local p = tr.position
        tr.position = vec3(p.x + variables.spacing * spawnCount, p.y, p.z)
    end

    print("[prefab_spawner] instance #" .. tostring(spawnCount))
    dumpHandles(root)
end

function Start()
    print("[prefab_spawner] Press I to instantiate pairPrefab")
end

function Update()
    if getInput():isKeyPressedThisFrame(KEY_I) then
        spawnOne()
    end
end
