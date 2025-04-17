#include "AudioMixer.h"
#include "AudioSystem.h"
#include <iostream>
#include <algorithm>

// AudioSnapshot Implementation

// Constructor
AudioSnapshot::AudioSnapshot(const std::string& name) :
    m_name(name)
{
}

// Set a group volume for this snapshot
void AudioSnapshot::setGroupVolume(const std::string& groupName, float volume)
{
    m_groupVolumes[groupName] = volume;
}

// Set a global effect parameter for this snapshot
void AudioSnapshot::setEffectParameter(const std::string& effectName,
                                       const std::string& paramName,
                                       float value)
{
    m_effectParameters[effectName][paramName] = value;
}

// Get snapshot name
const std::string& AudioSnapshot::getName() const
{
    return m_name;
}

// AudioMixer Implementation

// Constructor
AudioMixer::AudioMixer(const std::string& name, AudioSystem* audioSystem) :
    m_name(name),
    m_audioSystem(audioSystem),
    m_initialized(false),
    m_currentSnapshot(nullptr),
    m_targetSnapshot(nullptr),
    m_transitionTime(0.0f),
    m_transitionProgress(0.0f)
{
}

// Destructor
AudioMixer::~AudioMixer()
{
    // Clean up FMOD DSP effects
    for (auto& groupEffects : m_groupEffects)
    {
        for (auto& effect : groupEffects.second)
        {
            if (effect.second.dsp)
            {
                effect.second.dsp->release();
            }
        }
    }
}

// Initialize the mixer
bool AudioMixer::initialize()
{
    if (!m_audioSystem || m_initialized)
        return false;

    // Create default snapshot
    auto defaultSnapshot = createSnapshot("Default");
    if (!defaultSnapshot)
        return false;

    // Set as current snapshot
    m_currentSnapshot = defaultSnapshot;

    m_initialized = true;
    return true;
}

// Update mixer state
void AudioMixer::update(float deltaTime)
{
    if (!m_initialized)
        return;

    // Process snapshot transitions
    if (m_targetSnapshot && m_transitionTime > 0.0f)
    {
        // Update progress
        m_transitionProgress += deltaTime / m_transitionTime;

        if (m_transitionProgress >= 1.0f)
        {
            // Transition complete
            applySnapshot(m_targetSnapshot, 1.0f);
            m_currentSnapshot = m_targetSnapshot;
            m_targetSnapshot = nullptr;
            m_transitionProgress = 0.0f;
        }
        else
        {
            // Blend between snapshots
            applySnapshot(m_currentSnapshot, 1.0f - m_transitionProgress);
            applySnapshot(m_targetSnapshot, m_transitionProgress);
        }
    }
}

// Get mixer name
const std::string& AudioMixer::getName() const
{
    return m_name;
}

// Set group volume
void AudioMixer::setGroupVolume(const std::string& groupName, float volume)
{
    if (!m_initialized)
        return;

    // Store the volume setting
    m_groupVolumes[groupName] = volume;

    // Apply to FMOD
    if (m_audioSystem)
    {
        m_audioSystem->setGroupVolume(groupName, volume);
    }
}

// Get group volume
float AudioMixer::getGroupVolume(const std::string& groupName) const
{
    auto it = m_groupVolumes.find(groupName);
    if (it != m_groupVolumes.end())
    {
        return it->second;
    }

    // Default to full volume
    return 1.0f;
}

// Add a DSP effect to a group
bool AudioMixer::addEffect(const std::string& groupName, FMOD_DSP_TYPE effectType, const std::string& effectName)
{
    if (!m_initialized || !m_audioSystem)
        return false;

    // Get FMOD system
    FMOD::System* system = m_audioSystem->getFMODSystem();
    if (!system)
        return false;

    // Check if effect already exists
    auto groupIt = m_groupEffects.find(groupName);
    if (groupIt != m_groupEffects.end())
    {
        auto effectIt = groupIt->second.find(effectName);
        if (effectIt != groupIt->second.end())
        {
            // Effect already exists
            return true;
        }
    }

    // Get channel group
    FMOD::ChannelGroup* group = m_audioSystem->getChannelGroup(groupName.c_str());
    if (!group)
        return false;

    // Create DSP effect
    FMOD::DSP* dsp = nullptr;
    FMOD_RESULT result = system->createDSPByType(effectType, &dsp);
    if (result != FMOD_OK || !dsp)
        return false;

    // Add DSP to channel group
    int dspIndex = 0;
    result = group->addDSP(FMOD_CHANNELCONTROL_DSP_TAIL, dsp);
    if (result != FMOD_OK)
    {
        dsp->release();
        return false;
    }

    // Get the index of the DSP
    int numDSPs = 0;
    group->getNumDSPs(&numDSPs);
    for (int i = 0; i < numDSPs; i++)
    {
        FMOD::DSP* checkDsp = nullptr;
        group->getDSP(i, &checkDsp);
        if (checkDsp == dsp)
        {
            dspIndex = i;
            break;
        }
    }

    // Create effect data
    DSPEffect effect;
    effect.dsp = dsp;
    effect.dspIndex = dspIndex;

    // Map parameter names to indices for common effects
    switch (effectType)
    {
        case FMOD_DSP_TYPE_LOWPASS:
            effect.paramNameToIndex["cutoff"] = FMOD_DSP_LOWPASS_CUTOFF;
            effect.paramNameToIndex["resonance"] = FMOD_DSP_LOWPASS_RESONANCE;
            break;

        case FMOD_DSP_TYPE_HIGHPASS:
            effect.paramNameToIndex["cutoff"] = FMOD_DSP_HIGHPASS_CUTOFF;
            effect.paramNameToIndex["resonance"] = FMOD_DSP_HIGHPASS_RESONANCE;
            break;

        case FMOD_DSP_TYPE_ECHO:
            effect.paramNameToIndex["delay"] = FMOD_DSP_ECHO_DELAY;
            effect.paramNameToIndex["feedback"] = FMOD_DSP_ECHO_FEEDBACK;
            effect.paramNameToIndex["dry"] = FMOD_DSP_ECHO_DRYLEVEL;
            effect.paramNameToIndex["wet"] = FMOD_DSP_ECHO_WETLEVEL;
            break;

        case FMOD_DSP_TYPE_SFXREVERB:
            effect.paramNameToIndex["decaytime"] = FMOD_DSP_SFXREVERB_DECAYTIME;
            effect.paramNameToIndex["earlydelay"] = FMOD_DSP_SFXREVERB_EARLYDELAY;
            effect.paramNameToIndex["latedelay"] = FMOD_DSP_SFXREVERB_LATEDELAY;
            effect.paramNameToIndex["diffusion"] = FMOD_DSP_SFXREVERB_DIFFUSION;
            effect.paramNameToIndex["density"] = FMOD_DSP_SFXREVERB_DENSITY;
            effect.paramNameToIndex["wetlevel"] = FMOD_DSP_SFXREVERB_WETLEVEL;
            break;

            // Add more effects as needed

        default:
            break;
    }

    // Store the effect
    m_groupEffects[groupName][effectName] = effect;

    return true;
}

// Remove a DSP effect from a group
bool AudioMixer::removeEffect(const std::string& groupName, const std::string& effectName)
{
    if (!m_initialized || !m_audioSystem)
        return false;

    // Find the effect
    auto groupIt = m_groupEffects.find(groupName);
    if (groupIt == m_groupEffects.end())
        return false;

    auto effectIt = groupIt->second.find(effectName);
    if (effectIt == groupIt->second.end())
        return false;

    // Get the DSP
    FMOD::DSP* dsp = effectIt->second.dsp;
    if (!dsp)
        return false;

    // Get the channel group
    FMOD::System* system = m_audioSystem->getFMODSystem();
    if (!system)
        return false;

    FMOD::ChannelGroup* group = m_audioSystem->getChannelGroup(groupName.c_str());
    if (!group)
        return false;

    // Remove the DSP from the group
    group->removeDSP(dsp);

    // Release the DSP
    dsp->release();

    // Remove from our map
    groupIt->second.erase(effectIt);

    return true;
}

// Set an effect parameter
bool AudioMixer::setEffectParameter(const std::string& effectName, int paramIndex, float value)
{
    if (!m_initialized)
        return false;

    // Find the effect (search in all groups)
    for (auto& groupPair : m_groupEffects)
    {
        auto effectIt = groupPair.second.find(effectName);
        if (effectIt != groupPair.second.end())
        {
            // Found the effect
            FMOD::DSP* dsp = effectIt->second.dsp;
            if (dsp)
            {
                dsp->setParameterFloat(paramIndex, value);
                return true;
            }
        }
    }

    return false;
}

// Set an effect parameter by name
bool AudioMixer::setEffectParameter(const std::string& effectName, const std::string& paramName, float value)
{
    if (!m_initialized)
        return false;

    // Find the effect (search in all groups
    for (auto& groupPair : m_groupEffects)
    {
        auto effectIt = groupPair.second.find(effectName);
        if (effectIt != groupPair.second.end())
        {
            // Found the effect
            FMOD::DSP* dsp = effectIt->second.dsp;
            if (dsp)
            {
                // Look up parameter index by name
                auto paramIt = effectIt->second.paramNameToIndex.find(paramName);
                if (paramIt != effectIt->second.paramNameToIndex.end())
                {
                    dsp->setParameterFloat(paramIt->second, value);
                    return true;
                }
                else
                {
                    // Try to get parameter index by name
                    int index = getDSPParameterIndexByName(dsp, paramName);
                    if (index >= 0)
                    {
                        dsp->setParameterFloat(index, value);

                        // Cache for future use
                        effectIt->second.paramNameToIndex[paramName] = index;

                        return true;
                    }
                }
            }
        }
    }

    return false;
}

// Get an effect parameter value
float AudioMixer::getEffectParameter(const std::string& effectName, int paramIndex) const
{
    if (!m_initialized)
        return 0.0f;

    // Find the effect (search in all groups)
    for (const auto& groupPair : m_groupEffects)
    {
        auto effectIt = groupPair.second.find(effectName);
        if (effectIt != groupPair.second.end())
        {
            // Found the effect
            FMOD::DSP* dsp = effectIt->second.dsp;
            if (dsp)
            {
                float value = 0.0f;
                dsp->getParameterFloat(paramIndex, &value, nullptr, 0);
                return value;
            }
        }
    }

    return 0.0f;
}

// Create a snapshot
AudioSnapshot* AudioMixer::createSnapshot(const std::string& snapshotName)
{
    if (!m_initialized)
        return nullptr;

    // Check if snapshot already exists
    auto it = m_snapshots.find(snapshotName);
    if (it != m_snapshots.end())
    {
        return it->second.get();
    }

    // Create new snapshot
    auto snapshot = std::make_unique<AudioSnapshot>(snapshotName);

    // Store current state if this is the first snapshot
    if (m_snapshots.empty())
    {
        // Store current group volumes
        for (const auto& pair : m_groupVolumes)
        {
            snapshot->setGroupVolume(pair.first, pair.second);
        }

        // Store current effect parameters
        for (const auto& groupPair : m_groupEffects)
        {
            for (const auto& effectPair : groupPair.second)
            {
                FMOD::DSP* dsp = effectPair.second.dsp;
                if (dsp)
                {
                    for (const auto& paramPair : effectPair.second.paramNameToIndex)
                    {
                        float value = 0.0f;
                        dsp->getParameterFloat(paramPair.second, &value, nullptr, 0);
                        snapshot->setEffectParameter(effectPair.first, paramPair.first, value);
                    }
                }
            }
        }
    }

    // Store and return the snapshot
    AudioSnapshot* result = snapshot.get();
    m_snapshots[snapshotName] = std::move(snapshot);
    return result;
}

// Get a snapshot by name
AudioSnapshot* AudioMixer::getSnapshot(const std::string& snapshotName) const
{
    auto it = m_snapshots.find(snapshotName);
    if (it != m_snapshots.end())
    {
        return it->second.get();
    }

    return nullptr;
}

// Transition to a snapshot
void AudioMixer::transitionToSnapshot(const std::string& snapshotName, float transitionTime)
{
    if (!m_initialized)
        return;

    // Find the target snapshot
    AudioSnapshot* targetSnapshot = getSnapshot(snapshotName);
    if (!targetSnapshot)
        return;

    // If no current snapshot, set it immediately
    if (!m_currentSnapshot)
    {
        m_currentSnapshot = targetSnapshot;
        applySnapshot(targetSnapshot, 1.0f);
        return;
    }

    // If already transitioning to this snapshot, do nothing
    if (m_targetSnapshot == targetSnapshot)
        return;

    // If transition time is zero, apply immediately
    if (transitionTime <= 0.0f)
    {
        applySnapshot(targetSnapshot, 1.0f);
        m_currentSnapshot = targetSnapshot;
        m_targetSnapshot = nullptr;
        m_transitionProgress = 0.0f;
        return;
    }

    // Set up transition
    m_targetSnapshot = targetSnapshot;
    m_transitionTime = transitionTime;
    m_transitionProgress = 0.0f;
}

// Set global reverb properties
void AudioMixer::setReverbProperties(
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
    if (!m_initialized)
        return;

    // Add a reverb effect to the master group if it doesn't exist
    bool hasReverb = false;
    for (const auto& pair : m_groupEffects)
    {
        if (pair.first == "Master")
        {
            auto it = pair.second.find("MasterReverb");
            if (it != pair.second.end())
            {
                hasReverb = true;
                break;
            }
        }
    }

    if (!hasReverb)
    {
        addEffect("Master", FMOD_DSP_TYPE_SFXREVERB, "MasterReverb");
    }

    // Set reverb parameters
    setEffectParameter("MasterReverb", "decaytime", decayTime);
    setEffectParameter("MasterReverb", "earlydelay", earlyDelay);
    setEffectParameter("MasterReverb", "latedelay", lateDelay);
    setEffectParameter("MasterReverb", "hfreference", hfReference);
    setEffectParameter("MasterReverb", "hfdecayratio", hfDecayRatio);
    setEffectParameter("MasterReverb", "diffusion", diffusion);
    setEffectParameter("MasterReverb", "density", density);
    setEffectParameter("MasterReverb", "lowshelffrequency", lowShelfFrequency);
    setEffectParameter("MasterReverb", "lowshelfgain", lowShelfGain);
    setEffectParameter("MasterReverb", "highcut", highCut);
    setEffectParameter("MasterReverb", "earlylatemix", earlyLateMix);
    setEffectParameter("MasterReverb", "wetlevel", wetLevel);
}

// Set master lowpass filter
void AudioMixer::setMasterLowpassFilter(float cutoff)
{
    if (!m_initialized)
        return;

    // Add a lowpass effect to the master group if it doesn't exist
    bool hasLowpass = false;
    for (const auto& pair : m_groupEffects)
    {
        if (pair.first == "Master")
        {
            auto it = pair.second.find("MasterLowpass");
            if (it != pair.second.end())
            {
                hasLowpass = true;
                break;
            }
        }
    }

    if (!hasLowpass)
    {
        addEffect("Master", FMOD_DSP_TYPE_LOWPASS, "MasterLowpass");
    }

    // Set cutoff parameter (map 0-1 to reasonable frequency range)
    float frequencyCutoff = 100.0f + cutoff * 19900.0f; // 100Hz to 20kHz
    setEffectParameter("MasterLowpass", "cutoff", frequencyCutoff);
}

// Set master highpass filter
void AudioMixer::setMasterHighpassFilter(float cutoff)
{
    if (!m_initialized)
        return;

    // Add a highpass effect to the master group if it doesn't exist
    bool hasHighpass = false;
    for (const auto& pair : m_groupEffects)
    {
        if (pair.first == "Master")
        {
            auto it = pair.second.find("MasterHighpass");
            if (it != pair.second.end())
            {
                hasHighpass = true;
                break;
            }
        }
    }

    if (!hasHighpass)
    {
        addEffect("Master", FMOD_DSP_TYPE_HIGHPASS, "MasterHighpass");
    }

    // Set cutoff parameter (map 0-1 to reasonable frequency range)
    float frequencyCutoff = cutoff * 2000.0f; // 0Hz to 2kHz
    setEffectParameter("MasterHighpass", "cutoff", frequencyCutoff);
}

// Set master pitch shift
void AudioMixer::setMasterPitch(float pitch)
{
    if (!m_initialized || !m_audioSystem)
        return;

    // Get FMOD system
    FMOD::System* system = m_audioSystem->getFMODSystem();
    if (!system)
        return;

    // Get master channel group
    FMOD::ChannelGroup* masterGroup = nullptr;
    FMOD_RESULT result = system->getMasterChannelGroup(&masterGroup);
    if (result != FMOD_OK || !masterGroup)
        return;

    // Set pitch
    masterGroup->setPitch(pitch);
}

// Get master pitch shift
float AudioMixer::getMasterPitch() const
{
    if (!m_initialized || !m_audioSystem)
        return 1.0f;

    // Get FMOD system
    FMOD::System* system = m_audioSystem->getFMODSystem();
    if (!system)
        return 1.0f;

    // Get master channel group
    FMOD::ChannelGroup* masterGroup = nullptr;
    FMOD_RESULT result = system->getMasterChannelGroup(&masterGroup);
    if (result != FMOD_OK || !masterGroup)
        return 1.0f;

    // Get pitch
    float pitch = 1.0f;
    masterGroup->getPitch(&pitch);
    return pitch;
}

// Set duck volume for a group when another group is active
void AudioMixer::setGroupDucking(const std::string& targetGroup,
                                 const std::string& triggerGroup,
                                 float amount,
                                 float attackTime,
                                 float releaseTime,
                                 float threshold)
{
    if (!m_initialized || !m_audioSystem)
        return;

    // Setup ducking setup
    DuckingSetup setup;
    setup.targetGroup = targetGroup;
    setup.triggerGroup = triggerGroup;
    setup.amount = amount;
    setup.attackTime = attackTime;
    setup.releaseTime = releaseTime;
    setup.threshold = threshold;

    // Configure compressor DSP for ducking
    if (setupDucking(setup))
    {
        // Add to ducking setups
        m_duckingSetups.push_back(setup);
    }
}

// Remove ducking between groups
void AudioMixer::removeGroupDucking(const std::string& targetGroup, const std::string& triggerGroup)
{
    if (!m_initialized)
        return;

    // Find and remove ducking setups
    for (auto it = m_duckingSetups.begin(); it != m_duckingSetups.end(); )
    {
        if (it->targetGroup == targetGroup && it->triggerGroup == triggerGroup)
        {
            // Remove the ducking DSP
            if (it->compressor)
            {
                // Get the channel group
                FMOD::System* system = m_audioSystem->getFMODSystem();
                if (system)
                {
                    FMOD::ChannelGroup* group = m_audioSystem->getChannelGroup(targetGroup.c_str());
                    if (group)
                    {
                        // Remove from group
                        group->removeDSP(it->compressor);

                        // Release DSP
                        it->compressor->release();
                    }
                }
            }

            // Erase from list
            it = m_duckingSetups.erase(it);
        }
        else
        {
            it++;
        }
    }
}

// Set echo
void AudioMixer::setMasterEcho(float delay, float feedback, float wetLevel)
{
    // Add echo to master group if it doesn't exist
    bool hasEcho = false;
    for (const auto& pair : m_groupEffects)
    {
        if (pair.first == "Master")
        {
            auto it = pair.second.find("Echo");
            if (it != pair.second.end())
            {
                hasEcho = true;
                break;
            }
        }
    }

    if (!hasEcho)
    {
        addEffect("Master", FMOD_DSP_TYPE_ECHO, "Echo");
    }

    // Set echo parameters
    setEffectParameter("Echo", "delay", delay);
    setEffectParameter("Echo", "feedback", feedback);
    setEffectParameter("Echo", "dry", 1.0f - wetLevel);
    setEffectParameter("Echo", "wet", wetLevel);
}

// Private helper methods

// Apply snapshot with blend weight
void AudioMixer::applySnapshot(AudioSnapshot* snapshot, float blendWeight)
{
    if (!snapshot || !m_audioSystem)
        return;

    // Apply group volumes
    for (const auto& pair : snapshot->m_groupVolumes)
    {
        // Get current volume
        float currentVolume = getGroupVolume(pair.first);

        // Blend with snapshot volume
        float newVolume = currentVolume * (1.0f - blendWeight) + pair.second * blendWeight;

        // Apply to group
        m_audioSystem->setGroupVolume(pair.first, newVolume);
    }

    // Apply effect parameters
    for (const auto& effectPair : snapshot->m_effectParameters)
    {
        for (const auto& paramPair : effectPair.second)
        {
            // Get current parameter value
            float currentValue = getEffectParameter(effectPair.first,
                                                    getDSPParameterIndexByName(nullptr, paramPair.first));

            // Blend with snapshot value
            float newValue = currentValue * (1.0f - blendWeight) + paramPair.second * blendWeight;

            // Apply to effect
            setEffectParameter(effectPair.first, paramPair.first, newValue);
        }
    }
}

// Get DSP parameter index by name
int AudioMixer::getDSPParameterIndexByName(FMOD::DSP* dsp, const std::string& paramName)
{
    // First check if this is a known DSP and parameter
    for (const auto& groupPair : m_groupEffects)
    {
        for (const auto& effectPair : groupPair.second)
        {
            // If dsp is provided, only check that specific DSP
            if (dsp && effectPair.second.dsp != dsp)
                continue;

            // Check if parameter name is in map
            auto paramIt = effectPair.second.paramNameToIndex.find(paramName);
            if (paramIt != effectPair.second.paramNameToIndex.end())
            {
                return paramIt->second;
            }
        }
    }

    // Not found in cache - would need to query DSP in full implementation
    return -1;
}

// Setup ducking with sidechain compression
bool AudioMixer::setupDucking(const DuckingSetup& setup)
{
    if (!m_audioSystem)
        return false;

    // Get FMOD system
    FMOD::System* system = m_audioSystem->getFMODSystem();
    if (!system)
        return false;

    // Get target and trigger channel groups
    FMOD::ChannelGroup* targetGroup = m_audioSystem->getChannelGroup(setup.targetGroup.c_str());
    FMOD::ChannelGroup* triggerGroup = m_audioSystem->getChannelGroup(setup.triggerGroup.c_str());

    if (!targetGroup || !triggerGroup)
        return false;

    // Create compressor DSP for ducking
    FMOD::DSP* compressor = nullptr;
    FMOD_RESULT result = system->createDSPByType(FMOD_DSP_TYPE_COMPRESSOR, &compressor);
    if (result != FMOD_OK || !compressor)
        return false;

    // Add compressor to target group
    result = targetGroup->addDSP(0, compressor);
    if (result != FMOD_OK)
    {
        compressor->release();
        return false;
    }

    // Set up compressor parameters for ducking
    compressor->setParameterFloat(FMOD_DSP_COMPRESSOR_THRESHOLD, setup.threshold);
    compressor->setParameterFloat(FMOD_DSP_COMPRESSOR_RATIO, 10.0f); // High ratio for ducking
    compressor->setParameterFloat(FMOD_DSP_COMPRESSOR_ATTACK, setup.attackTime * 1000.0f); // Convert to ms
    compressor->setParameterFloat(FMOD_DSP_COMPRESSOR_RELEASE, setup.releaseTime * 1000.0f); // Convert to ms
    compressor->setParameterFloat(FMOD_DSP_COMPRESSOR_GAINMAKEUP, -setup.amount); // Negative gain for ducking
    
    // In a full implementation, we would set up sidechain input from the trigger group to the compressor
    // FMOD doesn't directly support sidechain routing, so we'd need to use a custom DSP or other workaround

    // Store the compressor in the setup for later cleanup
    DuckingSetup& newSetup = const_cast<DuckingSetup&>(setup);
    newSetup.compressor = compressor;

    return true;
}