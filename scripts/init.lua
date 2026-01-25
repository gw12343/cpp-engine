
function EditorInit()
    print("Editor init called!")

    --[[local floor = createEntity("Floor")
    local tr = floor:AddTransform()
    local mr = floor:AddModelRenderer();
    local rb = floor:AddRigidBodyComponent();
    tr.scale = vec3(60, 2, 60)

    mr:setModel("resources/models/cube.obj")
    rb:setKinematic(true)
    rb:setBoxShape(BoxShape(vec3(30.0, 1.0, 30.0)))--]]
end



    -- State container
    speed = { v = 1.5 }
    enabled = { v = true }


local ui = getUI()

function EditorUpdate(dt)
    local selectedEntity = ui:getSelectedEntity()


    imgui.Begin("Mesh Extractor")


   if selectedEntity:isValid() then
       imgui.Text("Selected: " .. selectedEntity:getName())

        if selectedEntity:HasModelRenderer() then
            imgui.Text("Model Renderer yay")
            local mr = selectedEntity:GetModelRenderer()

            local model = mr.model

            imgui.Text("Model Renderer has "..model:getGuid())
            imgui.Text("Model Renderer valid:  ".. tostring(model:isValid()))

            if imgui.Button("Extract") then
            end

        else
            imgui.Text("NO MODEL RENDERER")
        end


   else
       imgui.Text("No valid entity selected")
   end




    imgui.End()


    --info("hello editor ui")


    --local input = getInput()

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
    print("onShutdown called!")
end