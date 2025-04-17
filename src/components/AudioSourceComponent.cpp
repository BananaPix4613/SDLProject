#include "components/AudioSourceComponent.h"
#include "AudioSystem.h"
#include "AudioClip.h"
#include "Entity.h"
#include "Scene.h"
#include <fmod.hpp>

// Constructor
AudioSource::AudioSource() :
    m_clip(nullptr),
    m_channel(nullptr),
    m_channelId(0),
    m_isPlaying(false),
    m_isPaused(false),
    m_isInitialized(false),
    m_volume(1.0f),
    m_pitch(1.0f),
    m_loop(false),
    m_priority(128),
    m_group("SFX"),
    m_ignoreListenerPause(false),
    m_is3D(true),
    m_minDistance(1.0f),
    m_maxDistance(10000.0f),
    m_innerConeAngle(360.0f),
    m_outerConeAngle(360.0f),
    m_outerConeVolume(0.0f),
    m_dopplerScale(1.0f),
    m_spread(0.0f),
    m_velocity(0.0f),
    m_positionOffset(0.0f),
    m_useRaytracing(false),
    m_maxReflections(1),
    m_occlusionFactor(0.0f),
    m_reverbSend(0.0f),
    m_distortion(0.0f),
    m_lowpass(1.0f),
    m_audioSystem(nullptr)
{
}

// Destructor
AudioSource::~AudioSource()
{
    if (m_isPlaying)
        stop();

    if (m_isInitialized && m_audioSystem)
    {
        m_audioSystem->unregisterSource(this);
    }
}

// Initialize component
void AudioSource::initialize()
{
    if (m_isInitialized)
        return;

    // Get the audio system from the scene
    Scene* scene = getScene();
    if (!scene)
        return;

    m_audioSystem = static_cast<AudioSystem*>(scene->getSystem("AudioSystem"));
    if (!m_audioSystem)
        return;

    // Register with the audio system
    m_audioSystem->registerSource(this);

    m_isInitialized = true;
}

// Update component
void AudioSource::update(float deltaTime)
{
    if (!m_isInitialized || !m_isPlaying || !m_channel)
        return;

    // Update 3D attributes based on entity transform
    update3DAttributes();

    // Update effects based on occlusion
    updateEffects();

    // Check if sound has stopped playing
    bool isPlaying = false;
    m_channel->isPlaying(&isPlaying);

    if (!isPlaying)
    {
        m_isPlaying = false;
        m_isPaused = false;
        m_channel = nullptr;
    }
}

// Clean up resources when destroyed
void AudioSource::onDestroy()
{
    if (m_isPlaying)
        stop();

    if (m_isInitialized && m_audioSystem)
    {
        m_audioSystem->unregisterSource(this);
    }
}

// React to entity transform changes
void AudioSource::onTransformChanged()
{
    if (m_isInitialized && m_isPlaying && m_channel && m_is3D)
    {
        update3DAttributes();
    }
}

// Get component type name
const char* AudioSource::getTypeName() const
{
    return "AudioSource";
}

// Set audio clip
void AudioSource::setClip(AudioClip* clip)
{
    // Stop any current playback
    if (m_isPlaying)
        stop();

    m_clip = clip;

    // Set default properties from clip
    if (m_clip)
    {
        m_is3D = m_clip->is3D();
        m_minDistance = m_clip->getMinDistance();
        m_maxDistance = m_clip->getMaxDistance();
        m_loop = m_clip->getLoop();
        m_volume = m_clip->getDefaultVolume();
        m_pitch = m_clip->getDefaultPitch();
        m_priority = m_clip->getDefaultPriority();
        m_group = m_clip->getDefaultGroup();
    }
}

// Get current audio clip
AudioClip* AudioSource::getClip() const
{
    return m_clip;
}

// Play the audio source
void AudioSource::play()
{
    if (!m_isInitialized || !m_audioSystem || !m_clip)
        return;

    // Stop if already playing
    if (m_isPlaying)
        stop();

    // Get FMOD system
    FMOD::System* system = m_audioSystem->getFMODSystem();
    if (!system)
        return;

    // Get the channel group
    FMOD::ChannelGroup* group = nullptr;
    system->getChannelGroup(m_group.c_str(), &group);

    // Create a channel for the sound
    FMOD_RESULT result = system->playSound(m_clip->getSound(), group, true, &m_channel);

    if (result != FMOD_OK || !m_channel)
    {
        m_isPlaying = false;
        m_channel = nullptr;
        return;
    }

    // Apply settings to the channel
    applyChannelSettings();

    // Set 3D attributes
    if (m_is3D)
    {
        update3DAttributes();
    }

    // Start playing
    m_isPlaying = true;
    m_isPaused = false;
    m_channel->setPaused(false);
}

// Stop the audio source
void AudioSource::stop(bool fadeOut)
{
    if (!m_isInitialized || !m_isPlaying || !m_channel)
        return;

    if (fadeOut)
    {
        // Fade out over a short time (250ms)
        m_channel->addFadePoint(0, m_volume);
        m_channel->addFadePoint(250, 0.0f);
        m_channel->setDelay(0, 250, true);
    }
    else
    {
        // Stop immediately
        m_channel->stop();
    }

    m_isPlaying = false;
    m_isPaused = false;
    m_channel = nullptr;
}

// Pause the audio source
void AudioSource::pause()
{
    if (!m_isInitialized || !m_isPlaying || !m_channel || m_isPaused)
        return;

    m_channel->setPaused(true);
    m_isPaused = true;
}

// Resume the audio source
void AudioSource::resume()
{
    if (!m_isInitialized || !m_isPlaying || !m_channel || !m_isPaused)
        return;

    m_channel->setPaused(false);
    m_isPaused = false;
}

// Set volume
void AudioSource::setVolume(float volume)
{
    m_volume = volume;

    if (m_isPlaying && m_channel)
    {
        m_channel->setVolume(volume);
    }
}

// Get volume
float AudioSource::getVolume() const
{
    return m_volume;
}

// Set pitch
void AudioSource::setPitch(float pitch)
{
    m_pitch = pitch;

    if (m_isPlaying && m_channel)
    {
        m_channel->setPitch(pitch);
    }
}

// Get pitch
float AudioSource::getPitch() const
{
    return m_pitch;
}

// Set whether this source is looping
void AudioSource::setLoop(bool loop)
{
    m_loop = loop;

    if (m_isPlaying && m_channel)
    {
        m_channel->setLoopCount(loop ? -1 : 0);
    }
}

// Get loop state
bool AudioSource::getLoop() const
{
    return m_loop;
}

// Set the priority of this source
void AudioSource::setPriority(int priority)
{
    m_priority = priority;

    if (m_isPlaying && m_channel)
    {
        m_channel->setPriority(priority);
    }
}

// Get priority
int AudioSource::getPriority() const
{
    return m_priority;
}

// Set whether this is a 3D positional sound
void AudioSource::set3D(bool is3D)
{
    // Can't change 3D mode while playing
    if (m_isPlaying)
        return;

    m_is3D = is3D;
}

// Check if this is a 3D positional sound
bool AudioSource::is3D() const
{
    return m_is3D;
}

// Set minimum and maximum distance for 3D attenuation
void AudioSource::setDistanceRange(float minDistance, float maxDistance)
{
    m_minDistance = minDistance;
    m_maxDistance = maxDistance;

    if (m_isPlaying && m_channel && m_is3D)
    {
        m_channel->set3DMinMaxDistance(minDistance, maxDistance);
    }
}

// Get minimum distance
float AudioSource::getMinDistance() const
{
    return m_minDistance;
}

// Get maximum distance
float AudioSource::getMaxDistance() const
{
    return m_maxDistance;
}

// Set sound cone angles for directional audio
void AudioSource::setConeSettings(float innerAngle, float outerAngle, float outerVolume)
{
    m_innerConeAngle = innerAngle;
    m_outerConeAngle = outerAngle;

    if (m_isPlaying && m_channel && m_is3D)
    {
        m_channel->set3DConeSettings(innerAngle, outerAngle, outerVolume);
    }
}

// Get cone settings
void AudioSource::getConeSettings(float& outInnerAngle, float& outOuterAngle, float& outOuterVolume) const
{
    outInnerAngle = m_innerConeAngle;
    outOuterAngle = m_outerConeAngle;
    outOuterVolume = m_outerConeVolume;
}

// Set Doppler scale for this source
void AudioSource::setDopplerScale(float dopplerScale)
{
    m_dopplerScale = dopplerScale;

    if (m_isPlaying && m_channel && m_is3D)
    {
        // FMOD doesn't have per-source Doppler scale, so we'd need to use DSP effects
        // to simulate this. For simplicity, we're just storing the value.
    }
}

// Get Doppler scale
float AudioSource::getDopplerScale() const
{
    return m_dopplerScale;
}

// Set spread angle for 3D panning
void AudioSource::setSpread(float spread)
{
    m_spread = spread;

    if (m_isPlaying && m_channel && m_is3D)
    {
        m_channel->set3DSpread(spread);
    }
}

// Get spread angle
float AudioSource::getSpread() const
{
    return m_spread;
}

// Set occlusion factor manually
void AudioSource::setOcclusionFactor(float occlusionFactor)
{
    m_occlusionFactor = occlusionFactor;

    // Update effects based on new occlusion factor
    if (m_isPlaying && m_channel)
    {
        updateEffects();
    }
}

// Get current occlusion factor
float AudioSource::getOcclusionFactor() const
{
    return m_occlusionFactor;
}

// Check if audio is currently playing
bool AudioSource::isPlaying() const
{
    return m_isPlaying;
}

// Check if audio is currently paused
bool AudioSource::isPaused() const
{
    return m_isPaused;
}

// Get current playback position
float AudioSource::getCurrentTime() const
{
    if (!m_isPlaying || !m_channel)
        return 0.0f;

    unsigned int position = 0;
    m_channel->getPosition(&position, FMOD_TIMEUNIT_MS);

    return position / 1000.0f;
}

// Set playback position
void AudioSource::setCurrentTime(float time)
{
    if (!m_isPlaying || !m_channel)
        return;

    unsigned int positionMs = static_cast<unsigned int>(time * 1000.0f);
    m_channel->setPosition(positionMs, FMOD_TIMEUNIT_MS);
}

// Set velocity for Doppler effect calculation
void AudioSource::setVelocity(const glm::vec3& velocity)
{
    m_velocity = velocity;

    if (m_isPlaying && m_channel && m_is3D)
    {
        update3DAttributes();
    }
}

// Get velocity
glm::vec3 AudioSource::getVelocity() const
{
    return m_velocity;
}

// Set audio group
void AudioSource::setGroup(const std::string& groupName)
{
    // Can't change group while playing
    if (m_isPlaying)
        return;

    m_group = groupName;
}

// Get audio group
const std::string& AudioSource::getGroup() const
{
    return m_group;
}

// Set whether source ignores listener pausing
void AudioSource::setIgnoreListenerPause(bool ignoreListenerPause)
{
    m_ignoreListenerPause = ignoreListenerPause;
}

// Get whether source ignores listener pausing
bool AudioSource::getIgnoreListenerPause() const
{
    return m_ignoreListenerPause;
}

// Set whether source uses raytraced audio
void AudioSource::setUseRaytracing(bool useRaytracing)
{
    m_useRaytracing = useRaytracing;
}

// Get whether source uses raytraced audio
bool AudioSource::getUseRaytracing() const
{
    return m_useRaytracing;
}

// Set max number of reflections for this source
void AudioSource::setMaxReflections(int maxReflections)
{
    m_maxReflections = maxReflections;
}

// Get max number of reflections
int AudioSource::getMaxReflections() const
{
    return m_maxReflections;
}

// Set effect parameters
void AudioSource::setEffectParameters(float reverbSend, float distortion, float lowpass)
{
    m_reverbSend = reverbSend;
    m_distortion = distortion;
    m_lowpass = lowpass;

    if (m_isPlaying && m_channel)
    {
        updateEffects();
    }
}

// Get effect parameters
void AudioSource::getEffectParameters(float& outReverbSend, float& outDistortion, float& outLowpass) const
{
    outReverbSend = m_reverbSend;
    outDistortion = m_distortion;
    outLowpass = m_lowpass;
}

// Internal use - get FMOD channel
FMOD::Channel* AudioSource::getChannel() const
{
    return m_channel;
}

// Internal use - set FMOD channel
void AudioSource::setChannel(FMOD::Channel* channel)
{
    m_channel = channel;
}

// Set position offset from entity
void AudioSource::setPositionOffset(const glm::vec3& offset)
{
    m_positionOffset = offset;

    if (m_isPlaying && m_channel && m_is3D)
    {
        update3DAttributes();
    }
}

// Get position offset
glm::vec3 AudioSource::getPositionOffset() const
{
    return m_positionOffset;
}

// Play a one-shot sound without changing the assigned clip
bool AudioSource::playOneShot(AudioClip* clip, float volume)
{
    if (!m_isInitialized || !m_audioSystem || !clip)
        return false;

    // Just use the AudioSystem's direct play method
    int instanceId = m_audioSystem->playSound(
        clip->getFilename(),
        getEntity()->getWorldPosition() + m_positionOffset,
        volume * m_volume,
        m_pitch,
        false // One-shot sounds don't loop
    );

    return instanceId != 0;
}

// Private helper methods

// Update 3D attributes based on transform
void AudioSource::update3DAttributes()
{
    if (!m_is3D || !m_channel)
        return;

    // Get world position from entity
    glm::vec3 position = getEntity()->getWorldPosition() + m_positionOffset;

    // Get forward direction from entity rotation
    glm::quat rotation = getEntity()->getWorldRotation();
    glm::vec3 forward = rotation * glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 up = rotation * glm::vec3(0.0f, 1.0f, 0.0f);

    // Convert to FMOD vectors
    FMOD_VECTOR fmodPosition = {position.x, position.y, position.z};
    FMOD_VECTOR fmodVelocity = {m_velocity.x, m_velocity.y, m_velocity.z};
    FMOD_VECTOR fmodForward = {forward.x, forward.y, forward.z};
    FMOD_VECTOR fmodUp = {up.x, up.y, up.z};

    // Set 3D attributes
    m_channel->set3DAttributes(&fmodPosition, &fmodVelocity);
    m_channel->set3DConeOrientation(&fmodForward);
}

// Update effect parameters
void AudioSource::updateEffects()
{
    if (!m_channel)
        return;

    // Get lowpassCutoff and volumeAttenuation from audioSystem first
    float lowpassCutoff, volumeAttenuation;
    m_audioSystem->getOcclusionParameters(&lowpassCutoff, &volumeAttenuation);

    // Apply occlusion effects
    // 1. Apply lowpass filter based on occlusion factor
    float actualLowpass = m_lowpass;
    if (m_occlusionFactor > 0.0f)
    {
        // Mix between full lowpass cutoff (1.0) and occlusion cutoff
        float occlusionLowpass = 1.0f - (m_occlusionFactor * (1.0f - lowpassCutoff));
        actualLowpass = std::min(actualLowpass, occlusionLowpass);
    }

    // Create lowpass DSP if needed
    FMOD::DSP* lowpassDSP = nullptr;
    bool hasLowpass = false;
    int dspCount = 0;
    m_channel->getNumDSPs(&dspCount);

    for (int i = 0; i < dspCount; i++)
    {
        FMOD::DSP* dsp = nullptr;
        m_channel->getDSP(i, &dsp);
        if (dsp)
        {
            FMOD_DSP_TYPE dspType;
            dsp->getType(&dspType);
            if (dspType == FMOD_DSP_TYPE_LOWPASS)
            {
                lowpassDSP = dsp;
                hasLowpass = true;
                break;
            }
        }
    }

    // Create lowpass filter if needed
    if (!hasLowpass && actualLowpass < 1.0f)
    {
        FMOD_RESULT result = m_audioSystem->getFMODSystem()->createDSPByType(FMOD_DSP_TYPE_LOWPASS, &lowpassDSP);
        if (result == FMOD_OK && lowpassDSP)
        {
            m_channel->addDSP(0, lowpassDSP);
            hasLowpass = true;
        }
    }

    // Apply lowpass value
    if (hasLowpass && lowpassDSP)
    {
        // FMOD's lowpass cutoff is in Hz from 10 to 22000
        // We'll map our 0-1 range to a reasonable frequency range
        float cutoffFreq = 100.0f + (actualLowpass * 19900.0f);
        lowpassDSP->setParameterFloat(FMOD_DSP_LOWPASS_CUTOFF, cutoffFreq);
    }

    // 2. Apply volume reduction based on occlusion factor
    if (m_occlusionFactor > 0.0f)
    {
        // Calculate attenuated volume
        float attenuatedVolume = m_volume * (1.0f - (m_occlusionFactor * volumeAttenuation));
        m_channel->setVolume(attenuatedVolume);
    }
    else
    {
        // Normal volume
        m_channel->setVolume(m_volume);
    }

    // Apply reverb send
    if (m_reverbSend > 0.0f)
    {
        // Would connect to reverb DSP here in a full implementation
    }
}

// Apply playback settings to channel
void AudioSource::applyChannelSettings()
{
    if (!m_channel)
        return;

    // Basic playback settings
    m_channel->setVolume(m_volume);
    m_channel->setPitch(m_pitch);
    m_channel->setPriority(m_priority);
    m_channel->setLoopCount(m_loop ? -1 : 0);

    // 3D settings if applicable
    if (m_is3D)
    {
        m_channel->set3DMinMaxDistance(m_minDistance, m_maxDistance);
        m_channel->set3DConeSettings(m_innerConeAngle, m_outerConeAngle, m_outerConeVolume);
        m_channel->set3DSpread(m_spread);

        // Initial 3D position
        update3DAttributes();
    }
}