function Start()
	print("particle started!")
	subscribe("GameReset", function()
        if gameObject:HasParticleSystem() then
            info("has particle")
            --local pr = gameObject:GetParticleSystem()


            local pm  = getParticleManager();

            pm:playEffect(gameObject)

            --pr:play()

        else
            info("has no particle")
        end

    end)
end


function Update()
end
