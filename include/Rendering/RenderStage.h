// -------------------------------------------------------------------------
// RenderStage.h
// -------------------------------------------------------------------------
#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>
#include <glm/glm.hpp>

#include "Rendering/RenderTarget.h"
#include "Rendering/RenderContext.h"

namespace Log = PixelCraft::Core;

namespace PixelCraft::Rendering
{

    /**
     * @brief Base class for a single pass in a rendering pipeline
     */
    class RenderStage
    {
    public:
        /**
         * @brief Constructor
         * @param name Name of the render stage
         */
        RenderStage(const std::string& name);

        /**
         * @brief Destructor
         */
        virtual ~RenderStage();

        /**
         * @brief Initialize the render stage
         * @return True if initialization was successful
         */
        virtual bool initialize() = 0;

        /**
         * @brief Clean up resources used by the render stage
         */
        virtual void shutdown() = 0;

        /**
         * @brief Execute the render stage
         * @param context Current render context
         */
        virtual void execute(const RenderContext& context) = 0;

        /**
         * @brief Set an input render target
         * @param input Render target to use as input
         * @param index Index of the input (for multiple inputs)
         */
        void setInput(std::shared_ptr<RenderTarget> input, int index = 0);

        /**
         * @brief Set the output render target
         * @param output Render target to use as output
         */
        void setOutput(std::shared_ptr<RenderTarget> output);

        /**
         * @brief Get an input render target
         * @param index Index of the input
         * @return Shared pointer to the render target
         */
        std::shared_ptr<RenderTarget> getInput(int index = 0) const;

        /**
         * @brief Get the output render target
         * @return Shared pointer to the render target
         */
        std::shared_ptr<RenderTarget> getOutput() const;

        /**
         * @brief Set a parameter value
         * @param name Name of the parameter
         * @param value Value to set
         */
        template<typename T>
        void setParameter(const std::string& name, const T& value);

        /**
         * @brief Get a parameter value
         * @param name Name of the parameter
         * @param defaultValue Default value to return if parameter doesn't exist
         * @return Parameter value or default value
         */
        template<typename T>
        T getParameter(const std::string& name, const T& defaultValue = T()) const;

        /**
         * @brief Check if a parameter exists
         * @param name Name of the parameter
         * @return True if parameter exists
         */
        bool hasParameter(const std::string& name) const;

        /**
         * @brief Enable or disable the render stage
         * @param enabled Whether the stage should be enabled
         */
        void setEnabled(bool enabled);

        /**
         * @brief Check if the render stage is enabled
         * @return True if enabled
         */
        bool isEnabled() const;

        /**
         * @brief Get the name of the render stage
         * @return Name of the render stage
         */
        const std::string& getName() const;

    protected:
        /** Stage name */
        std::string m_name;

        /** Whether the stage is enabled */
        bool m_enabled;

        /** Input render targets */
        std::vector<std::shared_ptr<RenderTarget>> m_inputs;

        /** Output render target */
        std::shared_ptr<RenderTarget> m_output;

        /** Parameter storage */
        struct ParameterValue
        {
            std::variant<int, float, bool, glm::vec2, glm::vec3, glm::vec4, glm::mat4> value;
        };

        /** Map of parameters by name */
        std::unordered_map<std::string, ParameterValue> m_parameters;

        /**
         * @brief Bind an input render target
         * @param index Index of the input
         */
        void bindInput(int index = 0);

        /**
         * @brief Bind the output render target
         */
        void bindOutput();

        /**
         * @brief Clear the output render target
         * @param clearColor Whether to clear color buffers
         * @param clearDepth Whether to clear depth buffer
         * @param clearStencil Whether to clear stencil buffer
         */
        void clearOutput(bool clearColor = true, bool clearDepth = true, bool clearStencil = false);

        /**
         * @brief Set the viewport
         * @param x X coordinate
         * @param y Y coordinate
         * @param width Width of viewport
         * @param height Height of viewport
         */
        void setViewport(int x, int y, int width, int height);

        /**
         * @brief Reset viewport to output dimensions
         */
        void resetViewport();
    };

    // Template implementation
    template<typename T>
    void RenderStage::setParameter(const std::string& name, const T& value)
    {
        ParameterValue param;
        param.value = value;
        m_parameters[name] = param;
    }

    template<typename T>
    T RenderStage::getParameter(const std::string& name, const T& defaultValue) const
    {
        auto it = m_parameters.find(name);
        if (it == m_parameters.end())
        {
            return defaultValue;
        }

        try
        {
            return std::get<T>(it->second.value);
        }
        catch (const std::bad_variant_access&)
        {
            return defaultValue;
        }
    }

} // namespace PixelCraft::Rendering