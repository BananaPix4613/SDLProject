// -------------------------------------------------------------------------
// MeshRendererComponent.h
// -------------------------------------------------------------------------
#pragma once

#include "ECS/Component.h"
#include "Rendering/Material.h"
#include "Rendering/Mesh.h"
#include "Rendering/Renderable.h"
#include "Utility/Serialization/SerializationUtility.h"

namespace PixelCraft::Utility::Serialization {
class Serializer;
class Deserializer;
struct SerializationResult;
class Schema;
} // namespace PixelCraft::Utility::Serialization

namespace PixelCraft::ECS {
/**
 * @brief Component for rendering meshes with materials
 */
class MeshRendererComponent : public Component {
  public:
    /**
     * @brief Constructor
     */
    MeshRendererComponent();

    /**
     * @brief Constructor with mesh and material
     * @param _mesh The mesh to render
     * @param _material The material to use
     */
    MeshRendererComponent(std::shared_ptr<Rendering::Mesh> mesh, std::shared_ptr<Rendering::Material> material);

    /**
     * @brief Initialize the component
     */
    void initialize() override;

    /**
     * @brief Create a clone of this component
     * @return A unique pointer to a new MeshRendererComponent instance
     */
    std::unique_ptr<Component> clone() override;

    /**
     * @brief Get the mesh
     * @return The mesh
     */
    std::shared_ptr<Rendering::Mesh> getMesh() const { return m_mesh; }

    /**
     * @brief Set the mesh
     * @param _mesh The mesh to set
     */
    void setMesh(std::shared_ptr<Rendering::Mesh> mesh);

    /**
     * @brief Get the material
     * @return The material
     */
    std::shared_ptr<Rendering::Material> getMaterial() const { return m_material; }

    /**
     * @brief Set the material
     * @param _material The material to set
     */
    void setMaterial(std::shared_ptr<Rendering::Material> material);

    /**
     * @brief Check if the component is valid
     * @return True if the component has both a mesh and material assigned
     */
    bool isValid() const { return m_mesh != nullptr && m_material != nullptr; }

    /**
     * @brief Check if the component is invalid
     * @return True if the component is missing either mesh or material
     */
    bool operator!() const { return !isValid(); }

    /**
     * @brief Get whether the object is visible
     * @return True if visible
     */
    bool isVisible() const { return m_isVisible; }

    /**
     * @brief Set whether the object is visible
     * @param visible Whether the object should be visible
     */
    void setVisible(bool visible) { m_isVisible = visible; }

    /**
     * @brief Get whether the object casts shadows
     * @return True if the object casts shadows
     */
    bool getCastShadows() const { return m_castShadows; }

    /**
     * @brief Set whether the object casts shadows
     * @param castShadows Whether the object should cast shadows
     */
    void setCastShadows(bool castShadows) { m_castShadows = castShadows; }

    /**
     * @brief Get whether the object receives shadows
     * @return True if the object receives shadows
     */
    bool getReceiveShadows() const { return m_receiveShadows; }

    /**
     * @brief Set whether the object receives shadows
     * @param receiveShadows Whether the object should receive shadows
     */
    void setReceiveShadows(bool receiveShadows) { m_receiveShadows = receiveShadows; }

    /**
     * @brief Get the instance count for instanced rendering
     * @return The number of instances
     */
    uint32_t getInstanceCount() const { return m_instanceCount; }

    /**
     * @brief Set the instance count for instanced rendering
     * @param count The number of instances
     */
    void setInstanceCount(uint32_t count) { m_instanceCount = count; }

    /**
     * @brief Get the render layer
     * @return The render layer
     */
    uint32_t getLayer() const { return m_layer; }

    /**
     * @brief Set the render layer
     * @param layer The render layer
     */
    void setLayer(uint32_t layer) { m_layer = layer; }

    /**
     * @brief Get the mesh path for serialization
     * @return The mesh path
     */
    const std::string &getMeshPath() const { return m_meshPath; }

    /**
     * @brief Set the mesh path for serialization
     * @param path The mesh path
     */
    void setMeshPath(const std::string &path) { m_meshPath = path; }

    /**
     * @brief Get the material path for serialization
     * @return The material path
     */
    const std::string &getMaterialPath() const { return m_materialPath; }

    /**
     * @brief Set the material path for serialization
     * @param path The material path
     */
    void setMaterialPath(const std::string &path) { m_materialPath = path; }

    /**
     * @brief Serialize the mesh renderer component to the given serializer
     * @param serializer The serializer to use
     * @return SerializationResult with success/failure status
     */
    Utility::Serialization::SerializationResult
    serialize(Utility::Serialization::Serializer &serializer) const override {
        auto result = Component::serialize(serializer);
        if (!result)
            return result;

        // Now serialize mesh renderer-specific data
        result = serializer.beginObject("MeshRendererComponent");
        if (!result)
            return result;

        // Serialize mesh and material paths
        result = serializer.writeField("meshPath", m_meshPath);
        if (!result)
            return result;

        result = serializer.writeField("materialPath", m_materialPath);
        if (!result)
            return result;

        // Serialize other properties
        result = serializer.writeField("isVisible", m_isVisible);
        if (!result)
            return result;

        result = serializer.writeField("castShadows", m_castShadows);
        if (!result)
            return result;

        result = serializer.writeField("receiveShadows", m_receiveShadows);
        if (!result)
            return result;

        result = serializer.writeField("instanceCount", m_instanceCount);
        if (!result)
            return result;

        result = serializer.writeField("layer", m_layer);
        if (!result)
            return result;

        return serializer.endObject();
    }

    /**
     * @brief Deserialize the mesh renderer component from the given deserializer
     * @param deserializer The deserializer to use
     * @return SerializationResult with success/failure status
     */
    Utility::Serialization::SerializationResult
    deserialize(Utility::Serialization::Deserializer &deserializer) override {
        auto result = Component::deserialize(deserializer);
        if (!result)
            return result;

        // Now deserialize mesh renderer-specific data
        result = deserializer.beginObject("MeshRendererComponent");
        if (!result)
            return result;

        // Deserialize mesh and material paths
        result = deserializer.readField("meshPath", m_meshPath);
        if (!result)
            return result;

        result = deserializer.readField("materialPath", m_materialPath);
        if (!result)
            return result;

        // Deserialize other properties
        result = deserializer.readField("isVisible", m_isVisible);
        if (!result)
            return result;

        result = deserializer.readField("castShadows", m_castShadows);
        if (!result)
            return result;

        result = deserializer.readField("receiveShadows", m_receiveShadows);
        if (!result)
            return result;

        result = deserializer.readField("instanceCount", m_instanceCount);
        if (!result)
            return result;

        result = deserializer.readField("layer", m_layer);
        if (!result)
            return result;

        return deserializer.endObject();
    }

    /**
     * @brief Define the schema for mesh renderer component serialization
     * @param schema The schema to define
     */
    static void defineSchema(Utility::Serialization::Schema &schema) {
        schema.addField("meshPath", Utility::Serialization::ValueType::String);
        schema.addField("materialPath", Utility::Serialization::ValueType::String);
        schema.addField("isVisible", Utility::Serialization::ValueType::Bool);
        schema.addField("castShadows", Utility::Serialization::ValueType::Bool);
        schema.addField("receiveShadows", Utility::Serialization::ValueType::Bool);
        schema.addField("instanceCount", Utility::Serialization::ValueType::UInt32);
        schema.addField("layer", Utility::Serialization::ValueType::UInt32);
    }

    /**
     * @brief Get the type name for serialization
     * @return The type name string
     */
    static std::string getTypeName() { return "MeshRendererComponent"; }

  private:
    std::shared_ptr<Rendering::Mesh> m_mesh;         ///< The mesh to render
    std::shared_ptr<Rendering::Material> m_material; ///< The material to use
    bool m_isVisible = true;                         ///< Whether the object is visible
    bool m_castShadows = true;                       ///< Whether the object casts shadows
    bool m_receiveShadows = true;                    ///< Whether the object receives shadows
    uint32_t m_instanceCount = 1;                    ///< Number of instances for instanced rendering
    uint32_t m_layer = static_cast<uint32_t>(Rendering::RenderLayer::Default); ///< Render layer
    std::string m_meshPath;                                                    ///< Path to the mesh resource
    std::string m_materialPath;                                                ///< Path to the material resource
};

} // namespace PixelCraft::ECS