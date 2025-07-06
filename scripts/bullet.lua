function Start()

end


function Update()

end


function CollisionEnter(other)
    local tr = gameObject:GetTransform()
    local rb = gameObject:GetRigidBodyComponent()

    local currScale = tr.scale.x

    if other:getName() ~= "TerrainWrapper" then
        return
    end

    if currScale < 15.0 then

        local newScale = currScale + 0.05


        tr.scale = vec3(newScale, newScale, newScale)
        rb:setSphereShape(SphereShape(newScale / 2))
    else
        tr.scale = vec3(0.25, 0.25, 0.25)
        rb:setSphereShape(SphereShape(0.125))



        -- Summon entity
        local newBall = createEntity("BallClone")
        -- Add Components
        local tr2 = newBall:AddTransform()
        local mr2 = newBall:AddModelRenderer();
        local rb2 = newBall:AddRigidBodyComponent();
        local sc2 = newBall:AddLuaScript();
        newBall:AddShadowCaster();

        tr2.scale = vec3(0.25, 0.25, 0.25)
        mr2:setModel("resources/models/sphere.obj")
        rb2:setPosition(tr.position)
        rb2:setSphereShape(SphereShape(0.125))
        --rb2:addLinearVelocity(vec3(foward.x * speed, foward.y * speed, foward.z * speed))
        sc2:setScript(newBall, "scripts/bullet.lua")
    end
end
