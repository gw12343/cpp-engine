-- Example: Godot/Unity-style AnimationComponent control.
-- Attach to an entity with AnimationComponent + SkinnedMeshComponent
-- (e.g. scene "AnimatedEntity").
--
-- Controls (game PLAYING):
--   1              idle
--   2              walk
--   3              run
--   Left Shift     hold to run (from walk)
--   [ / ]          slower / faster clip speed
--   K              toggle skeleton debug draw

variables = {
    idleAnim = "resources/animations/idle.ozz",
    walkAnim = "resources/animations/walk_inplace.anim",
    runAnim  = "resources/animations/run_inplace.anim",
    fade     = 0.25,
}

local currentClip = nil

local function hasAnim()
    return gameObject:HasAnimationComponent()
end

local function anim()
    return gameObject:GetAnimationComponent()
end

local function playClip(path, fade)
    if not hasAnim() then
        return
    end
    if currentClip == path then
        return
    end
    currentClip = path
    local f = fade
    if f == nil then
        f = variables.fade
    end
    anim():play(path, true, f)
    info("play -> " .. path)
end

function Start()
    if not hasAnim() then
        info("animation_example: need AnimationComponent on this entity")
        return
    end

    local ac = anim()
    if ac.skeletonPath == nil or ac.skeletonPath == "" then
        ac:setSkeleton("resources/animations/skeleton.ozz")
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
    end

    if currentClip ~= variables.idleAnim then
        if input:isKeyPressed(KEY_LEFT_SHIFT) then
            playClip(variables.runAnim)
        elseif currentClip == variables.runAnim and not input:isKeyPressed(KEY_3) then
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
