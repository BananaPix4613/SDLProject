// -------------------------------------------------------------------------
// RenderStage.h
// -------------------------------------------------------------------------
#pragma once

#include "Utility/Serialization/SerializationUtility.h"
#include "Utility/Serialization/ParameterValue.h"
#include "Core/Logger.h"

#include <memory>
#include <string>
#include <unordered_map>
#include <glm/glm.hpp>

namespace PixelCraft::Rendering
{
    class RenderTarget;
    class RenderContext;
}

namespace PixelCraft::Rendering
{

    /**
     * @brief Stage configuration data for serialization
     */
    struct StageConfig
    {
        std::string name;                                                          ///< Stage name
        std::string type;                                                          ///< Stage type
        bool enabled = true;                                                       ///< Whether stage is enabled
        std::vector<std::string> inputTargets;                                     ///< Input target names
        std::string outputTarget;                                                  ///< Output target name
        std::unordered_map<std::string, Utility::Serialization::ParameterValue> parameters;  ///< Stage parameters

        /**
         * @brief Serialize the stage configuration
         * @param serializer The serializer to use
         * @return SerializationResult with success/failure status
         */
        Utility::Serialization::SerializationResult serialize(Utility::Serialization::Serializer& serializer) const;

        /**
         * @brief Deserialize the stage configuration
         * @param deserializer The deserializer to use
         * @return SerializationResult with success/failure status
         */
        Utility::Serialization::SerializationResult deserialize(Utility::Serialization::Deserializer& deserializer);

        /**
         * @brief Define the schema for stage configuration serialization
         * @param schema The schema to define
         */
        static void defineSchema(Utility::Serialization::Schema& schema);

        /**
         * @brief Get the type name for serialization
         * @return The type name string
         */
        static std::string getTypeName()
        {
            return "StageConfig";
        }
    };

    /**
     * @brief Base class for all rendering pipeline stages
     * 
     * A render stage represents a single step in the rendering pipeline.
     * Each stage can have input and output render targets, and can be
     * enabled/disabled and configured with parameters.
     */
    class RenderStage
    {
    public:
        static constexpr int MAX_INPUTS = 8;

        /**
         * @brief Constructor
         * @param name Name of the render stage
         */
        explicit RenderStage(const std::string& name = "RenderStage");

        /**
         * @brief Virtual destructor
         */
        virtual ~RenderStage();

        /**
         * @brief Initialize the render stage
         * @return True if initialization was successful
         */
        virtual bool initialize() = 0;

        /**
         * @brief Clean up resources
         */
        virtual void shutdown() = 0;

        /**
         * @brief Execute the rendering stage
         * @param context Current render context
         */
        virtual void execute(const std::shared_ptr<RenderContext> context) = 0;

        /**
         * @brief Get the stage name
         * @return Stage name
         */
        const std::string& getName() const
        {
            return m_name;
        }

        /**
         * @brief Set the stage name
         * @param name New stage name
         */
        void setName(const std::string& name)
        {
            m_name = name;
        }

        /**
         * @brief Check if the stage is enabled
         * @return True if enabled
         */
        bool isEnabled() const
        {
            return m_enabled;
        }

        /**
         * @brief Enable or disable the stage
         * @param enabled Whether the stage is enabled
         */
        void setEnabled(bool enabled)
        {
            m_enabled = enabled;
        }

        /**
         * @brief Check if the stage is initialized
         * @return True if initialized
         */
        bool isInitialized() const
        {
            return m_initialized;
        }

        /**
         * @brief Set the initialized state
         * @param initialized Whether the stage is initialized
         */
        void setInitialized(bool initialized)
        {
            m_initialized = initialized;
        }

        /**
         * @brief Set the output render target
         * @param target Output render target
         */
        void setOutput(std::shared_ptr<RenderTarget> target);

        /**
         * @brief Get the output render target
         * @return Output render target
         */
        std::shared_ptr<RenderTarget> getOutput() const;

        /**
         * @brief Set an input render target
         * @param target Input render target
         * @param index Input index (0-7)
         */
        void setInput(std::shared_ptr<RenderTarget> target, int index);

        /**
         * @brief Get an input render target
         * @param index Input index (0-7)
         * @return Input render target
         */
        std::shared_ptr<RenderTarget> getInput(int index) const;

        /**
         * @brief Check if an input is available
         * @param index Input index (0-7)
         * @return True if input is available
         */
        bool hasInput(int index) const;

        /**
         * @brief Bind the output render target for rendering
         */
        void bindOutput();

        /**
         * @brief Unbind the output render target
         */
        void unbindOutput();

        /**
         * @brief Clear the output render target
         * @param clearColor Whether to clear color buffer
         * @param clearDepth Whether to clear depth buffer
         * @param clearStencil Whether to clear stencil buffer
         */
        void clearOutput(bool clearColor = true, bool clearDepth = true, bool clearStencil = false);

        /**
         * @brief Clear a specific color attachment
         * @param index Color attachment index
         * @param color Clear color
         */
        void clearOutputColor(int index, const glm::vec4& color);

        /**
         * @brief Set a parameter value
         * @param name Parameter name
         * @param value Parameter value
         */
        template<typename T>
        void setParameter(const std::string& name, const T& value)
        {
            m_parameters[name] = Utility::Serialization::ParameterValue(value);
        }

        /**
         * @brief Get a parameter value
         * @param name Parameter name
         * @param defaultValue Default value if parameter doesn't exist
         * @return Parameter value or default
         */
        template<typename T>
        T getParameter(const std::string& name, const T& defaultValue = T{}) const
        {
            auto it = m_parameters.find(name);
            if (it != m_parameters.end())
            {
                try
                {
                    return it->second.get<T>(defaultValue);
                }
                catch (const std::bad_variant_access&)
                {
                    Core::warn("RenderStage '" + m_name + "': Failed to get parameter '" + name + "': type mismatch");
                }
            }
            return defaultValue;
        }

        /**
         * @brief Check if a parameter exists
         * @param name Parameter name
         * @return True if parameter exists
         */
        bool hasParameter(const std::string& name) const;

        /**
         * @brief Get the number of draw calls issued by this stage in the current frame
         * @return Draw call count for current frame
         */
        virtual uint32_t getFrameDrawCalls() const
        {
            return 0;
        }

        /**
         * @brief Get the number of triangles rendered by this stage in the current frame
         * @return Triangle count for current frame
         */
        virtual uint32_t getFrameTriangles() const
        {
            return 0;
        }

        /**
         * @brief Serialize the render stage configuration
         * @param serializer Serializer to use
         * @return Serialization result
         */
        virtual Utility::Serialization::SerializationResult serialize(Utility::Serialization::Serializer& serializer) const;

        /**
         * @brief Deserialize the render stage configuration
         * @param deserializer Deserializer to use
         * @return Deserialization result
         */
        virtual Utility::Serialization::SerializationResult deserialize(Utility::Serialization::Deserializer& deserializer);

        /**
         * @brief Get the configuration for this stage
         * @return StageConfig with current stage settings
         */
        virtual StageConfig getConfig() const;

        /**
         * @brief Apply configuration to this stage
         * @param config The configuration to apply
         * @return True if configuration was applied successfully
         */
        virtual bool applyConfig(const StageConfig& config);

        /**
         * @brief Get the version info for serialization
         * @return Version info
         */
        virtual Utility::Serialization::VersionInfo getVersion() const;

        /**
         * @brief Get the type name for serialization
         * @return The type name string
         */
        virtual std::string getTypeName() const = 0;

        /**
         * @brief Define the schema for serialization
         * @param schema Schema to define
         */
        static void defineSchema(Utility::Serialization::Schema& schema);

    protected:
        /**
         * @brief Load stage-specific configuration during deserialization
         * @param deserializer The deserializer to use
         * @return SerializationResult with success/failure status
         */
        virtual Utility::Serialization::SerializationResult loadStageSpecificConfig(Utility::Serialization::Deserializer& deserializer);

        /**
         * @brief Save stage-specific configuration during serialization
         * @param serializer The serializer to use
         * @return SerializationResult with success/failure status
         */
        virtual Utility::Serialization::SerializationResult saveStageSpecificConfig(Utility::Serialization::Serializer& serializer) const;

    private:
        /** Stage name */
        std::string m_name;

        /** Whether the stage is enabled */
        bool m_enabled;

        /** Whether the stage is initialized */
        bool m_initialized;

        /** Output render target */
        std::shared_ptr<RenderTarget> m_output;

        /** Input render targets */
        std::shared_ptr<RenderTarget> m_inputs[MAX_INPUTS];

        /** Stage parameters */
        std::unordered_map<std::string, Utility::Serialization::ParameterValue> m_parameters;
    };

} // namespace PixelCraft::Rendering
