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
end


function Update()

end


function CollisionEnter(other)

end