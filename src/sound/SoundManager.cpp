#include "SoundManager.h"
#include <spdlog/spdlog.h>
#include <sndfile.h>
#include <vector>
#include <cstring>
#include "components/Components.h"

namespace Engine{
    namespace Audio {
// SoundBuffer implementation
SoundBuffer::SoundBuffer(const std::string& filename) : m_loaded(false) {
    // Generate buffer
    alGenBuffers(1, &m_bufferID);
    
    // Open audio file with libsndfile
    SF_INFO fileInfo;
    memset(&fileInfo, 0, sizeof(SF_INFO));
    
    SNDFILE* file = sf_open(filename.c_str(), SFM_READ, &fileInfo);
    if (!file) {
        spdlog::error("Failed to open sound file: {}", filename);
        return;
    }
    
    // Read the audio data
    std::vector<short> samples(fileInfo.frames * fileInfo.channels);
    sf_read_short(file, samples.data(), samples.size());
    sf_close(file);
    
    // Determine format based on channels
    ALenum format;
    if (fileInfo.channels == 1)
        format = AL_FORMAT_MONO16;
    else if (fileInfo.channels == 2)
        format = AL_FORMAT_STEREO16;
    else {
        spdlog::error("Unsupported channel count: {}", fileInfo.channels);
        return;
    }
    
    // Load data into OpenAL buffer
    alBufferData(m_bufferID, format, samples.data(), 
                 samples.size() * sizeof(short), fileInfo.samplerate);
    
    // Check for errors
    ALenum error = alGetError();
    if (error != AL_NO_ERROR) {
        spdlog::error("Failed to load sound data: {}", error);
        return;
    }
    
    m_loaded = true;
}

SoundBuffer::~SoundBuffer() {
    alDeleteBuffers(1, &m_bufferID);
}

// SoundSource implementation
SoundSource::SoundSource(bool looping) {
    alGenSources(1, &m_sourceID);
    alSourcef(m_sourceID, AL_PITCH, 1.0f);
    alSourcef(m_sourceID, AL_GAIN, 1.0f);
    alSource3f(m_sourceID, AL_POSITION, 0.0f, 0.0f, 0.0f);
    alSource3f(m_sourceID, AL_VELOCITY, 0.0f, 0.0f, 0.0f);
    alSourcei(m_sourceID, AL_LOOPING, looping ? AL_TRUE : AL_FALSE);
}

SoundSource::~SoundSource() {
    alDeleteSources(1, &m_sourceID);
}

void SoundSource::SetDistanceModel(ALenum model) {
    alDistanceModel(model);
}

void SoundSource::SetPosition(float x, float y, float z) {
    alGetError(); // Clear previous errors
    alSource3f(m_sourceID, AL_POSITION, x, y, z);
    ALenum error = alGetError();
    if (error != AL_NO_ERROR) {
        spdlog::error("OpenAL error setting source position: {}", error);
    }
}

void SoundSource::ConfigureAttenuation(float refDistance, float maxDistance, float rolloffFactor) {
    alGetError(); // Clear previous errors
    
    alSourcef(m_sourceID, AL_REFERENCE_DISTANCE, refDistance);
    ALenum error = alGetError();
    if (error != AL_NO_ERROR) {
        spdlog::error("OpenAL error setting reference distance: {}", error);
        return;
    }
    
    alSourcef(m_sourceID, AL_MAX_DISTANCE, maxDistance);
    error = alGetError();
    if (error != AL_NO_ERROR) {
        spdlog::error("OpenAL error setting max distance: {}", error);
        return;
    }
    
    alSourcef(m_sourceID, AL_ROLLOFF_FACTOR, rolloffFactor);
    error = alGetError();
    if (error != AL_NO_ERROR) {
        spdlog::error("OpenAL error setting rolloff factor: {}", error);
        return;
    }
}


void SoundSource::Play(const SoundBuffer& buffer) {
    // Clear any previous errors
    alGetError();
    
    if (!buffer.IsLoaded()) {
        spdlog::error("Attempted to play unloaded buffer");
        return;
    }
    
    // Get buffer ID
    ALuint bufferID = buffer.GetBufferID();
    
    // Verify buffer exists and has data
    ALint size = 0;
    alGetBufferi(bufferID, AL_SIZE, &size);
    
    ALenum error = alGetError();
    if (error != AL_NO_ERROR) {
        spdlog::error("Buffer validation failed: {}", error);
        return;
    }
    
    if (size <= 0) {
        spdlog::error("Buffer has no data (size = {})", size);
        return;
    }
    
    spdlog::info("Playing sound buffer: ID {}, size {} bytes", bufferID, size);
    
    // Stop any currently playing sound
    alSourceStop(m_sourceID);
    error = alGetError();
    if (error != AL_NO_ERROR) {
        spdlog::warn("Error stopping previous sound: {}", error);
        // Continue anyway
    }
    
    // Detach any previous buffer
    alSourcei(m_sourceID, AL_BUFFER, 0);
    error = alGetError();
    if (error != AL_NO_ERROR) {
        spdlog::warn("Error detaching previous buffer: {}", error);
        // Continue anyway
    }
    
    // Attach buffer to source
    alSourcei(m_sourceID, AL_BUFFER, bufferID);
    
    error = alGetError();
    if (error != AL_NO_ERROR) {
        spdlog::error("Failed to attach buffer to source: {}", error);
        return;
    }
    
    // Play the source
    alSourcePlay(m_sourceID);
    
    error = alGetError();
    if (error != AL_NO_ERROR) {
        spdlog::error("Failed to play source: {}", error);
        return;
    }
    
    // Check if it's actually playing
    ALint state;
    alGetSourcei(m_sourceID, AL_SOURCE_STATE, &state);
    spdlog::info("Source state after play: {}", 
        state == AL_PLAYING ? "PLAYING" : 
        state == AL_PAUSED ? "PAUSED" : 
        state == AL_STOPPED ? "STOPPED" : "UNKNOWN");
}

void SoundSource::Stop() {
    alSourceStop(m_sourceID);
}

void SoundSource::Pause() {
    alSourcePause(m_sourceID);
}

void SoundSource::Resume() {
    alSourcePlay(m_sourceID);
}

void SoundSource::SetVelocity(float x, float y, float z) {
    alSource3f(m_sourceID, AL_VELOCITY, x, y, z);
}

void SoundSource::SetPitch(float pitch) {
    alSourcef(m_sourceID, AL_PITCH, pitch);
}

void SoundSource::SetGain(float gain) {
    alSourcef(m_sourceID, AL_GAIN, gain);
}

void SoundSource::SetLooping(bool looping) {
    alSourcei(m_sourceID, AL_LOOPING, looping ? AL_TRUE : AL_FALSE);
}

// SoundManager implementation
SoundManager::SoundManager() 
    : m_device(nullptr)
    , m_context(nullptr)
    , m_initialized(false) {
}

SoundManager::~SoundManager() {
    Shutdown();
}

void SoundManager::Play(const std::string& soundName, bool looping, float volume) {
    auto buffer = GetSound(soundName);
    if (!buffer) {
        spdlog::error("Failed to play sound: sound not found");
        return;
    }
    
    auto source = std::make_shared<SoundSource>(looping);
    source->SetGain(volume);
    source->Play(*buffer);
    m_sources.push_back(source);
}

bool SoundManager::Initialize() {
    // Open the default audio device
    m_device = alcOpenDevice(nullptr);
    if (!m_device) {
        spdlog::error("Failed to open OpenAL device");
        return false;
    }
    
    // Create context
    m_context = alcCreateContext(m_device, nullptr);
    if (!m_context) {
        spdlog::error("Failed to create OpenAL context");
        alcCloseDevice(m_device);
        m_device = nullptr;
        return false;
    }
    
    // Make context current
    if (!alcMakeContextCurrent(m_context)) {
        spdlog::error("Failed to make OpenAL context current");
        alcDestroyContext(m_context);
        alcCloseDevice(m_device);
        m_context = nullptr;
        m_device = nullptr;
        return false;
    }
    
    m_initialized = true;
    spdlog::info("OpenAL initialized successfully");
    
   

    
    if (!m_initialized)
    {
        spdlog::critical("Failed to initialize sound manager");
        return false;
    }

    if (!LoadDefaultSounds())
    {
        spdlog::critical("Failed to load default sounds");
        return false;
    }

    return true;
}

void SoundManager::Shutdown() {
    if (!m_initialized)
        return;
        
    // Clear all sound buffers
    m_soundBuffers.clear();
    m_sources.clear();
    
    // Destroy context and close device
    alcMakeContextCurrent(nullptr);
    alcDestroyContext(m_context);
    alcCloseDevice(m_device);
    
    m_context = nullptr;
    m_device = nullptr;
    m_initialized = false;
    
    spdlog::info("OpenAL shutdown complete");
}

std::shared_ptr<SoundBuffer> SoundManager::LoadSound(const std::string& name, const std::string& filename) {
    // Check if sound is already loaded
    auto it = m_soundBuffers.find(name);
    if (it != m_soundBuffers.end()) {
        return it->second;
    }
    
    // Load new sound
    auto buffer = std::make_shared<SoundBuffer>(filename);
    if (!buffer->IsLoaded()) {
        return nullptr;
    }
    
    // Store and return
    m_soundBuffers[name] = buffer;
    return buffer;
}

std::shared_ptr<SoundBuffer> SoundManager::GetSound(const std::string& name) {
    auto it = m_soundBuffers.find(name);
    if (it != m_soundBuffers.end()) {
        return it->second;
    }
    return nullptr;
}

void SoundManager::SetListenerPosition(float x, float y, float z) {
    alListener3f(AL_POSITION, x, y, z);
}

void SoundManager::SetListenerVelocity(float x, float y, float z) {
    alListener3f(AL_VELOCITY, x, y, z);
}

void SoundManager::SetListenerOrientation(float atX, float atY, float atZ, float upX, float upY, float upZ) {
    float orientation[6] = { atX, atY, atZ, upX, upY, upZ };
    alListenerfv(AL_ORIENTATION, orientation);
}

bool SoundManager::LoadDefaultSounds() {
    // Clear any previous errors
    alGetError();

    // Set the distance attenuation model to linear distance clamped
    alDistanceModel(AL_LINEAR_DISTANCE_CLAMPED);

    // Load sounds
    auto backgroundMusic = LoadSound(
        "background_music",
        "/home/gabe/CLionProjects/cpp-engine/resources/sounds/quietmoments.wav");

    LoadSound(
        "birds",
        "/home/gabe/CLionProjects/cpp-engine/resources/sounds/bird_chirp.wav");

    Play("background_music", true, 1.0f);
        
    return true;
}

void SoundManager::CheckOpenALError(const char* operation) {
    ALenum error = alGetError();
    if (error != AL_NO_ERROR) {
        spdlog::error("OpenAL error after {}: {}", operation, error);
    }
}

void SoundManager::UpdateAudioEntities(entt::registry& registry, const Camera& camera) {
    // Clear any previous errors
    alGetError();

    // Update listener position and orientation based on camera
    glm::vec3 cameraPos = camera.GetPosition();
    glm::vec3 cameraFront = camera.GetFront();
    glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f); // Assuming Y-up world

    // Set listener position to camera position
    SetListenerPosition(cameraPos.x, cameraPos.y, cameraPos.z);
    CheckOpenALError("setting listener position");

    // Set listener orientation (at vector and up vector)
    SetListenerOrientation(
        cameraFront.x, cameraFront.y, cameraFront.z, // "At" vector (where camera is looking)
        cameraUp.x, cameraUp.y, cameraUp.z           // "Up" vector
    );
    CheckOpenALError("setting listener orientation");

    // Update all audio sources based on their entity transforms
    auto audioView = registry.view<Components::EntityMetadata, Components::Transform, Components::AudioSource>();

    for (auto [entity, metadata, transform, audio] : audioView.each()) {
        // Update audio source position based on entity transform
        if (audio.source) {
            // Get the world position from the transform
            glm::vec3 worldPos = transform.position;

            // Set the 3D position of the sound source
            audio.source->SetPosition(
                worldPos.x,
                worldPos.y,
                worldPos.z);
            CheckOpenALError("setting source position");

            // Ensure attenuation parameters are up-to-date
            audio.source->ConfigureAttenuation(
                audio.referenceDistance,
                audio.maxDistance,
                audio.rolloffFactor);
            CheckOpenALError("configuring attenuation");
            
            // Set the base volume (OpenAL will handle the attenuation)
            audio.source->SetGain(audio.volume);
            CheckOpenALError("setting gain");
        }
    }
}
    }}
