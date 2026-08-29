function Start()
    print("Started script platform!");
end


local platformSpeed = 1.0 -- units per second
local platformMinY = -2.0
local platformMaxY = 5.0
local platformDirection = 1.0


function Update()
    local tr = gameObject:GetTransform()
    local rb = gameObject:GetRigidBodyComponent()

    local pos = rb:getPosition();
    --print (pos.x .. "  " .. pos.y .. "  " .. pos.z)

    pos.x = pos.x + platformDirection * platformSpeed * deltaTime;

    if pos.x >= platformMaxY then
        pos.x = platformMaxY
        platformDirection = -1.0
    elseif pos.x <= platformMinY then
        pos.x = platformMinY
        platformDirection = 1.0
    end

    local vel = vec3(0, platformDirection * platformSpeed, 0)

    rb:moveKinematic(pos, quat(0,0,0,1), deltaTime);
end
