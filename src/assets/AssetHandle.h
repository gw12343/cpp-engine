//
// Created by gabe on 6/30/25.
//

#ifndef CPP_ENGINE_ASSETHANDLE_H
#define CPP_ENGINE_ASSETHANDLE_H


namespace Engine
{
	namespace Audio
	{
		class SoundBuffer;
	}

	class Particle;

	namespace Terrain
	{
		class TerrainTile;
	}

	namespace Rendering
	{
		class Model;
	}

	class Texture;
	class Material;
	class Animation;
}

namespace Engine {
	template <typename T>
	class AssetHandle {
		std::string guid {};

	  public:
		AssetHandle() = default;
		explicit AssetHandle(const std::string& guid) : guid(guid) {}
		[[nodiscard]] const std::string& GetID() const { return guid; }
		[[nodiscard]] bool               IsValid() const { return !guid.empty() && guid.size() == 32; }

		bool operator==(const AssetHandle<T>& other) const { return guid == other.guid; }
		bool operator<(const AssetHandle<T>& other) const { return guid < other.guid; }
	};


	using TextureHandle = AssetHandle<Engine::Texture>;
	using ModelHandle = AssetHandle<Engine::Rendering::Model>;
	using MaterialHandle = AssetHandle<Engine::Material>;
	using SceneHandle = AssetHandle<Engine::Scene>;
	using TerrainHandle = AssetHandle<Engine::Terrain::TerrainTile>;
	using ParticleHandle = AssetHandle<Engine::Particle>;
	using SoundHandle = AssetHandle<Engine::Audio::SoundBuffer>;
	using AnimationHandle = AssetHandle<Engine::Animation>;

	using TextureHandleList = std::vector<TextureHandle>;
	using ModelHandleList = std::vector<ModelHandle>;
	using MaterialHandleList = std::vector<MaterialHandle>;
	using SceneHandleList = std::vector<SceneHandle>;
	using TerrainHandleList = std::vector<TerrainHandle>;
	using ParticleHandleList = std::vector<ParticleHandle>;
	using SoundHandleList = std::vector<SoundHandle>;
	using AnimationHandleList = std::vector<AnimationHandle>;

} // namespace Engine

#endif // CPP_ENGINE_ASSETHANDLE_H
