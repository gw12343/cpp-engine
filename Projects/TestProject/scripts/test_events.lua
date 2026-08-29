-- Test script for the Event System
-- This script tests multiple subscribers to the same event

function Start()
    print("=== Event System Test Script Started ===")
    
    -- Test multiple subscribers to the same event
    subscribe("TestEvent", function()
        print("[Subscriber 1] TestEvent received!")
    end)
    
    subscribe("TestEvent", function()
        print("[Subscriber 2] TestEvent received!")
    end)
    
    subscribe("TestEvent", function()
        print("[Subscriber 3] TestEvent received!")
    end)
    
    -- Test event with data
    subscribe("DataEvent", function(data)
        print("[Data Event] Received: " .. tostring(data))
    end)
    
    -- Immediately publish a test event
    publish("TestEvent")
    publish("DataEvent", "Hello from Event System!")
    publish("DataEvent", 42)
    publish("DataEvent", 3.14)
    publish("DataEvent", true)
    
    print("=== Event System Test Complete ===")
end

function Update()
    -- Nothing needed here for this test
end
