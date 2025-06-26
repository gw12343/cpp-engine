function Start()
    print("Started script!");
end

function MoveObject(entity)

    if entity:HasTransform() then
        print(" - Transform")

       local tf = entity:GetTransform()

       print("Position:", tf.position.x, tf.position.y, tf.position.z)

       -- Set new position
       tf.position = vec3(1.0, 2.0, 3.0)
       -- Set rotation using Euler angles (degrees)
       tf:SetRotation(vec3(90, 90, 0))

       -- Read rotation as Euler
       local angles = tf:GetEulerAngles()
       print("Rotation (euler):", angles.x, angles.y, angles.z)
    end

end


function Update()
    local input = getInput()
    if input:isMousePressed(1) then
        MoveObject(gameObject)
    end

    if input:isKeyPressedThisFrame(KEY_W) then
        print("W")
    end
end


function Shutdown()
    print("Shutdown!");
end