#include "components/AudioListenerComponent.h"
#include "AudioSystem.h"
#include "Entity.h"
#include "Scene.h"

// Constructor
AudioListener::AudioListener() :
    m_isActive(false),
    m_isPaused(false),
    m_isInitialized(false),
    m_velocity(0.0f),
    m_scale(1.0f),
    m_positionOffset(0.0f),
    m_audioSystem(nullptr)
{
}

// Destructor
AudioListener::~AudioListener()
{
    if (m_isInitialized && m_audioSystem)
    {
        m_audioSystem->unregisterListener(this);
    }
}

// Initialize component
void AudioListener::initialize()
{
    if (m_isInitialized)
        return;

    // Get the audio system from the scene
    Scene* scene = getScene();
    if (!scene)
        return;

    m_audioSystem = static_cast<AudioSytem*>(scene->getSystem("AudioSystem"));
    if (!m_audioSystem)
        return;

    // Register with the audio system
    m_audioSystem->registerListener(this);

    m_isInitialized = true;
}

// Update component
void AudioListener::update(float deltaTime)
{
    if (!m_isInitialized || !m_isActive)
        return;

    // Update attributes based on entity transform
    updateAttributes();
}

// Clean up resources when destroyed
void AudioListener::onDestroy()
{
    if (m_isInitialized && m_audioSystem)
    {
        m_audioSystem->unregisterListener(this);
    }
}

// React to entity transform changes
void AudioListener::onTransformChanged()
{
    if (m_isInitialized && m_isActive)
    {
        updateAttributes();
    }
}

// Get component type name
const char* AudioListener::getTypeName() const
{
    return "AudioListener";
}

// Set whether this is the active listener
void AudioListener::setActive(bool active)
{
    if (m_isActive == active)
        return;

    m_isActive = active;

    if (m_isInitialized && m_audioSystem && active)
    {
        // Register as the active listener
        m_audioSystem->registerListener(this);
    }
}

// Check if this is the active listener
bool AudioListener::isActive() const
{
    return m_isActive;
}

// Set velocity for Doppler effect calculation
void AudioListener::setVelocity(const glm::vec3& velocity)
{
    m_velocity = velocity;

    if (m_isInitialized && m_isActive)
    {
        updateAttributes();
    }
}

// Get velocity
glm::vec3 AudioListener::getVelocity() const
{
    return m_velocity;
}

// Get position
glm::vec3 AudioListener::getPosition() const
{
    if (!getEntity())
        return m_positionOffset;

    return getEntity()->getWorldPosition() + m_positionOffset;
}

// Get forward vector
glm::vec3 AudioListener::getForward() const
{
    if (!getEntity())
        return glm::vec3(0.0f, 0.0f, -1.0f);

    glm::quat rotation = getEntity()->getWorldRotation();
    return rotation * glm::vec3(0.0f, 0.0f, -1.0f);
}

// Get up vector
glm::vec3 AudioListener::getUp() const
{
    if (!getEntity())
        return glm::vec3(0.0f, 1.0f, 0.0f);

    glm::quat rotation = getEntity()->getWorldRotation();
    return rotation * glm::vec3(0.0f, 1.0f, 0.0f);
}

// Pause all sounds except for those that ignore listener pause
void AudioListener::setPaused(bool pause)
{
    m_isPaused = pause;

    // In a full implementation, we'd use FMOD channel groups to pause
    // all sounds except those marked to ignore listener pause
    if (m_isInitialized && m_isActive && m_audioSystem)
    {
        if (pause)
        {
            m_audioSystem->pauseAll();
        }
        else
        {
            m_audioSystem->resumeAll();
        }
    }
}

// Check if listener is paused
bool AudioListener::isPaused() const
{
    return m_isPaused;
}

// Set global audio scale
void AudioListener::setScale(float scale)
{
    m_scale = scale;

    // In a full implementation, we'd scale the master volume
    if (m_isInitialized && m_isActive && m_audioSystem)
    {
        m_audioSystem->setMasterVolume(scale);
    }
}

// Get global audio scale
float AudioListener::getScale() const
{
    return m_scale;
}

// Get right vector
glm::vec3 AudioListener::getRight() const
{
    // Right is cross product of forward and up
    return glm::cross(getForward(), getUp());
}

// Set position offset from entity
void AudioListener::setPositionOffset(const glm::vec3& offset)
{
    m_positionOffset = offset;

    if (m_isInitialized && m_isActive)
    {
        updateAttributes();
    }
}

// Get position offset
glm::vec3 AudioListener::getPositionOffset() const
{
    return m_positionOffset;
}

// Private helper methods

// Update attributes based on transform
void AudioListener::updateAttributes()
{
    if (!m_audioSystem || !getEntity())
        return;

    // Get world position from entity
    glm::vec3 position = getEntity()->getWorldPosition() + m_positionOffset;

    // Get forward and up direction from entity rotation
    glm::quat rotation = getEntity()->getWorldRotation();
    glm::vec3 forward = rotation * glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 up = rotation * glm::vec3(0.0f, 1.0f, 0.0f);

    // Update listener position in audio system
    m_audioSystem->setListenerPosition(position, forward, up);
}