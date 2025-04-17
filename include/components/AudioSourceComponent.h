#pragma once

#include "Component.h"
#include <glm.hpp>
#include <string>
#include <fmod.hpp>

// Forward declarations
class AudioSystem;
class AudioClip;

/**
 * @class AudioSource
 * @brief Component that emits sound in the game world
 * 
 * AudioSource is a component that allows an entity to emit sounds
 * at its position. It supports 3D spatial audio, sound occlusion,
 * and various playback controls.
 */
class AudioSource : public Component
{
public:
    /**
     * @brief Constructor
     */
    AudioSource();

    /**
     * @brief Destructor
     */
    ~AudioSource() override;

    /**
     * @brief Initialize component
     */
    void initialize() override;

    /**
     * @brief Update component
     * @param deltaTime Time since last frame
     */
    void update(float deltaTime) override;

    /**
     * @brief Clean up resources when destroyed
     */
    void onDestroy() override;

    /**
     * @brief React to entity transform changes
     */
    void onTransformChanged() override;

    /**
     * @brief Get component type name
     * @return Component type name
     */
    const char* getTypeName() const override;

    /**
     * @brief Set audio clip
     * @param clip Audio clip to play
     */
    void setClip(AudioClip* clip);

    /**
     * @brief Get current audio clip
     * @return Current audio clip
     */
    AudioClip* getClip() const;

    /**
     * @brief Play the audio source
     */
    void play();

    /**
     * @brief Stop the audio source
     * @param fadeOut Whether to fade out or stop immediately
     */
    void stop(bool fadeOut = false);

    /**
     * @brief Pause the audio source
     */
    void pause();

    /**
     * @brief Resume the audio source
     */
    void resume();

    /**
     * @brief Set volume
     * @param volume Volume level (0.0 to 1.0)
     */
    void setVolume(float volume);

    /**
     * @brief Get volume
     * @return Current volume level
     */
    float getVolume() const;

    /**
     * @brief Set pitch
     * @param pitch Pitch multiplier (1.0 = normal)
     */
    void setPitch(float pitch);

    /**
     * @brief Get pitch
     * @return Current pitch multiplier
     */
    float getPitch() const;

    /**
     * @brief Set whether this source is looping
     * @param loop Whether source should loop
     */
    void setLoop(bool loop);

    /**
     * @brief Get loop state
     * @return Whether source is looping
     */
    bool getLoop() const;

    /**
     * @brief Set the priority of this source
     * @param priority Priority value (0 = highest, 255 = lowest)
     */
    void setPriority(int priority);

    /**
     * @brief Get priority
     * @return Current priority value
     */
    int getPriority() const;

    /**
     * @brief Set whether this is a 3D positional sound
     * @param is3D Whether to enable 3D positioning
     */
    void set3D(bool is3D);

    /**
     * @brief Check if this is a 3D positional sound
     * @return True if 3D positioning is enabled
     */
    bool is3D() const;

    /**
     * @brief Set minimum and maximum distance for 3D attenuation
     * @param minDistance Distance at which sound is at full volume
     * @param maxDistance Distance at which attenuation stops
     */
    void setDistanceRange(float minDistance, float maxDistance);

    /**
     * @brief Get minimum distance
     * @return Minimum distance for 3D attenuation
     */
    float getMinDistance() const;

    /**
     * @brief Get maximum distance
     * @return Maximum distance for 3D attenuation
     */
    float getMaxDistance() const;

    /**
     * @brief Set sound cone angles for directional audio
     * @param innerAngle Inner cone angle in degrees (full volume)
     * @param outerAngle Outer cone angle in degrees (attenuated volume)
     * @param outerVolume Volume scale at outer angle (0.0 to 1.0)
     */
    void setConeSettings(float innerAngle, float outerAngle, float outerVolume);

    /**
     * @brief Get cone settings
     * @param outInnerAngle Reference to store inner angle
     * @param outOuterAngle Reference to store outer angle
     * @param outOuterVolume Reference to store outer volume
     */
    void getConeSettings(float& outInnerAngle, float& outOuterAngle, float& outOuterVolume) const;

    /**
     * @brief Set Doppler scale for this source
     * @param dopplerScale Doppler effect scale (1.0 = realistic)
     */
    void setDopplerScale(float dopplerScale);

    /**
     * @brief Get Doppler scale
     * @return Current Doppler scale
     */
    float getDopplerScale() const;

    /**
     * @brief Set spread angle for 3D panning
     * @param spread Spread angle in degrees (0-360)
     */
    void setSpread(float spread);

    /**
     * @brief Get spread angle
     * @return Current spread angle
     */
    float getSpread() const;

    /**
     * @brief Set occlusion factor manually
     * @param occlusionFactor Occlusion factor (0.0 = no occlusion, 1.0 = fully occluded)
     */
    void setOcclusionFactor(float occlusionFactor);

    /**
     * @brief Get current occlusion factor
     * @return Occlusion factor
     */
    float getOcclusionFactor() const;

    /**
     * @brief Check if audio is currently playing
     * @return True if playing
     */
    bool isPlaying() const;

    /**
     * @brief Check if audio is currently paused
     * @return True if paused
     */
    bool isPaused() const;

    /**
     * @brief Get current playback position
     * @return Position in seconds
     */
    float getCurrentTime() const;

    /**
     * @brief Set playback position
     * @param time Position in seconds
     */
    void setCurrentTime(float time);

    /**
     * @brief Set velocity for Doppler effect calculation
     * @param velocity Velocity vector
     */
    void setVelocity(const glm::vec3& velocity);

    /**
     * @brief Get velocity
     * @return Current velocity vector
     */
    glm::vec3 getVelocity() const;

    /**
     * @brief Set audio group
     * @param groupName Name of audio group
     */
    void setGroup(const std::string& groupName);

    /**
     * @brief Get audio group
     * @return Current audio group name
     */
    const std::string& getGroup() const;

    /**
     * @brief Set whether source ignores listener pausing
     * @param ignoreListenerPause Whether to ignore listener pause
     */
    void setIgnoreListenerPause(bool ignoreListenerPause);

    /**
     * @brief Get whether source ignores listener pausing
     * @return True if source ignores listener pause
     */
    bool getIgnoreListenerPause() const;

    /**
     * @brief Set whether source uses raytraced audio
     * @param useRaytracing Whether to use raytraced audio
     */
    void setUseRaytracing(bool useRaytracing);

    /**
     * @brief Get whether source uses raytraced audio
     * @return True if source uses raytraced audio
     */
    bool getUseRaytracing() const;

    /**
     * @brief Set max number of reflections for this source
     * @param maxReflections Maximum reflection count
     */
    void setMaxReflections(int maxReflections);

    /**
     * @brief Get max number of reflections
     * @return Maximum reflection count
     */
    int getMaxReflections() const;

    /**
     * @brief Set effect parameters
     * @param reverbSend Reverb send amount (0-1)
     * @param distortion Distortion amount (0-1)
     * @param lowpass Lowpass cutoff frequency (0-1, 1=disabled)
     */
    void setEffectParameters(float reverbSend, float distortion, float lowpass);

    /**
     * @brief Get effect parameters
     * @param outReverbSend Reference to store reverb send amount
     * @param outDistortion Reference to store distortion amount
     * @param outLowpass Reference to store lowpass cutoff
     */
    void getEffectParameters(float& outReverbSend, float& outDistortion, float& outLowpass) const;

    /**
     * @brief Internal use - get FMOD channel
     * @return Pointer to FMOD channel
     */
    FMOD::Channel* getChannel() const;

    /**
     * @brief Internal use - set FMOD channel
     * @param channel Pointer to FMOD channel
     */
    void setChannel(FMOD::Channel* channel);

    /**
     * @brief Set position offset from entity
     * @param offset Position offset in local space
     */
    void setPositionOffset(const glm::vec3& offset);

    /**
     * @brief Get position offset
     * @return Current position offset
     */
    glm::vec3 getPositionOffset() const;

    /**
     * @brief Play a one-shot sound without changing the assigned clip
     * @param clip Audio clip to play
     * @param volume Volume scale
     * @return True if sound was played successfully
     */
    bool playOneShot(AudioClip* clip, float volume = 1.0f);

private:
    // State
    AudioClip* m_clip;
    FMOD::Channel* m_channel;
    int m_channelId;
    bool m_isPlaying;
    bool m_isPaused;
    bool m_isInitialized;

    // Playback parameters
    float m_volume;
    float m_pitch;
    bool m_loop;
    int m_priority;
    std::string m_group;
    bool m_ignoreListenerPause;

    // 3D audio parameters
    bool m_is3D;
    float m_minDistance;
    float m_maxDistance;
    float m_innerConeAngle;
    float m_outerConeAngle;
    float m_outerConeVolume;
    float m_dopplerScale;
    float m_spread;
    glm::vec3 m_velocity;
    glm::vec3 m_positionOffset;

    // Audio raytracing parameters
    bool m_useRaytracing;
    int m_maxReflections;
    float m_occlusionFactor;

    // Effects
    float m_reverbSend;
    float m_distortion;
    float m_lowpass;

    // Reference to audio system
    AudioSystem* m_audioSystem;

    // Update 3D attributes based on transform
    void update3DAttributes();

    // Update effect parameters
    void updateEffects();

    // Apply playback settings to channel
    void applyChannelSettings();
};