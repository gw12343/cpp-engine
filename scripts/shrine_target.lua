-- A shrine lantern. Any collision lights it once and publishes TargetHit.
-- GameReset restores the unlit material.

variables = {
    POINTS = 100,
    LIT_COLOR = material(),
    UNLIT_COLOR = material(),
    LABEL = "Lantern",
}

local lit = false

local function applyMaterial(handle)
    local mr = gameObject:GetModelRenderer()
    if mr and handle then
        mr:setMaterial(handle)
    end
end

function Start()
    lit = false
    applyMaterial(variables.UNLIT_COLOR)
    print("[Shrine] " .. tostring(variables.LABEL) .. " ready  +" .. tostring(variables.POINTS))

    subscribe("GameReset", function()
        lit = false
        applyMaterial(variables.UNLIT_COLOR)
    end)
end

function CollisionEnter(other)
    if lit then
        return
    end
    lit = true
    applyMaterial(variables.LIT_COLOR)
    publish("TargetHit", variables.POINTS)
end

function Update()
end
