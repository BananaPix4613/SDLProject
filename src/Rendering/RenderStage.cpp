// -------------------------------------------------------------------------
// RenderStage.cpp
// -------------------------------------------------------------------------
#include "Rendering/RenderStage.h"
#include "Core/Logger.h"

// OpenGL headers
#include <glad/glad.h>

namespace PixelCraft::Rendering
{

    RenderStage::RenderStage(const std::string& name)
        : m_name(name)
        , m_enabled(true)
    {
    }

    RenderStage::~RenderStage()
    {
    }

    void RenderStage::setInput(std::shared_ptr<RenderTarget> input, int index)
    {
        if (index < 0)
        {
            Log::error("RenderStage::setInput - Invalid index: " + std::to_string(index));
            return;
        }

        // Resize inputs vector if necessary
        if (index >= m_inputs.size())
        {
            m_inputs.resize(index + 1);
        }

        m_inputs[index] = input;
    }

    void RenderStage::setOutput(std::shared_ptr<RenderTarget> output)
    {
        m_output = output;
    }

    std::shared_ptr<RenderTarget> RenderStage::getInput(int index) const
    {
        if (index < 0 || index >= m_inputs.size())
        {
            Log::error("RenderStage::getInput - Invalid index: " + std::to_string(index));
            return nullptr;
        }

        return m_inputs[index];
    }

    std::shared_ptr<RenderTarget> RenderStage::getOutput() const
    {
        return m_output;
    }

    bool RenderStage::hasParameter(const std::string& name) const
    {
        return m_parameters.find(name) != m_parameters.end();
    }

    void RenderStage::setEnabled(bool enabled)
    {
        m_enabled = enabled;
    }

    bool RenderStage::isEnabled() const
    {
        return m_enabled;
    }

    const std::string& RenderStage::getName() const
    {
        return m_name;
    }

    void RenderStage::bindInput(int index)
    {
        auto input = getInput(index);
        if (input && input->isInitialized())
        {
            input->bind();
        }
        else
        {
            if (!input)
            {
                Log::warn("RenderStage::bindInput - No input at index: " + std::to_string(index));
            }
            else
            {
                Log::error("RenderStage::bindInput - Input at index " + std::to_string(index) + " is not initialized");
            }

            // Bind default framebuffer
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }
    }

    void RenderStage::bindOutput()
    {
        if (m_output && m_output->isInitialized())
        {
            m_output->bind();
        }
        else
        {
            if (!m_output)
            {
                Log::warn("RenderStage::bindOutput - No output set, binding default framebuffer");
            }
            else
            {
                Log::error("RenderStage::bindOutput - Output is not initialized");
            }

            // Bind default framebuffer
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }
    }

    void RenderStage::clearOutput(bool clearColor, bool clearDepth, bool clearStencil)
    {
        if (m_output && m_output->isInitialized())
        {
            m_output->clear(clearColor, clearDepth, clearStencil);
        }
        else
        {
            if (!m_output)
            {
                Log::warn("RenderStage::clearOutput - No output set");
            }
            else
            {
                Log::error("RenderStage::clearOutput - Output is not initialized");
            }

            // Clear default framebuffer
            GLbitfield clearFlags = 0;
            if (clearColor) clearFlags |= GL_COLOR_BUFFER_BIT;
            if (clearDepth) clearFlags |= GL_DEPTH_BUFFER_BIT;
            if (clearStencil) clearFlags |= GL_STENCIL_BUFFER_BIT;

            if (clearFlags != 0)
            {
                glClear(clearFlags);
            }
        }
    }

    void RenderStage::setViewport(int x, int y, int width, int height)
    {
        if (width <= 0 || height <= 0)
        {
            Log::error("RenderStage::setViewport - Invalid dimensions: " +
                       std::to_string(width) + "x" + std::to_string(height));
            return;
        }

        glViewport(x, y, width, height);
    }

    void RenderStage::resetViewport()
    {
        if (m_output && m_output->isInitialized())
        {
            glViewport(0, 0, m_output->getWidth(), m_output->getHeight());
        }
        else
        {
            // Get viewport from context or use defaults
            // In a real implementation, this would come from the RenderContext
            int viewportDims[4];
            glGetIntegerv(GL_VIEWPORT, viewportDims);

            Log::warn("RenderStage::resetViewport - No valid output, using current viewport: " +
                      std::to_string(viewportDims[2]) + "x" + std::to_string(viewportDims[3]));
        }
    }

} // namespace PixelCraft::Rendering