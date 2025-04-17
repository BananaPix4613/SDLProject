#pragma once

#include "Component.h"
#include <glm.hpp>

// Forward declarations
class AudioSystem;

/**
 * @class AudioListener
 * @brief Component that receives sound in the game world
 * 
 * The AudioListener represents the "ears" of the game world.
 * It's typically attached to the player's camera and determines
 * how sounds are heard based on position and orientation.
 */
class AudioListener : public Component
{
public:
    /**
     * @brief Constructor
     */
    AudioListener();

    /**
     * @brief Destructor
     */
    ~AudioListener() override;

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
     * @brief Set whether this is the active listener
     * @param active Whether this listener should be active
     */
    void setActive(bool active);

    /**
     * @brief Check if this is the active listener
     * @return True if this is the active listener
     */
    bool isActive() const;

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
     * @brief Get position
     * @return Current position in world space
     */
    glm::vec3 getPosition() const;

    /**
     * @brief Get forward vector
     * @return Forward direction vector
     */
    glm::vec3 getForward() const;

    /**
     * @brief Get up vector
     * @return Up direction vector
     */
    glm::vec3 getUp() const;

    /**
     * @brief Pause all sounds except for those that ignore listener pause
     * @param pause Whether to pause
     */
    void setPaused(bool pause);

    /**
     * @brief Check if listener is paused
     * @return True if paused
     */
    bool isPaused() const;

    /**
     * @brief Set global audio scale
     * @param scale Global scale for all received sound (0.0 to 1.0)
     */
    void setScale(float scale);

    /**
     * @brief Get global audio scale
     * @return Current global scale
     */
    float getScale() const;

    /**
     * @brief Get right vector
     * @return Right direction vector
     */
    glm::vec3 getRight() const;

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

private:
    // State
    bool m_isActive;
    bool m_isPaused;
    bool m_isInitialized;

    // Audio parameters
    glm::vec3 m_velocity;
    float m_scale;
    glm::vec3 m_positionOffset;

    // Reference to audio system
    AudioSystem* m_audioSystem;

    // Update attributes based on transform
    void updateAttributes();
};