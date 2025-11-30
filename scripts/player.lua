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
    CAMERA_Y_OFFSET =  1.1, --1.45,
    MOUSE_SENSITIVITY = 0.1,
    WALK_SPEED = 7.0,
    RUN_SPEED = 12.0,
    JUMP_POWER = 8.0,
    GRAVITY_SCALE = 2.0,
    SHOOT_POWER = 15,

    BULLET_PARENT = ehandle(),

    BULLET_MATERIAL = material(),

	shootSound = sound(),
	victorySound = sound()
}





function Start()
    print("Started script!");



	subscribe("TargetHit", function(points)
        local playerSource = gameObject:GetAudioSource()
		playerSource:setSound(variables.shootSound)
		playerSource:play()
    end)

	subscribe("AllTargetsDestroyed", function()
        local playerSource = gameObject:GetAudioSource()
		playerSource:setSound(variables.victorySound)
		playerSource:play()
    end)
    

end

ballCount = 0

function ShootObject(model, shape, speed, scale)
	
    local cam = getCamera()
    local cpos = cam:getPosition()
    local foward = cam:getFront()
    -- Summon entity
    local newBall = createEntity("Ball"..ballCount)
    ballCount = ballCount + 1
    -- Add Components
    local tr = newBall:AddTransform()
    local mr = newBall:AddModelRenderer();

    mr:setMaterial(variables.BULLET_MATERIAL)

    local rb = newBall:AddRigidBodyComponent();
    local sc = newBall:AddLuaScript();
    newBall:AddShadowCaster()
    newBall:setParent(variables.BULLET_PARENT)

    tr.scale = vec3(scale, scale, scale)
    mr:setModel(model)

    local camPos = cam:getPosition()

    local inFrontPos = vec3(
        camPos.x + foward.x * 2,
        camPos.y + foward.y * 2,
        camPos.z + foward.z * 2
    )

    rb:setPosition(inFrontPos)
    rb:addLinearVelocity(vec3(foward.x * speed, foward.y * speed, foward.z * speed))
    local t = shape:getType()
    if t == "BoxShape" then
        rb:setBoxShape(shape)
    elseif t == "SphereShape" then
        rb:setSphereShape(shape)
    elseif t == "CapsuleShape" then
        rb:setCapsuleShape(shape)
    elseif t == "CylinderShape" then
        rb:setCylinderShape(shape)
    elseif t == "TriangleShape" then
        rb:setTriangleShape(shape)
    else
        print("Unknown shape type: " .. tostring(t))
    end
    sc:setScript(newBall, "scripts/bullet.lua")
end


function Update()
    local input = getInput()
    local camera = getCamera()
    local tr = gameObject:GetTransform()
    local cr = gameObject:GetPlayerControllerComponent()
    input:setCursorMode(CURSOR_DISABLED)

    local inMovementDirection = vec3(0, 0, 0)
    local front = camera:getFront()

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

    front.y = 0.0
    front = front:normalize()


    local right = front:cross(vec3(0, 1, 0)):normalize()

    local mov = vec3(
    front.x * yAxis + right.x * xAxis,
    front.y * yAxis + right.y * xAxis,
    front.z * yAxis + right.z * xAxis
    )

    if mov:length() > 0.0 then
        mov = mov:normalize()
    end

    inMovementDirection = vec3(mov.x, 0, mov.z)



    -- Determine new basic velocity
    local current_vertical_velocity_mag = cr:getLinearVelocity():dot(vec3(0, 1, 0))
    local current_vertical_velocity = vec3(0, current_vertical_velocity_mag, 0)
    local ground_velocity = cr:getGroundVelocity()
    local new_velocity = vec3(0, 0, 0)
    local moving_towards_ground = (current_vertical_velocity_mag - ground_velocity.y) < 0.1
    local inJump = input:isKeyPressed(KEY_SPACE)

    local moveSpeed = variables.WALK_SPEED
    if input:isKeyPressed(KEY_LEFT_SHIFT) and cr:isOnGround() then
        if yAxis > 0 then
            if xAxis == 0 then
                moveSpeed = variables.RUN_SPEED
            else
                moveSpeed = variables.WALK_SPEED + (variables.RUN_SPEED - variables.WALK_SPEED) / 2
            end
        end
    end

    local desiredVelocity = vec3(
        inMovementDirection.x * moveSpeed,
        inMovementDirection.y * moveSpeed,
        inMovementDirection.z * moveSpeed
    )

    if cr:isOnGround() and moving_towards_ground then
        new_velocity = ground_velocity

        if inJump and moving_towards_ground then
            -- Jump along up
            new_velocity = vec3(
                new_velocity.x + 0 * variables.JUMP_POWER,
                new_velocity.y + 1 * variables.JUMP_POWER,
                new_velocity.z + 0 * variables.JUMP_POWER
            )
        end
    else
        new_velocity = current_vertical_velocity
    end


    -- Gravity
    local g = getPhysics():getGravity()
    new_velocity = vec3(
        new_velocity.x + g.x * deltaTime,
        new_velocity.y + g.y * deltaTime * variables.GRAVITY_SCALE,
        new_velocity.z + g.z * deltaTime
    )

    -- Player input
    new_velocity = vec3(
        new_velocity.x + desiredVelocity.x,
        new_velocity.y + desiredVelocity.y,
        new_velocity.z + desiredVelocity.z
    )


    -- Update character velocity
    cr:setLinearVelocity(new_velocity)


    -- Camera Controls
    local mouseDelta = input:getMouseDelta()
    local xoffset = mouseDelta.x;
    local yoffset = mouseDelta.y;
    xoffset = xoffset * variables.MOUSE_SENSITIVITY
    yoffset = yoffset * variables.MOUSE_SENSITIVITY
    camera.yaw = camera.yaw + xoffset
    camera.pitch = camera.pitch + yoffset

    -- Clamp Pitch
    if camera.pitch < -89.0 then
        camera.pitch = -89.0
    elseif camera.pitch > 89.0 then
        camera.pitch = 89.0
    end

    -- Move Camera
    local crPos = cr:getPosition()
    local nextPos = vec3(crPos.x, crPos.y + variables.CAMERA_Y_OFFSET, crPos.z)
    camera:setPosition(nextPos)

    -- Shoot Balls
    if input:isKeyPressedThisFrame(KEY_E) then
        local shape = SphereShape(0.5 / 2)
        ShootObject("resources/models/sphere.obj", shape, variables.SHOOT_POWER, 0.5)
    end
end
