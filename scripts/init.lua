function onInit()
    print("[Lua] onInit called!!!")
    floor = create_entity("TestCube2")

    add_transform(floor,
        { x = 0.0, y = -10.0, z = 0.0 },
        { x = 0.0, y = 0.0, z = 0.0 },
        { x = 40, y = 1, z = 40 }
    )
    add_model_renderer(floor, "/home/gabe/CLionProjects/cpp-engine/resources/models/cube.obj")

end

function onUpdate(dt)


    local win = getWindow()

    --print("Window size:", win:getWidth(), "x", win:getHeight())
    --print("Aspect ratio:", win:getAspectRatio())

    local cam = getCamera()

    --print("Camera position:", cam:getPosition().x, cam:getPosition().y, cam:getPosition().z)

    local input = getInput()
    if input:isKeyPressed(KEY_W) then
    	print("W key is down")
    end

    if input:isMousePressed(0) then
    	print("Left mouse button pressed")
    end
end

function onShutdown()
    print("[Lua] onShutdown called!!! yae")
end