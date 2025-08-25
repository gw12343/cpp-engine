function Start()
    print("Started script platform!");
end


function rotationAroundY(time, speed)
    local angle = time * speed -- radians
    local half = angle * 0.5
    return quat(0, math.sin(half), 0, math.cos(half))
end


local turnSpeed = 1.25

local r = 0

function Update()
    local rb = gameObject:GetRigidBodyComponent()
    r = r + deltaTime

    local pos = rb:getPosition();
    local rot = rotationAroundY(r, turnSpeed)
    rb:moveKinematic(pos, rot, deltaTime)
end
