// -------------------------------------------------------------------------
// RenderContext.cpp
// -------------------------------------------------------------------------
#include "Rendering/RenderContext.h"
#include "Rendering/Shader.h"
#include "Core/Logger.h"

namespace Log = PixelCraft::Core;

namespace PixelCraft::Rendering
{

    RenderContext::RenderContext()
        : m_viewMatrix(1.0f)
        , m_projMatrix(1.0f)
        , m_viewProjMatrix(1.0f)
        , m_cameraPosition(0.0f)
        , m_cameraDirection(0.0f, 0.0f, -1.0f)
        , m_viewport(0, 0, 1, 1)
        , m_renderSize(1, 1)
        , m_matricesDirty(true)
        , m_frustumDirty(true)
    {
        // Default initialization
    }

    RenderContext::~RenderContext()
    {
        // No specific cleanup needed
    }

    void RenderContext::setViewMatrix(const glm::mat4& view)
    {
        m_viewMatrix = view;
        m_matricesDirty = true;
        m_frustumDirty = true;
    }

    void RenderContext::setProjectionMatrix(const glm::mat4& projection)
    {
        m_projMatrix = projection;
        m_matricesDirty = true;
        m_frustumDirty = true;
    }

    void RenderContext::updateMatrices()
    {
        if (m_matricesDirty)
        {
            m_viewProjMatrix = m_projMatrix * m_viewMatrix;
            m_matricesDirty = false;
        }
    }

    const glm::mat4& RenderContext::getViewMatrix() const
    {
        return m_viewMatrix;
    }

    const glm::mat4& RenderContext::getProjectionMatrix() const
    {
        return m_projMatrix;
    }

    const glm::mat4& RenderContext::getViewProjectionMatrix() const
    {
        // Note: The caller should ensure updateMatrices() is called when needed
        return m_viewProjMatrix;
    }

    void RenderContext::setCameraPosition(const glm::vec3& position)
    {
        m_cameraPosition = position;
    }

    void RenderContext::setCameraDirection(const glm::vec3& direction)
    {
        m_cameraDirection = glm::normalize(direction);
    }

    const glm::vec3& RenderContext::getCameraPosition() const
    {
        return m_cameraPosition;
    }

    const glm::vec3& RenderContext::getCameraDirection() const
    {
        return m_cameraDirection;
    }

    void RenderContext::setViewport(int x, int y, int width, int height)
    {
        m_viewport = glm::ivec4(x, y, width, height);
    }

    void RenderContext::setRenderSize(int width, int height)
    {
        m_renderSize = glm::ivec2(width, height);
    }

    const glm::ivec4& RenderContext::getViewport() const
    {
        return m_viewport;
    }

    const glm::ivec2& RenderContext::getRenderSize() const
    {
        return m_renderSize;
    }

    const Utility::Frustum& RenderContext::getFrustum() const
    {
        // Note: The caller should ensure updateFrustum() is called when needed
        return m_frustum;
    }

    void RenderContext::updateFrustum()
    {
        if (m_frustumDirty)
        {
            // Make sure matrices are up to date
            if (m_matricesDirty)
            {
                updateMatrices();
            }

            // Update the frustum from the view-projection matrix
            m_frustum.extractPlanes(m_viewProjMatrix);
            m_frustumDirty = false;
        }
    }

    void RenderContext::bindShaderUniforms(std::shared_ptr<Shader> shader)  
    {  
       if (!shader)  
       {  
           Log::warn("RenderContext::bindShaderUniforms - Null shader provided");  
           return;  
       }  

       // Make sure matrices are up to date  
       if (m_matricesDirty)  
       {  
           updateMatrices();  
       }  

       // Bind common matrices  
       shader->setUniform("viewMatrix", m_viewMatrix);  
       shader->setUniform("projMatrix", m_projMatrix);  
       shader->setUniform("viewProjMatrix", m_viewProjMatrix);  

       // Bind camera information  
       shader->setUniform("cameraPosition", m_cameraPosition);  
       shader->setUniform("cameraDirection", m_cameraDirection);  

       // Bind viewport and render size information  
       shader->setUniform("viewport", m_viewport);  
       shader->setUniform("renderSize", m_renderSize);  
    }

    glm::vec3 RenderContext::worldToViewSpace(const glm::vec3& worldPos) const
    {
        glm::vec4 viewPos = m_viewMatrix * glm::vec4(worldPos, 1.0f);
        return glm::vec3(viewPos);
    }

    glm::vec4 RenderContext::worldToClipSpace(const glm::vec3& worldPos) const
    {
        return m_viewProjMatrix * glm::vec4(worldPos, 1.0f);
    }

} // namespace PixelCraft::Rendering