isSmall = true

function Start()

end


function Update()

end


function CollisionEnter(other)
    if not isSmall then
        return
    end

    if other:getName() ~= "TerrainWrapper" then
        return
    end

    local tr = gameObject:GetTransform()
    tr.scale = vec3(1, 1, 1)
    local rb = gameObject:GetRigidBodyComponent()
    rb:setSphereShape(SphereShape(0.5))
    isSmall = false
end
