-- Game UI Script - Subscribes to events and displays feedback
-- Demonstrates multiple independent subscribers to same events

function Start()
    print("Game UI initialized")
    
    -- Subscribe to score changes
    subscribe("ScoreChanged", function(newScore)
        print("[UI] Score updated: " .. tostring(newScore))
        -- In a real game, this would update UI text elements
    end)
    
    -- Subscribe to target hits for visual feedback
    subscribe("TargetHit", function(points)
        print("[UI] +", tostring(points), "points!")
        -- In a real game, this would show a floating text or particle effect
    end)
    
    -- Subscribe to game events
    subscribe("GameReset", function()
        print("[UI] Game Reset - UI cleared")
    end)
    
    subscribe("AllTargetsDestroyed", function()
        print("[UI] 🎉 VICTORY! 🎉")
    end)
end

function Update()
    -- In a real game, this would render UI elements
end
