#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <memory>
#include <fmod.hpp>

// Forward declarations
class AudioSystem;

/**
 * @class AudioSnapshot
 * @brief Represents a state of the audio mixer
 * 
 * Audio snapshots store mixer parameter settings that
 * can be transitioned to for different game states.
 */
class AudioSnapshot
{
public:
    /**
     * @brief Constructor
     * @param name Snapshot name
     */
    AudioSnapshot(const std::string& name);

    /**
     * @brief Set a group volume for this snapshot
     * @param groupName Audio group name
     * @param volume Volume level
     */
    void setGroupVolume(const std::string& groupName, float volume);

    /**
     * @brief Set a global effect parameter for this snapshot
     * @param effectName Effect name
     * @param paramName Parameter name
     * @param value Parameter value
     */
    void setEffectParameter(const std::string& effectName,
                            const std::string& paramName,
                            float value);

    /**
     * @brief Get snapshot name
     * @return Snapshot name
     */
    const std::string& getName() const;

    // For internal use by AudioMixer
    friend class AudioMixer;

private:
    std::string m_name;
    std::unordered_map<std::string, float> m_groupVolumes;
    std::unordered_map<std::string, std::unordered_map<std::string, float>> m_effectParameters;
};

/**
 * @class AudioMixer
 * @brief Manages audio mixing, effects, and snapshots
 * 
 * The AudioMixer provides control over groups of audio channels,
 * manages dynamic mixing transitions, and applies global effects.
 */
class AudioMixer
{
public:
    /**
     * @brief Constructor
     * @param name Mixer name
     * @param audioSystem Reference to audio system
     */
    AudioMixer(const std::string& name, AudioSystem* audioSystem);

    /**
     * @brief Destructor
     */
    ~AudioMixer();

    /**
     * @brief Initialize the mixer
     * @return True if initialization succeeded
     */
    bool initialize();

    /**
     * @brief Update mixer state
     * @param deltaTime Time since last frame
     */
    void update(float deltaTime);

    /**
     * @brief Get mixer name
     * @return Mixer name
     */
    const std::string& getName() const;

    /**
     * @brief Set group volume
     * @param groupName Audio group name
     * @param volume Volume level (0.0 to 1.0)
     */
    void setGroupVolume(const std::string& groupName, float volume);

    /**
     * @brief Get group volume
     * @param groupName Audio group name
     * @return Current volume level
     */
    float getGroupVolume(const std::string& groupName) const;

    /**
     * @brief Add a DSP effect to a group
     * @param groupName Audio group name
     * @param effectType FMOD DSP effect type
     * @param effectName Name to identify the effect
     * @return True if effect was added successfully
     */
    bool addEffect(const std::string& groupName, FMOD_DSP_TYPE effectType, const std::string& effectName);
    
    /**
     * @brief Remove a DSP effect from a group
     * @param groupName Audio group name
     * @param effectName Name of effect to remove
     * @return True if effect was removed
     */
    bool removeEffect(const std::string& groupName, const std::string& effectName);

    /**
     * @brief Set an effect parameter
     * @param effectName Name of the effect
     * @param paramIndex Parameter index
     * @param value Parameter value
     * @return True if parameter was set
     */
    bool setEffectParameter(const std::string& effectName, int paramIndex, float value);

    /**
     * @brief Set an effect parameter by name
     * @param effectName Name of the effect
     * @param paramName Parameter name
     * @param value Parameter value
     * @return True if parameter was set
     */
    bool setEffectParameter(const std::string& effectName, const std::string& paramName, float value);

    /**
     * @brief Get an effect parameter value
     * @param effectName Name of the effect
     * @param paramIndex Parameter index
     * @return Parameter value
     */
    float getEffectParameter(const std::string& effectName, int paramIndex) const;

    /**
     * @brief Create a snapshot
     * @param snapshotName Name for the snapshot
     * @return Pointer to created snapshot
     */
    AudioSnapshot* createSnapshot(const std::string& snapshotName);

    /**
     * @brief Get a snapshot by name
     * @param snapshotName Name of the snapshot
     * @return Pointer to snapshot or nullptr or nullptr if not found
     */
    AudioSnapshot* getSnapshot(const std::string& snapshotName) const;

    /**
     * @brief Transition to a snapshot
     * @param snapshotName Name of the snapshot
     * @param transitionTime Time to transition in seconds
     */
    void transitionToSnapshot(const std::string& snapshotName, float transitionTime);

    /**
     * @brief Set global reverb properties
     * @param decayTime Decay time in seconds
     * @param earlyDelay Early reflection delay in seconds
     * @param lateDelay Late reflection delay in seconds
     * @param hfReference HF reference in Hz
     * @param hfDecayRatio HF decay ratio (0-1)
     * @param diffusion Diffusion amount (0-1)
     * @param density Density amount (0-1)
     * @param lowShelfFrequency Low shelf frequency in Hz
     * @param lowShelfGain Low shelf gain in dB
     * @param highCut High cut frequency in Hz
     * @param earlyLateMix Early/late mix ratio (0-1)
     * @param wetLevel Wet level (0-1)
     */
    void setReverbProperties(
        float decayTime = 1.0f,
        float earlyDelay = 0.007f,
        float lateDelay = 0.011f,
        float hfReference = 5000.0f,
        float hfDecayRatio = 0.5f,
        float diffusion = 1.0f,
        float density = 1.0f,
        float lowShelfFrequency = 250.0f,
        float lowShelfGain = 0.0f,
        float highCut = 20000.0f,
        float earlyLateMix = 0.5f,
        float wetLevel = 0.33f
    );

    /**
     * @brief Set master lowpass filter
     * @param cutoff Cutoff frequency (0.0 to 1.0, 1.0 = no filtering)
     */
    void setMasterLowpassFilter(float cutoff);

    /**
     * @brief Set master highpass filter
     * @param cutoff Cutoff frequency (0.0 to 1.0, 0.0 = no filtering)
     */
    void setMasterHighpassFilter(float cutoff);

    /**
     * @brief Set master pitch shift
     * @param pitch Pitch multiplier (1.0 = normal)
     */
    void setMasterPitch(float pitch);

    /**
     * @brief Get master pitch shift
     * @return Current pitch multiplier
     */
    float getMasterPitch() const;

    /**
     * @brief Set duck volume for a group when another group is active
     * @param targetGroup Group to reduce volume for
     * @param triggerGroup Group that triggers the ducking
     * @param amount Amount to reduce volume (0-1)
     * @param attackTime Time to reach ducked volume in seconds
     * @param releaseTime Time to return to normal volume in seconds
     * @param threshold Threshold above which ducking occurs (0-1)
     */
    void setGroupDucking(const std::string& targetGroup,
                         const std::string& triggerGroup,
                         float amount,
                         float attackTime = 0.05f,
                         float releaseTime = 0.5f,
                         float threshold = 0.1f);

    /**
     * @brief Remove ducking between groups
     * @param targetGroup Group to stop ducking
     * @param triggerGroup Triggering group
     */
    void removeGroupDucking(const std::string& targetGroup, const std::string& triggerGroup);

    /**
     * @brief Set master echo effect
     * @param delay Delay time in seconds
     * @param feedback Feedback amount (0-1)
     * @param wetLevel Wet level (0-1)
     */
    void setMasterEcho(float delay, float feedback, float wetLevel);

private:
    std::string m_name;
    AudioSystem* m_audioSystem;
    bool m_initialized;

    // Group volumes
    std::unordered_map<std::string, float> m_groupVolumes;

    // DSP effects
    struct DSPEffect
    {
        FMOD::DSP* dsp;
        int dspIndex;
        std::unordered_map<std::string, int> paramNameToIndex;
    };
    std::unordered_map<std::string, std::unordered_map<std::string, DSPEffect>> m_groupEffects;

    // Snapshots
    std::unordered_map<std::string, std::unique_ptr<AudioSnapshot>> m_snapshots;
    AudioSnapshot* m_currentSnapshot;
    AudioSnapshot* m_targetSnapshot;
    float m_transitionTime;
    float m_transitionProgress;

    // Ducking
    struct DuckingSetup
    {
        FMOD::DSP* compressor;
        std::string triggerGroup;
        std::string targetGroup;
        float amount;
        float attackTime;
        float releaseTime;
        float threshold;
    };
    std::vector<DuckingSetup> m_duckingSetups;

    // Helper methods
    void applySnapshot(AudioSnapshot* snapshot, float blendWeight);
    int getDSPParameterIndexByName(FMOD::DSP* dsp, const std::string& paramName);
    bool setupDucking(const DuckingSetup& setup);
};