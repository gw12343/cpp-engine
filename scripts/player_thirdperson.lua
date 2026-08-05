-- Third-person character controller + orbit camera.
-- Uses the same Jolt CharacterVirtual capsule (PlayerControllerComponent) as FPS.
-- Attach to the Player entity instead of scripts/player.lua.
--
-- Controls:
--   WASD           move (relative to camera yaw)
--   Left Shift     run
--   Space          jump
--   Mouse          orbit camera
--   Mouse wheel    zoom (if available)
--   E              shoot (same as FPS)

variables = {
    -- Camera orbit
    CAMERA_DISTANCE     = 5.0,
    CAMERA_MIN_DISTANCE = 2.0,
    CAMERA_MAX_DISTANCE = 12.0,
    CAMERA_HEIGHT       = 1.2,   -- look-at height above capsule center
    CAMERA_PITCH_MIN    = -30.0,
    CAMERA_PITCH_MAX    = 60.0,
    MOUSE_SENSITIVITY   = 0.12,
    ZOOM_SENSITIVITY    = 0.5,

    -- Spawn projectiles from capsule "head", pushed forward along look yaw
    SHOOT_HEAD_HEIGHT   = 1.1,   -- above character center
    SHOOT_FORWARD_OFFSET = 1.0,  -- horizontal offset along cos/sin(yaw)

    -- Movement (same capsule feel as FPS)
    WALK_SPEED    = 7.0,
    RUN_SPEED     = 12.0,
    JUMP_POWER    = 8.0,
    GRAVITY_SCALE = 2.0,
    TURN_SPEED    = 720.0, -- deg/sec when rotating to face move dir

    SHOOT_POWER     = 15,
    BULLET_PARENT   = ehandle(),
    BULLET_MATERIAL = material(),
    shootSound      = sound(),
    victorySound    = sound(),
}

-- Orbit state (degrees / meters)
local orbitYaw      = -90.0
local orbitPitch    = 15.0
local orbitDistance = nil
local faceYaw       = 0.0

local function clamp(v, lo, hi)
    if v < lo then return lo end
    if v > hi then return hi end
    return v
end

-- Lua 5.3+ removed math.atan2; two-arg math.atan is the replacement.
local function atan2(y, x)
    if math.atan2 then
        return math.atan2(y, x)
    end
    return math.atan(y, x)
end

local function length2(x, z)
    return math.sqrt(x * x + z * z)
end

local function normalize2(x, z)
    local len = length2(x, z)
    if len < 1e-5 then
        return 0.0, 0.0, 0.0
    end
    return x / len, z / len, len
end

local function lerpAngle(from, to, maxStep)
    local diff = to - from
    while diff > 180.0 do diff = diff - 360.0 end
    while diff < -180.0 do diff = diff + 360.0 end
    if math.abs(diff) <= maxStep then
        return to
    end
    if diff > 0.0 then
        return from + maxStep
    end
    return from - maxStep
end

function Start()
    orbitDistance = variables.CAMERA_DISTANCE
    local camera = getCamera()
    orbitYaw   = camera.yaw
    orbitPitch = clamp(camera.pitch, variables.CAMERA_PITCH_MIN, variables.CAMERA_PITCH_MAX)

    subscribe("TargetHit", function()
        local src = gameObject:GetAudioSource()
        src:setSound(variables.shootSound)
        src:play()
    end)

    subscribe("AllTargetsDestroyed", function()
        local src = gameObject:GetAudioSource()
        src:setSound(variables.victorySound)
        src:play()
    end)

    info("third-person controller ready")
end

ballCount = 0

function ShootObject(model, shape, speed, scale)
    local cam = getCamera()
    local cr = gameObject:GetPlayerControllerComponent()
    local body = cr:getPosition()
    local aim = cam:getFront()

    -- Head origin on the capsule, then offset on XZ by look yaw (camera front flat).
    local yawRad = math.rad(orbitYaw)
    local fwdX = math.cos(yawRad)
    local fwdZ = math.sin(yawRad)
    local spawnPos = vec3(
        body.x + fwdX * variables.SHOOT_FORWARD_OFFSET,
        body.y + variables.SHOOT_HEAD_HEIGHT,
        body.z + fwdZ * variables.SHOOT_FORWARD_OFFSET
    )

    local newBall = createEntity("Ball" .. ballCount)
    ballCount = ballCount + 1

    local tr = newBall:AddTransform()
    local mr = newBall:AddModelRenderer()
    mr:setMaterial(variables.BULLET_MATERIAL)

    local rb = newBall:AddRigidBodyComponent()
    local sc = newBall:AddLuaScript()
    newBall:AddShadowCaster()
    newBall:setParent(variables.BULLET_PARENT)

    tr.scale = vec3(scale, scale, scale)
    mr:setModel(model)

    rb:setPosition(spawnPos)
    -- Fly along full camera aim (includes pitch) so shots still go where you look
    rb:addLinearVelocity(vec3(aim.x * speed, aim.y * speed, aim.z * speed))

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
    end
    sc:setScript(newBall, "scripts/bullet.lua")
end

function Update()
    local input = getInput()
    local camera = getCamera()
    local cr = gameObject:GetPlayerControllerComponent()
    input:setCursorMode(CURSOR_DISABLED)

    -- Orbit look (independent of character facing)
    local mouseDelta = input:getMouseDelta()
    orbitYaw   = orbitYaw + mouseDelta.x * variables.MOUSE_SENSITIVITY
    orbitPitch = orbitPitch + mouseDelta.y * variables.MOUSE_SENSITIVITY
    orbitPitch = clamp(orbitPitch, variables.CAMERA_PITCH_MIN, variables.CAMERA_PITCH_MAX)

    -- Drive engine camera yaw/pitch so getFront() matches the orbit
    camera.yaw   = orbitYaw
    camera.pitch = orbitPitch

    -- Zoom
    if input.isMouseScrolled then
        -- optional if bound later
    end

    -- Camera-relative movement on XZ (matches Camera::UpdateCameraVectors yaw:
    -- front.xz = (cos yaw, sin yaw), right = front × up = (-sin yaw, cos yaw))
    local yawRad = math.rad(orbitYaw)
    local camForwardX = math.cos(yawRad)
    local camForwardZ = math.sin(yawRad)
    local camRightX   = -math.sin(yawRad)
    local camRightZ   =  math.cos(yawRad)

    local xAxis = 0.0
    local yAxis = 0.0
    if input:isKeyPressed(KEY_W) then yAxis = yAxis + 1.0 end
    if input:isKeyPressed(KEY_S) then yAxis = yAxis - 1.0 end
    if input:isKeyPressed(KEY_A) then xAxis = xAxis - 1.0 end
    if input:isKeyPressed(KEY_D) then xAxis = xAxis + 1.0 end

    local moveX = camForwardX * yAxis + camRightX * xAxis
    local moveZ = camForwardZ * yAxis + camRightZ * xAxis
    local moveLen = length2(moveX, moveZ)
    if moveLen > 1e-5 then
        moveX = moveX / moveLen
        moveZ = moveZ / moveLen
    else
        moveX, moveZ = 0.0, 0.0
    end

    local moveSpeed = variables.WALK_SPEED
    if input:isKeyPressed(KEY_LEFT_SHIFT) and cr:isOnGround() and yAxis > 0.0 then
        if xAxis == 0.0 then
            moveSpeed = variables.RUN_SPEED
        else
            moveSpeed = variables.WALK_SPEED + (variables.RUN_SPEED - variables.WALK_SPEED) * 0.5
        end
    end

    local desiredVelocity = vec3(moveX * moveSpeed, 0.0, moveZ * moveSpeed)

    -- Face move direction (smooth). Yaw matches engine camera convention.
    local moving = length2(moveX, moveZ) > 0.1
    if moving then
        local targetFace = math.deg(atan2(moveZ, moveX))
        local maxStep = variables.TURN_SPEED * deltaTime
        faceYaw = lerpAngle(faceYaw, targetFace, maxStep)
        cr:setRotationEuler(vec3(0.0, faceYaw, 0.0))
    end

    -- Vertical velocity / jump / gravity (same model as FPS controller)
    local current_vertical_velocity_mag = cr:getLinearVelocity():dot(vec3(0, 1, 0))
    local current_vertical_velocity = vec3(0, current_vertical_velocity_mag, 0)
    local ground_velocity = cr:getGroundVelocity()
    local new_velocity = vec3(0, 0, 0)
    local moving_towards_ground = (current_vertical_velocity_mag - ground_velocity.y) < 0.1
    local inJump = input:isKeyPressed(KEY_SPACE)

    if cr:isOnGround() and moving_towards_ground then
        new_velocity = ground_velocity
        if inJump then
            new_velocity = vec3(
                new_velocity.x,
                new_velocity.y + variables.JUMP_POWER,
                new_velocity.z
            )
        end
    else
        new_velocity = current_vertical_velocity
    end

    local g = getPhysics():getGravity()
    new_velocity = vec3(
        new_velocity.x + g.x * deltaTime,
        new_velocity.y + g.y * deltaTime * variables.GRAVITY_SCALE,
        new_velocity.z + g.z * deltaTime
    )

    new_velocity = vec3(
        new_velocity.x + desiredVelocity.x,
        new_velocity.y + desiredVelocity.y,
        new_velocity.z + desiredVelocity.z
    )

    cr:setLinearVelocity(new_velocity)

    if input:isKeyPressedThisFrame(KEY_E) then
        ShootObject("resources/models/sphere.obj", SphereShape(0.25), variables.SHOOT_POWER, 0.5)
    end
end

-- After physics so the follow pivot matches the capsule this frame.
function LateUpdate()
    local camera = getCamera()
    local cr = gameObject:GetPlayerControllerComponent()
    local bodyPos = cr:getPosition()

    local pivot = vec3(
        bodyPos.x,
        bodyPos.y + variables.CAMERA_HEIGHT,
        bodyPos.z
    )

    -- Camera module has already rebuilt front from yaw/pitch this frame.
    local front = camera:getFront()
    local dist = orbitDistance or variables.CAMERA_DISTANCE
    local camPos = vec3(
        pivot.x - front.x * dist,
        pivot.y - front.y * dist,
        pivot.z - front.z * dist
    )

    camera:setPosition(camPos)
end
