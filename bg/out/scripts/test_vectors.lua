-- Test script for AssetHandle vectors

variables = {
    textures = TextureVector.new(),
    models = ModelVector.new(),
    materials = MaterialVector.new(),
    scenes = SceneVector.new(),
    tiles = TerrainTileVector.new(),
    particles = ParticleVector.new(),
    sounds = SoundVector.new()
}

function Start()
    print("Testing AssetHandle vectors...")
    
    print("Textures size: " .. variables.textures:size())
    print("Models size: " .. variables.models:size())

    for i = 1, variables.models:size() do
        local handle = variables.models[i]
        print("Model " .. i .. " valid: " .. tostring(handle:isValid()))
    end
end

function Update(dt)
end
