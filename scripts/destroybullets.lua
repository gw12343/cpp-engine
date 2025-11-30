-- Random child destoryer

local lastTime = 0

variables = {
    destroyTime = 1,
    sounds={sound()}
}

function Start()

end



function Update()
    lastTime = lastTime + deltaTime
    local chi = gameObject:getChildren()

    if lastTime >= variables.destroyTime then
        lastTime = 0

        if #chi > 0 then
            local chosenIndex = 1--math.random(#chi)
            local chosen = getEntityFromHandle(chi[chosenIndex])
            chosen:destroy()
        end
    end


end