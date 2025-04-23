// -------------------------------------------------------------------------
// Component.h
// -------------------------------------------------------------------------
#pragma once

#include <memory>
#include <functional>
#include <string>
#include <unordered_map>
#include <typeindex>
#include <vector>
#include <mutex>

#include "Core/DataNode.h"

namespace PixelCraft::ECS
{

    using EntityID = uint32_t;
    using ComponentTypeID = uint16_t;

    // Forward declaration
    class Component;

    /**
     * @brief Type alias for component smart pointers
     */
    using ComponentPtr = std::shared_ptr<Component>;

    /**
     * @brief Base class for all components in the ECS architecture.
     *
     * Components are data containers that can be attached to entities. They
     * define the characteristics and behaviors of entities through composition.
     * This base class provides the common interface that all components must implement.
     */
    class Component : public std::enable_shared_from_this<Component>
    {
    public:
        /**
         * @brief Default constructor
         */
        Component();

        /**
         * @brief Virtual destructor to allow proper cleanup of derived classes
         */
        virtual ~Component();

        /**
         * @brief Initialize the component
         * @return True if initialization was successful, false otherwise
         */
        virtual bool initialize();

        /**
         * @brief Called when component is first added to an entity
         * Performs initial setup that requires the component to be attached to an entity
         */
        virtual void start();

        /**
         * @brief Update component state
         * @param deltaTime Time elapsed since last update in seconds
         */
        virtual void update(float deltaTime);

        /**
         * @brief Render component visualization (if applicable)
         */
        virtual void render();

        /**
         * @brief Called when component is being destroyed
         * Perform any cleanup operations here
         */
        virtual void onDestroy();

        /**
         * @brief Set the owner entity of this component
         * @param owner EntityID of the owning entity
         */
        void setOwner(EntityID owner);

        /**
         * @brief Get the owner entity of this component
         * @return EntityID of the owning entity
         */
        EntityID getOwner() const;

        /**
         * @brief Get the type name of this component
         * @return String representation of the component type
         */
        virtual std::string getTypeName() const = 0;

        /**
         * @brief Get the type ID of this component
         * @return Numeric type identifier
         */
        virtual ComponentTypeID getTypeID() const = 0;

        /**
         * @brief Get the type index of this component
         * @return std::type_index representing the component type
         */
        virtual std::type_index getTypeIndex() const = 0;

        /**
         * @brief Serialize component state to a data node
         * @param node DataNode to serialize to
         */
        virtual void serialize(Core::DataNode& node) const;

        /**
         * @brief Deserialize component state from a data node
         * @param node DataNode to deserialize from
         */
        virtual void deserialize(const Core::DataNode& node);

        /**
         * @brief Create a clone of this component
         * @return Shared pointer to a new component with the same type and state
         */
        virtual ComponentPtr clone() const = 0;

        /**
         * @brief Enable or disable the component
         * @param enabled True to enable, false to disable
         */
        void setEnabled(bool enabled);

        /**
         * @brief Check if the component is enabled
         * @return True if enabled, false if disabled
         */
        bool isEnabled() const;

        /**
         * @brief Check if the component is initialized
         * @return True if initialized, false otherwise
         */
        bool isInitialized() const;

    protected:
        /** Owner entity ID */
        EntityID m_owner;

        /** Enabled state */
        bool m_enabled;

        /** Initialization state */
        bool m_initialized;
    };

    /**
     * @brief Factory for component registration and creation
     *
     * Provides a central registry for component types and handles
     * creation of components by type name or ID.
     */
    class ComponentFactory
    {
    public:
        /** Type definition for component creation function */
        using CreateComponentFunc = std::function<ComponentPtr()>;

        /**
         * @brief Register a component type with the factory
         * @param name Type name of the component
         * @param typeID Unique type ID for the component
         * @param createFunc Function to create instances of the component
         * @return True if registration was successful, false otherwise
         */
        static bool registerComponent(const std::string& name, ComponentTypeID typeID, CreateComponentFunc createFunc);

        /**
         * @brief Create a component by type name
         * @param name Type name of the component to create
         * @return Shared pointer to the created component, or nullptr if type not found
         */
        static ComponentPtr createComponent(const std::string& name);

        /**
         * @brief Create a component by type ID
         * @param typeID Type ID of the component to create
         * @return Shared pointer to the created component, or nullptr if type not found
         */
        static ComponentPtr createComponent(ComponentTypeID typeID);

        /**
         * @brief Get the type ID for a component type name
         * @param name Type name to look up
         * @return Component type ID, or 0 if not found
         */
        static ComponentTypeID getComponentTypeID(const std::string& name);

        /**
         * @brief Get the type name for a component type ID
         * @param typeID Type ID to look up
         * @return Component type name, or empty string if not found
         */
        static std::string getComponentTypeName(ComponentTypeID typeID);

        /**
         * @brief Template helper for component registration
         * @tparam T Component type to register
         * @param name Type name for the component
         * @return True if registration was successful, false otherwise
         */
        template<typename T>
        static bool registerComponent(const std::string& name)
        {
            auto typeID = T::getStaticTypeID();
            return registerComponent(name, typeID, []() -> ComponentPtr {
                return std::make_shared<T>();
                                     });
        }

        /**
         * @brief Get all registered component type names
         * @return Vector of component type names
         */
        static std::vector<std::string> getRegisteredComponentNames();

        /**
         * @brief Check if a component type is registered
         * @param name Type name to check
         * @return True if registered, false otherwise
         */
        static bool isComponentRegistered(const std::string& name);

        /**
         * @brief Check if a component type ID is registered
         * @param typeID Type ID to check
         * @return True if registered, false otherwise
         */
        static bool isComponentRegistered(ComponentTypeID typeID);

    private:
        /** Map of component type names to type IDs */
        static std::unordered_map<std::string, ComponentTypeID> s_typeIDs;

        /** Map of component type IDs to type names */
        static std::unordered_map<ComponentTypeID, std::string> s_typeNames;

        /** Map of component type IDs to factory functions */
        static std::unordered_map<ComponentTypeID, CreateComponentFunc> s_factories;

        /** Mutex for thread-safe access to registration maps */
        static std::mutex s_registryMutex;
    };

    /**
     * @brief Macro for defining component type information
     *
     * Automatically implements the type-related methods required by the Component base class.
     *
     * @param TypeName The class name of the component
     * @param TypeID Unique numeric ID for the component type
     */
#define DEFINE_COMPONENT_TYPE(TypeName, TypeID) \
    static ComponentTypeID getStaticTypeID() { return TypeID; } \
    ComponentTypeID getTypeID() const override { return TypeID; } \
    std::string getTypeName() const override { return #TypeName; } \
    std::type_index getTypeIndex() const override { return std::type_index(typeid(TypeName)); } \
    ComponentPtr clone() const override { \
        auto clone = std::make_shared<TypeName>(*this); \
        clone->initialize(); \
        return clone; \
    }

} // namespace PixelCraft::ECS