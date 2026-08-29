-- Third-person character controller + orbit camera + locomotion clips.
-- Uses Jolt CharacterVirtual capsule + AnimationComponent / SkinnedMesh
-- (same skeleton/mesh as AnimatedEntity).
--
-- Controls:
--   WASD / left stick / d-pad   move (camera-relative; Horizontal/Vertical axes)
--   Left Shift / LB             run
--   Space / A                   jump (or jump-off wall while climbing)
--   Mouse / right stick         orbit camera
--   E / X / RB                  shoot PROJECTILE_PREFAB
--   Hold into a steep wall      BOTW-style climb attach (physics)

variables = {
    -- Camera orbit
    CAMERA_DISTANCE          = 5.0,
    CAMERA_MIN_DISTANCE      = 2.0,
    CAMERA_MAX_DISTANCE      = 12.0,
    CAMERA_HEIGHT            = 1.2,
    CAMERA_PITCH_MIN         = -60.0, -- more negative = look down on player more
    CAMERA_PITCH_MAX         = 60.0,
    -- Pull camera slightly off the hit surface so the near plane doesn't clip
    CAMERA_COLLISION_OFFSET   = 0.2,
    CAMERA_COLLISION_MIN_DIST = 0.25,
    -- Snap in on collision; ease out when space opens up (units/sec)
    CAMERA_ZOOM_OUT_SPEED     = 4.0,
    MOUSE_SENSITIVITY         = 0.12,
    -- Right-stick look (degrees per frame at 60 FPS, full deflection).
    -- Scaled by deltaTime*60 so rotation speed is the same at any framerate.
    GAMEPAD_LOOK_SENSITIVITY  = 2.5,
    ZOOM_SENSITIVITY          = 0.5,

    -- Projectiles from capsule head along look yaw
    SHOOT_HEAD_HEIGHT    = 1.1,
    SHOOT_FORWARD_OFFSET = 1.0,

    -- Movement
    WALK_SPEED    = 7.0,
    RUN_SPEED     = 9.0,   -- sprint (any direction); kept close to walk
    JUMP_POWER    = 8.0,
    GRAVITY_SCALE = 2.0,
    TURN_SPEED    = 540.0, -- deg/sec when lerping face toward move dir

    -- Climbing (physics only; no climb anims yet)
    CLIMB_SPEED           = 3.5,  -- units/sec along wall
    CLIMB_PROBE_DIST      = 1.15, -- how far ahead to search for a wall
    CLIMB_ATTACH_DIST     = 0.95, -- must be this close to attach
    CLIMB_STICK_SKIN      = 0.06, -- gap between capsule and wall while stuck
    CLIMB_INTO_DOT        = 0.25, -- move must push into wall this much to grab
    CLIMB_LOST_MAX        = 8,    -- frames without wall before detach
    CLIMB_JUMP_PUSH       = 7.0,  -- push off along wall normal
    CLIMB_JUMP_UP         = 5.0,  -- upward boost when jump-off
    CLIMB_TOP_PROBE       = 0.85, -- ledge top-out checks
    CLIMB_MANTLE_FORWARD  = 0.55, -- how far past the lip to search for floor
    CLIMB_MANTLE_DOWN     = 1.6,  -- down-ray length for ledge floor
    CLIMB_MANTLE_MIN_NY   = 0.65, -- floor must be this flat to stand on
    CLIMB_MANTLE_DURATION = 0.55, -- seconds to pull up/over (no teleport snap)
    CLIMB_MANTLE_ARC      = 0.2,  -- extra height at mid of pull-up arc
    CLIMB_MIN_NORMAL_Y    = -0.25,-- allow mild overhang
    CLIMB_MAX_NORMAL_Y    = 0.55, -- reject walkable floors / shallow slopes


    -- Locomotion clips — AnimationHandle assets (assign in inspector / scene).
    IDLE_ANIM      = animation(),
    WALK_ANIM      = animation(),
    RUN_ANIM       = animation(),
    FALL_IDLE_ANIM = animation(), -- loop while airborne
    FALL_LAND_ANIM = animation(), -- one-shot on touchdown
    -- Climbing
    CLIMB_UP_ANIM   = animation(),
    CLIMB_DOWN_ANIM = animation(),
    ANIM_FADE      = 0.2,
    -- SkeletonReference asset — assign in inspector / scene JSON.
    SKELETON       = skeleton(),

    SHOOT_POWER       = 15,
    PROJECTILE_PREFAB = prefab(), -- assign a .prefab (e.g. a cube with prefab_created.lua)
    BULLET_PARENT     = ehandle(),
    shootSound        = sound(),
    victorySound      = sound(),
}

local orbitYaw         = -90.0
local orbitPitch       = 15.0
local orbitDistance    = nil
local currentCameraDist = nil -- actual distance after collision (snaps in, eases out)
local faceYaw          = 0.0
local currentClip      = nil
local wasMoving        = false
local wasRunning       = false
local wasGrounded      = true
local visualEntity     = nil -- child with AnimationComponent + SkinnedMesh
local climbLostFrames  = 0
local climbNormal      = nil -- last wall normal while climbing (outward)
local mantle           = nil -- active pull-up: eased path onto ledge

local function clamp(v, lo, hi)
    if v < lo then return lo end
    if v > hi then return hi end
    return v
end

local function atan2(y, x)
    --if math.atan2 then
    --    return math.atan2(y, x)
    --end
    return math.atan(y, x)
end

local function length2(x, z)
    return math.sqrt(x * x + z * z)
end

local function length3(v)
    return math.sqrt(v.x * v.x + v.y * v.y + v.z * v.z)
end

local function normalize3(x, y, z)
    local len = math.sqrt(x * x + y * y + z * z)
    if len < 1e-6 then
        return 0.0, 0.0, 0.0, 0.0
    end
    return x / len, y / len, z / len, len
end

local function dot3(ax, ay, az, bx, by, bz)
    return ax * bx + ay * by + az * bz
end

-- Wall basis: right along wall, up along wall (world-up projected), outward normal.
local function wallBasis(nx, ny, nz)
    -- wallRight = normalize(cross(worldUp, normal)) = (nz, 0, -nx)
    local rx, ry, rz, rlen = normalize3(nz, 0.0, -nx)
    if rlen < 1e-5 then
        -- Normal almost vertical: pick arbitrary horizontal right
        rx, ry, rz = 1.0, 0.0, 0.0
    end
    -- wallUp = normalize(cross(normal, wallRight))
    local ux, uy, uz = normalize3(
        ny * rz - nz * ry,
        nz * rx - nx * rz,
        nx * ry - ny * rx
    )
    return rx, ry, rz, ux, uy, uz
end

local function tryClimbProbe(cr, dirX, dirY, dirZ, maxDist)
    local minNy = variables.CLIMB_MIN_NORMAL_Y or -0.25
    local maxNy = variables.CLIMB_MAX_NORMAL_Y or 0.55
    return cr:probeClimbSurface(vec3(dirX, dirY, dirZ), maxDist, minNy, maxNy)
end

local function stickToWall(cr, nx, ny, nz)
    local radius = cr:getCapsuleRadius()
    local skin = variables.CLIMB_STICK_SKIN or 0.06
    local desired = radius + skin
    local body = cr:getPosition()
    -- Re-probe into the wall from body center
    local probeDist = desired + 0.75
    if not tryClimbProbe(cr, -nx, -ny, -nz, probeDist) then
        return false
    end
    local n = cr:getClimbNormal()
    local p = cr:getClimbPoint()
    local toBodyX = body.x - p.x
    local toBodyY = body.y - p.y
    local toBodyZ = body.z - p.z
    local dist = dot3(toBodyX, toBodyY, toBodyZ, n.x, n.y, n.z)
    local err = dist - desired
    -- Snap mostly onto the surface (keep contact without burying capsule)
    cr:setPosition(vec3(
        body.x - n.x * err,
        body.y - n.y * err,
        body.z - n.z * err
    ))
    climbNormal = n
    return true
end

-- Visual lives on a child so it can sit at capsule feet (physics is at center).
-- Defined early: climb/mantle helpers need these before locomotion code below.
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
    return resolveVisual() ~= nil
end

local function anim()
    local v = resolveVisual()
    if not v then
        return nil
    end
    return v:GetAnimationComponent()
end

local function sameClip(a, b)
    if a == nil or b == nil then
        return false
    end
    if not a.isValid or not b.isValid then
        return a == b
    end
    if not a:isValid() or not b:isValid() then
        return false
    end
    return a:getGuid() == b:getGuid()
end

local function playClip(clip, fade, loop, force)
    local ac = anim()
    if not ac then
        return
    end
    if clip == nil or (clip.isValid and not clip:isValid()) then
        return
    end
    if not force and sameClip(currentClip, clip) then
        return
    end
    currentClip = clip
    local f = fade
    if f == nil then
        f = variables.ANIM_FADE
    end
    local shouldLoop = true
    if loop ~= nil then
        shouldLoop = loop
    end
    ac:play(clip, shouldLoop, f)
end

local function setClipSpeed(speed)
    local ac = anim()
    if ac then
        ac.speed = speed
    end
end

-- Mixamo climb clips often bake a large hips/root Y offset (character floats).
local function setCounteractRoot(enabled)
    local ac = anim()
    if ac then
        ac.counteractRootOffset = enabled and true or false
    end
end

local function beginClimb(cr, n)
    climbNormal = n
    climbLostFrames = 0
    mantle = nil
    cr:setClimbing(true)
    cr:setLinearVelocity(vec3(0, 0, 0))
    -- Face into the wall (away from outward normal on XZ)
    cr:setFacingDirection(vec3(-n.x, 0.0, -n.z))
end

local function endClimb(cr)
    cr:setClimbing(false)
    climbNormal = nil
    climbLostFrames = 0
    setCounteractRoot(false)
    setClipSpeed(1.0)
end

local function smoothstep(u)
    u = clamp(u, 0.0, 1.0)
    return u * u * (3.0 - 2.0 * u)
end

local function isFiniteNum(v)
    return v == v and v > -1e8 and v < 1e8
end

-- Drive active pull-up. Returns true while mantling (caller should skip climb move).
local function updateMantle(cr, dt)
    if mantle == nil then
        return false
    end

    -- Snapshot targets first so we never touch nil after clear
    local fromX, fromY, fromZ = mantle.fromX, mantle.fromY, mantle.fromZ
    local midX, midY, midZ = mantle.midX, mantle.midY, mantle.midZ
    local toX, toY, toZ = mantle.toX, mantle.toY, mantle.toZ
    local fx, fz = mantle.fx, mantle.fz
    local dur = mantle.duration
    if dur == nil or dur < 0.05 then
        dur = 0.05
    end

    if not (isFiniteNum(fromX) and isFiniteNum(fromY) and isFiniteNum(fromZ)
        and isFiniteNum(midX) and isFiniteNum(midY) and isFiniteNum(midZ)
        and isFiniteNum(toX) and isFiniteNum(toY) and isFiniteNum(toZ)
        and isFiniteNum(fx) and isFiniteNum(fz)) then
        mantle = nil
        endClimb(cr)
        return false
    end

    mantle.t = (mantle.t or 0.0) + (dt or 0.0)
    local u = clamp(mantle.t / dur, 0.0, 1.0)
    local s = smoothstep(u)

    -- Quadratic Bezier: climb pose → high mid (clear lip) → stand on ledge
    local omt = 1.0 - s
    local x = omt * omt * fromX + 2.0 * omt * s * midX + s * s * toX
    local y = omt * omt * fromY + 2.0 * omt * s * midY + s * s * toY
    local z = omt * omt * fromZ + 2.0 * omt * s * midZ + s * s * toZ

    if not (isFiniteNum(x) and isFiniteNum(y) and isFiniteNum(z)) then
        mantle = nil
        endClimb(cr)
        return false
    end

    cr:setClimbing(true) -- keep zero gravity during pull-up
    cr:setLinearVelocity(vec3(0, 0, 0))
    cr:setPosition(vec3(x, y, z))
    if (fx * fx + fz * fz) > 1e-8 then
        cr:setFacingDirection(vec3(fx, 0.0, fz))
    end

    if u >= 1.0 then
        mantle = nil
        endClimb(cr)
        cr:setPosition(vec3(toX, toY, toZ))
        cr:setLinearVelocity(vec3(fx * 1.5, -0.35, fz * 1.5))
        faceYaw = math.deg(atan2(fx, fz))
        return false -- finished this frame; allow normal loco next
    end
    return true
end

-- Find a ledge and start a timed pull-up (no instant teleport).
-- Works even when the wall ray no longer hits (head above the ledge).
local function tryMantle(cr, n)
    if n == nil or mantle ~= nil then
        return false
    end

    local body = cr:getPosition()
    local radius = cr:getCapsuleRadius()
    local halfH = cr:getCapsuleHalfHeight()
    local fwdAmt = variables.CLIMB_MANTLE_FORWARD or 0.55
    local downLen = variables.CLIMB_MANTLE_DOWN or 1.6
    local minNy = variables.CLIMB_MANTLE_MIN_NY or 0.65

    -- Horizontal into-wall direction (toward / over the lip)
    local fx, fy, fz, flen = normalize3(-n.x, 0.0, -n.z)
    if flen < 1e-5 then
        fx, fy, fz = normalize3(-n.x, -n.y, -n.z)
    end

    local standClear = halfH + radius + 0.04
    local heights = {
        0.15,
        0.45,
        0.75,
        halfH + 0.1,
        halfH + radius * 0.5,
        halfH + radius + 0.15,
        halfH + radius + 0.45,
    }

    local best = nil
    for i = 1, #heights do
        local h = heights[i]
        local origin = vec3(body.x, body.y + h, body.z)

        local block = getPhysics():raycast(origin, vec3(fx, 0.0, fz), radius + 0.2)
        if not (block and block.distance < radius + 0.12) then
            local over = vec3(
                body.x + fx * (radius + fwdAmt),
                body.y + h + 0.25,
                body.z + fz * (radius + fwdAmt)
            )
            local ground = getPhysics():raycast(over, vec3(0, -1, 0), downLen)
            if ground and ground.normal and ground.normal.y >= minNy then
                if best == nil or ground.point.y > best.gy then
                    best = {
                        x = ground.point.x + fx * 0.2,
                        y = ground.point.y + standClear,
                        z = ground.point.z + fz * 0.2,
                        gy = ground.point.y,
                    }
                end
            end
        end
    end

    if best == nil then
        return false
    end

    local feetY = body.y - halfH - radius
    if best.gy + 0.05 < feetY - 0.35 then
        return false
    end

    local arc = variables.CLIMB_MANTLE_ARC or 0.2
    local dur = variables.CLIMB_MANTLE_DURATION or 0.55
    -- Mid control point: up to clear the lip, then halfway over
    local midY = math.max(body.y, best.y) + arc
    if midY < best.y + 0.05 then
        midY = best.y + 0.05
    end

    mantle = {
        t = 0.0,
        duration = dur,
        fromX = body.x,
        fromY = body.y,
        fromZ = body.z,
        midX = body.x * 0.35 + best.x * 0.65,
        midY = midY,
        midZ = body.z * 0.35 + best.z * 0.65,
        toX = best.x,
        toY = best.y,
        toZ = best.z,
        fx = fx,
        fz = fz,
    }
    climbLostFrames = 0
    cr:setClimbing(true)
    cr:setLinearVelocity(vec3(0, 0, 0))
    cr:setFacingDirection(vec3(fx, 0.0, fz))
    return true
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

local function locomotionTarget(moving, running)
    if not moving then
        return variables.IDLE_ANIM
    elseif running then
        return variables.RUN_ANIM
    end
    return variables.WALK_ANIM
end

-- Climb vertical intent: +1 up wall, -1 down wall, 0 hang.
-- Prefer stick/camera vertical (yAxis); falls back to wall-up velocity if provided.
local function updateClimbAnim(climbVert)
    if not hasAnim() then
        return
    end
    local upClip = variables.CLIMB_UP_ANIM
    local downClip = variables.CLIMB_DOWN_ANIM
    local thr = 0.2

    -- Always counteract root on climb clips (up and down both offset on Mixamo).
    setCounteractRoot(true)

    if climbVert > thr then
        playClip(upClip, variables.ANIM_FADE, true)
        setClipSpeed(1.0)
    elseif climbVert < -thr then
        playClip(downClip, variables.ANIM_FADE, true)
        setClipSpeed(1.0)
    else
        -- Hang on wall: hold climb_up pose (paused)
        playClip(upClip, variables.ANIM_FADE, true)
        setClipSpeed(0.0)
    end
end

local function isLandClipFinished()
    local ac = anim()
    if not ac then
        return true
    end
    local len = ac:getLength()
    if len <= 0.0 then
        return true
    end
    -- Start blending into locomotion slightly before the clip ends
    local fade = variables.ANIM_FADE or 0.2
    return ac:getTime() >= math.max(0.0, len - fade)
end

local function updateLocomotionAnim(moving, running, grounded)
    if not hasAnim() then
        return
    end

    -- Leaving climb: restore normal playback speed + keep root offsets for loco clips
    setClipSpeed(1.0)
    setCounteractRoot(false)

    local fallIdle = variables.FALL_IDLE_ANIM
    local fallLand = variables.FALL_LAND_ANIM

    -- Airborne: loop fall pose (covers jump + walk-off)
    if not grounded then
        if currentClip ~= fallIdle then
            playClip(fallIdle, variables.ANIM_FADE, true)
        end
        wasGrounded = false
        wasMoving = moving
        wasRunning = running
        return
    end

    -- Touchdown: plant if standing still; if already moving, go straight to walk/run
    -- so the land pose doesn't lock and slide under the character.
    if not wasGrounded then
        if moving then
            playClip(locomotionTarget(moving, running), variables.ANIM_FADE, true)
        else
            playClip(fallLand, variables.ANIM_FADE * 0.5, false, true)
        end
        wasGrounded = true
        wasMoving = moving
        wasRunning = running
        return
    end

    -- Hold land only while idle. Moving cancels land immediately (no foot slide).
    if currentClip == fallLand and not isLandClipFinished() and not moving then
        wasGrounded = true
        wasMoving = moving
        wasRunning = running
        return
    end

    local target = locomotionTarget(moving, running)
    if target ~= currentClip then
        playClip(target, variables.ANIM_FADE, true)
    end

    wasGrounded = true
    wasMoving = moving
    wasRunning = running
end

function Start()
    orbitDistance = variables.CAMERA_DISTANCE
    currentCameraDist = orbitDistance
    local camera = getCamera()
    orbitYaw   = camera.yaw
    orbitPitch = clamp(camera.pitch, variables.CAMERA_PITCH_MIN, variables.CAMERA_PITCH_MAX)

    local ac = anim()
    if ac then
        if variables.SKELETON and variables.SKELETON.isValid and variables.SKELETON:isValid() then
            if not ac:hasSkeleton() then
                ac:setSkeleton(variables.SKELETON)
            end
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

local function offsetHierarchy(entity, dx, dy, dz, vel)
    if entity:HasRigidBodyComponent() then
        local rb = entity:GetRigidBodyComponent()
        local p = rb:getPosition()
        rb:setPosition(vec3(p.x + dx, p.y + dy, p.z + dz))
        if vel then
            rb:addLinearVelocity(vel)
        end
    elseif entity:HasTransform() then
        local tr = entity:GetTransform()
        local p = tr.position
        tr.position = vec3(p.x + dx, p.y + dy, p.z + dz)
    end
    local kids = entity:getChildren()
    if not kids then
        return
    end
    for i = 1, kids:size() do
        local child = getEntityFromHandle(kids[i])
        if child and child:isValid() then
            offsetHierarchy(child, dx, dy, dz, vel)
        end
    end
end

function ShootPrefab()
    local prefab = variables.PROJECTILE_PREFAB
    if not prefab or not prefab:isValid() then
        print("[player] assign PROJECTILE_PREFAB on the third-person controller")
        return
    end

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

    local root = instantiatePrefab(prefab)
    if not root or not root:isValid() then
        print("[player] instantiatePrefab failed")
        return
    end

    if variables.BULLET_PARENT and variables.BULLET_PARENT:isValid() then
        root:setParent(variables.BULLET_PARENT)
    end

    local origin = spawnPos
    if root:HasTransform() then
        origin = root:GetTransform().position
    end
    local vel = vec3(aim.x * variables.SHOOT_POWER, aim.y * variables.SHOOT_POWER, aim.z * variables.SHOOT_POWER)
    offsetHierarchy(root, spawnPos.x - origin.x, spawnPos.y - origin.y, spawnPos.z - origin.z, vel)
end

function Update()
    local input = getInput()
    local camera = getCamera()
    local cr = gameObject:GetPlayerControllerComponent()
    input:setCursorMode(CURSOR_DISABLED)

    -- Look: mouse + right stick (Look Horizontal / Look Vertical axes)
    -- Mouse delta is pixels this frame (already frame-rate independent).
    -- Stick is continuous [-1,1], so scale by dt*60 to match "per frame @ 60fps" sens.
    local mouseDelta = input:getMouseDelta()
    local padLookX = input:getAxisRaw("Look Horizontal")
    local padLookY = input:getAxisRaw("Look Vertical")
    local padSens = (variables.GAMEPAD_LOOK_SENSITIVITY or 2.5) * deltaTime * 60.0
    orbitYaw   = orbitYaw + mouseDelta.x * variables.MOUSE_SENSITIVITY + padLookX * padSens
    orbitPitch = orbitPitch + mouseDelta.y * variables.MOUSE_SENSITIVITY + padLookY * padSens
    orbitPitch = clamp(orbitPitch, variables.CAMERA_PITCH_MIN, variables.CAMERA_PITCH_MAX)

    camera.yaw   = orbitYaw
    camera.pitch = orbitPitch

    -- Same camera-relative basis as scripts/player.lua (FPS):
    --   front.xz = (cos yaw, sin yaw)   -- Camera::UpdateCameraVectors
    --   right    = cross(front, up).xz = (-sin yaw, cos yaw)
    --   move     = front * Vertical + right * Horizontal
    local yawRad = math.rad(orbitYaw)
    local camForwardX = math.cos(yawRad)
    local camForwardZ = math.sin(yawRad)
    local camRightX   = -math.sin(yawRad)
    local camRightZ   =  math.cos(yawRad)

    -- Unity-style axes: keyboard WASD/arrows + left stick + d-pad
    local xAxis = input:getAxisRaw("Horizontal") -- strafe
    local yAxis = input:getAxisRaw("Vertical")   -- forward / climb up

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
    local inJumpPressed = input:isKeyPressedThisFrame(KEY_SPACE)
        or input:isGamepadButtonPressedThisFrame(GAMEPAD_A)
    local inJumpHeld = input:isKeyPressed(KEY_SPACE) or input:isGamepadButtonPressed(GAMEPAD_A)

    local probeDist = variables.CLIMB_PROBE_DIST or 1.15
    local attachDist = variables.CLIMB_ATTACH_DIST or 0.95
    local intoDot = variables.CLIMB_INTO_DOT or 0.25
    local climbSpeed = variables.CLIMB_SPEED or 3.5

    --------------------------------------------------------------------
    -- CLIMBING (BOTW-style wall stick + surface movement). Physics only.
    --------------------------------------------------------------------
    -- Timed pull-up over the lip (blocks other climb logic while active).
    if updateMantle(cr, deltaTime) then
        updateClimbAnim(1.0) -- pull-up uses climb_up
    elseif cr:isClimbing() then
        local n = climbNormal
        if n == nil and cr:hasClimbSurface() then
            n = cr:getClimbNormal()
            climbNormal = n
        end

        local wantUp = yAxis > 0.25
        -- Climb anim vertical from stick (camera Vertical → wall up/down)
        local climbVert = yAxis

        -- Start mantle while holding up (eased over multiple frames; no teleport).
        if wantUp and n ~= nil and tryMantle(cr, n) then
            updateClimbAnim(1.0)
        elseif inJumpPressed and n ~= nil then
            -- Jump off wall: push along outward normal + up
            local push = variables.CLIMB_JUMP_PUSH or 7.0
            local up = variables.CLIMB_JUMP_UP or 5.0
            mantle = nil
            endClimb(cr)
            cr:setLinearVelocity(vec3(
                n.x * push,
                up,
                n.z * push
            ))
            updateLocomotionAnim(false, false, false)
        else
            -- Maintain / refresh wall contact along last normal (into wall).
            local stillOnWall = false
            if n ~= nil then
                stillOnWall = stickToWall(cr, n.x, n.y, n.z)
                if stillOnWall then
                    n = climbNormal
                end
            end
            -- Also try facing direction if contact lost
            if not stillOnWall then
                local faceX, faceZ = camForwardX, camForwardZ
                if n ~= nil then
                    faceX, faceZ = -n.x, -n.z
                end
                if tryClimbProbe(cr, faceX, 0.0, faceZ, probeDist) then
                    n = cr:getClimbNormal()
                    stillOnWall = stickToWall(cr, n.x, n.y, n.z)
                    n = climbNormal
                end
            end

            if stillOnWall and n ~= nil then
                climbLostFrames = 0

                if wantUp and tryMantle(cr, n) then
                    updateClimbAnim(1.0)
                else
                    local rx, ry, rz, ux, uy, uz = wallBasis(n.x, n.y, n.z)
                    -- Camera-relative on wall: Horizontal → along wallRight, Vertical → along wallUp
                    local climbVelX = (rx * xAxis + ux * yAxis) * climbSpeed
                    local climbVelY = (ry * xAxis + uy * yAxis) * climbSpeed
                    local climbVelZ = (rz * xAxis + uz * yAxis) * climbSpeed
                    -- Small into-wall bias keeps contact without fighting the stick snap
                    local into = 0.35
                    climbVelX = climbVelX - n.x * into
                    climbVelY = climbVelY - n.y * into
                    climbVelZ = climbVelZ - n.z * into
                    cr:setLinearVelocity(vec3(climbVelX, climbVelY, climbVelZ))
                    cr:setFacingDirection(vec3(-n.x, 0.0, -n.z))
                    faceYaw = math.deg(atan2(-n.x, -n.z))
                    updateClimbAnim(climbVert)
                end
            else
                -- Crest: wall gone but still holding up — one more mantle attempt
                if wantUp and n ~= nil and tryMantle(cr, n) then
                    updateClimbAnim(1.0)
                else
                    climbLostFrames = climbLostFrames + 1
                    local lostMax = variables.CLIMB_LOST_MAX or 8
                    if climbLostFrames >= lostMax then
                        endClimb(cr)
                    else
                        -- Brief grace: hold / nudge up if still trying to crest
                        if wantUp and n ~= nil then
                            local _, _, _, ux, uy, uz = wallBasis(n.x, n.y, n.z)
                            cr:setLinearVelocity(vec3(ux * climbSpeed, uy * climbSpeed, uz * climbSpeed))
                            updateClimbAnim(1.0)
                        else
                            cr:setLinearVelocity(vec3(0, 0, 0))
                            updateClimbAnim(0.0)
                        end
                    end
                end
            end
        end

    else
        ----------------------------------------------------------------
        -- NORMAL locomotion + climb attach attempts
        ----------------------------------------------------------------
        local wantRun = (input:isKeyPressed(KEY_LEFT_SHIFT) or input:isGamepadButtonPressed(GAMEPAD_LEFT_BUMPER))
            and cr:isOnGround() and moving
        local moveSpeed = wantRun and variables.RUN_SPEED or variables.WALK_SPEED
        local running = wantRun

        local desiredVelocity = vec3(moveX * moveSpeed, 0.0, moveZ * moveSpeed)

        if moving then
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
        local grounded = cr:isOnGround() and moving_towards_ground

        if grounded then
            new_velocity = ground_velocity
            if inJumpHeld then
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

        -- Climb attach: probe along movement / facing for a steep wall we're pushing into.
        local attach = false
        local probeDirX, probeDirZ = moveX, moveZ
        if moveLen < 0.1 then
            local faceRad = math.rad(faceYaw)
            probeDirX = math.sin(faceRad)
            probeDirZ = math.cos(faceRad)
        end
        if tryClimbProbe(cr, probeDirX, 0.0, probeDirZ, probeDist) then
            local n = cr:getClimbNormal()
            local d = cr:getClimbPoint()
            local body = cr:getPosition()
            local toWallX = d.x - body.x
            local toWallY = d.y - body.y
            local toWallZ = d.z - body.z
            local dist = math.sqrt(toWallX * toWallX + toWallY * toWallY + toWallZ * toWallZ)
            -- Into-wall: movement (or facing) toward the wall (against outward normal)
            local into = 0.0
            if moveLen > 0.1 then
                into = dot3(moveX, 0.0, moveZ, -n.x, 0.0, -n.z)
            else
                into = intoDot -- allow grab when jammed against wall with little input
            end
            local closeEnough = dist <= attachDist
            -- Air: easier grab when falling into wall. Ground: need clear push into wall.
            local canGrab = closeEnough and into >= intoDot
            if not grounded and closeEnough and into >= (intoDot * 0.5) then
                canGrab = true
            end
            if canGrab then
                beginClimb(cr, n)
                stickToWall(cr, n.x, n.y, n.z)
                attach = true
            end
        end

        if not attach then
            cr:setLinearVelocity(new_velocity)
            updateLocomotionAnim(moving, running, cr:isOnGround())
        else
            updateClimbAnim(0.0) -- hang pose on attach
        end
    end

    -- Shoot: E or gamepad X / right bumper
    if input:isKeyPressedThisFrame(KEY_E)
        or input:isGamepadButtonPressedThisFrame(GAMEPAD_X)
        or input:isGamepadButtonPressedThisFrame(GAMEPAD_RIGHT_BUMPER) then
        ShootPrefab()
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
    local desiredDist = orbitDistance or variables.CAMERA_DISTANCE
    if currentCameraDist == nil then
        currentCameraDist = desiredDist
    end

    -- Ray from orbit pivot toward the desired camera position (-look direction).
    -- maxAllowed is how far we can be this frame without clipping.
    local maxAllowed = desiredDist
    local toCamera = vec3(-front.x, -front.y, -front.z)
    local hit = getPhysics():raycast(pivot, toCamera, desiredDist)
    if hit then
        local skin = variables.CAMERA_COLLISION_OFFSET or 0.2
        local minD = variables.CAMERA_COLLISION_MIN_DIST or 0.25
        maxAllowed = math.max(hit.distance - skin, minD)
    end

    -- Snap in immediately when geometry is closer; ease out when space opens up.
    if maxAllowed < currentCameraDist then
        currentCameraDist = maxAllowed
    else
        local zoomOutSpeed = variables.CAMERA_ZOOM_OUT_SPEED or 4.0
        local step = zoomOutSpeed * deltaTime
        if currentCameraDist + step < maxAllowed then
            currentCameraDist = currentCameraDist + step
        else
            currentCameraDist = maxAllowed
        end
    end

    local dist = currentCameraDist
    camera:setPosition(vec3(
        pivot.x - front.x * dist,
        pivot.y - front.y * dist,
        pivot.z - front.z * dist
    ))
end
