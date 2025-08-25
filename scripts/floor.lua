-- Define exposed variables
variables = {
    pos_x = 0.0,
    pos_y = 0.0,
    pos_z = 0.0
}

function Start()
end


function Update()

end


function CollisionEnter(other)

end

function PlayerCollisionEnter()
    local playerEntity = getPlayerEntity()
    local playerName = playerEntity:getName()
    local pc = playerEntity:GetPlayerControllerComponent()

    pc:setPosition(vec3(variables.pos_x, variables.pos_y, variables.pos_z))
end