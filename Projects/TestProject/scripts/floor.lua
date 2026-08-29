-- Define exposed variables
variables = {
    hit = sound(),
    playerEnt = ehandle(),
    ev = EntityVector.new()
}

function PlayerCollisionEnter()
    local playerEntity = getEntityFromHandle(variables.playerEnt)
    local idx = math.random(variables.ev:size())
    local spawnEntity = getEntityFromHandle(variables.ev[idx])
    print(spawnEntity:getName())

    if playerEntity and playerEntity:isValid() then
        local pc = playerEntity:GetPlayerControllerComponent()
        local ac = playerEntity:GetAudioSource()

        local ts = spawnEntity:GetTransform().position

        ac:setSound(variables.hit);
        ac:play();


        pc:setPosition(ts)
    end
end


function Start()
    print("There are", variables.ev:size(), "spawnpoints")

end


function Update()

end


function CollisionEnter(other)

end