-- Third-person character controller + orbit camera + locomotion clips.
-- Uses Jolt CharacterVirtual capsule + AnimationComponent / SkinnedMesh
-- (same skeleton/mesh as AnimatedEntity).
--
-- Controls:
--   WASD           move (relative to camera yaw)
--   Left Shift     run
--   Space          jump
--   Mouse          orbit camera
--   E              shoot

variables = {
    -- Camera orbit
    CAMERA_DISTANCE     = 5.0,
    CAMERA_MIN_DISTANCE = 2.0,
    CAMERA_MAX_DISTANCE = 12.0,
    CAMERA_HEIGHT       = 1.2,
    CAMERA_PITCH_MIN    = -30.0,
    CAMERA_PITCH_MAX    = 60.0,
    MOUSE_SENSITIVITY   = 0.12,
    ZOOM_SENSITIVITY    = 0.5,

    -- Projectiles from capsule head along look yaw
    SHOOT_HEAD_HEIGHT    = 1.1,
    SHOOT_FORWARD_OFFSET = 1.0,

    -- Movement
    WALK_SPEED    = 7.0,
    RUN_SPEED     = 9.0,   -- sprint (any direction); kept close to walk
    JUMP_POWER    = 8.0,
    GRAVITY_SCALE = 2.0,
    TURN_SPEED    = 540.0, -- deg/sec when lerping face toward move dir

    -- Locomotion clips (same set as AnimatedEntity / animation_example)
    IDLE_ANIM = "resources/animations/idle.ozz",
    WALK_ANIM = "resources/animations/walk_inplace.anim",
    RUN_ANIM  = "resources/animations/run_inplace.anim",
    ANIM_FADE = 0.2,
    SKELETON  = "resources/animations/skeleton.ozz",

    SHOOT_POWER     = 15,
    BULLET_PARENT   = ehandle(),
    BULLET_MATERIAL = material(),
    shootSound      = sound(),
    victorySound    = sound(),
}

local orbitYaw      = -90.0
local orbitPitch    = 15.0
local orbitDistance = nil
local faceYaw       = 0.0
local currentClip   = nil
local wasMoving     = false
local wasRunning    = false
local wasGrounded   = true
local visualEntity  = nil -- child with AnimationComponent + SkinnedMesh

local function clamp(v, lo, hi)
    if v < lo then return lo end
    if v > hi then return hi end
    return v
end

local function atan2(y, x)
    if math.atan2 then
        return math.atan2(y, x)
    end
    return math.atan(y, x)
end

local function length2(x, z)
    return math.sqrt(x * x + z * z)
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

-- Visual lives on a child so it can sit at capsule feet (physics is at center).
local function resolveVisual()
    if visualEntity and visualEntity:isValid() and visualEntity:HasAnimationComponent() then
        return visualEntity
    end
    visualEntity = nil
    local kids = gameObject:getChildren()
    if kids then
        for _, h in pairs(kids) do
            local e = getEntityFromHandle(h)
            if e and e:isValid() and e:HasAnimationComponent() then
                visualEntity = e
                return visualEntity
            end
        end
    end
    return nil
end

local function hasAnim()
    local v = resolveVisual()
    return v ~= nil
end

local function anim()
    local v = resolveVisual()
    if not v then
        return nil
    end
    return v:GetAnimationComponent()
end

local function playClip(path, fade)
    local ac = anim()
    if not ac then
        return
    end
    if currentClip == path then
        return
    end
    currentClip = path
    local f = fade
    if f == nil then
        f = variables.ANIM_FADE
    end
    ac:play(path, true, f)
end

local function updateLocomotionAnim(moving, running, grounded)
    if not hasAnim() then
        return
    end

    -- Airborne: keep last locomotion clip (or idle if never moved)
    if not grounded then
        if wasGrounded then
            -- left ground; leave current clip playing
        end
        wasGrounded = false
        wasMoving = moving
        wasRunning = running
        return
    end

    local target
    if not moving then
        target = variables.IDLE_ANIM
    elseif running then
        target = variables.RUN_ANIM
    else
        target = variables.WALK_ANIM
    end

    -- Only crossfade when state changes
    if target ~= currentClip then
        playClip(target, variables.ANIM_FADE)
    end

    wasGrounded = true
    wasMoving = moving
    wasRunning = running
end

function Start()
    orbitDistance = variables.CAMERA_DISTANCE
    local camera = getCamera()
    orbitYaw   = camera.yaw
    orbitPitch = clamp(camera.pitch, variables.CAMERA_PITCH_MIN, variables.CAMERA_PITCH_MAX)

    local ac = anim()
    if ac then
        if ac.skeletonPath == nil or ac.skeletonPath == "" then
            ac:setSkeleton(variables.SKELETON)
        end
        ac.defaultFadeDuration = variables.ANIM_FADE
        playClip(variables.IDLE_ANIM, 0.0)
        info(string.format("player visual ready (joints=%d)", ac:jointCount()))
    else
        info("player_thirdperson: no PlayerVisual child with AnimationComponent")
    end

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
    publish("AllTargetsDestroyed")
    local cam = getCamera()
    local cr = gameObject:GetPlayerControllerComponent()
    local body = cr:getPosition()
    local aim = cam:getFront()

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

    local mouseDelta = input:getMouseDelta()
    orbitYaw   = orbitYaw + mouseDelta.x * variables.MOUSE_SENSITIVITY
    orbitPitch = orbitPitch + mouseDelta.y * variables.MOUSE_SENSITIVITY
    orbitPitch = clamp(orbitPitch, variables.CAMERA_PITCH_MIN, variables.CAMERA_PITCH_MAX)

    camera.yaw   = orbitYaw
    camera.pitch = orbitPitch

    -- Same camera-relative basis as scripts/player.lua (FPS):
    --   front.xz = (cos yaw, sin yaw)   -- Camera::UpdateCameraVectors
    --   right    = cross(front, up).xz = (-sin yaw, cos yaw)
    --   move     = front * forwardAxis + right * strafeAxis
    local yawRad = math.rad(orbitYaw)
    local camForwardX = math.cos(yawRad)
    local camForwardZ = math.sin(yawRad)
    local camRightX   = -math.sin(yawRad)
    local camRightZ   =  math.cos(yawRad)

    local xAxis = 0.0 -- A/D strafe
    local yAxis = 0.0 -- W/S forward
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

    local moving = moveLen > 0.1
    -- Sprint in any move direction (forward, strafe, diagonal, back)
    local wantRun = input:isKeyPressed(KEY_LEFT_SHIFT) and cr:isOnGround() and moving
    local moveSpeed = wantRun and variables.RUN_SPEED or variables.WALK_SPEED
    local running = wantRun

    local desiredVelocity = vec3(moveX * moveSpeed, 0.0, moveZ * moveSpeed)

    if moving then
        -- Lerp yaw toward move direction (mesh +Z = (sin yaw, cos yaw)).
        local targetFace = math.deg(atan2(moveX, moveZ))
        local maxStep = variables.TURN_SPEED * deltaTime
        faceYaw = lerpAngle(faceYaw, targetFace, maxStep)
        local faceRad = math.rad(faceYaw)
        cr:setFacingDirection(vec3(math.sin(faceRad), 0.0, math.cos(faceRad)))
    end

    local current_vertical_velocity_mag = cr:getLinearVelocity():dot(vec3(0, 1, 0))
    local current_vertical_velocity = vec3(0, current_vertical_velocity_mag, 0)
    local ground_velocity = cr:getGroundVelocity()
    local new_velocity = vec3(0, 0, 0)
    local moving_towards_ground = (current_vertical_velocity_mag - ground_velocity.y) < 0.1
    local inJump = input:isKeyPressed(KEY_SPACE)
    local grounded = cr:isOnGround() and moving_towards_ground

    if grounded then
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

    updateLocomotionAnim(moving, running, cr:isOnGround())

    if input:isKeyPressedThisFrame(KEY_E) then
        ShootObject("resources/models/sphere.obj", SphereShape(0.25), variables.SHOOT_POWER, 0.5)
    end
end

function LateUpdate()
    local camera = getCamera()
    local cr = gameObject:GetPlayerControllerComponent()
    local bodyPos = cr:getPosition()

    local pivot = vec3(
        bodyPos.x,
        bodyPos.y + variables.CAMERA_HEIGHT,
        bodyPos.z
    )

    local front = camera:getFront()
    local dist = orbitDistance or variables.CAMERA_DISTANCE
    camera:setPosition(vec3(
        pivot.x - front.x * dist,
        pivot.y - front.y * dist,
        pivot.z - front.z * dist
    ))
end
