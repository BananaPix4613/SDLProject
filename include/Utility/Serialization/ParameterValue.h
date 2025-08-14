// -------------------------------------------------------------------------
// ParameterValue.h
// -------------------------------------------------------------------------
#pragma once

#include "Utility/Serialization/SerializationTypes.h"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <string>
#include <variant>

namespace PixelCraft::Utility::Serialization {
class Serializer;
class Deserializer;
struct SerializationResult;

/**
 * @brief Type-safe parameter storage with serialization support
 */
class ParameterValue {
  public:
    using ValueVariant =
        std::variant<bool, int32_t, uint32_t, int64_t, uint64_t, float, double, std::string, glm::vec2, glm::vec3,
                     glm::vec4, glm::ivec2, glm::ivec3, glm::ivec4, glm::mat3, glm::mat4, glm::quat>;

    /**
     * @brief Default constructor
     */
    ParameterValue() : m_value(0) {}

    /**
     * @brief Constructor from value
     * @param value The value to store
     */
    template <typename T> ParameterValue(const T &value) : m_value(value) {}

    /**
     * @brief Get the stored value
     * @return The value as the specified type
     * @throws std::bad_variant_access if type doesn't match
     */
    template <typename T> T get() const { return std::get<T>(m_value); }

    /**
     * @brief Get the stored value with default fallback
     * @param defaultValue Default value if type doesn't match
     * @return The value or default if type mismatch
     */
    template <typename T> T get(const T &defaultValue) const {
        try {
            return std::get<T>(m_value);
        } catch (const std::bad_variant_access &) {
            return defaultValue;
        }
    }

    /**
     * @brief Set the stored value
     * @param value The new value
     */
    template <typename T> void set(const T &value) { m_value = value; }

    /**
     * @brief Get the type of the stored value
     * @return ValueType of the stored value
     */
    ValueType getType() const;

    /**
     * @brief Check if the parameter holds a specific type
     * @return True if the parameter holds type T
     */
    template <typename T> bool holds() const { return std::holds_alternative<T>(m_value); }

    /**
     * @brief Serialize the parameter value
     * @param serializer The serializer to use
     * @return SerializationResult with success/failure status
     */
    virtual SerializationResult serialize(Serializer &serializer) const;

    /**
     * @brief Deserialize the parameter value
     * @param deserializer The deserializer to use
     * @return SerializationResult with success/failure status
     */
    virtual SerializationResult deserialize(Deserializer &deserializer);

    /**
     * @brief Get the raw variant
     * @return Reference to the internal variant
     */
    const ValueVariant &getValue() const { return m_value; }

  private:
    ValueVariant m_value;
};

} // namespace PixelCraft::Utility::Serialization