#include "AudioSystem.h"
#include "AudioClip.h"
#include "components/AudioSourceComponent.h"
#include "components/AudioListenerComponent.h"
#include "AudioReverb.h"
#include "AudioMixer.h"
#include "Scene.h"
#include "Entity.h"
#include "EventSystem.h"
#include "CubeGrid.h"
#include <algorithm>
#include <sstream>
#include <cmath>
#include <random>

// Constructor
AudioSystem::AudioSystem() :
    m_fmodSystem(nullptr),
    m_scene(nullptr),
    m_eventSystem(nullptr),
    m_cubeGrid(nullptr),
    m_activeListener(nullptr),
    m_initialized(false),
    m_enabled(true),
    m_masterVolume(1.0f),
    m_globalPitch(1.0f),
    m_nextChannelId(1),
    m_nextReverbZoneId(1),
    m_rayMode(AudioRayMode::NONE),
    m_maxBounces(1),
    m_raysPerSource(8),
    m_maxRayDistance(50.0f),
    m_distanceModel(AudioDistanceModel::INVERSE),
    m_occlusionLowpassCutoff(0.3f),
    m_occlusionVolumeAttenuation(0.5f),
    m_maxSources(64)
{
    // Set default material properties
    m_materials["default"] = {0.5f, 0.5f}; // Default absorption/reflection
    m_materials["stone"] = {0.2f, 0.8f};
    m_materials["wood"] = {0.4f, 0.6f};
    m_materials["metal"] = {0.1f, 0.9f};
    m_materials["glass"] = {0.1f, 0.9f};
    m_materials["fabric"] = {0.7f, 0.3f};
    m_materials["water"] = {0.3f, 0.7f};
}

// Destructor
AudioSystem::~AudioSystem()
{
    shutdown();
}

// Initialize the audio system
bool AudioSystem::initialize(Scene* scene, EventSystem* eventSystem)
{
    if (m_initialized)
        return true;

    m_scene = scene;
    m_eventSystem = eventSystem;

    // Initialize FMOD
    if (!initializeFMOD())
        return false;

    // Initialize default channel groups
    FMOD::ChannelGroup* masterGroup = nullptr;
    m_fmodSystem->getMasterChannelGroup(&masterGroup);
    m_channelGroups["Master"] = masterGroup;

    // Create standard channel groups
    FMOD::ChannelGroup* musicGroup = nullptr;
    FMOD::ChannelGroup* sfxGroup = nullptr;
    FMOD::ChannelGroup* voiceGroup = nullptr;
    FMOD::ChannelGroup* ambientGroup = nullptr;

    m_fmodSystem->createChannelGroup("Music", &musicGroup);
    m_fmodSystem->createChannelGroup("SFX", &sfxGroup);
    m_fmodSystem->createChannelGroup("Voice", &voiceGroup);
    m_fmodSystem->createChannelGroup("Ambient", &ambientGroup);

    masterGroup->addGroup(musicGroup);
    masterGroup->addGroup(sfxGroup);
    masterGroup->addGroup(voiceGroup);
    masterGroup->addGroup(ambientGroup);

    m_channelGroups["Music"] = musicGroup;
    m_channelGroups["SFX"] = sfxGroup;
    m_channelGroups["Voice"] = voiceGroup;
    m_channelGroups["Ambient"] = ambientGroup;

    // Create default mixer
    createMixer("DefaultMixer");

    m_initialized = true;
    return true;
}

// Initialize FMOD API
bool AudioSystem::initializeFMOD()
{
    // Create FMOD System
    FMOD_RESULT result = FMOD::System_Create(&m_fmodSystem);
    if (result != FMOD_OK)
    {
        // Failed to create FMOD system
        return false;
    }

    // Initialize FMOD
    result = m_fmodSystem->init(512, FMOD_INIT_NORMAL | FMOD_INIT_3D_RIGHTHANDED, nullptr);
    if (result != FMOD_OK)
    {
        // Failed to initialize FMOD
        m_fmodSystem->release();
        m_fmodSystem = nullptr;
        return false;
    }

    // Set up global settings
    m_fmodSystem->set3DSettings(1.0f, 1.0f, 1.0f); // Doppler scale, distance factor, rolloff scale

    return true;
}

// Shutdown the audio system
void AudioSystem::shutdown()
{
    if (!m_initialized)
        return;

    // Stop all sounds
    stopAll();

    // Release all audio clips
    m_audioClips.clear();

    // Release all reverb zones
    m_reverbZones.clear();

    // Release all channel groups
    m_channelGroups.clear();

    // Release all mixers
    m_mixers.clear();

    // Shutdown FMOD
    shutdownFMOD();

    m_initialized = false;
}

// Shutdown FMOD API
void AudioSystem::shutdownFMOD()
{
    if (m_fmodSystem)
    {
        m_fmodSystem->release();
        m_fmodSystem = nullptr;
    }
}

// Update audio system
void AudioSystem::update(float deltaTime)
{
    if (!m_initialized || !m_enabled)
        return;

    std::lock_guard<std::mutex> lock(m_audioMutex);

    // Update listener position
    updateListenerPosition();

    // Update reverb zones
    updateReverbZones();

    // Update reverb zones
    for (AudioSource* source : m_sources)
    {
        if (source->isPlaying() && source->getUseRaytracing())
        {
            if (m_rayMode != AudioRayMode::NONE)
            {
                calculateSoundPropagation(source);
            }
            else
            {
                // Just calculate occlusion for simple mode
                calculateOcclusion(source);
            }
        }
    }

    // Clean up stopped channels
    cleanupStoppedChannels();

    // Update all mixers
    for (auto& mixerPair : m_mixers)
    {
        mixerPair.second->update(deltaTime);
    }

    // Update FMOD
    m_fmodSystem->update();
}

// Set the cube grid for environmental audio calculations
void AudioSystem::setCubeGrid(CubeGrid* cubeGrid)
{
    m_cubeGrid = cubeGrid;
}

// Load an audio clip from file
AudioClip* AudioSystem::loadClip(const std::string& filename, bool streaming, bool is3D)
{
    if (!m_initialized)
        return nullptr;

    std::lock_guard<std::mutex> lock(m_audioMutex);

    // Check if clip is already loaded
    auto it = m_audioClips.find(filename);
    if (it != m_audioClips.end())
    {
        return it->second.get();
    }

    // Create new audio clip
    auto clip = std::make_unique<AudioClip>(filename, streaming, is3D);

    // Initialize the clip
    if (!clip->initialize(m_fmodSystem))
    {
        return nullptr;
    }

    // Store and return the clip
    AudioClip* result = clip.get();
    m_audioClips[filename] = std::move(clip);
    return result;
}

// Unload an audio clip
void AudioSystem::unloadClip(AudioClip* clip)
{
    if (!m_initialized || !clip)
        return;

    std::lock_guard<std::mutex> lock(m_audioMutex);

    // Find and remove the clip
    for (auto it = m_audioClips.begin(); it != m_audioClips.end(); it++)
    {
        if (it->second.get() == clip)
        {
            m_audioClips.erase(it);
            return;
        }
    }
}

// Create an audio source
AudioSource* AudioSystem::createSource(const std::string& name)
{
    if (!m_initialized)
        return nullptr;

    AudioSource* source = new AudioSource();
    return source;
}

// Destroy an audio source
void AudioSystem::destroySource(AudioSource* source)
{
    if (!m_initialized || !source)
        return;

    std::lock_guard<std::mutex> lock(m_audioMutex);

    // Unregister and delete the source
    unregisterSource(source);
    delete source;
}

// Set volume for an audio group
void AudioSystem::setGroupVolume(const std::string& groupName, float volume)
{
    if (!m_initialized)
        return;

    std::lock_guard<std::mutex> lock(m_audioMutex);

    // Get the channel group
    FMOD::ChannelGroup* group = getChannelGroup(groupName);
    if (group)
    {
        group->setVolume(volume);
    }
}

// Get volume for an audio group
float AudioSystem::getGroupVolume(const std::string& groupName) const
{
    if (!m_initialized)
        return 0.0f;

    std::lock_guard<std::mutex> lock(m_audioMutex);

    // Get the channel group
    auto it = m_channelGroups.find(groupName);
    if (it != m_channelGroups.end() && it->second)
    {
        float volume = 0.0f;
        it->second->getVolume(&volume);
        return volume;
    }

    return 0.0f;
}

// Play a sound directly
int AudioSystem::playSound(const std::string& clipName, const glm::vec3& position,
                           float volume, float pitch, bool loop)
{
    if (!m_initialized || !m_enabled)
        return 0;

    std::lock_guard<std::mutex> lock(m_audioMutex);

    // Get the audio clip
    AudioClip* clip = getClip(clipName);
    if (!clip)
        return 0;

    // Check if we're over the source limit
    if (isOverSourceLimit())
    {
        // Try to stop a low priority source
        AudioSource* sourceToStop = prioritizeSourceToStop();
        if (sourceToStop)
        {
            sourceToStop->stop();
        }
        else
        {
            // Can't play new sounds
            return 0;
        }
    }

    // Create a channel for the sound
    FMOD::Channel* channel = nullptr;
    FMOD_RESULT result = m_fmodSystem->playSound(clip->getSound(),
                                                 getChannelGroup(clip->getDefaultGroup()),
                                                 true, &channel);

    if (result != FMOD_OK || !channel)
        return 0;

    // Set up the channel
    channel->setVolume(volume);
    channel->setPitch(pitch);
    channel->setLoopCount(loop ? -1 : 0);

    // Set 3D attributes if needed
    if (clip->is3D())
    {
        FMOD_VECTOR fmodPos = {position.x, position.y, position.z};
        FMOD_VECTOR fmodVel = {0.0f, 0.0f, 0.0f};
        channel->set3DAttributes(&fmodPos, &fmodVel);
        channel->set3DMinMaxDistance(clip->getMinDistance(), clip->getMaxDistance());
    }

    // Assign a channel ID
    int channelId = m_nextChannelId++;
    m_activeChannels[channelId] = channel;

    // Start playing
    channel->setPaused(false);

    return channelId;
}

// Stop a sound instance
void AudioSystem::stopSound(int instanceId, bool fadeOut)
{
    if (!m_initialized || instanceId == 0)
        return;

    std::lock_guard<std::mutex> lock(m_audioMutex);

    // Find the channel
    auto it = m_activeChannels.find(instanceId);
    if (it != m_activeChannels.end() && it->second)
    {
        if (fadeOut)
        {
            // Fade out over a short time
            it->second->addFadePoint(0, 1.0f);
            it->second->addFadePoint(FMOD_TIMEUNIT_MS, 0.0f);
            it->second->setDelay(0, 0, true);
        }
        else
        {
            // Stop immediately
            it->second->stop();
        }

        // Remove from active channels (cleanup will handle this on next update)
    }
}

// Set listener position manually
void AudioSystem::setListenerPosition(const glm::vec3& position, const glm::vec3& forward, const glm::vec3& up)
{
    if (!m_initialized)
        return;

    // Convert  to FMOD vectors
    FMOD_VECTOR fmodPos = {position.x, position.y, position.z};
    FMOD_VECTOR fmodVel = {0.0f, 0.0f, 0.0f};
    FMOD_VECTOR fmodForward = {forward.x, forward.y, forward.z};
    FMOD_VECTOR fmodUp = {up.x, up.y, up.z};

    // Set 3D listener attributes
    m_fmodSystem->set3DListenerAttributes(0, &fmodPos, &fmodVel, &fmodForward, &fmodUp);
}

// Set global sound properties
void AudioSystem::setGlobalAudioProperties(float dopplerScale, float distanceFactor, float rolloffScale)
{
    if (!m_initialized)
        return;

    m_fmodSystem->set3DSettings(dopplerScale, distanceFactor, rolloffScale);
}

// Set environment preset
void AudioSystem::setEnvironmentPreset(const std::string& presetName)
{
    if (!m_initialized)
        return;

    // Find the default mixer
    AudioMixer* defaultMixer = getMixer("DefaultMixer");
    if (defaultMixer)
    {
        // Apply preset to mixer
        FMOD_REVERB_PROPERTIES props;

        // Set properties based on preset name
        if (presetName == "CAVE")
        {
            props = FMOD_PRESET_CAVE;
        }
        else if (presetName == "HALL")
        {
            props = FMOD_PRESET_CONCERTHALL;
        }
        else if (presetName == "ROOM")
        {
            props = FMOD_PRESET_ROOM;
        }
        else if (presetName == "BATHROOM")
        {
            props = FMOD_PRESET_BATHROOM;
        }
        else if (presetName == "FOREST")
        {
            props = FMOD_PRESET_FOREST;
        }
        else if (presetName == "MOUNTAINS")
        {
            props = FMOD_PRESET_MOUNTAINS;
        }
        else if (presetName == "PLAIN")
        {
            props = FMOD_PRESET_PLAIN;
        }
        else if (presetName == "UNDERWATER")
        {
            props = FMOD_PRESET_UNDERWATER;
        }
        else
        {
            // Default to generic
            props = FMOD_PRESET_GENERIC;
        }

        // Apply properties to the mixer
        defaultMixer->setReverbProperties(
            props.DecayTime,
            props.EarlyDelay,
            props.LateDelay,
            props.HFReference,
            props.HFDecayRatio,
            props.Diffusion,
            props.Density,
            props.LowShelfFrequency,
            props.LowShelfGain,
            props.HighCut,
            props.EarlyLateMix,
            props.WetLevel
        );
    }
}

// Apply a custom environment
void AudioSystem::setCustomEnvironment(float roomSize, float damping, float diffusion, float wetLevel)
{
    if (!m_initialized)
        return;

    // Find the default mixer
    AudioMixer* defaultMixer = getMixer("DefaultMixer");
    if (defaultMixer)
    {
        // Calculate reverb properties based on parameters
        float decayTime = 1.0f + roomSize * 5.0f; // 1-6 seconds
        float hfDecayRatio = 1.0f - damping;      // HF decay ratio

        // Apply properties to the mixer
        defaultMixer->setReverbProperties(
            decayTime,                  // Decay time
            0.02f,                      // Early delay
            0.03f,                      // Late delay
            5000.0f,                    // HF reference
            hfDecayRatio,               // HF decay ratio
            diffusion,                  // Diffusion
            1.0f,                       // Density
            250.0f,                     // Low shelf frequency
            0.0f,                       // Low shelf gain
            20000.0f,                   // High cut
            0.5f,                       // Early/late mix
            wetLevel                    // Wet level
        );
    }
}

// Set master volume
void AudioSystem::setMasterVolume(float volume)
{
    if (!m_initialized)
        return;

    std::lock_guard<std::mutex> lock(m_audioMutex);

    m_masterVolume = volume;

    // Apply to master channel group
    FMOD::ChannelGroup* masterGroup = getChannelGroup("Master");
    if (masterGroup)
    {
        masterGroup->setVolume(volume);
    }
}

// Get master volume
float AudioSystem::getMasterVolume() const
{
    return m_masterVolume;
}

// Set ray tracing mode
void AudioSystem::setRayTracingMode(AudioRayMode mode)
{
    m_rayMode = mode;
}

// Get ray tracing mode
AudioRayMode AudioSystem::getRayTracingMode() const
{
    return m_rayMode;
}

// Set ray tracing parameters
void AudioSystem::setRayTracingParameters(int maxBounces, int raysPerSource, float maxDistance)
{
    m_maxBounces = maxBounces;
    m_raysPerSource = raysPerSource;
    m_maxRayDistance = maxDistance;
}

// Set default distance attenuation model
void AudioSystem::setDistanceModel(AudioDistanceModel model)
{
    m_distanceModel = model;
}

// Pause all audio
void AudioSystem::pauseAll()
{
    if (!m_initialized)
        return;

    std::lock_guard<std::mutex> lock(m_audioMutex);

    // Pause master channel group
    FMOD::ChannelGroup* masterGroup = getChannelGroup("Master");
    if (masterGroup)
    {
        masterGroup->setPaused(true);
    }
}

// Resume all audio
void AudioSystem::resumeAll()
{
    if (!m_initialized)
        return;

    std::lock_guard<std::mutex> lock(m_audioMutex);

    // Resume master channel group
    FMOD::ChannelGroup* masterGroup = getChannelGroup("Master");
    if (masterGroup)
    {
        masterGroup->setPaused(false);
    }
}

// Stop all audio immediately
void AudioSystem::stopAll()
{
    if (!m_initialized)
        return;

    std::lock_guard<std::mutex> lock(m_audioMutex);

    // Stop all active channels
    for (auto& pair : m_activeChannels)
    {
        if (pair.second)
        {
            pair.second->stop();
        }
    }

    m_activeChannels.clear();
}

// Set sound occlusion parameters
void AudioSystem::setOcclusionParameters(float lowpassCutoff, float volumeAttenuation)
{
    m_occlusionLowpassCutoff = lowpassCutoff;
    m_occlusionVolumeAttenuation = volumeAttenuation;
}

void AudioSystem::getOcclusionParameters(float& outLowpassCutoff, float& outVolumeAttenuation)
{
    outLowpassCutoff = m_occlusionLowpassCutoff;
    outVolumeAttenuation = m_occlusionVolumeAttenuation;
}

// Create a reverb zone
int AudioSystem::createReverbZone(const glm::vec3& position, float radius, const std::string& preset)
{
    if (!m_initialized)
        return;

    std::lock_guard<std::mutex> lock(m_audioMutex);

    // Create a new reverb zone
    int id = m_nextReverbZoneId++;
    auto reverb = std::make_unique<AudioReverb>(id, position, radius, this);

    // Initialize and set preset
    if (reverb->initialize())
    {
        reverb->setPreset(preset);
        m_reverbZones.push_back(std::move(reverb));
        return id;
    }

    return 0;
}

// Remove a reverb zone
void AudioSystem::removeReverbZone(int zoneId)
{
    if (!m_initialized || zoneId == 0)
        return;

    std::lock_guard<std::mutex> lock(m_audioMutex);

    // Find and remove the zone
    for (auto it = m_reverbZones.begin(); it != m_reverbZones.end(); it++)
    {
        if ((*it)->getId() == zoneId)
        {
            m_reverbZones.erase(it);
            return;
        }
    }
}

// Create an audio mixer
AudioMixer* AudioSystem::createMixer(const std::string& name)
{
    if (!m_initialized)
        return nullptr;

    std::lock_guard<std::mutex> lock(m_audioMutex);

    // Check if mixer already exists
    auto it = m_mixers.find(name);
    if (it != m_mixers.end())
    {
        return it->second.get();
    }

    // Create new mixer
    auto mixer = std::make_unique<AudioMixer>(name, this);

    // Initialize the mixer
    if (!mixer->initialize())
    {
        return nullptr;
    }

    // Store and return the mixer
    AudioMixer* result = mixer.get();
    m_mixers[name] = std::move(mixer);
    return result;
}

// Get an audio mixer by name
AudioMixer* AudioSystem::getMixer(const std::string& name) const
{
    if (!m_initialized)
        return nullptr;

    auto it = m_mixers.find(name);
    if (it != m_mixers.end())
    {
        return it->second.get();
    }

    return nullptr;
}

// Define a new audio group
void AudioSystem::defineAudioGroup(const std::string& groupName, const std::string& parentName)
{
    if (!m_initialized || groupName.empty())
        return;

    std::lock_guard<std::mutex> lock(m_audioMutex);

    // Check if group already exists
    if (m_channelGroups.find(groupName) != m_channelGroups.end())
        return;

    // Get parent group
    FMOD::ChannelGroup* parentGroup = nullptr;
    if (parentName.empty())
    {
        // Use master group if no parent specified
        parentGroup = getChannelGroup("Master");
    }
    else
    {
        parentGroup = getChannelGroup(parentName);
    }

    if (!parentGroup)
        return;

    // Create new channel group
    FMOD::ChannelGroup* newGroup = nullptr;
    FMOD_RESULT result = m_fmodSystem->createChannelGroup(groupName.c_str(), &newGroup);

    if (result == FMOD_OK && newGroup)
    {
        // Add to parent group
        parentGroup->addGroup(newGroup, false, nullptr);

        // Store the group
        m_channelGroups[groupName] = newGroup;
    }
}

// Set parameters for a specific material type
void AudioSystem::defineMaterialProperties(const std::string& materialName, float absorption, float reflection)
{
    if (materialName.empty())
        return;

    m_materials[materialName] = {absorption, reflection};
}

// Set global lowpass filter
void AudioSystem::setGlobalLowpassFilter(float cutoff)
{
    if (!m_initialized)
        return;

    // Find the default mixer
    AudioMixer* defaultMixer = getMixer("DefaultMixer");
    if (defaultMixer)
    {
        defaultMixer->setMasterLowpassFilter(cutoff);
    }
}

// Set global highpass filter
void AudioSystem::setGlobalHighpassFilter(float cutoff)
{
    if (!m_initialized)
        return;

    // Find the default mixer
    AudioMixer* defaultMixer = getMixer("DefaultMixer");
    if (defaultMixer)
    {
        defaultMixer->setMasterHighpassFilter(cutoff);
    }
}

// Cast an audio ray for sound propagation
bool AudioSystem::castAudioRay(const glm::vec3& origin, const glm::vec3& direction,
                               float maxDistance, AudioRayHit& hitInfo)
{
    if (!m_cubeGrid)
        return false;

    // Initialize hit info
    hitInfo.hasHit = false;
    hitInfo.distance = maxDistance;

    // Step through the grid
    float stepSize = m_cubeGrid->getSpacing() * 0.5f;
    glm::vec3 currentPos = origin;
    glm::vec3 step = direction * stepSize;
    float distanceTraveled = 0.0f;

    while (distanceTraveled < maxDistance)
    {
        // Get grid position
        glm::ivec3 gridPos = m_cubeGrid->worldToGridCoordinates(currentPos);

        // Check if there's a cube here
        if (m_cubeGrid->isCubeActive(gridPos.x, gridPos.y, gridPos.z))
        {
            // We hit something
            hitInfo.hasHit = true;
            hitInfo.point = currentPos;
            hitInfo.distance = distanceTraveled;

            // Get cube data
            const Cube& cube = m_cubeGrid->getCube(gridPos.x, gridPos.y, gridPos.z);

            // Calculate surface normal (simplified - just the major axis)
            float dx = std::abs(currentPos.x - (cube.position.x + m_cubeGrid->getSpacing() * 0.5f));
            float dy = std::abs(currentPos.y - (cube.position.y + m_cubeGrid->getSpacing() * 0.5f));
            float dz = std::abs(currentPos.z - (cube.position.z + m_cubeGrid->getSpacing() * 0.5f));

            if (dx > dy && dx > dz)
            {
                hitInfo.normal = glm::vec3(currentPos.x < cube.position.x ? -1.0f : 1.0f, 0.0f, 0.0f);
            }
            else if (dy > dx && dy > dz)
            {
                hitInfo.normal = glm::vec3(0.0f, currentPos.y < cube.position.y ? -1.0f : 1.0f, 0.0f);
            }
            else
            {
                hitInfo.normal = glm::vec3(0.0f, 0.0f, currentPos.z < cube.position.z ? -1.0f : 1.0f);
            }

            // Set material properties
            // For now, we use a default material
            auto materialProps = m_materials.find("default");
            if (materialProps != m_materials.end())
            {
                hitInfo.absorption = materialProps->second.absorption;
                hitInfo.reflection = materialProps->second.reflection;
            }

            return true;
        }

        // Step forward
        currentPos += step;
        distanceTraveled += stepSize;
    }

    // No hit
    return false;
}

// Get the number of active sound instances
int AudioSystem::getActiveInstanceCount() const
{
    return static_cast<int>(m_activeChannels.size());
}

// Get the number of active audio sources
int AudioSystem::getActiveSourceCount() const
{
    // Count only playing sources
    int count = 0;
    for (AudioSource* source : m_sources)
    {
        if (source->isPlaying())
        {
            count++;
        }
    }
    return count;
}

// Set global audio pitch
void AudioSystem::setGlobalPitch(float pitch)
{
    if (!m_initialized)
        return;

    m_globalPitch = pitch;

    // Apply to master channel group
    FMOD::ChannelGroup* masterGroup = getChannelGroup("Master");
    if (masterGroup)
    {
        masterGroup->setPitch(pitch);
    }
}

// Get global audio pitch
float AudioSystem::getGlobalPitch() const
{
    return m_globalPitch;
}

// Check if an audio clip is loaded
bool AudioSystem::isClipLoaded(const std::string& filename) const
{
    return m_audioClips.find(filename) != m_audioClips.end();
}

// Get an audio clip by name
AudioClip* AudioSystem::getClip(const std::string& filename) const
{
    auto it = m_audioClips.find(filename);
    if (it != m_audioClips.end())
    {
        return it->second.get();
    }

    return nullptr;
}

// Register a listener component
void AudioSystem::registerListener(AudioListener* listener)
{
    if (!m_initialized || !listener)
        return;

    std::lock_guard<std::mutex> lock(m_audioMutex);

    // Add to listeners list if not already present
    if (std::find(m_listeners.begin(), m_listeners.end(), listener) == m_listeners.end())
    {
        m_listeners.push_back(listener);

        // If this is the first listener, make it active
        if (m_listeners.size() == 1 || listener->isActive())
        {
            m_activeListener = listener;

            // Update other listeners' active state
            for (AudioListener* l : m_listeners)
            {
                if (l != listener)
                {
                    l->setActive(false);
                }
            }
        }
    }
}

// Unregister a listener component
void AudioSystem::unregisterListener(AudioListener* listener)
{
    if (!m_initialized || !listener)
        return;

    std::lock_guard<std::mutex> lock(m_audioMutex);

    // Remove from listeners list
    auto it = std::find(m_listeners.begin(), m_listeners.end(), listener);
    if (it != m_listeners.end())
    {
        // If this was the active listener, find a new one
        if (m_activeListener == listener)
        {
            m_listeners.erase(it);

            // Set first remaining listener as active, if any
            if (!m_listeners.empty())
            {
                m_activeListener = m_listeners[0];
                m_activeListener->setActive(true);
            }
            else
            {
                m_activeListener = nullptr;
            }
        }
        else
        {
            m_listeners.erase(it);
        }
    }
}

// Register a source component
void AudioSystem::registerSource(AudioSource* source)
{
    if (!m_initialized || !source)
        return;

    std::lock_guard<std::mutex> lock(m_audioMutex);

    // Add to source list if not already present
    if (std::find(m_sources.begin(), m_sources.end(), source) == m_sources.end())
    {
        m_sources.push_back(source);
    }
}

// Unregister a source component
void AudioSystem::unregisterSource(AudioSource* source)
{
    if (!m_initialized || !source)
        return;

    std::lock_guard<std::mutex> lock(m_audioMutex);

    // Stop the source if playing
    if (source->isPlaying())
    {
        source->stop();
    }

    // Remove from sources list
    auto it = std::find(m_sources.begin(), m_sources.end(), source);
    if (it != m_sources.end())
    {
        m_sources.erase(it);
    }
}

// Set whether audio is enabled
void AudioSystem::setEnabled(bool enabled)
{
    if (m_enabled == enabled)
        return;

    m_enabled = enabled;

    if (!enabled)
    {
        // Pause all sounds
        pauseAll();
    }
    else if (m_initialized)
    {
        // Resume sounds
        resumeAll();
    }
}

// Check if audio is enabled
bool AudioSystem::isEnabled() const
{
    return m_enabled;
}

// Set max number of audio sources that can play simultaneously
void AudioSystem::setMaxSources(int maxSources)
{
    m_maxSources = maxSources;
}

// Get max number of audio sources that can play simultaneously
int AudioSystem::getMaxSources() const
{
    return m_maxSources;
}

// Get FMOD system
FMOD::System* AudioSystem::getFMODSystem()
{
    return m_fmodSystem;
}

// Get a channel group by name
FMOD::ChannelGroup* AudioSystem::getChannelGroup(const std::string& name)
{
    auto it = m_channelGroups.find(name);
    if (it != m_channelGroups.end())
    {
        return it->second;
    }

    // Default to master group if not found
    auto masterIt = m_channelGroups.find("Master");
    if (masterIt != m_channelGroups.end())
    {
        return masterIt->second;
    }

    return nullptr;
}

// Private helper methods

// Update listener position
void AudioSystem::updateListenerPosition()
{
    if (!m_activeListener)
        return;

    // Get listener properties
    glm::vec3 position = m_activeListener->getPosition();
    glm::vec3 forward = m_activeListener->getForward();
    glm::vec3 up = m_activeListener->getUp();
    glm::vec3 velocity = m_activeListener->getVelocity();

    // Set 3D listener attributes
    FMOD_VECTOR fmodPosition = {position.x, position.y, position.z};
    FMOD_VECTOR fmodVelocity = {velocity.x, velocity.y, velocity.z};
    FMOD_VECTOR fmodForward = {forward.x, forward.y, forward.z};
    FMOD_VECTOR fmodUp = {up.x, up.y, up.z};

    m_fmodSystem->set3DListenerAttributes(0, &fmodPosition, &fmodVelocity, &fmodForward, &fmodUp);
}

// Update reverb zones
void AudioSystem::updateReverbZones()
{
    if (m_reverbZones.empty() || !m_activeListener)
        return;

    // Get listener position
    glm::vec3 listenerPos = m_activeListener->getPosition();

    // Update each reverb zone
    for (auto& reverb : m_reverbZones)
    {
        reverb->update(listenerPos);
    }
}

// Calculate sound propagation from source to listener
void AudioSystem::calculateSoundPropagation(AudioSource* source)
{
    if (!m_activeListener || !m_cubeGrid)
        return;

    // Get source and listener positions
    glm::vec3 sourcePos = source->getEntity()->getWorldPosition() + source->getPositionOffset();
    glm::vec3 listenerPos = m_activeListener->getPosition();

    // Direct path check (for occlusion)
    glm::vec3 dirToListener = listenerPos - sourcePos;
    float distance = glm::length(dirToListener);

    // Skip if too far away
    if (distance > m_maxRayDistance)
    {
        source->setOcclusionFactor(0.0f);
        return;
    }

    // Normalize direction
    dirToListener = glm::normalize(dirToListener);

    // Cast ray from source to listener
    AudioRayHit hit;
    bool occluded = castAudioRay(sourcePos, dirToListener, distance, hit);

    // Set occlusion factor
    float  occlusionFactor = occluded ? 1.0f : 0.0f;

    // For REFLECTION mode or higher, calculate reflections
    if (m_rayMode >= AudioRayMode::REFLECTION && source->getMaxReflections() > 0)
    {
        // Random number generator for ray directions
        static std::mt19937 gen(std::random_device{}());
        static std::uniform_real_distribution<float> dist(-1.0f, 1.0f);

        // Number of reflections to calculate
        int reflectionCount = std::min(source->getMaxReflections(), m_maxBounces);

        // Number of rays to shoot
        int rayCount = m_raysPerSource;

        // For each ray
        for (int i = 0; i < rayCount; i++)
        {
            // Generate a random ray direction (spherical distribution)
            glm::vec3 rayDir;
            do
            {
                rayDir = glm::vec3(dist(gen), dist(gen), dist(gen));
            }
            while (glm::dot(rayDir, rayDir) > 1.0f);

            rayDir = glm::normalize(rayDir);

            // Start from source position
            glm::vec3 rayPos = sourcePos;
            glm::vec3 rayDirection = rayDir;
            float totalReflectionCoeff = 1.0f;

            // For each bounce
            for (int bounce = 0; bounce < reflectionCount; bounce++)
            {
                // Cast ray
                AudioRayHit bounceHit;
                if (!castAudioRay(rayPos, rayDirection, m_maxRayDistance, bounceHit))
                {
                    // No hit, ray went into the void
                    break;
                }

                // Move to hit point
                rayPos = bounceHit.point;

                // Calculate reflection direction
                rayDirection = glm::reflect(rayDirection, bounceHit.normal);

                // Reduce energy based on surface absorption
                totalReflectionCoeff *= bounceHit.reflection;

                // Cast ray from reflection point to listener
                glm::vec3 toListener = listenerPos - rayPos;
                float distToListener = glm::length(toListener);

                // Skip if too far
                if (distToListener > m_maxRayDistance)
                    break;

                toListener = glm::normalize(toListener);

                // Check if there's a clear path to listener
                AudioRayHit listenerHit;;
                if (!castAudioRay(rayPos, toListener, distToListener, listenerHit))
                {
                    // Clear path to listener - we have a reflection path!

                    // Calculate total path length (source to bounce point to listener)
                    float pathLength = bounceHit.distance + distToListener;

                    // Calculate attenuation based on distance
                    float distanceAttenuation = calculateAttenuation(
                        pathLength,
                        source->getMinDistance(),
                        source->getMaxDistance()
                    );

                    // Reduce occlusion factor based on reflection strength
                    // This simulates sound reaching the listener through reflections
                    float reflectionStrength = totalReflectionCoeff * distanceAttenuation;
                    occlusionFactor -= reflectionStrength * 0.3f; // Scale factor for reflection effectiveness

                    // Ensure occlusion doesn't go negative
                    occlusionFactor = std::max(0.0f, occlusionFactor);

                    // Don't need more bounces if we found a path
                    break;
                }

                // No clear path to listener, continue with next bounce
            }
        }
    }

    // Apply the occlusion factor
    source->setOcclusionFactor(occlusionFactor);
}

// Calculate occlusion between source and listener
void AudioSystem::calculateOcclusion(AudioSource* source)
{
    if (!m_activeListener || !m_cubeGrid)
        return;

    // Get source and listener positions
    glm::vec3 sourcePos = source->getEntity()->getWorldPosition() + source->getPositionOffset();
    glm::vec3 listenerPos = m_activeListener->getPosition();

    // Direct path check
    glm::vec3 dirToListener = listenerPos - sourcePos;
    float distance = glm::length(dirToListener);

    // Skip if too far away
    if (distance > m_maxRayDistance)
    {
        source->setOcclusionFactor(0.0f);
        return;
    }

    // Normalize direction
    dirToListener = glm::normalize(dirToListener);

    // Cast ray from source to listener
    AudioRayHit hit;
    bool occluded = castAudioRay(sourcePos, dirToListener, distance, hit);

    // Set occlusion factor
    float occlusionFactor = occluded ? 1.0f : 0.0f;
    source->setOcclusionFactor(occlusionFactor);
}

// Calculate distance attenuation based on model
float AudioSystem::calculateAttenuation(float distance, float minDistance, float maxDistance)
{
    // Clamp distance between min and max
    distance = std::max(minDistance, std::min(distance, maxDistance));

    // Calculate attenuation based on model
    float attenuation = 1.0f;

    switch (m_distanceModel)
    {
        case AudioDistanceModel::LINEAR:
            // Linear attenuation
            attenuation = 1.0f - (distance - minDistance) / (maxDistance - minDistance);
            break;

        case AudioDistanceModel::INVERSE:
            // Inverse distance model (1/d)
            attenuation = minDistance / distance;
            break;

        case AudioDistanceModel::EXPONENTIAL:
            // Exponential attenuation
            attenuation = std::pow(distance / minDistance, -1.0f);
            break;

        case AudioDistanceModel::CUSTOM:
            // Custom curve (logarithmic in this case)
            attenuation = 1.0f / (1.0f + 0.5f * std::log(distance / minDistance));
            break;
    }

    // Clamp result between 0 and 1
    return std::max(0.0f, std::min(1.0f, attenuation));
}

// Clean up stopped channels
void AudioSystem::cleanupStoppedChannels()
{
    // Temporary list of channels to remove
    std::vector<int> channelsToRemove;

    // Check each active channel
    for (const auto& pair : m_activeChannels)
    {
        if (pair.second)
        {
            bool isPlaying = false;
            pair.second->isPlaying(&isPlaying);

            if (!isPlaying)
            {
                // Channel is no longer playing
                channelsToRemove.push_back(pair.first);
            }
        }
        else
        {
            // Null channel
            channelsToRemove.push_back(pair.first);
        }
    }

    // Remove stopped channels
    for (int id : channelsToRemove)
    {
        m_activeChannels.erase(id);
    }
}

// Check if we're over the source limit
bool AudioSystem::isOverSourceLimit() const
{
    return getActiveInstanceCount() >= m_maxSources;
}

// Find a source to stop when we're over the limit
AudioSource* AudioSystem::prioritizeSourceToStop()
{
    AudioSource* lowestPrioritySource = nullptr;
    int lowestPriority = -1;

    // Find a non-looping source with the lowest priority
    for (AudioSource* source : m_sources)
    {
        if (source->isPlaying() && !source->getLoop())
        {
            int priority = source->getPriority();
            if (lowestPrioritySource == nullptr || priority > lowestPriority)
            {
                lowestPrioritySource = source;
                lowestPriority = priority;
            }
        }
    }

    // If no non-looping sources, consider looping ones too
    if (lowestPrioritySource == nullptr)
    {
        for (AudioSource* source : m_sources)
        {
            if (source->isPlaying())
            {
                int priority = source->getPriority();
                if (lowestPrioritySource == nullptr || priority > lowestPriority)
                {
                    lowestPrioritySource = source;
                    lowestPriority = priority;
                }
            }
        }
    }

    return lowestPrioritySource;
}