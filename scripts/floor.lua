-- Define exposed variables
variables = {
    pos2 = vec3(1, 2, 3),
    hit = sound()
}

function PlayerCollisionEnter()
    local playerEntity = getPlayerEntity()
    local playerName = playerEntity:getName()
    local pc = playerEntity:GetPlayerControllerComponent()
    local ac = playerEntity:GetAudioSource()


    ac:setSound(variables.hit);
    ac:play();

    pc:setPosition(variables.pos2)
end


function Start()
    StartCoroutine(function()
        print("Start waiting")
        coroutine.yield(2.0)
        print("2 seconds passed")
        coroutine.yield(2.0)
        print("5 seconds total, now destroy")
        local rb = gameObject:GetRigidBodyComponent()
        local up = rb:getPosition()
        up.y = 5
        rb:setPosition(up)
    end)
end


function Update()

end


function CollisionEnter(other)

end