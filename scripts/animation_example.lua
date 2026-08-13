-- Example: Godot/Unity-style AnimationComponent control.
-- Attach to an entity with AnimationComponent + SkinnedMeshComponent
-- (e.g. scene "AnimatedEntity").
--
-- Controls (game PLAYING):
--   1              idle
--   2              walk
--   3              run
--   4              jump (one-shot)
--   Left Shift     hold to run (from walk)
--   [ / ]          slower / faster clip speed
--   K              toggle skeleton debug draw

variables = {
    -- AnimationHandle assets — assign clips in the inspector (or scene JSON).
    idleAnim = animation(),
    walkAnim = animation(),
    runAnim  = animation(),
    jumpAnim = animation(),
    fade     = 0.25,
}

local currentClip = nil

local function hasAnim()
    return gameObject:HasAnimationComponent()
end

local function anim()
    return gameObject:GetAnimationComponent()
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

local function playClip(clip, fade, loop)
    if not hasAnim() then
        return
    end
    if clip == nil or (clip.isValid and not clip:isValid()) then
        return
    end
    if sameClip(currentClip, clip) then
        return
    end
    currentClip = clip
    local f = fade
    if f == nil then
        f = variables.fade
    end
    local shouldLoop = true
    if loop ~= nil then
        shouldLoop = loop
    end
    anim():play(clip, shouldLoop, f)
    if clip.getGuid then
        info("play -> " .. clip:getGuid())
    end
end

function Start()
    if not hasAnim() then
        info("animation_example: need AnimationComponent on this entity")
        return
    end

    local ac = anim()
    if not ac:hasSkeleton() then
        -- Prefer loadSkeleton so the asset is registered with a GUID.
        ac:setSkeleton(loadSkeleton("resources/animations/skeleton.ozz"))
    end

    ac.defaultFadeDuration = variables.fade
    playClip(variables.walkAnim, 0.0)

    info(string.format(
        "animation ready (joints=%d, length=%.2f)",
        ac:jointCount(),
        ac:getLength()
    ))
end

function Update()
    if not hasAnim() then
        return
    end

    local input = getInput()
    local ac = anim()
    local am = getAnimationManager()

    if input:isKeyPressedThisFrame(KEY_1) then
        playClip(variables.idleAnim)
    elseif input:isKeyPressedThisFrame(KEY_2) then
        playClip(variables.walkAnim)
    elseif input:isKeyPressedThisFrame(KEY_3) then
        playClip(variables.runAnim)
    elseif input:isKeyPressedThisFrame(KEY_4) then
        playClip(variables.jumpAnim, variables.fade, false)
    end

    if not sameClip(currentClip, variables.idleAnim) then
        if input:isKeyPressed(KEY_LEFT_SHIFT) then
            playClip(variables.runAnim)
        elseif sameClip(currentClip, variables.runAnim) and not input:isKeyPressed(KEY_3) then
            playClip(variables.walkAnim)
        end
    end

    if input:isKeyPressedThisFrame(KEY_LEFT_BRACKET) then
        ac.speed = math.max(0.1, ac.speed - 0.25)
        info(string.format("speed=%.2f", ac.speed))
    elseif input:isKeyPressedThisFrame(KEY_RIGHT_BRACKET) then
        ac.speed = math.min(3.0, ac.speed + 0.25)
        info(string.format("speed=%.2f", ac.speed))
    end

    if input:isKeyPressedThisFrame(KEY_K) then
        am.drawSkeleton = not am.drawSkeleton
        info("drawSkeleton=" .. tostring(am.drawSkeleton))
    end
end
