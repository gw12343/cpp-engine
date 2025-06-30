isSmall = true

function Start()
    print("Spawned bullet!");
end


function Update()

end


function CollisionEnter(other)
    if not isSmall then
        return
    end
    print("Entered collision with other!!!!")
    print(other:getName())

    local tr = gameObject:GetTransform()
    tr.scale = vec3(1, 1, 1)
    --local rb = gameObject:GetRigidBodyComponent()
    --print(rb:getLinearVelocity().x)
    --rb:setBoxShape(BoxShape(vec3(0.5, 0.5, 0.5)))
    isSmall = false
end
