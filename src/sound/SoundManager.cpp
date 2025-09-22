#include "SoundManager.h"

#include "components/Components.h"
#include "core/EngineData.h"
#include "components/impl/EntityMetadataComponent.h"
#include "components/impl/AudioSourceComponent.h"
#include "components/impl/TransformComponent.h"

#include "assets/AssetManager.h"
#include "utils/Utils.h"

#include <cstring>
#include <sndfile.h>
#include <spdlog/spdlog.h>
#include <vector>
#include <tracy/Tracy.hpp>


namespace Engine::Audio {
	// SoundBuffer implementation
	SoundBuffer::SoundBuffer(const std::string& filename) : m_loaded(false)
	{
		name = GetFileName(filename);
		// Generate buffer
		alGenBuffers(1, &m_bufferID);

		// Open audio file with libsndfile
		SF_INFO fileInfo;
		memset(&fileInfo, 0, sizeof(SF_INFO));

		SNDFILE* file = sf_open(filename.c_str(), SFM_READ, &fileInfo);
		if (!file) {
			GetSoundManager().log->error("Failed to open sound file: {}", filename);
			return;
		}

		// Read the audio data
		std::vector<short> samples(fileInfo.frames * fileInfo.channels);
		sf_read_short(file, samples.data(), static_cast<long>(samples.size()));
		sf_close(file);

		// Determine format based on channels
		ALenum format;
		if (fileInfo.channels == 1)
			format = AL_FORMAT_MONO16;
		else if (fileInfo.channels == 2)
			format = AL_FORMAT_STEREO16;
		else {
			GetSoundManager().log->error("Unsupported channel count: {}", fileInfo.channels);
			return;
		}

		// Load data into OpenAL buffer
		int size = samples.size() * sizeof(short);
		alBufferData(m_bufferID, format, samples.data(), size, fileInfo.samplerate);

		// Check for errors
		ALenum error = alGetError();
		if (error != AL_NO_ERROR) {
			GetSoundManager().log->error("Failed to load sound data: {}", error);
			return;
		}

		m_loaded = true;
	}

	SoundBuffer::~SoundBuffer()
	{
		alDeleteBuffers(1, &m_bufferID);
	}

	// SoundSource implementation
	SoundSource::SoundSource(bool looping)
	{
		alGenSources(1, &m_sourceID);
		alSourcef(m_sourceID, AL_PITCH, 1.0f);
		alSourcef(m_sourceID, AL_GAIN, 1.0f);
		alSource3f(m_sourceID, AL_POSITION, 0.0f, 0.0f, 0.0f);
		alSource3f(m_sourceID, AL_VELOCITY, 0.0f, 0.0f, 0.0f);
		alSourcei(m_sourceID, AL_LOOPING, looping ? AL_TRUE : AL_FALSE);
	}

	SoundSource::~SoundSource()
	{
		alDeleteSources(1, &m_sourceID);
	}

	[[maybe_unused]] void SoundSource::SetDistanceModel(ALenum model)
	{
		alDistanceModel(model);
	}

	void SoundSource::SetPosition(float x, float y, float z) const
	{
		alGetError(); // Clear previous errors
		alSource3f(m_sourceID, AL_POSITION, x, y, z);
		ALenum error = alGetError();
		if (error != AL_NO_ERROR) {
			GetSoundManager().log->error("OpenAL error setting source position: {}", error);
		}
	}

	void SoundSource::ConfigureAttenuation(float refDistance, float maxDistance, float rolloffFactor) const
	{
		alGetError(); // Clear previous errors

		alSourcef(m_sourceID, AL_REFERENCE_DISTANCE, refDistance);
		ALenum error = alGetError();
		if (error != AL_NO_ERROR) {
			GetSoundManager().log->error("OpenAL error setting reference distance: {}", error);
			return;
		}

		alSourcef(m_sourceID, AL_MAX_DISTANCE, maxDistance);
		error = alGetError();
		if (error != AL_NO_ERROR) {
			GetSoundManager().log->error("OpenAL error setting max distance: {}", error);
			return;
		}

		alSourcef(m_sourceID, AL_ROLLOFF_FACTOR, rolloffFactor);
		error = alGetError();
		if (error != AL_NO_ERROR) {
			GetSoundManager().log->error("OpenAL error setting rolloff factor: {}", error);
			return;
		}
	}


	void SoundSource::Play(AssetHandle<Audio::SoundBuffer> buffer) const
	{
		// Clear any previous errors
		alGetError();

		SoundBuffer* buff = GetAssetManager().Get(buffer);

		if (!buff->IsLoaded()) {
			GetSoundManager().log->error("Attempted to play unloaded buffer");
			return;
		}

		// Get buffer ID
		ALuint bufferID = buff->GetBufferID();

		// Verify buffer exists and has data
		ALint size = 0;
		alGetBufferi(bufferID, AL_SIZE, &size);

		ALenum error = alGetError();
		if (error != AL_NO_ERROR) {
			GetSoundManager().log->error("Buffer validation failed: {}", error);
			return;
		}

		if (size <= 0) {
			GetSoundManager().log->error("Buffer has no data (size = {})", size);
			return;
		}

		GetSoundManager().log->info("Playing sound buffer: ID {}, size {} bytes", bufferID, size);

		// Stop any currently playing sound
		alSourceStop(m_sourceID);
		error = alGetError();
		if (error != AL_NO_ERROR) {
			GetSoundManager().log->warn("Error stopping previous sound: {}", error);
			// Continue anyway
		}

		// Detach any previous buffer
		alSourcei(m_sourceID, AL_BUFFER, 0);
		error = alGetError();
		if (error != AL_NO_ERROR) {
			GetSoundManager().log->warn("Error detaching previous buffer: {}", error);
			// Continue anyway
		}

		// Attach buffer to source
		alSourcei(m_sourceID, AL_BUFFER, static_cast<ALint>(bufferID));

		error = alGetError();
		if (error != AL_NO_ERROR) {
			GetSoundManager().log->error("Failed to attach buffer to source: {}", error);
			return;
		}

		// Play the source
		alSourcePlay(m_sourceID);

		error = alGetError();
		if (error != AL_NO_ERROR) {
			GetSoundManager().log->error("Failed to play source: {}", error);
			return;
		}

		// Check if it's actually playing
		ALint state;
		alGetSourcei(m_sourceID, AL_SOURCE_STATE, &state);
		GetSoundManager().log->info("Source state after play: {}", state == AL_PLAYING ? "PLAYING" : state == AL_PAUSED ? "PAUSED" : state == AL_STOPPED ? "STOPPED" : "UNKNOWN");
	}

	void SoundSource::Stop() const
	{
		alSourceStop(m_sourceID);
	}

	void SoundSource::Pause() const
	{
		alSourcePause(m_sourceID);
	}

	[[maybe_unused]] void SoundSource::Resume() const
	{
		alSourcePlay(m_sourceID);
	}

	[[maybe_unused]] void SoundSource::SetVelocity(float x, float y, float z) const
	{
		alSource3f(m_sourceID, AL_VELOCITY, x, y, z);
	}

	void SoundSource::SetPitch(float pitch) const
	{
		alSourcef(m_sourceID, AL_PITCH, pitch);
	}

	void SoundSource::SetGain(float gain) const
	{
		alSourcef(m_sourceID, AL_GAIN, gain);
	}

	[[maybe_unused]] void SoundSource::SetLooping(bool looping) const
	{
		alSourcei(m_sourceID, AL_LOOPING, looping ? AL_TRUE : AL_FALSE);
	}


	void SoundManager::Play(AssetHandle<Audio::SoundBuffer> soundBuffer, bool looping, float volume)
	{
		// auto buffer = GetSound(soundName);
		//		if (!buffer) {
		//			log->error("Failed to play sound: sound not found");
		//			return;
		//		}

		auto source = std::make_shared<SoundSource>(looping);
		source->SetGain(volume);
		source->Play(soundBuffer);
		m_sources.push_back(source);
	}


	void SoundManager::onUpdate(float dt)
	{
		ZoneScoped;
		// Clear any previous errors
		alGetError();

		// Update listener position and orientation based on camera
		glm::vec3 cameraPos   = GetCamera().GetPosition();
		glm::vec3 cameraFront = GetCamera().GetFront();
		glm::vec3 cameraUp    = glm::vec3(0.0f, 1.0f, 0.0f); // Assuming Y-up world

		// Set listener position to camera position
		SetListenerPosition(cameraPos.x, cameraPos.y, cameraPos.z);
		CheckOpenALError("setting listener position");

		// Set listener orientation (at vector and up vector)
		SetListenerOrientation(cameraFront.x,
		                       cameraFront.y,
		                       cameraFront.z, // "At" vector (where camera is looking)
		                       cameraUp.x,
		                       cameraUp.y,
		                       cameraUp.z // "Up" vector
		);
		CheckOpenALError("setting listener orientation");

		// Update all audio sources based on their entity transforms
		auto audioView = GetCurrentSceneRegistry().view<Components::EntityMetadata, Components::Transform, Components::AudioSource>();

		for (auto [entity, metadata, transform, audio] : audioView.each()) {
			// Update audio source position based on entity transform
			if (audio.source) {
				// Get the world position from the transform
				glm::vec3 worldPos = transform.position;

				// Set the 3D position of the sound source
				audio.source->SetPosition(worldPos.x, worldPos.y, worldPos.z);
				CheckOpenALError("setting source position");

				// Ensure attenuation parameters are up-to-date
				audio.source->ConfigureAttenuation(audio.referenceDistance, audio.maxDistance, audio.rolloffFactor);
				CheckOpenALError("configuring attenuation");

				// Set the base volume (OpenAL will handle the attenuation)
				audio.source->SetGain(audio.volume);
				CheckOpenALError("setting gain");
			}
		}
	}


	void SoundManager::onInit()
	{
		// Open the default audio device
		m_device = alcOpenDevice(nullptr);
		if (!m_device) {
			log->error("Failed to open OpenAL device");
			return;
		}

		// Create context
		m_context = alcCreateContext(m_device, nullptr);
		if (!m_context) {
			log->error("Failed to create OpenAL context");
			alcCloseDevice(m_device);
			m_device = nullptr;
			return;
		}

		// Make context current
		if (!alcMakeContextCurrent(m_context)) {
			log->error("Failed to make OpenAL context current");
			alcDestroyContext(m_context);
			alcCloseDevice(m_device);
			m_context = nullptr;
			m_device  = nullptr;
			return;
		}

		m_initialized = true;
		log->debug("OpenAL initialized successfully");


		if (!m_initialized) {
			log->critical("Failed to initialize sound manager");
			return;
		}

		LoadDefaultSounds();
	}

	void SoundManager::onShutdown()
	{
		if (!m_initialized) return;

		// Clear all sound buffers
		m_soundBuffers.clear();
		m_sources.clear();

		// Destroy context and close device
		alcMakeContextCurrent(nullptr);
		alcDestroyContext(m_context);
		alcCloseDevice(m_device);

		m_context     = nullptr;
		m_device      = nullptr;
		m_initialized = false;

		GetDefaultLogger()->info("OpenAL shutdown complete");
	}


	void SoundManager::SetListenerPosition(float x, float y, float z)
	{
		alListener3f(AL_POSITION, x, y, z);
	}

	[[maybe_unused]] void SoundManager::SetListenerVelocity(float x, float y, float z)
	{
		alListener3f(AL_VELOCITY, x, y, z);
	}

	void SoundManager::SetListenerOrientation(float atX, float atY, float atZ, float upX, float upY, float upZ)
	{
		float orientation[6] = {atX, atY, atZ, upX, upY, upZ};
		alListenerfv(AL_ORIENTATION, orientation);
	}

	void SoundManager::LoadDefaultSounds()
	{
		// Clear any previous errors
		alGetError();

		// Set the distance attenuation model to linear distance clamped
		alDistanceModel(AL_LINEAR_DISTANCE_CLAMPED);

		// Load sounds
		// auto backgroundMusic = LoadSound("background_music", "resources/sounds/quietmoments.wav");

		// LoadSound("birds", "resources/sounds/bird_chirp.wav");

		// Play("background_music", true, 1.0f);
	}

	void SoundManager::CheckOpenALError(const char* operation)
	{
		ALenum error = alGetError();
		if (error != AL_NO_ERROR) {
			GetSoundManager().log->error("OpenAL error after {}: {}", operation, error);
		}
	}
	void SoundManager::onGameStart()
	{
		auto audioView = GetCurrentSceneRegistry().view<Components::EntityMetadata, Components::Transform, Components::AudioSource>();

		for (auto [entity, metadata, transform, audio] : audioView.each()) {
			// Update audio source position based on entity transform
			if (audio.source) {
				// Get the world position from the transform
				glm::vec3 worldPos = transform.position;

				// If autoPlay is enabled, try to play the sound
				if (audio.autoPlay && audio.buffer.IsValid()) {
					audio.Play();
				}
			}
		}
	}


} // namespace Engine::Audio
#include "assets/AssetManager.inl"