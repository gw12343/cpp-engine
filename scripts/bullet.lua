function Start()

end


function Update()
end


function CollisionEnter(other)
    local tr = gameObject:GetTransform()
    local rb = gameObject:GetRigidBodyComponent()


    if other:getName() == "Cylinder" then
        print("collided! 1234")
    end
end
