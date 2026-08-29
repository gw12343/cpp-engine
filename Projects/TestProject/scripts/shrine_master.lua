-- Shrine of Five Lanterns — game state.
-- Attach to the GameMaster entity in scenes/shrine.json.
--
-- Open scenes/shrine.json, press Play:
--   WASD move, mouse look, Space jump, hold into a wall to climb
--   E / gamepad X throws PROJECTILE_PREFAB orbs
--   Light all five lanterns, then R to reset

variables = {
    TOTAL_LANTERNS = 5,
    LANTERN_WEST  = ehandle(),
    LANTERN_EAST  = ehandle(),
    LANTERN_PAD   = ehandle(),
    LANTERN_LEDGE = ehandle(),
    LANTERN_PEAK  = ehandle(),
}

local remaining = 0
local score = 0
local won = false

local function lanternList()
    return {
        variables.LANTERN_WEST,
        variables.LANTERN_EAST,
        variables.LANTERN_PAD,
        variables.LANTERN_LEDGE,
        variables.LANTERN_PEAK,
    }
end

local function describeLanterns()
    local list = lanternList()
    for i = 1, #list do
        local h = list[i]
        if h and h.isValid and h:isValid() then
            local e = getEntityFromHandle(h)
            local name = (e and e:isValid()) and e:getName() or "?"
            print(string.format("[Shrine] lantern %d  %s  guid=%s", i, name, h:getGuid()))
        else
            print(string.format("[Shrine] lantern %d  (unset handle)", i))
        end
    end
end

local function resetState()
    remaining = variables.TOTAL_LANTERNS
    score = 0
    won = false
    publish("ScoreChanged", 0)
    publish("TargetsHitUpdated", 0)
end

function Start()
    print("=== SHRINE MASTER ===")
    describeLanterns()
    resetState()
    print("[Shrine] light " .. tostring(remaining) .. " lanterns  (E to throw, R to reset)")

    subscribe("TargetHit", function(points)
        if won then
            return
        end
        local p = points or 0
        remaining = remaining - 1
        score = score + p
        local hit = variables.TOTAL_LANTERNS - remaining
        print(string.format("[Shrine] lantern lit  +%s  remaining=%s  score=%s",
            tostring(p), tostring(remaining), tostring(score)))
        publish("ScoreChanged", score)
        publish("TargetsHitUpdated", hit)
        if remaining <= 0 then
            won = true
            print("[Shrine] all lanterns lit")
            publish("AllTargetsDestroyed")
        end
    end)

    subscribe("GameReset", function()
        print("[Shrine] reset")
        resetState()
    end)
end

function Update()
    local input = getInput()
    if input:isKeyPressedThisFrame(KEY_R) then
        publish("GameReset")
    end
end
