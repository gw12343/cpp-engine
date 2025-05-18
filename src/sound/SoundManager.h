#pragma once

#include <AL/al.h>
#include <AL/alc.h>
#include <string>
#include <unordered_map>
#include <memory>
#include <vector>
#include <entt/entt.hpp>
#include "Camera.h"

namespace Engine{
    namespace Audio {
        class SoundBuffer {
        public:
            SoundBuffer(const std::string& filename);
            ~SoundBuffer();
            
            ALuint GetBufferID() const { return m_bufferID; }
            bool IsLoaded() const { return m_loaded; }

        private:
            ALuint m_bufferID;
            bool m_loaded;
        };

        class SoundSource {
        public:
            SoundSource(bool looping = false);
            ~SoundSource();
            
            void Play(const SoundBuffer& buffer);
            void Stop();
            void Pause();
            void Resume();
            
            void SetPosition(float x, float y, float z);
            void SetVelocity(float x, float y, float z);
            void SetPitch(float pitch);
            void SetGain(float gain);
            void SetLooping(bool looping);
            void SetDistanceModel(ALenum model);
            void ConfigureAttenuation(float refDistance, float maxDistance, float rolloffFactor);

        private:
            ALuint m_sourceID;
        };

        class SoundManager {
        public:
            SoundManager();
            ~SoundManager();
            
            bool Initialize();
            void Shutdown();
            
            std::shared_ptr<SoundBuffer> LoadSound(const std::string& name, const std::string& filename);
            std::shared_ptr<SoundBuffer> GetSound(const std::string& name);
            
            void SetListenerPosition(float x, float y, float z);
            void SetListenerVelocity(float x, float y, float z);
            void SetListenerOrientation(float atX, float atY, float atZ, float upX, float upY, float upZ);
            void Play(const std::string& soundName, bool looping = false, float volume = 1.0f);

            void UpdateAudioEntities(entt::registry& registry, const Camera& camera);
            void CheckOpenALError(const char* operation);
            
            private:
            bool LoadDefaultSounds();
            ALCdevice* m_device;
            ALCcontext* m_context;
            std::unordered_map<std::string, std::shared_ptr<SoundBuffer>> m_soundBuffers;
            std::vector<std::shared_ptr<SoundSource>> m_sources;
            bool m_initialized;
        };
    }
}
