// -------------------------------------------------------------------------
// MeshRendererComponent.cpp
// -------------------------------------------------------------------------
#include "ECS/Components/MeshRendererComponent.h"
#include "ECS/ComponentRegistry.h"
#include "Core/Logger.h"

namespace PixelCraft::ECS
{

    MeshRendererComponent::MeshRendererComponent()
        : m_mesh(nullptr)
        , m_material(nullptr)
    {
    }

    MeshRendererComponent::MeshRendererComponent(std::shared_ptr<Rendering::Mesh> mesh,
                                               std::shared_ptr<Rendering::Material> material)
        : m_mesh(mesh)
        , m_material(material)
    {
        if (m_mesh)
        {
            m_meshPath = m_mesh->getPath();
        }
        if (m_material)
        {
            m_materialPath = m_material->getPath();
        }
    }

    void MeshRendererComponent::initialize()
    {
        Component::initialize();
        m_typeId = ComponentRegistry::getComponentTypeID<MeshRendererComponent>();
        m_typeName = getTypeName();
    }

    std::unique_ptr<Component> MeshRendererComponent::clone()
    {
        auto clone = std::make_unique<MeshRendererComponent>();
        
        // Copy all member variables
        clone->m_mesh = m_mesh;
        clone->m_material = m_material;
        clone->m_isVisible = m_isVisible;
        clone->m_castShadows = m_castShadows;
        clone->m_receiveShadows = m_receiveShadows;
        clone->m_instanceCount = m_instanceCount;
        clone->m_layer = m_layer;
        clone->m_meshPath = m_meshPath;
        clone->m_materialPath = m_materialPath;
        
        return clone;
    }

    void MeshRendererComponent::setMesh(std::shared_ptr<Rendering::Mesh> mesh)
    {
        m_mesh = mesh;
        if (m_mesh)
        {
            m_meshPath = m_mesh->getPath();
        }
        else
        {
            m_meshPath.clear();
        }
    }

    void MeshRendererComponent::setMaterial(std::shared_ptr<Rendering::Material> material)
    {
        m_material = material;
        if (m_material)
        {
            m_materialPath = m_material->getPath();
        }
        else
        {
            m_materialPath.clear();
        }
    }

} // namespace PixelCraft::ECS 