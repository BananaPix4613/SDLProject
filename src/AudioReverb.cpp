#include "AudioReverb.h"
#include "AudioSystem.h"
#include <algorithm>
#include <cmath>

// Constructor
AudioReverb::AudioReverb(int id, const glm::vec3& position, float radius, AudioSystem* audioSystem) :
    m_id(id),
    m_position(position),
    m_radius(radius),
    m_transitionDistance(0.2f), // 20% of radius is transition zone
    m_preset("GENERIC"),
    m_isInitialized(false),
    m_reverbDSP(nullptr),
    m_audioSystem(audioSystem)
{
    // Initialize default properties
    setDefaultProperties();
}

// Destructor
AudioReverb::~AudioReverb()
{
    // Release FMOD DSP
    if (m_reverbDSP)
    {
        m_reverbDSP->release();
        m_reverbDSP = nullptr;
    }
}

// Initialize the reverb
bool AudioReverb::initialize()
{
    if (m_isInitialized || !m_audioSystem)
        return false;

    // Get FMOD system
    FMOD::System* system = m_audioSystem->getFMODSystem();
    if (!system)
        return false;

    // Create reverb DSP
    FMOD_RESULT result = system->createDSPByType(FMOD_DSP_TYPE_SFXREVERB, &m_reverbDSP);
    if (result != FMOD_OK || !m_reverbDSP)
        return false;

    // Apply named preset
    applyPreset(m_preset);

    m_isInitialized = true;
    return true;
}

// Update reverb properties
void AudioReverb::update(const glm::vec3& listenerPosition)
{
    if (!m_isInitialized || !m_reverbDSP)
        return;

    // Calculate distance from listener to center of reverb zone
    float distance = glm::distance(listenerPosition, m_position);

    // Calculate influence factor based on distance
    float influenceFactor = getInfluenceFactor(listenerPosition);

    // Apply reverb by setting wet level based on influence
    m_reverbDSP->setParameterFloat(FMOD_DSP_SFXREVERB_WETLEVEL, m_properties.WetLevel * influenceFactor);

    // Active/bypass the DSP based on influence
    bool bypass = (influenceFactor <= 0.0f);
    m_reverbDSP->setBypass(bypass);
}

// Set position
void AudioReverb::setPosition(const glm::vec3& position)
{
    m_position = position;
}

// Get position
const glm::vec3& AudioReverb::getPosition() const
{
    return m_position;
}

// Set radius
void AudioReverb::setRadius(float radius)
{
    m_radius = radius;
}

// Get radius
float AudioReverb::getRadius() const
{
    return m_radius;
}

// Set preset
void AudioReverb::setPreset(const std::string& presetName)
{
    m_preset = presetName;

    if (m_isInitialized && m_reverbDSP)
    {
        applyPreset(presetName);
    }
}

// Get preset
const std::string& AudioReverb::getPreset() const
{
    return m_preset;
}

// Set properties directly
void AudioReverb::setProperties(
    float decayTime,
    float earlyDelay,
    float lateDelay,
    float hfReference,
    float hfDecayRatio,
    float diffusion,
    float density,
    float lowShelfFrequency,
    float lowShelfGain,
    float highCut,
    float earlyLateMix,
    float wetLevel)
{
    if (!m_isInitialized || !m_reverbDSP)
        return;

    // Set reverb parameters
    m_reverbDSP->setParameterFloat(FMOD_DSP_SFXREVERB_DECAYTIME, decayTime);
    m_reverbDSP->setParameterFloat(FMOD_DSP_SFXREVERB_EARLYDELAY, earlyDelay);
    m_reverbDSP->setParameterFloat(FMOD_DSP_SFXREVERB_LATEDELAY, lateDelay);
    m_reverbDSP->setParameterFloat(FMOD_DSP_SFXREVERB_HFREFERENCE, hfReference);
    m_reverbDSP->setParameterFloat(FMOD_DSP_SFXREVERB_HFDECAYRATIO, hfDecayRatio);
    m_reverbDSP->setParameterFloat(FMOD_DSP_SFXREVERB_DIFFUSION, diffusion);
    m_reverbDSP->setParameterFloat(FMOD_DSP_SFXREVERB_DENSITY, density);
    m_reverbDSP->setParameterFloat(FMOD_DSP_SFXREVERB_LOWSHELFFREQUENCY, lowShelfFrequency);
    m_reverbDSP->setParameterFloat(FMOD_DSP_SFXREVERB_LOWSHELFGAIN, lowShelfGain);
    m_reverbDSP->setParameterFloat(FMOD_DSP_SFXREVERB_HIGHCUT, highCut);
    m_reverbDSP->setParameterFloat(FMOD_DSP_SFXREVERB_EARLYLATEMIX, earlyLateMix);
    m_reverbDSP->setParameterFloat(FMOD_DSP_SFXREVERB_WETLEVEL, wetLevel);

    // Store properties
    m_properties.DecayTime = decayTime;
    m_properties.EarlyDelay = earlyDelay;
    m_properties.LateDelay = lateDelay;
    m_properties.HFReference = hfReference;
    m_properties.HFDecayRatio = hfDecayRatio;
    m_properties.Diffusion = diffusion;
    m_properties.Density = density;
    m_properties.LowShelfFrequency = lowShelfFrequency;
    m_properties.LowShelfGain = lowShelfGain;
    m_properties.HighCut = highCut;
    m_properties.EarlyLateMix = earlyLateMix;
    m_properties.WetLevel = wetLevel;
}

// Set transition distance
void AudioReverb::setTransitionDistance(float transitionDistance)
{
    m_transitionDistance = std::max(0.0f, std::min(1.0f, transitionDistance));
}

// Get transition distance
float AudioReverb::getTransitionDistance() const
{
    return m_transitionDistance;
}

// Get unique ID
int AudioReverb::getId() const
{
    return m_id;
}

// Calculate influence factor based on distance from center
float AudioReverb::getInfluenceFactor(const glm::vec3& position) const
{
    // Calculate distance
    float distance = glm::distance(position, m_position);

    // If beyond radius, no influence
    if (distance > m_radius)
        return 0.0f;

    // Calculate inner radius (where influence is full
    float innerRadius = m_radius * (1.0f - m_transitionDistance);

    // If within inner radius, full influence
    if (distance <= innerRadius)
        return 1.0f;

    // In transition zone, linear falloff
    float transitionWidth = m_radius - innerRadius;
    return 1.0f - ((distance - innerRadius) / transitionWidth);
}

// Get the FMOD DSP effect
FMOD::DSP* AudioReverb::getDSP() const
{
    return m_reverbDSP;
}

// Private helper methods

// Apply named preset
void AudioReverb::applyPreset(const std::string& presetName)
{
    if (!m_reverbDSP)
        return;

    // Set properties based on preset name
    FMOD_REVERB_PROPERTIES props;

    if (presetName == "CAVE")
        props = FMOD_PRESET_CAVE;
    else if (presetName == "HANGAR")
        props = FMOD_PRESET_HANGAR;
    else if (presetName == "CONCERTHALL")
        props = FMOD_PRESET_CONCERTHALL;
    else if (presetName == "AUDITORIUM")
        props = FMOD_PRESET_AUDITORIUM;
    else if (presetName == "ROOM")
        props = FMOD_PRESET_ROOM;
    else if (presetName == "BATHROOM")
        props = FMOD_PRESET_BATHROOM;
    else if (presetName == "STONEROOM")
        props = FMOD_PRESET_STONEROOM;
    else if (presetName == "FOREST")
        props = FMOD_PRESET_FOREST;
    else if (presetName == "MOUNTAINS")
        props = FMOD_PRESET_MOUNTAINS;
    else if (presetName == "UNDERWATER")
        props = FMOD_PRESET_UNDERWATER;
    else
    {
        // Default to generic
        props = FMOD_PRESET_GENERIC;
    }

    // Store and apply properties
    m_properties = props;

    // Apply to FMOD DSP
    m_reverbDSP->setParameterFloat(FMOD_DSP_SFXREVERB_DECAYTIME, props.DecayTime);
    m_reverbDSP->setParameterFloat(FMOD_DSP_SFXREVERB_EARLYDELAY, props.EarlyDelay);
    m_reverbDSP->setParameterFloat(FMOD_DSP_SFXREVERB_LATEDELAY, props.LateDelay);
    m_reverbDSP->setParameterFloat(FMOD_DSP_SFXREVERB_HFREFERENCE, props.HFReference);
    m_reverbDSP->setParameterFloat(FMOD_DSP_SFXREVERB_HFDECAYRATIO, props.HFDecayRatio);
    m_reverbDSP->setParameterFloat(FMOD_DSP_SFXREVERB_DIFFUSION, props.Diffusion);
    m_reverbDSP->setParameterFloat(FMOD_DSP_SFXREVERB_DENSITY, props.Density);
    m_reverbDSP->setParameterFloat(FMOD_DSP_SFXREVERB_LOWSHELFFREQUENCY, props.LowShelfFrequency);
    m_reverbDSP->setParameterFloat(FMOD_DSP_SFXREVERB_LOWSHELFGAIN, props.LowShelfGain);
    m_reverbDSP->setParameterFloat(FMOD_DSP_SFXREVERB_HIGHCUT, props.HighCut);
    m_reverbDSP->setParameterFloat(FMOD_DSP_SFXREVERB_EARLYLATEMIX, props.EarlyLateMix);
    m_reverbDSP->setParameterFloat(FMOD_DSP_SFXREVERB_WETLEVEL, props.WetLevel);
}

// Set up default properties
void AudioReverb::setDefaultProperties()
{
    // Initialize with generic preset
    m_properties = FMOD_PRESET_GENERIC;
}