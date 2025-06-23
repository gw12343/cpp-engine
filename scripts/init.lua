function onInit()
    print("[Lua] onInit called!!!")
    floor = create_entity("TestCube2")
    print("flooor ")
    print(floor:getName())
    add_transform(floor,
        { x = 0.0, y = -10.0, z = 0.0 },
        { x = 0.0, y = 0.0, z = 0.0 },
        { x = 40, y = 1, z = 40 }
    )
    add_model_renderer(floor, "/home/gabe/CLionProjects/cpp-engine/resources/models/cube.obj")

end

function onUpdate(dt)
    if Input.isKeyPressed(87) then -- W key
        print("W is pressed")
        floor:setName(floor:getName().."w");
    end
    --print("hi") --string.format("[Lua] onUpdate: dt = %.2f", dt))
end

function onShutdown()
    print("[Lua] onShutdown called!!! yae")
end