// -------------------------------------------------------------------------
// ComponentPool.h
// -------------------------------------------------------------------------
#pragma once

#include "Utility/Serialization/SerializationUtility.h"
#include "ECS/ComponentTypes.h"

#include <vector>
#include <unordered_map>
#include <memory>
#include <mutex>

namespace PixelCraft::ECS
{

    /**
     * @brief Interface for type-erased component pool
     */
    class IComponentPool
    {
    public:
        /**
         * @brief Virtual destructor
         */
        virtual ~IComponentPool() = default;

        /**
         * @brief Get raw component data
         * @param entity Entity ID
         * @return Pointer to component data or nullptr
         */
        virtual void* getComponentRaw(EntityID entity) = 0;

        /**
         * @brief Remove all components
         */
        virtual void clear() = 0;

        /**
         * @brief Remove a component
         * @param entity Entity ID
         * @return True if component was removed
         */
        virtual bool destroyComponent(EntityID entity) = 0;

        /**
         * @brief Serialize a component
         * @param entity Entity ID
         * @param serializer Serializer to use
         * @return Result of serialization
         */
        virtual Utility::Serialization::SerializationResult serializeComponent(
            EntityID entity,
            Utility::Serialization::Serializer& serializer) = 0;

        /**
         * @brief Deserialize a component
         * @param entity Entity ID
         * @param deserializer Deserializer to use
         * @return Result of deserialization
         */
        virtual Utility::Serialization::SerializationResult deserializeComponent(
            EntityID entity,
            Utility::Serialization::Deserializer& deserializer) = 0;
    };

    /**
     * @brief Type-safe component storage container
     */
    template<typename T>
    class ComponentPool : public IComponentPool
    {
    public:
        /**
         * @brief Create a component for an entity
         * @param entity Entity ID
         * @param args Constructor arguments
         * @return Reference to the created component
         */
        template<typename... Args>
        T& create(EntityID entity, Args&&... args)
        {
            std::lock_guard<std::mutex> lock(m_mutex);

            // Check if entity already has this component
            auto it = m_entityMap.find(entity);
            if (it != m_entityMap.end())
            {
                return m_components[it->second];
            }

            // Create new component
            size_t index;
            if (!m_freeIndices.empty())
            {
                // Reuse a free slot
                index = m_freeIndices.front();
                m_freeIndices.pop();
                m_components[index] = T(std::forward<Args>(args)...);
            }
            else
            {
                // Add to end of vector
                index = m_components.size();
                m_components.emplace_back(std::forward<Args>(args)...);
            }

            // Set owner entity ID
            m_components[index].owner = entity;

            // Map entity to component index
            m_entityMap[entity] = index;

            return m_components[index];
        }

        /**
         * @brief Destroy a component for an entity
         * @param entity Entity ID
         * @return True if component was destroyed
         */
        bool destroy(EntityID entity) override
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            return destroyComponent(entity);
        }

        /**
         * @brief Get component for an entity
         * @param entity Entity ID
         * @return Reference to the component
         * @throws std::out_of_range if entity doesn't have the component
         */
        T& get(EntityID entity)
        {
            std::lock_guard<std::mutex> lock(m_mutex);

            auto it = m_entityMap.find(entity);
            if (it == m_entityMap.end())
            {
                throw std::out_of_range("Entity does not have this component");
            }

            return m_components[it->second];
        }

        /**
         * @brief Check if an entity has this component
         * @param entity Entity ID
         * @return True if the entity has the component
         */
        bool has(EntityID entity) const
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            return m_entityMap.find(entity) != m_entityMap.end();
        }

        /**
         * @brief Execute a function for each component
         * @param func Function to execute
         */
        void forEach(std::function<void(EntityID, T&)> func)
        {
            std::lock_guard<std::mutex> lock(m_mutex);

            for (const auto& [entity, index] : m_entityMap)
            {
                func(entity, m_components[index]);
            }
        }

        /**
         * @brief Get the number of components
         * @return Component count
         */
        size_t size() const
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            return m_entityMap.size();
        }

        /**
         * @brief Clear all components
         */
        void clear() override
        {
            std::lock_guard<std::mutex> lock(m_mutex);

            m_components.clear();
            m_entityMap.clear();

            // Clear free indices queue
            std::queue<size_t> empty;
            std::swap(m_freeIndices, empty);
        }

        /**
         * @brief Get raw component data
         * @param entity Entity ID
         * @return Pointer to component data or nullptr
         */
        void* getComponentRaw(EntityID entity) override
        {
            std::lock_guard<std::mutex> lock(m_mutex);

            auto it = m_entityMap.find(entity);
            if (it == m_entityMap.end())
            {
                return nullptr;
            }

            return &m_components[it->second];
        }

        /**
         * @brief Serialize a component
         * @param entity Entity ID
         * @param serializer Serializer to use
         * @return Result of the operation
         */
        Utility::Serialization::SerializationResult serializeComponent(
            EntityID entity,
            Utility::Serialization::Serializer& serializer) override
        {
            std::lock_guard<std::mutex> lock(m_mutex);

            auto it = m_entityMap.find(entity);
            if (it == m_entityMap.end())
            {
                return Utility::Serialization::SerializationResult(
                    "Entity does not have this component");
            }

            T& component = m_components[it->second];

            // Write component type name
            auto result = serializer.writeField("typeName", T::getTypeName());
            if (!result) return result;

            // Let the component serialize itself
            if constexpr (std::is_member_function_pointer_v<decltype(&T::serialize)>)
            {
                return component.serialize(serializer);
            }
            else
            {
                // Get schema and use it for serialization
                auto schema = Utility::Serialization::SchemaRegistry::getInstance().getSchema(T::getTypeName());
                if (!schema)
                {
                    return Utility::Serialization::SerializationResult(
                        "No schema registered for component type: " + T::getTypeName());
                }

                return schema->serialize(&component, serializer);
            }
        }

        /**
         * @brief Deserialize a component
         * @param entity Entity ID
         * @param deserializer Deserializer to use
         * @return Result of the operation
         */
        Utility::Serialization::SerializationResult deserializeComponent(
            EntityID entity,
            Utility::Serialization::Deserializer& deserializer) override
        {
            // Get or create component
            T* component = nullptr;

            {
                std::lock_guard<std::mutex> lock(m_mutex);

                auto it = m_entityMap.find(entity);
                if (it != m_entityMap.end())
                {
                    component = &m_components[it->second];
                }
                else
                {
                    // Create new component
                    component = &create(entity);
                }
            }

            // Let the component deserialize itself
            if constexpr (std::is_member_function_pointer_v<decltype(&T::deserialize)>)
            {
                return component->deserialize(deserializer);
            }
            else
            {
                // Get schema and use it for deserialization
                auto schema = Utility::Serialization::SchemaRegistry::getInstance().getSchema(T::getTypeName());
                if (!schema)
                {
                    return Utility::Serialization::SerializationResult(
                        "No schema registered for component type: " + T::getTypeName());
                }

                return schema->deserialize(component, deserializer);
            }
        }

    private:
        mutable std::mutex m_mutex;
        std::vector<T> m_components;
        std::unordered_map<EntityID, size_t> m_entityMap;
        std::queue<size_t> m_freeIndices;

        bool destroyComponent(EntityID entity)
        {
            // Assumes mutex is already locked

            auto it = m_entityMap.find(entity);
            if (it == m_entityMap.end())
            {
                return false;
            }

            size_t index = it->second;

            // Add index to free list
            m_freeIndices.push(index);

            // Remove from entity map
            m_entityMap.erase(it);

            return true;
        }
    };

} // namespace PixelCraft::ECS