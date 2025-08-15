
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


function ShootObject(model, shape, speed, scale)

    local cam = getCamera()
    local cpos = cam:getPosition()
    local foward = cam:getFront()
    -- Summon entity
    local newBall = createEntity("Ball")
    -- Add Components
    local tr = newBall:AddTransform()
    local mr = newBall:AddModelRenderer();
    local rb = newBall:AddRigidBodyComponent();
    local sc = newBall:AddLuaScript();
    newBall:AddShadowCaster();

    tr.scale = vec3(scale, scale, scale)
    mr:setModel(model)
    rb:setPosition(cam:getPosition())
    rb:addLinearVelocity(vec3(foward.x * speed, foward.y * speed, foward.z * speed))
    local t = shape:getType()
    if t == "BoxShape" then
        rb:setBoxShape(shape)
    elseif t == "SphereShape" then
        rb:setSphereShape(shape)
    elseif t == "CapsuleShape" then
        rb:setCapsuleShape(shape)
    elseif t == "CylinderShape" then
        rb:setCylinderShape(shape)
    elseif t == "TriangleShape" then
        rb:setTriangleShape(shape)
    else
        print("Unknown shape type: " .. tostring(t))
    end
    sc:setScript(newBall, "scripts/bullet.lua")
end

function EditorUpdate(dt)

    local input = getInput()

    if input:isKeyPressedThisFrame(KEY_P) then
        local physics = getPhysics()
        physics.isPhysicsPaused = not physics.isPhysicsPaused
    end


    if input:isKeyPressedThisFrame(KEY_E) then
        local shape = SphereShape(0.5 / 2)
        ShootObject("resources/models/sphere.obj", shape, 12, 0.5)
    end
end




function EditorShutdown()
    print("[Lua] onShutdown called!")
end