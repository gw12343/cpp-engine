function protect(tbl)
    return setmetatable({}, {
        __index = tbl,
        __newindex = function(t, key, value)
            error("attempting to change constant " ..
                   tostring(key) .. " to " .. tostring(value), 2)
        end
    })
end


variables = {
    CAMERA_Y_OFFSET =  2,
    CAMERA_Z_OFFSET = 2.700,
    CAMERA_PITCH = -15,
    MOVE_POWER = 0.1,
    MAX_HORIZONTAL_VEL = 0.4,
    SLOW_POWER = 10,


    SPAWN_POINT = ehandle(),
    KILL_SOUND = sound()
}





-- Vars
local input
local camera
local tr
local rb

function SendToSpawn()
    local spawnPos = getEntityFromHandle(variables.SPAWN_POINT):GetTransform().position
    rb:setPosition(spawnPos)
    rb:setLinearVelocity(vec3(0,0,0))

    local ac = gameObject:GetAudioSource()
    ac:setSound(variables.KILL_SOUND);
    ac:play();

    --TODO reset rotational momentum
end


function Start()
    print("Started script!");
    input = getInput()
    camera = getCamera()
    tr = gameObject:GetTransform()
    rb = gameObject:GetRigidBodyComponent()

    SendToSpawn()
end

function CollisionEnter(other)
    if other:getTag() == "KillPlayer" then
        print("Game Over!!")
        SendToSpawn()
    end
end


function Update()
    --input:setCursorMode(CURSOR_DISABLED)
    local xAxis = 0
    local yAxis = 0

    if input:isKeyPressed(KEY_W) then
        yAxis = yAxis + 1
    end
    if input:isKeyPressed(KEY_S) then
        yAxis = yAxis - 1
    end
    if input:isKeyPressed(KEY_A) then
        xAxis = xAxis - 1
    end
    if input:isKeyPressed(KEY_D) then
        xAxis = xAxis + 1
    end


    local newVel = rb:getLinearVelocity()
    newVel.x = newVel.x + xAxis * variables.MOVE_POWER * deltaTime
    -- Limit horizontal velocity
    if newVel.x < -math.abs(variables.MAX_HORIZONTAL_VEL) then
        newVel.x = -math.abs(variables.MAX_HORIZONTAL_VEL)
    elseif newVel.x > math.abs(variables.MAX_HORIZONTAL_VEL) then
        newVel.x = math.abs(variables.MAX_HORIZONTAL_VEL)
    end


    if newVel.x > 0 then
        newVel.x = newVel.x - variables.SLOW_POWER * deltaTime
    elseif newVel.x < 0 then
        newVel.x = newVel.x + variables.SLOW_POWER * deltaTime
    end


    rb:setLinearVelocity(newVel)

    -- Set fixed rotation
    camera.yaw = -90
    camera.pitch = variables.CAMERA_PITCH

    -- Move Camera
    local newPos = tr.position
    newPos.y = newPos.y +  variables.CAMERA_Y_OFFSET
    newPos.z = newPos.z +  variables.CAMERA_Z_OFFSET
    camera:setPosition(tr.position)

end
