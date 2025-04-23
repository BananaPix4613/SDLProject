#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <glm/glm.hpp>
#include <fmod.hpp>

// Forward declarations
class Scene;
class Entity;
class EventSystem;
class CubeGrid;
class AudioListener;
class AudioSource;
class AudioClip;
class AudioReverb;
class AudioMixer;
struct AudioRayHit;

/**
 * @struct AudioRayHit
 * @brief Information about a raycast hit for audio propagation
 */
struct AudioRayHit
{
    glm::vec3 point;       // Hit point in world space
    glm::vec3 normal;      // Surface normal
    float distance;        // Distance from ray origin
    float absorption;      // Sound absorption coefficient (0-1)
    float reflection;      // Sound reflection coefficient (0-1)
    bool hasHit;           // Whether the ray hit anything

    AudioRayHit() :
        point(0.0f), normal(0.0f, 1.0f, 0.0f), distance(0.0f),
        absorption(0.5f), reflection(0.5f), hasHit(false)
    {
    }
};

/**
 * @enum AudioDistanceModel
 * @brief Defines how sound attenuates over distance
 */
enum class AudioDistanceModel
{
    LINEAR,         // Linear attenuation with distance
    INVERSE,        // Inverse attenuation (realistic, 1/d)
    EXPONENTIAL,    // Exponential attenuation
    CUSTOM          // Custom attenuation curve
};

/**
 * @enum AudioRayMode
 * @brief Defines audio ray tracing detail level
 */
enum class AudioRayMode
{
    NONE,           // No ray tracing, direct path only
    OCCLUSION,      // Just check for occlusion between source and listener
    REFLECTION,     // Single-bounce reflection
    ADVANCED        // Multiple bounces with advanced propagation
};

/**
 * @class AudioSystem
 * @brief Central system for 3D spatial audio and sound management
 * 
 * The AudioSystem manages audio playback, 3D positioning, raytraced sound propagation,
 * and environmental effects. It integrates with the Entity-Component System
 * and handles all interactions with the FMOD audio library.
 */
class AudioSystem
{
public:
    /**
     * @brief Constructor
     */
    AudioSystem();

    /**
     * @brief Destructor
     */
    ~AudioSystem();

    /**
     * @brief Initialize the audio system
     * @param scene Pointer to the scene
     * @param eventSystem Pointer to the event system
     * @return True if initialization succeeded
     */
    bool initialize(Scene* scene, EventSystem* eventSystem);

    /**
     * @brief Shut down the audio system
     */
    void shutdown();

    /**
     * @brief Update audio system state
     * @param deltaTime Time elapsed since last update
     */
    void update(float deltaTime);

    /**
     * @brief Set the cube grid for environmental audio calculations
     * @param cubeGrid Pointer to the cube grid
     */
    void setCubeGrid(CubeGrid* cubeGrid);

    /**
     * @brief Load an audio clip from file
     * @param filename Path to audio file
     * @param streaming Whether to stream from disk (for longer audio)
     * @param is3D Whether the sound is positional
     * @return Pointer to loaded audio clip or nullptr on failure
     */
    AudioClip* loadClip(const std::string& filename, bool streaming = false, bool is3D = true);

    /**
     * @brief Unload an audio clip
     * @param clip Pointer to clip to unload
     */
    void unloadClip(AudioClip* clip);

    /**
     * @brief Create an audio source
     * @param name Name for the source
     * @return Pointer to created audio source
     */
    AudioSource* createSource(const std::string& name);

    /**
     * @brief Destroy an audio source
     * @param source Pointer to source to destroy
     */
    void destroySource(AudioSource* source);

    /**
     * @brief Set volume for the specified audio group
     * @param groupName Name of audio group ("Master", "Music", "SFX", etc.)
     * @param volume Volume level (0.0 to 1.0)
     */
    void setGroupVolume(const std::string& groupName, float volume);

    /**
     * @brief Get volume for the specified audio group
     * @param groupName Name of audio group
     * @return Current volume (0.0 to 1.0)
     */
    float getGroupVolume(const std::string& groupName) const;

    /**
     * @brief Play a sound directly (without creating an AudioSource)
     * @param clipName Name of registered audio clip
     * @param position Position in 3D space (use glm::vec3(0) for non-positional)
     * @param volume Volume scale (0.0 to 1.0)
     * @param pitch Pitch adjustment (1.0 = normal)
     * @param loop Whether to loop the sound
     * @return ID of the playing sound instance (for stopping/tracking)
     */
    int playSound(const std::string& clipName, const glm::vec3& position = glm::vec3(0.0f),
                  float volume = 1.0f, float pitch = 1.0f, bool loop = false);

    /**
     * @brief Stop a sound instance
     * @param instanceId ID returned from playSound
     * @param fadeOut Whether to fade out or stop immediately
     */
    void stopSound(int instanceId, bool fadeOut = false);

    /**
     * @brief Set listener position manually (for non-component use)
     * @param position Position in 3D space
     * @param forward Forward direction vector
     * @param up Up direction vector
     */
    void setListenerPosition(const glm::vec3& position,
                             const glm::vec3& forward = glm::vec3(0.0f, 0.0f, -1.0f),
                             const glm::vec3& up = glm::vec3(0.0f, 1.0f, 0.0f));

    /**
     * @brief Set global sound properties
     * @param dopplerScale Doppler effect scale (1.0 = realistic)
     * @param distanceFactor Distance scaling factor for 3D audio
     * @param rolloffScale Rolloff scaling factor for attenuation
     */
    void setGlobalAudioProperties(float dopplerScale = 1.0f,
                                  float distanceFactor = 1.0f,
                                  float rolloffScale = 1.0f);

    /**
     * @brief Set environment preset
     * @param presetName Name of environment preset ("CAVE", "HALL", "FOREST", etc.)
     */
    void setEnvironmentPreset(const std::string& presetName);

    /**
     * @brief Apply a custom environment
     * @param roomSize Room size factor (0.0 to 1.0)
     * @param damping Damping factor (0.0 to 1.0)
     * @param diffusion Diffusion factor (0.0 to 1.0)
     * @param wetLevel Wet level for reverb (0.0 to 1.0)
     */
    void setCustomEnvironment(float roomSize, float damping, float diffusion, float wetLevel);

    /**
     * @brief Set master volume
     * @param volume Volume level (0.0 to 1.0)
     */
    void setMasterVolume(float volume);

    /**
     * @brief Get master volume
     * @return Current master volume
     */
    float getMasterVolume() const;

    /**
     * @brief Set ray tracing mode for audio
     * @param mode Ray tracing detail level
     */
    void setRayTracingMode(AudioRayMode mode);

    /**
     * @brief Get current ray tracing mode
     * @return Current ray tracing mode
     */
    AudioRayMode getRayTracingMode() const;

    /**
     * @brief Set ray tracing parameters
     * @param maxBounces Maximum reflection bounces
     * @param raysPerSource Number of rays per sound source
     * @param maxDistance Maximum ray distance
     */
    void setRayTracingParameters(int maxBounces, int raysPerSource, float maxDistance);

    /**
     * @brief Set default distance attenuation model
     * @param model Distance attenuation model
     */
    void setDistanceModel(AudioDistanceModel model);

    /**
     * @brief Pause all audio
     */
    void pauseAll();

    /**
     * @brief Resume all audio
     */
    void resumeAll();

    /**
     * @brief Stop all audio immediately
     */
    void stopAll();

    /**
     * @brief Set sound occlusion parameters
     * @param lowpassCutoff Low-pass filter cutoff when fully occluded (0-1)
     * @param volumeAttenuation Volume attenuation when fully occluded (0-1)
     */
    void setOcclusionParameters(float lowpassCutoff, float volumeAttenuation);

    /**
     * @brief Get sound occlusion parameters
     * @param outLowpassCutoff Low-pass filter cutoff when fully occluded (0-1)
     * @param outVolumeAttenuation Volume attenuation when fully occluded (0-1)
     */
    void getOcclusionParameters(float& outLowpassCutoff, float& outVolumeAttenuation);

    /**
     * @brief Create a reverb zone
     * @param position Center position
     * @param radius Radius of influence
     * @param preset Reverb preset name
     * @return ID of the reverb zone
     */
    int createReverbZone(const glm::vec3& position, float radius, const std::string& preset);

    /**
     * @brief Remove a reverb zone
     * @param zoneId ID of zone to remove
     */
    void removeReverbZone(int zoneId);

    /**
     * @brief Add an audio mixer
     * @param name Name of the mixer
     * @return Pointer to the created mixer
     */
    AudioMixer* createMixer(const std::string& name);

    /**
     * @brief Get an audio mixer by name
     * @param name Name of the mixer
     * @return Pointer to the mixer or nullptr if not found
     */
    AudioMixer* getMixer(const std::string& name) const;

    /**
     * @brief Define a new audio group
     * @param groupName Name of the group
     * @param parentName Parent group name ("Master" if not specified)
     */
    void defineAudioGroup(const std::string& groupName, const std::string& parentName = "Master");

    /**
     * @brief Set parameters for a specific material type
     * @param materialName Material name
     * @param absorption Absorption coefficient (0-1)
     * @param reflection Reflection coefficient (0-1)
     */
    void defineMaterialProperties(const std::string& materialName, float absorption, float reflection);

    /**
     * @brief Set global lowpass filter
     * @param cutoff Cutoff frequency (0.0 to 1.0, 1.0 = no filtering)
     */
    void setGlobalLowpassFilter(float cutoff);

    /**
     * @brief Set global highpass filter
     * @param cutoff Cutoff frequency (0.0 to 1.0, 0.0 = no filtering)
     */
    void setGlobalHighpassFilter(float cutoff);

    /**
     * @brief Cast an audio ray for sound propagation
     * @param origin Ray origin
     * @param direction Ray direction
     * @param maxDistance Maximum ray distance
     * @param hitInfo Reference to store hit information
     * @return True if the ray hit something
     */
    bool castAudioRay(const glm::vec3& origin, const glm::vec3& direction,
                      float maxDistance, AudioRayHit& hitInfo);

    /**
     * @brief Get the number of active sound instances
     * @return Active instance count
     */
    int getActiveInstanceCount() const;

    /**
     * @brief Get the number of active audio sources
     * @return Active source count
     */
    int getActiveSourceCount() const;

    /**
     * @brief Set global audio pitch
     * @param pitch Pitch multiplier (1.0 = normal)
     */
    void setGlobalPitch(float pitch);

    /**
     * @brief Get global audio pitch
     * @return Current pitch multiplier
     */
    float getGlobalPitch() const;

    /**
     * @brief Check if an audio clip is loaded
     * @param filename Path to the audio file
     * @return True if the clip is loaded
     */
    bool isClipLoaded(const std::string& filename) const;

    /**
     * @brief Get an audio clip by name
     * @param filename Path to the audio file
     * @return Pointer to the clip or nullptr if not found
     */
    AudioClip* getClip(const std::string& filename) const;

    /**
     * @brief Register a listener component
     * @param listener Listener component to register
     */
    void registerListener(AudioListener* listener);

    /**
     * @brief Unregister a listener component
     * @param listener Listener component to unregister
     */
    void unregisterListener(AudioListener* listener);

    /**
     * @brief Register a source component
     * @param source Source component to register
     */
    void registerSource(AudioSource* source);

    /**
     * @brief Unregister a source component
     * @param source Source component to unregister
     */
    void unregisterSource(AudioSource* source);

    /**
     * @brief Set whether audio is enabled
     * @param enabled Whether audio is enabled
     */
    void setEnabled(bool enabled);

    /**
     * @brief Check if audio is enabled
     * @return True if audio is enabled
     */
    bool isEnabled() const;

    /**
     * @brief Set max number of audio sources that can play simultaneously
     * @param maxSources Maximum number of sources
     */
    void setMaxSources(int maxSources);

    /**
     * @brief Get max number of audio sources that can play simultaneously
     * @return Maximum number of sources
     */
    int getMaxSources() const;

    /**
     * @brief Get FMOD system (for advanced usage)
     * @return Pointer to FMOD system
     */
    FMOD::System* getFMODSystem();

    /**
     * @brief Gets the channel group using name
     * @param name Name of the channel group
     * @return Pointer to requested channel group
     */
    FMOD::ChannelGroup* getChannelGroup(const std::string& name);

private:
    // FMOD API objects
    FMOD::System* m_fmodSystem;
    
    // Scene reference
    Scene* m_scene;
    EventSystem* m_eventSystem;
    CubeGrid* m_cubeGrid;

    // Audio resources
    std::unordered_map<std::string, std::unique_ptr<AudioClip>> m_audioClips;
    std::unordered_map<std::string, FMOD::ChannelGroup*> m_channelGroups;
    std::unordered_map<int, FMOD::Channel*> m_activeChannels;
    std::unordered_map<std::string, std::unique_ptr<AudioMixer>> m_mixers;

    // Listener management
    std::vector<AudioListener*> m_listeners;
    AudioListener* m_activeListener;

    // Sources and reverb
    std::vector<AudioSource*> m_sources;
    std::vector<std::unique_ptr<AudioReverb>> m_reverbZones;

    // State
    bool m_initialized;
    bool m_enabled;
    float m_masterVolume;
    float m_globalPitch;
    int m_nextChannelId;
    int m_nextReverbZoneId;

    // Audio ray tracing
    AudioRayMode m_rayMode;
    int m_maxBounces;
    int m_raysPerSource;
    float m_maxRayDistance;

    // Audio propagation
    struct MaterialProperties
    {
        float absorption;
        float reflection;
    };
    std::unordered_map<std::string, MaterialProperties> m_materials;

    // Distance models
    AudioDistanceModel m_distanceModel;

    // Occlusion parameters
    float m_occlusionLowpassCutoff;
    float m_occlusionVolumeAttenuation;

    // Settings
    int m_maxSources;

    // Mutex for thread safety
    mutable std::mutex m_audioMutex;

    // Private helper methods
    bool initializeFMOD();
    void shutdownFMOD();
    void updateListenerPosition();
    void updateReverbZones();
    void calculateSoundPropagation(AudioSource* source);
    void calculateOcclusion(AudioSource* source);
    float calculateAttenuation(float distance, float minDistance, float maxDistance);
    void applyEnvironmentToSource(AudioSource* source);
    void cleanupStoppedChannels();
    bool isOverSourceLimit() const;
    AudioSource* prioritizeSourceToStop();
};