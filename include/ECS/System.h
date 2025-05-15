// -------------------------------------------------------------------------
// System.h
// -------------------------------------------------------------------------
#pragma once

#include <bitset>
#include <functional>
#include <memory>
#include <string>
#include <typeindex>
#include <unordered_map>
#include <vector>

#include "Core/Logger.h"
#include "ECS/Types.h"
#include "ECS/Entity.h"
#include "ECS/ComponentRegistry.h"

namespace Log = PixelCraft::Core;

namespace PixelCraft::ECS
{

    // Forward declarations
    class World;
    class Serializer;
    class Deserializer;

    /**
     * @brief Base class for all ECS systems
     * 
     * Systems contain that logic that operates on entities with specific component
     * combinations. They define behavior and update logic in the ECS architecture.
     */
    class System
    {
    public:
        /**
         * @brief Constructor
         */
        System();

        /**
         * @brief Virtual destructor
         */
        virtual ~System() = default;

        /**
         * @brief Initialize the system
         * Called when the system is first registered with the world
         */
        virtual bool initialize();

        /**
         * @brief Update the system
         * Called each frame to update system logic
         * @param deltaTime Time since last update in seconds
         */
        virtual void update(float deltaTime);

        /**
         * @brief Render the system
         * Called during the render phase, if the system has visual output
         */
        virtual void render();

        /**
         * @brief Configure the system
         * Called after initialization for additional setup
         */
        virtual void configure();

        /**
         * @brief Get the component mask that defines which components this system requires
         * @return Component mask with bits set for required component types
         */
        virtual ComponentMask getRequiredComponents() const = 0;

        /**
         * @brief Set the world reference
         * @param world Weak pointer to the world this system belongs to
         */
        void setWorld(std::weak_ptr<World> world);

        /**
         * @brief Get the world reference
         * @return Weak pointer to the world
         */
        std::weak_ptr<World> getWorld() const
        {
            return m_world;
        }

        /**
         * @brief Get the list of system types this system depends on
         * @return Vector of system type names that this system depends on
         */
        virtual std::vector<std::string> getDependencies() const
        {
            return m_dependencies;
        }

        /**
         * @brief Add a dependency to another system
         * @param systemTypeName The type name of the system this system depends on
         */
        void addDependency(const std::string& systemTypeName)
        {
            // Check if dependency already exists
            if (std::find(m_dependencies.begin(), m_dependencies.end(), systemTypeName) == m_dependencies.end())
            {
                m_dependencies.push_back(systemTypeName);
            }
        }

        /**
         * @brief Process a system event
         * @param eventName Name of the event to process
         */
        virtual void onEvent(const std::string& eventName)
        {
            // Default implementation does nothing
            // Derived systems should override this to handle specific events
        }

        /**
         * @brief Hook called before the world is serialized
         * @param world Reference to the world being serialized
         * @param serializer Reference to the serializer
         */
        virtual void preSerialize(World& world, Serializer& serializer);

        /**
         * @brief Hook called after the world is deserialized
         * @param world Reference to the world that was deserialized
         * @param deserializer Reference to the deserializer
         */
        virtual void postDeserialize(World& world, Deserializer& deserializer);

        /**
         * @brief Determine if this system should be serialized
         * @return True if the system should be serialized, false otherwise
         */
        virtual bool shouldSerialize() const;

        /**
         * @brief Set the execution priority
         * Systems with higher priority are updated first
         * @param priority The priority value
         */
        void setPriority(int priority);

        /**
         * @brief Get the execution priority
         * @return The priority value
         */
        int getPriority() const
        {
            return m_priority;
        }

        /**
         * @brief Set the serialization priority
         * Systems with higher serialization priority are serialized first
         * @param priority The serialization priority value
         */
        void setSerializationPriority(int priority);

        /**
         * @brief Get the serialization priority
         * @return The serialization priority value
         */
        int getSerializationPriority() const
        {
            return m_serializationPriority;
        }

        /**
         * @brief Set the active state of the system
         * @param active True to activate the system, false to deactivate
         */
        void setActive(bool active);

        /**
         * @brief Check if the system is active
         * @return True if the system is active, false otherwise
         */
        bool isActive() const
        {
            return m_active;
        }

        /**
         * @brief Get the type name of the system
         * @return String containing the type name
         */
        virtual std::string getTypeName() const = 0;

        /**
         * @brief Get the type index of the system
         * @return std::type_index represeting the system type
         */
        virtual std::type_index getTypeIndex() const = 0;

    protected:
        /**
         * @brief Helper method to get a component mask for specific component types
         * @tparam Components Component types to include in the mask
         * @return ComponentMask with bits set for the specified component types
         */
        template<typename... Components>
        static ComponentMask getComponentMask();

        /**
         * @brief Iterate over entities that have all specified component types
         * @tparam Components Component types that entities must have
         * @param func Function to call for each matching entity and its components
         */
        template<typename... Components>
        void each(std::function<void(Entity entity, Components&...)> func);

        /**
         * @brief Get all entities that have the components specified in the mask
         * @param mask Component mask to filter entities
         * @return Vector of entities that match the mask
         */
        std::vector<Entity> getEntitiesWithComponents(const ComponentMask& mask);

        /**
         * @brief Get an entity by ID
         * @param id The entity ID
         * @return Entity object representing the entity
         */
        Entity getEntity(EntityID id);

        std::weak_ptr<World> m_world;   ///< Reference to the world this system belongs to
        std::vector<std::string> m_dependencies; ///< List of system type names this system depends on
        bool m_active;                  ///< System active state
        int m_priority;                 ///< Execution priority
        int m_serializationPriority;    ///< Serialization priority
        ComponentMask m_componentMask;  ///< Component mask representing required components
    };

    /**
     * @brief Macro to define system type information
     * @param TypeName The name of the system class
     */
#define DEFINE_SYSTEM_TYPE(TypeName) \
    std::string getTypeName() const override { return #TypeName; } \
    std::type_index getTypeIndex() const override { return std::type_index(typeid(TypeName)); }

    /**
     * @brief Factory for system registration and creation
     */
    class SystemFactory
    {
    public:
        /**
         * @brief Function type for creating system instances
         */
        using CreateSystemFunc = std::function<std::shared_ptr<System>()>;

        /**
         * @brief Get the singleton factory instance
         * @return Reference to the system factory singleton
         */
        static SystemFactory& getInstance();

        /**
         * @brief Register a system type
         * @tparam T The system type to register
         * @param name The name to register the system under
         * @return True if registration was successful, false otherwise
         */
        template<typename T>
        static bool registerSystem(const std::string& name);

        /**
         * @brief Create a system instance by name
         * @param name The registered name of the system
         * @return Shared pointer to the created system
         */
        static std::shared_ptr<System> createSystem(const std::string& name);

        /**
         * @brief Create a system instance by type index
         * @param typeIndex The type index of the system
         * @return Shared pointer to the created system
         */
        static std::shared_ptr<System> createSystem(const std::type_index& typeIndex);

        /**
         * @brief Get the registered name of a system type
         * @param typeIndex The type index of the system
         * @return The registered name
         */
        static std::string getSystemName(const std::type_index& typeIndex);

        /**
         * @brief Get the type index for a registered system name
         * @param name The registered name of the system
         * @return The type index
         */
        static std::type_index getSystemTypeIndex(const std::string& name);

    private:
        /**
         * @brief Private constructor for singleton pattern
         */
        SystemFactory() = default;

        /**
         * @brief Private destructor for singleton pattern
         */
        ~SystemFactory() = default;

        static std::unordered_map<std::string, std::type_index> s_systemTypeIndices;     ///< Map of system names to type indices
        static std::unordered_map<std::type_index, std::string> s_systemNames;           ///< Map of type indices to system names
        static std::unordered_map<std::type_index, CreateSystemFunc> s_systemFactories;  ///< Map of type indices to system factory functions
    };

    template<typename... Components>
    ComponentMask System::getComponentMask()
    {
        ComponentMask mask;
        // Use fold expression to set bits for each component type
        (mask.set(ComponentRegistry::getComponentTypeID<Components>()), ...);
        return mask;
    }

    template<typename... Components>
    void System::each(std::function<void(Entity entity, Components&...)> func)
    {
        auto world = m_world.lock();
        if (!world)
        {
            Log::error("System::each: World reference is invalid");
            return;
        }

        // Get mask for required components
        ComponentMask mask = getComponentMask<Components...>();

        // Get matching entities
        auto entities = getEntitiesWithComponents(mask);

        // Call function for each entity with its components
        for (auto& entity : entities)
        {
            if (entity.isValid())
            {
                // Check if entity has all required components
                if ((entity.getComponentMask() & mask) == mask)
                {
                    // Call the user function with the entity and its components
                    func(entity, entity.getComponent<Components>()...);
                }
            }
        }
    }

    template<typename T>
    bool SystemFactory::registerSystem(const std::string& name)
    {
        static_assert(std::is_base_of<System, T>::value, "Type must derive from System");

        auto& instance = getInstance();
        auto typeIndex = std::type_index(typeid(T));

        // Check if already registered
        if (s_systemNames.find(typeIndex) != s_systemNames.end())
        {
            Log::warn("SystemFactory::registerSystem: System already registered: " + name);
            return false;
        }

        // Create factory function
        CreateSystemFunc createFunc = []() -> std::shared_ptr<System> {
            return std::make_shared<T>();
            };

        // Register system
        s_systemNames[typeIndex] = name;
        s_systemTypeIndices[name] = typeIndex;
        s_systemFactories[typeIndex] = createFunc;

        Log::info("SystemFactory::registerSystem: Registered system: " + name);
        return true;
    }

} // namespace PixelCraft::ECS