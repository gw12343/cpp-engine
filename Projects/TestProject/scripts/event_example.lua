-- Example Lua script demonstrating the Event System
-- This script shows how to use subscribe() and publish() functions

function Start()
    print("Event Example Script Started!")
    
    -- Subscribe to the OnCollisionEnter event
    subscribe("OnCollisionEnter", function(otherEntity)
        if otherEntity:isValid() then
            print("[EVENT] Collision detected with: " .. otherEntity:getName())
        end
    end)
    
    -- Subscribe to custom events
    subscribe("PlayerDied", function()
        print("[EVENT] Player has died!")
    end)
    
    subscribe("ItemCollected", function(itemName)
        print("[EVENT] Collected item: " .. itemName)
    end)
    
    subscribe("ScoreChanged", function(newScore)
        print("[EVENT] Score changed to: " .. tostring(newScore))
    end)
    
    -- Subscribe to a position update event
    subscribe("PlayerMoved", function(position)
        print("[EVENT] Player moved to: " .. tostring(position))
    end)
    
    print("Subscribed to events: OnCollisionEnter, PlayerDied, ItemCollected, ScoreChanged, PlayerMoved")
end

local timer = 0

function Update()
    timer = timer + deltaTime
    
    -- Publish custom events for demonstration every 5 seconds
    if timer > 5 then
        timer = 0
        
        -- Publish events with different data types
        publish("ItemCollected", "Health Potion")
        publish("ScoreChanged", 100)
        
        -- Example of publishing a vec3 position
        -- publish("PlayerMoved", vec3(10, 20, 30))
    end
end

-- Note: OnCollisionEnter will be automatically triggered when this object collides with another
-- The old callback system (CollisionEnter function) still works for backward compatibility
