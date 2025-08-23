
function EditorInit()
    print("[Lua] editor init called!")

    --[[local floor = createEntity("Floor")
    local tr = floor:AddTransform()
    local mr = floor:AddModelRenderer();
    local rb = floor:AddRigidBodyComponent();
    tr.scale = vec3(60, 2, 60)

    mr:setModel("resources/models/cube.obj")
    rb:setKinematic(true)
    rb:setBoxShape(BoxShape(vec3(30.0, 1.0, 30.0)))--]]
end




function EditorUpdate(dt)

    --local input = getInput()
--
    --if input:isKeyPressedThisFrame(KEY_P) then
    --    local physics = getPhysics()
    --    --physics.isPhysicsPaused = not physics.isPhysicsPaused
    --end
--
--
    --if input:isKeyPressedThisFrame(KEY_E) then
    --    local shape = SphereShape(0.5 / 2)
    --    ShootObject("resources/models/sphere.obj", shape, 12, 0.5)
    --end
end




function EditorShutdown()
    print("[Lua] onShutdown called!")
end