-- Attach to any entity in a prefab. Start() runs when the instance is created
-- (play mode, or the frame a prefab is instantiated while playing).
-- Destroys this entity after LIFETIME seconds.

variables = {
    LIFETIME = 5.0,
}

local age = 0.0

function Start()
    local handle = gameObject:getHandle()
    print("[prefab_created] spawned '" .. gameObject:getName() .. "'  guid=" .. handle:getGuid())

    subscribe("GameReset", function()
        gameObject:destroy()
    end)
end

function Update()
    age = age + deltaTime
    if age >= variables.LIFETIME then
        print("[prefab_created] despawn '" .. gameObject:getName() .. "'")
        gameObject:destroy()
    end
end
