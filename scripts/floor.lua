-- Define exposed variables
variables = {
    pos2 = vec3(1, 2, 3),
    hit = sound(),
    playerEnt = ehandle()
}

function PlayerCollisionEnter()
    local playerEntity = getEntityFromHandle(variables.playerEnt)

    if playerEntity and playerEntity:isValid() then
        local pc = playerEntity:GetPlayerControllerComponent()
        local ac = playerEntity:GetAudioSource()

        ac:setSound(variables.hit);
        ac:play();

        pc:setPosition(variables.pos2)
    end
end


function Start()

end


function Update()

end


function CollisionEnter(other)

end