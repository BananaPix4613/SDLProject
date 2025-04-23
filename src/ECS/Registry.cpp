// -------------------------------------------------------------------------
// Registry.cpp
// -------------------------------------------------------------------------
#include "ECS/Registry.h"
#include "Core/Logger.h"

namespace Log = PixelCraft::Core;

namespace PixelCraft::ECS
{

    // Initialize static members
    std::unordered_map<std::type_index, ComponentTypeID> Registry::s_componentTypes;
    ComponentTypeID Registry::s_nextComponentTypeID = 0;
    std::mutex Registry::s_componentTypesMutex;

    Registry::Registry()
        : m_nextEntityID(1) // Start at 1; 0 is reserved for INVALID_ENTITY
    {
        // Reserve capacity for containers to avoid early reallocations
        m_entities.reserve(1000);
        m_entityMasks.reserve(1000);
    }

    Registry::~Registry()
    {
        clear();
    }

    EntityID Registry::createEntity()
    {
        EntityID entity = generateEntityID();

        // Add the entity to the active set
        m_entities.insert(entity);

        // Initialize an empty component mask
        m_entityMasks[entity] = ComponentMask();

        return entity;
    }

    bool Registry::destroyEntity(EntityID entity)
    {
        if (!isValid(entity))
        {
            Log::warn("Registry: Attempting to destroy invalid entity: " + std::to_string(entity));
            return false;
        }

        // Remove all components
        for (auto& [type, pool] : m_componentPools)
        {
            pool->remove(entity);
        }

        // Remove entity from active set
        m_entities.erase(entity);

        // Remove component mask
        m_entityMasks.erase(entity);

        // Recycle entity ID
        recycleEntityID(entity);

        return true;
    }

    bool Registry::isValid(EntityID entity) const
    {
        return entity != INVALID_ENTITY && m_entities.find(entity) != m_entities.end();
    }

    size_t Registry::getEntityCount() const
    {
        return m_entities.size();
    }

    const std::unordered_set<EntityID>& Registry::getEntities() const
    {
        return m_entities;
    }

    const ComponentMask& Registry::getComponentMask(EntityID entity) const
    {
        static const ComponentMask emptyMask;

        auto it = m_entityMasks.find(entity);
        if (it == m_entityMasks.end())
        {
            return emptyMask;
        }

        return it->second;
    }

    void Registry::serialize(Core::DataNode& node) const
    {
        // Serialize entities
        auto entitiesNode = node.createChild("entities");
        for (EntityID entity : m_entities)
        {
            auto entityNode = entitiesNode.createChild("entity");
            entityNode.setAttribute("id", entity);
        }

        // Serialize component pools
        auto poolsNode = node.createChild("componentPools");
        for (const auto& [type, pool] : m_componentPools)
        {
            auto poolNode = poolsNode.createChild("pool");
            poolNode.setAttribute("typeID", pool->getComponentTypeID());

            // Serialize components in this pool
            pool->serialize(poolNode);
        }
    }

    void Registry::deserialize(const Core::DataNode& node)
    {
        // Clear existing data
        clear();

        // Deserialize entities
        auto entitiesNode = node.getChild("entities");
        if (entitiesNode.isValid())
        {
            for (size_t i = 0; i < entitiesNode.getChildCount(); ++i)
            {
                auto entityNode = entitiesNode.getChild(i);
                EntityID entity = entityNode.getAttribute<EntityID>("id", INVALID_ENTITY);

                if (entity != INVALID_ENTITY)
                {
                    // Ensure our entity ID generation is ahead of deserialized IDs
                    if (entity >= m_nextEntityID)
                    {
                        m_nextEntityID = entity + 1;
                    }

                    // Register the entity
                    m_entities.insert(entity);
                    m_entityMasks[entity] = ComponentMask();
                }
            }
        }

        // Deserialize component pools
        auto poolsNode = node.getChild("componentPools");
        if (poolsNode.isValid())
        {
            for (size_t i = 0; i < poolsNode.getChildCount(); ++i)
            {
                auto poolNode = poolsNode.getChild(i);
                ComponentTypeID typeID = poolNode.getAttribute<ComponentTypeID>("typeID", 0);

                // Find the corresponding pool
                bool poolFound = false;
                for (auto& [type, pool] : m_componentPools)
                {
                    if (pool->getComponentTypeID() == typeID)
                    {
                        // Deserialize components into this pool
                        pool->deserialize(poolNode);

                        // Update component masks
                        for (EntityID entity : m_entities)
                        {
                            if (pool->has(entity))
                            {
                                m_entityMasks[entity].set(typeID);
                            }
                        }

                        poolFound = true;
                        break;
                    }
                }

                if (!poolFound)
                {
                    Log::warn("Registry: Could not find component pool for type ID " + std::to_string(typeID) + " during deserialization");
                }
            }
        }
    }

    void Registry::clear()
    {
        // Clear all component pools
        for (auto& [type, pool] : m_componentPools)
        {
            pool->clear();
        }

        // Clear entity set and masks
        m_entities.clear();
        m_entityMasks.clear();

        // Reset entity ID generation
        m_nextEntityID = 1;

        // Clear recycled entity IDs
        std::queue<EntityID> empty;
        std::swap(m_freeEntityIDs, empty);
    }

    EntityID Registry::generateEntityID()
    {
        if (!m_freeEntityIDs.empty())
        {
            // Reuse a recycled ID
            EntityID id = m_freeEntityIDs.front();
            m_freeEntityIDs.pop();
            return id;
        }

        // Generate a new ID
        return m_nextEntityID++;
    }

    void Registry::recycleEntityID(EntityID entity)
    {
        // Add to queue for reuse
        m_freeEntityIDs.push(entity);
    }

} // namespace PixelCraft::ECS