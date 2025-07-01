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

    if other:getName() ~= "Floor" then
        return
    end

    local tr = gameObject:GetTransform()
    tr.scale = vec3(1, 1, 1)
    local rb = gameObject:GetRigidBodyComponent()
    rb:setSphereShape(SphereShape(0.5))
    isSmall = false
end
