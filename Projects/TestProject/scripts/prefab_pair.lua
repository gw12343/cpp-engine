-- Pair member. Assign `other` to the other entity in the inspector.
-- After you save the pair as a prefab and instantiate it, select the NEW
-- cubes: `other` must resolve to the NEW sibling (new GUID), not the original.

variables = {
    other = ehandle(),
}

local lastLabel = ""

local function shortGuid(handle)
    if not handle or not handle:isValid() then
        return "(empty)"
    end
    local id = handle:getGuid()
    if #id >= 8 then
        return string.sub(id, 1, 8)
    end
    return id
end

local function resolveOther()
    if not variables.other or not variables.other:isValid() then
        return nil
    end
    local e = getEntityFromHandle(variables.other)
    if e and e:isValid() then
        return e
    end
    return nil
end

local function writeLabel(text)
    if lastLabel == text then
        return
    end
    lastLabel = text
    if gameObject:HasText3DComponent() then
        gameObject:GetText3DComponent().text = text
    end
end

function Start()
    local me = gameObject:getHandle()
    local other = resolveOther()
    local otherName = "(unresolved)"
    if other then
        otherName = other:getName()
    end

    print("========== prefab_pair ==========")
    print("  entity : " .. gameObject:getName())
    print("  my GUID: " .. me:getGuid())
    print("  other  : " .. variables.other:getGuid() .. "  ->  " .. otherName)
    print("=================================")

    writeLabel(gameObject:getName() .. "\nme    " .. shortGuid(me) .. "\nother " .. shortGuid(variables.other) .. "\n" .. otherName)
end

function Update()
    local me = gameObject:getHandle()
    local other = resolveOther()
    local otherName = "(unresolved)"
    if other then
        otherName = other:getName()
    end
    writeLabel(gameObject:getName() .. "\nme    " .. shortGuid(me) .. "\nother " .. shortGuid(variables.other) .. "\n" .. otherName)
end
