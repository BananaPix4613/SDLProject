#include "AudioClip.h"
#include <iostream>

// Constructor
AudioClip::AudioClip(const std::string& filename, bool streaming, bool is3D) :
    m_sound(nullptr),
    m_filename(filename),
    m_is3D(is3D),
    m_isStreaming(streaming),
    m_isInitialized(false),
    m_defaultVolume(1.0f),
    m_defaultPitch(1.0f),
    m_defaultPriority(128), // Middle priority
    m_minDistance(1.0f),
    m_maxDistance(10000.0f),
    m_loop(false),
    m_defaultGroup("SFX") // Default to SFX group
{
}

// Destructor
AudioClip::~AudioClip()
{
    release();
}

// Initialize the audio clip
bool AudioClip::initialize(FMOD::System* system)
{
    if (!system || m_isInitialized)
        return false;

    // Determine mode flags
    FMOD_MODE mode = FMOD_DEFAULT;

    // 3D positioning
    if (m_is3D)
    {
        mode |= FMOD_3D;
    }
    else
    {
        mode |= FMOD_2D;
    }

    // Streaming for longer sounds
    if (m_isStreaming)
    {
        mode |= FMOD_CREATESTREAM;
    }
    else
    {
        mode |= FMOD_CREATECOMPRESSEDSAMPLE;
    }

    // Loop settings
    if (m_loop)
    {
        mode |= FMOD_LOOP_NORMAL;
    }
    else
    {
        mode |= FMOD_LOOP_OFF;
    }

    // Create the sound
    FMOD_RESULT result = system->createSound(m_filename.c_str(), mode, nullptr, &m_sound);

    if (result != FMOD_OK || !m_sound)
    {
        std::cerr << "Failed to load sound file: " << m_filename << std::endl;
        return false;
    }

    // Set default 3D attributes if needed
    if (m_is3D)
    {
        m_sound->set3DMinMaxDistance(m_minDistance, m_maxDistance);
    }

    m_isInitialized = true;
    return true;
}

// Release audio resources
void AudioClip::release()
{
    if (m_sound)
    {
        m_sound->release();
        m_sound = nullptr;
    }

    m_isInitialized = false;
}

// Get the FMOD sound object
FMOD::Sound* AudioClip::getSound() const
{
    return m_sound;
}

// Get the filename of the audio
const std::string& AudioClip::getFilename() const
{
    return m_filename;
}

// Check if this is a 3D positional sound
bool AudioClip::is3D() const
{
    return m_is3D;
}

// Check if this is a streaming sound
bool AudioClip::isStreaming() const
{
    return m_isStreaming;
}

// Set default volume
void AudioClip::setDefaultVolume(float volume)
{
    m_defaultVolume = volume;
}

// Get default volume
float AudioClip::getDefaultVolume() const
{
    return m_defaultVolume;
}

// Set default pitch
void AudioClip::setDefaultPitch(float pitch)
{
    m_defaultPitch = pitch;
}

// Get default pitch
float AudioClip::getDefaultPitch() const
{
    return m_defaultPitch;
}

// Set default priority
void AudioClip::setDefaultPriority(int priority)
{
    m_defaultPriority = priority;
}

// Get default priority
int AudioClip::getDefaultPriority() const
{
    return m_defaultPriority;
}

// Set min and max distance for 3D audio
void AudioClip::setDistanceRange(float minDistance, float maxDistance)
{
    m_minDistance = minDistance;
    m_maxDistance = maxDistance;

    if (m_sound && m_is3D)
    {
        m_sound->set3DMinMaxDistance(minDistance, maxDistance);
    }
}

// Get min distance
float AudioClip::getMinDistance() const
{
    return m_minDistance;
}

// Get max distance
float AudioClip::getMaxDistance() const
{
    return m_maxDistance;
}

// Set loop mode
void AudioClip::setLoop(bool loop)
{
    m_loop = loop;

    if (m_sound)
    {
        m_sound->setMode(loop ? FMOD_LOOP_NORMAL : FMOD_LOOP_OFF);
    }
}

// Get loop mode
bool AudioClip::getLoop() const
{
    return m_loop;
}

// Get sound length in seconds
float AudioClip::getLength() const
{
    if (!m_sound)
    {
        return 0.0f;
    }

    unsigned int lengthMs = 0;
    m_sound->getLength(&lengthMs, FMOD_TIMEUNIT_MS);

    return lengthMs / 1000.0f;
}

// Set default group name
void AudioClip::setDefaultGroup(const std::string& groupName)
{
    m_defaultGroup = groupName;
}

// Get default group name
const std::string& AudioClip::getDefaultGroup() const
{
    return m_defaultGroup;
}