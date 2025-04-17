#pragma once

#include <string>
#include <glm.hpp>
#include <fmod.hpp>

// Forward declarations
class AudioSystem;

/**
 * @class AudioReverb
 * @brief Represents a reverb zone in the game world
 * 
 * AudioReverb creates a spatial zone with specific reverb properties
 * that affect audio sources within its radius of influence.
 */
class AudioReverb
{
public:
    /**
     * @brief Constructor
     * @param id Unique identifier
     * @param position Center position of the reverb zone
     * @param radius Radius of influence
     * @param audioSystem Reference to audio system
     */
    AudioReverb(int id, const glm::vec3& position, float radius, AudioSystem* audioSystem);

    /**
     * @brief Destructor
     */
    ~AudioReverb();

    /**
     * @brief Initialize the reverb
     * @return True if initialization succeeded
     */
    bool initialize();

    /**
     * @brief Update reverb properties
     * @param listenerPosition Position of the active listener
     */
    void update(const glm::vec3& listenerPosition);

    /**
     * @brief Set position
     * @param position Center position of the reverb zone
     */
    void setPosition(const glm::vec3& position);

    /**
     * @brief Get position
     * @return Current position
     */
    const glm::vec3& getPosition() const;

    /**
     * @brief Set radius
     * @param radius Radius of influence
     */
    void setRadius(float radius);

    /**
     * @brief Get radius
     * @return Current radius
     */
    float getRadius() const;

    /**
     * @brief Set preset
     * @param presetName Reverb preset name
     */
    void setPreset(const std::string& presetName);

    /**
     * @brief Get preset
     * @return Current preset name
     */
    const std::string& getPreset() const;

    /**
     * @brief Set properties directly
     * @param decayTime Decay time in seconds
     * @param earlyDelay Early reflection delay
     * @param lateDelay Late reflection delay
     * @param hfReference HF reference frequency
     * @param hfDecayRatio HF decay ratio
     * @param diffusion Diffusion percentage
     * @param density Density percentage
     * @param lowShelfFrequency Low shelf frequency
     * @param lowShelfGain Low shelf gain
     * @param highCut High cut frequency
     * @param earlyLateMix Early/late mix ratio
     * @param wetLevel Wet level (0-1)
     */
    void setProperties(
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
        float wetLevel
    );

    /**
     * @brief Set transition distance
     * @param transitionDistance Distance over which reverb blends (percentage of radius)
     */
    void setTransitionDistance(float transitionDistance);

    /**
     * @brief Get transition distance
     * @return Current transition distance
     */
    float getTransitionDistance() const;

    /**
     * @brief Get unique ID
     * @return Reverb zone ID
     */
    int getId() const;

    /**
     * @brief Calculate influence factor based on distance from center
     * @param position Position to check
     * @return Influence factor (0-1)
     */
    float getInfluenceFactor(const glm::vec3& position) const;

    /**
     * @brief Get the FMOD DSP effect
     * @return FMOD DSP effect
     */
    FMOD::DSP* getDSP() const;

private:
    int m_id;
    glm::vec3 m_position;
    float m_radius;
    float m_transitionDistance;
    std::string m_preset;
    bool m_isInitialized;

    // FMOD reverb DSP
    FMOD::DSP* m_reverbDSP;

    // Reverb properties
    FMOD_REVERB_PROPERTIES m_properties;

    // Reference to audio system
    AudioSystem* m_audioSystem;

    // Apply named preset
    void applyPreset(const std::string& presetName);

    // Set up default properties
    void setDefaultProperties();
};