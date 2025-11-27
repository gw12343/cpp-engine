-- Score Manager Script - Subscribes to game events and tracks score
-- Attach this to a single entity in your scene

local score = 0
local targetsHit = 0

function Start()
    print("=== SCORE MANAGER STARTED ===")
    
    -- Subscribe to target hit events
    subscribe("TargetHit", function(points)
        score = score + points
        targetsHit = targetsHit + 1
        --print("TARGET HIT! +", tostring(points)) -- "points | Total Score:", tostring(score), "| Targets Hit:", tostring(targetsHit))
        
        -- Publish score changed event for UI or other systems
        publish("ScoreChanged", score)
        publish("TargetsHitUpdated", targetsHit)
    end)
    
    -- Subscribe to game reset events
    subscribe("GameReset", function()
        score = 0
        targetsHit = 0
        print("Score reset!")
        publish("ScoreChanged", 0)
        publish("TargetsHitUpdated", 0)
    end)
    
    -- Optional: Subscribe to level complete events
    subscribe("AllTargetsDestroyed", function()
        --print("=== LEVEL COMPLETE ===")
        --print("Final Score:", tostring(score))
        --print("Targets Hit:", tostring(targetsHit))
    end)
end

function Update()
    -- Nothing needed here
end

-- Helper function to get current score (can be called by other scripts)
function GetScore()
    return score
end

function GetTargetsHit()
    return targetsHit
end
