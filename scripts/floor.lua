function Start()
    --print("Started script!");
end


function Update()
    --local input = getInput()
    --print("hello i am old ")
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


    pc:setPosition(vec3(-16.93, 2.634, 8.558))
end