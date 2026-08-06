-- Replay this entity's particle effect on GameReset (e.g. key R).

function Start()
    subscribe("GameReset", function()
        if not gameObject:HasParticleSystem() then
            return
        end
        getParticleManager():playEffect(gameObject)
    end)
end

function Update()
end
