// -------------------------------------------------------------------------
// View.h
// -------------------------------------------------------------------------
#pragma once

#include <tuple>
#include <unordered_set>
#include <utility>

#include "ECS/Registry.h"

namespace PixelCraft::ECS
{

    // Forward declaration
    class Registry;
    using ComponentMask = std::bitset<MAX_COMPONENT_TYPES>;

    /**
     * @brief View class for efficient entity-component iteration
     *
     * Provides a way to iterate over all entities that have a specific
     * set of component types, with direct access to those components.
     *
     * @tparam Components Component types to include in the view
     */
    template<typename... Components>
    class View
    {
    public:
        /**
         * @brief Constructor
         * @param registry Reference to the registry this view belongs to
         */
        View(Registry& registry)
            : m_registry(registry)
        {
            // Create component mask for the requested component types
            (m_componentMask.set(Registry::registerComponentType<Components>()), ...);
        }

        /**
         * @brief Iterator class for efficient component iteration
         *
         * Provides iteration over entities with the required components,
         * automatically skipping entities that don't match the component mask.
         */
        class Iterator
        {
        public:
            /**
             * @brief Constructor
             * @param registry Reference to the registry
             * @param entityIt Current position in the entity set
             * @param entityEnd End position in the entity set
             * @param componentMask Mask of required components
             */
            Iterator(Registry& registry, std::unordered_set<EntityID>::const_iterator entityIt,
                     std::unordered_set<EntityID>::const_iterator entityEnd, const ComponentMask& componentMask)
                : m_registry(registry)
                , m_entityIt(entityIt)
                , m_entityEnd(entityEnd)
                , m_componentMask(componentMask)
            {
                // Find the first valid entity that matches the component mask
                findNext();
            }

            /**
             * @brief Pre-increment operator
             * @return Reference to this iterator after increment
             */
            Iterator& operator++()
            {
                ++m_entityIt;
                findNext();
                return *this;
            }

            /**
             * @brief Inequality comparison operator
             * @param other Iterator to compare with
             * @return True if iterators are not equal
             */
            bool operator!=(const Iterator& other) const
            {
                return m_entityIt != other.m_entityIt;
            }

            /**
             * @brief Dereference operator
             * @return Tuple containing entity ID and pointers to components
             */
            std::tuple<EntityID, Components*...> operator*() const
            {
                return std::make_tuple(*m_entityIt, m_registry.getComponent<Components>(*m_entityIt)...);
            }

        private:
            /**
             * @brief Find the next entity that matches the component mask
             *
             * Advances the entity iterator until finding an entity that has
             * all the required components, or reaching the end.
             */
            void findNext()
            {
                while (m_entityIt != m_entityEnd &&
                       (m_registry.getComponentMask(*m_entityIt) & m_componentMask) != m_componentMask)
                {
                    ++m_entityIt;
                }
            }

            Registry& m_registry;
            std::unordered_set<EntityID>::const_iterator m_entityIt;
            std::unordered_set<EntityID>::const_iterator m_entityEnd;
            ComponentMask m_componentMask;
        };

        /**
         * @brief Get an iterator to the beginning of the view
         * @return Iterator pointing to the first matching entity
         */
        Iterator begin()
        {
            return Iterator(
                m_registry,
                m_registry.getEntities().begin(),
                m_registry.getEntities().end(),
                m_componentMask
            );
        }

        /**
         * @brief Get an iterator to the end of the view
         * @return Iterator pointing past the last entity
         */
        Iterator end()
        {
            return Iterator(
                m_registry,
                m_registry.getEntities().end(),
                m_registry.getEntities().end(),
                m_componentMask
            );
        }

        /**
         * @brief Execute a function for each matching entity
         * @param func Function to execute for each entity
         *
         * The function should accept an entity ID followed by component pointers
         * matching the component types of the view.
         */
        template<typename Func>
        void each(Func&& func)
        {
            for (auto it = begin(); it != end(); ++it)
            {
                std::apply(std::forward<Func>(func), *it);
            }
        }

    private:
        Registry& m_registry;
        ComponentMask m_componentMask;
    };

} // namespace PixelCraft::ECS