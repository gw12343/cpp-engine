#pragma once

#include "Camera.h"
#include "core/module/Module.h"
#include "assets/AssetHandle.h"

#include <AL/al.h>
#include <AL/alc.h>
#include <entt/entt.hpp>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>


namespace Engine::Audio {
	class SoundBuffer {
	  public:
		explicit SoundBuffer(const std::string& filename);
		~SoundBuffer();

		[[nodiscard]] ALuint GetBufferID() const { return m_bufferID; }
		[[nodiscard]] bool   IsLoaded() const { return m_loaded; }

		std::string name;

	  private:
		ALuint m_bufferID{};
		bool   m_loaded;
	};

	class SoundSource {
	  public:
		explicit SoundSource(bool looping = false);
		~SoundSource();

		void                  Play(AssetHandle<Audio::SoundBuffer> buffer) const;
		void                  Stop() const;
		void                  Pause() const;
		[[maybe_unused]] void Resume() const;

		void                         SetPosition(float x, float y, float z) const;
		[[maybe_unused]] void        SetVelocity(float x, float y, float z) const;
		void                         SetPitch(float pitch) const;
		void                         SetGain(float gain) const;
		[[maybe_unused]] void        SetLooping([[maybe_unused]] bool looping) const;
		[[maybe_unused]] static void SetDistanceModel(ALenum model);
		void                         ConfigureAttenuation(float refDistance, float maxDistance, float rolloffFactor) const;

	  private:
		[[maybe_unused]] ALuint m_sourceID{};
	};

	class SoundManager : public Module {
	  public:
		void        onInit() override;
		void        onUpdate(float dt) override;
		void        onGameStart() override;
		void        onShutdown() override;
		std::string name() const override { return "SoundManager"; };

		// std::unique_ptr<SoundBuffer> LoadSound(const std::string& name, const std::string& filename);

		// std::shared_ptr<SoundBuffer> GetSound(const std::string& name);

		static void                  SetListenerPosition(float x, float y, float z);
		[[maybe_unused]] static void SetListenerVelocity(float x, float y, float z);
		static void                  SetListenerOrientation(float atX, float atY, float atZ, float upX, float upY, float upZ);
		void                         Play(AssetHandle<Audio::SoundBuffer> buffer, bool looping = false, float volume = 1.0f);

		static void CheckOpenALError(const char* operation);

	  private:
		void                                                          LoadDefaultSounds();
		ALCdevice*                                                    m_device;
		ALCcontext*                                                   m_context;
		std::unordered_map<std::string, std::shared_ptr<SoundBuffer>> m_soundBuffers;
		std::vector<std::shared_ptr<SoundSource>>                     m_sources;
		bool                                                          m_initialized;
	};
} // namespace Engine::Audio
