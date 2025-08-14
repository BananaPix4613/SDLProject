// -------------------------------------------------------------------------
// System.cpp
// -------------------------------------------------------------------------
#include "ECS/System.h"
#include "ECS/World.h"
#include "ECS/Entity.h"
#include "ECS/Registry.h"
#include "Utility/Serialization/Serializer.h"
#include "Utility/Serialization/Deserializer.h"
#include "Core/Logger.h"

namespace PixelCraft::ECS
{

    System::System()
        : m_active(true)
        , m_priority(0)
        , m_serializationPriority(0)
    {
        // Initialize component mask to empty (no required components)
        m_componentMask.reset();
    }

    bool System::initialize()
    {
        // Default implementation does nothing
        // Derived systems should override this to perform initialization
        return true;
    }

    void System::update(float deltaTime)
    {
        // Default implementation does nothing
        // Derived systems should override this to perform per-frame updates
        (void)deltaTime;
    }

    void System::render()
    {
        // Default implementation does nothing
        // Derived systems with visual output should override this
    }

    void System::configure()
    {
        // Default implementation does nothing
        // Derived systems should override this to set up required components

        // Store the component mask for efficient entity filtering
        m_componentMask = getRequiredComponents();
    }

    void System::setWorld(std::weak_ptr<World> world)
    {
        m_world = world;
    }

    Utility::Serialization::SerializationResult System::serialize(World& world, Utility::Serialization::Serializer& serializer)
    {
        // Default implementation does nothing
        // Derived systems should override this to prepare for serialization
        (void)world;
        (void)serializer;

        return Utility::Serialization::SerializationResult();
    }

    Utility::Serialization::SerializationResult System::deserialize(World& world, Utility::Serialization::Deserializer& deserializer)
    {
        // Default implementation does nothing
        // Derived systems should override this to process after deserialization
        (void)world;
        (void)deserializer;

        return Utility::Serialization::SerializationResult();
    }

    bool System::shouldSerialize() const
    {
        // By default, systems participate in serialization
        return true;
    }

    void System::setPriority(int priority)
    {
        m_priority = priority;
    }

    void System::setSerializationPriority(int priority)
    {
        m_serializationPriority = priority;
    }

    void System::setActive(bool active)
    {
        m_active = active;
    }

    std::vector<Entity> System::getEntitiesWithComponents(const ComponentMask& mask)
    {
        std::vector<Entity> result;

        // Get world from weak pointer
        auto world = m_world.lock();
        if (!world)
        {
            Core::error("System::getEntitiesWithComponents: World reference is invalid");
            return result;
        }

        // Get registry from world
        auto registry = world->getRegistry();
        if (!registry)
        {
            Core::error("System::getEntitiesWithComponents: Registry reference is invalid");
            return result;
        }

        // Iterate through all entities in the registry
        for (const auto& entityID : registry->getEntities())
        {
            // Check if the entity's component mask matches the required mask
            if ((registry->getEntityMask(entityID) & mask) == mask)
            {
                result.emplace_back(entityID, registry);
            }
        }

        return result;
    }

    Entity System::getEntity(EntityID id)
    {
        // Get world from weak pointer
        auto world = m_world.lock();
        if (!world)
        {
            Core::error("System::getEntity: World reference is invalid");
            return Entity(); // Return an invalid entity
        }

        // Get entity from world
        return world->getEntity(id);
    }

    // SystemFactory implementation
    std::unordered_map<std::string, std::type_index> SystemFactory::s_systemTypeIndices;
    std::unordered_map<std::type_index, std::string> SystemFactory::s_systemNames;
    std::unordered_map<std::type_index, SystemFactory::CreateSystemFunc> SystemFactory::s_systemFactories;

    SystemFactory& SystemFactory::getInstance()
    {
        static SystemFactory instance;
        return instance;
    }

    std::shared_ptr<System> SystemFactory::createSystem(const std::string& name)
    {
        auto typeIt = s_systemTypeIndices.find(name);
        if (typeIt == s_systemTypeIndices.end())
        {
            Core::error("SystemFactory::createSystem: System not registered: " + name);
            return nullptr;
        }

        return createSystem(typeIt->second);
    }

    std::shared_ptr<System> SystemFactory::createSystem(const std::type_index& typeIndex)
    {
        auto it = s_systemFactories.find(typeIndex);
        if (it == s_systemFactories.end())
        {
            Core::error("SystemFactory::createSystem: System not registered for type index");
            return nullptr;
        }

        auto system = it->second();
        if (!system)
        {
            Core::error("SystemFactory::createSystem: Factory function returned null");
            return nullptr;
        }

        system->configure();
        return system;
    }

    std::string SystemFactory::getSystemName(const std::type_index& typeIndex)
    {
        auto it = s_systemNames.find(typeIndex);
        if (it == s_systemNames.end())
        {
            return "";
        }

        return it->second;
    }

    std::type_index SystemFactory::getSystemTypeIndex(const std::string& name)
    {
        auto it = s_systemTypeIndices.find(name);
        if (it == s_systemTypeIndices.end())
        {
            // Return void type index for invalid name
            return std::type_index(typeid(void));
        }

        return it->second;
    }

} // namespace PixelCraft::ECS