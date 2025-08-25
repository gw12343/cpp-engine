-- Define exposed variables
variables = {
    pos_x = 0.0,
    pos_y = 0.0,
    pos_z = 0.0
}

function Start()
    --print("Started script!");
end


function Update()
    --local input = getInput()
    --print("hello i am old ")

    --print("hi " .. variables.speed)
end


function CollisionEnter(other)
    --local tr = gameObject:GetTransform()
    --local rb = gameObject:GetRigidBodyComponent()


    --if other:getName() == "Player" then
        print("Player Dead!")
    --end
end

function PlayerCollisionEnter()
    --local tr = gameObject:GetTransform()
    --local rb = gameObject:GetRigidBodyComponent()

    local playerEntity = getPlayerEntity()
    local playerName = playerEntity:getName()
    local pc = playerEntity:GetPlayerControllerComponent()

    print("Player Dead! fr gng")


    pc:setPosition(vec3(variables.pos_x, variables.pos_y, variables.pos_z))
end