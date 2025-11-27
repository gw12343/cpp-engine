-- Target Script - Listens for collisions and publishes TargetHit event
-- Attach this to objects you want to use as targets

variables = {
    POINTS = 10,
    TARGET_COLOR = material(),
	NORMAL_COLOR = material()
}

local hasBeenHit = false

function Start()
    print("Target initialized with " .. variables.POINTS .. " points")

subscribe("GameReset", function()
        hasBeenHit = false
		local modelRenderer = gameObject:GetModelRenderer()
		
		if modelRenderer then
			modelRenderer:setMaterial(variables.NORMAL_COLOR)
		end
    end)

end

function CollisionEnter(other)
    --if not hasBeenHit and other:getName():find("Ball") then
    if not hasBeenHit then
        -- Publish the TargetHit event with score points
        publish("TargetHit", variables.POINTS)
        
        --print("Target hit! Publishing event with " .. variables.POINTS .. " points")
        
        hasBeenHit = true
        

		-- Get the model renderer first
		local modelRenderer = gameObject:GetModelRenderer()
		
		if modelRenderer then
			modelRenderer:setMaterial(variables.TARGET_COLOR)
		end
		

    end
end

function Update()
    -- Optional: Add bobbing or rotating animation
end
