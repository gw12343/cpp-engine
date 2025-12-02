variables = {
	SPAWNPOINT = ehandle()
}


function Start()
    print("teapot code")
    

    -- Subscribe to game events
    subscribe("GameReset", function()
        local spawnPoint = getEntityFromHandle(variables.SPAWNPOINT)

		local transform = spawnPoint:GetTransform()

		local position = transform.position

		local rb = gameObject:GetRigidBodyComponent()

		rb:setLinearVelocity(vec3(0, 0, 0))
		rb:setAngularVelocity(vec3(0, 0, 0))
		rb:setRotation(quat(1, 0, 0, -1))
		rb:setPosition(position)
    end)
    

end

function Update()
    -- In a real game, this would render UI elements
end
