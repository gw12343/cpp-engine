variables = {
  
}



function Start()

end

local resetTimer = 0

function Update()
    -- Example: Press a key to reset the game
    local input = getInput()

	local crosshair = gameObject:GetRmlUIComponent()



    if input:isKeyPressed(KEY_F) then
		if crosshair then
			crosshair:SetVisible(false)
		end
	else
		if crosshair then
			crosshair:SetVisible(true)
		end	
	end
    

end
