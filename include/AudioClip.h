#pragma once

#include <string>
#include <fmod.hpp>

/**
 * @class AudioClip
 * @brief Represents a loaded audio resource
 * 
 * AudioClip encapsulates an audio file loaded into memory
 * or streamed from disk. It contains all the data and settings
 * needed to play the audio through FMOD.
 */
class AudioClip
{
public:
    /**
     * @brief Constructor
     * @param filename Path to the audio file
     * @param streaming Whether to stream from disk (for longer audio)
     * @param is3D Whether this is a positional 3D sound
     */
    AudioClip(const std::string& filename, bool streaming, bool is3D);

    /**
     * @brief Destructor
     */
    ~AudioClip();

    /**
     * @brief Initialize the audio clip
     * @param system FMOD system to load with
     * @return True if initialization succeeded
     */
    bool initialize(FMOD::System* system);

    /**
     * @brief Release audio resources
     */
    void release();

    /**
     * @brief Get the FMOD sound object
     * @return Pointer to FMOD sound
     */
    FMOD::Sound* getSound() const;

    /**
     * @brief Get the filename of the audio
     * @return Path to the audio file
     */
    const std::string& getFilename() const;

    /**
     * @brief Check if this is a 3D positional sound
     * @return True if this is a 3D sound
     */
    bool is3D() const;

    /**
     * @brief Check if this is a streaming sound
     * @return True if this is a streaming sound
     */
    bool isStreaming() const;

    /**
     * @brief Set default volume
     * @param volume Volume level (0.0 to 1.0)
     */
    void setDefaultVolume(float volume);

    /**
     * @brief Get default volume
     * @return Default volume level
     */
    float getDefaultVolume() const;

    /**
     * @brief Set default pitch
     * @param pitch Pitch multiplier (1.0 = normal)
     */
    void setDefaultPitch(float pitch);

    /**
     * @brief Get default pitch
     * @return Default pitch multiplier
     */
    float getDefaultPitch() const;

    /**
     * @brief Set default priority
     * @param priority Priority level (0 = highest, 255 = lowest)
     */
    void setDefaultPriority(int priority);

    /**
     * @brief Get default priority
     * @return Default priority level
     */
    int getDefaultPriority() const;

    /**
     * @brief Set min and max distance for 3D audio
     * @param minDistance Distance at which sound is at full volume
     * @param maxDistance Distance at which attenuation stops
     */
    void setDistanceRange(float minDistance, float maxDistance);

    /**
     * @brief Get min distance
     * @return Min distance for 3D attenuation
     */
    float getMinDistance() const;

    /**
     * @brief Get max distance
     * @return Max distance for 3D attenuation
     */
    float getMaxDistance() const;

    /**
     * @brief Set loop mode
     * @param loop Whether this clip should loop by default
     */
    void setLoop(bool loop);

    /**
     * @brief Get loop mode
     * @return Whether this clip loops by default
     */
    bool getLoop() const;

    /**
     * @brief Get sound length in seconds
     * @return Length in seconds
     */
    float getLength() const;

    /**
     * @brief Set default group name
     * @param groupName Channel group name
     */
    void setDefaultGroup(const std::string& groupName);

    /**
     * @brief Get default group name
     * @return Channel group name
     */
    const std::string& getDefaultGroup() const;

private:
    // FMOD objects
    FMOD::Sound* m_sound;

    // Resource information
    std::string m_filename;
    bool m_is3D;
    bool m_isStreaming;
    bool m_isInitialized;

    // Default settings
    float m_defaultVolume;
    float m_defaultPitch;
    int m_defaultPriority;
    float m_minDistance;
    float m_maxDistance;
    bool m_loop;
    std::string m_defaultGroup;
};