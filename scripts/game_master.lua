-- Game Master Script - Demonstrates publishing various game events
-- This shows how to trigger game-wide events

variables = {
    TOTAL_TARGETS = 5  -- Configure based on how many targets you spawn
}

local remainingTargets = 0

function Start()
    print("=== GAME MASTER STARTED ===")
    remainingTargets = variables.TOTAL_TARGETS
    print("Total targets:" .. tostring(remainingTargets))
    
    -- Subscribe to target hits to track remaining targets
    subscribe("TargetHit", function(points)
        remainingTargets = remainingTargets - 1
        print("[GameMaster] Targets remaining:" .. tostring(remainingTargets))
        
        if remainingTargets <= 0 then
            publish("AllTargetsDestroyed")
        end
    end)
    
    print("Press R to reset game (example)")
end

local resetTimer = 0

function Update()
    -- Example: Press a key to reset the game
    local input = getInput()
    
    if input:isKeyPressedThisFrame(KEY_R) then
        print("[GameMaster] Resetting game...")
        publish("GameReset")
        remainingTargets = variables.TOTAL_TARGETS
    end
    
    -- Example: Auto-publish some events for testing
    resetTimer = resetTimer + deltaTime
    if resetTimer > 30 then
        -- Every 30 seconds, publish a custom event
        print("[GameMaster] Publishing periodic event")
        publish("TimeElapsed", 30)
        resetTimer = 0
    end
end
