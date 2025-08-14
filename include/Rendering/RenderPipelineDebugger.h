// -------------------------------------------------------------------------
// RenderPipelineDebugger.h
// -------------------------------------------------------------------------
#pragma once

#include "Rendering/Texture.h"

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <functional>

// In namespace PixelCraft
namespace PixelCraft::Rendering
{

    class RenderTarget;
    class Texture;
    class RenderStage;

    /**
     * @brief Configuration for debugging specific stages
     */
    struct StageDebugConfig
    {
        bool enabled = false;                           ///< Whether to debug this stage
        bool captureColor = true;                       ///< Capture color buffer
        bool captureDepth = false;                      ///< Capture depth buffer
        bool captureStencil = false;                    ///< Capture stencil buffer
        int colorAttachmentIndex = 0;                   ///< Which color attachment to capture
        std::string outputPath = "debug_output/";       ///< Where to save debug images
        std::string filenamePrefix = "";                ///< Prefix for debug filenames
    };

    /**
     * @brief Debugging utility for render pipeline stages
     * 
     * Provides functionality to capture and save render target outputs
     * from each stage of the rendering pipeline for debugging purposes.
     */
    class RenderPipelineDebugger
    {
    public:
        /**
         * @brief Constructor
         */
        RenderPipelineDebugger();

        /**
         * @brief Destructor
         */
        ~RenderPipelineDebugger();

        /**
         * @brief Initialize the debugger
         * @return True if initialization was successful
         */
        bool initialize();

        /**
         * @brief Shutdown and cleanup
         */
        void shutdown();

        /**
         * @brief Enable or disable debugging globally
         * @param enabled True to enable debugging
         */
        void setEnabled(bool enabled);

        /**
         * @brief Check if debugging is globally enabled
         * @return True if debugging is enabled
         */
        bool isEnabled() const;

        /**
         * @brief Configure debugging for a specific stage
         * @param stageName Name of the stage to configure
         * @param config Debug configuration for the stage
         */
        void configureStage(const std::string& stageName, const StageDebugConfig& config);

        /**
         * @brief Enable debugging for a stage with default settings
         * @param stageName Name of the stage
         * @param captureColor Whether to capture color buffer
         * @param captureDepth Whether to capture depth buffer
         */
        void enableStage(const std::string& stageName, bool captureColor = true, bool captureDepth = false);

        /**
         * @brief Disable debugging for a specific stage
         * @param stageName Name of the stage
         */
        void disableStage(const std::string& stageName);

        /**
         * @brief Check if a stage is configured for debugging
         * @param stageName Name of the stage
         * @return True if stage debugging is enabled
         */
        bool isStageEnabled(const std::string& stageName) const;

        /**
         * @brief Capture debug output from a render stage
         * @param stage The render stage to capture from
         * @param frameNumber Current frame number for filename generation
         * @param nearPlane Near clipping plane distance
         * @param farPlane Far clipping plane distance
         */
        void captureStage(std::shared_ptr<RenderStage> stage, uint32_t frameNumber,
                          float nearPlane, float farPlane);

        /**
         * @brief Capture debug output from a render target directly
         * @param target The render target to capture
         * @param stageName name for the capture (used in filename)
         * @param frameNumber Current frame number
         * @param nearPlane Near clipping plane distance
         * @param farPlane Far clipping plane distance
         */
        void captureRenderTarget(std::shared_ptr<RenderTarget> target,
                                 const std::string& stageName,
                                 uint32_t frameNumber,
                                 float nearPlane,
                                 float farPlane);
        
        /**
         * @brief Save a texture to an image file
         * @param target The render target to fetch from
         * @param texture The texture to save
         * @param filepath Path where to save the image
         * @param format Image format to save as
         * @return True if saving was successful
         */
        bool saveTextureToFile(std::shared_ptr<RenderTarget> target,
                               std::shared_ptr<Texture> texture,
                               const std::string& filepath,
                               const std::string& format = "png");

        /**
         * @brief Set the base output directory for debug images
         * @param path Directory path
         */
        void setOutputDirectory(const std::string& path);

        /**
         * @brief Get the current output directory
         * @return The output directory path
         */
        const std::string& getOutputDirectory() const;

        /**
         * @brief Set maximum number of frames to capture (0 = unlimited)
         * @param maxFrames Maximum frames to capture
         */
        void setMaxFramesToCapture(uint32_t maxFrames);

        /**
         * @brief Set frame interval for capture (1 = every frame, 10 = every 10th frame)
         * @param interval Frame interval
         */
        void setCaptureInterval(uint32_t interval);

        /**
         * @brief Clear all captured debug data
         */
        void clearCapturedData();

        /**
         * @brief Get statistics about captured data
         * @return String containing capture statistics
         */
        std::string getCaptureStatistics() const;

    private:
        /**
         * @brief Create output directory if it doesn't exist
         * @param path Directory path to create
         * @return True if creation was successful or directory already exists
         */
        bool createOutputDirectory(const std::string& path);

        /**
         * @brief Generate filename for a debug capture
         * @param stageName Name of the stage
         * @param frameNumber Frame number
         * @param bufferType Type of buffer (color, depth, stencil)
         * @param attachmentIndex Attachment index for color buffers
         * @return Generated filename
         */
        std::string generateFilename(const std::string& stageName,
                                     uint32_t frameNumber,
                                     const std::string& bufferType,
                                     int attachmentIndex = 0) const;

        /**
         * @brief Convert depth buffer to visible format for saving
         * @param depthTexture The depth texture to convert
         * @param nearPlane Near clipping plane distance
         * @param farPlane Far clipping plane distance
         * @return Converted texture suitable for saving as image
         */
        std::shared_ptr<Texture> convertDepthForSaving(std::shared_ptr<Texture> depthTexture, float nearPlane, float farPlane);

        /**
         * @brief Read pixels from a render target
         * @param target The render target to read from
         * @param width Width of the area to read
         * @param height Height of the area to read
         * @param format Texture format to read as
         * @return Pixel data as byte array
         */
        std::vector<uint8_t> readPixelsFromRenderTarget(std::shared_ptr<RenderTarget> target,
                                                        int width, int height,
                                                        TextureFormat format);

        /**
         * @brief Save raw pixel data to image file
         * @param data Pixel data
         * @param width Image width
         * @param height Image height
         * @param channels Number of channels
         * @param filepath Output file path
         * @param format Image format
         * @return True if successful
         */
        bool savePixelDataToFile(const std::vector<uint8_t>& data,
                                 int width, int height, int channels,
                                 const std::string& filepath,
                                 const std::string& format);

        bool m_enabled;                                                    ///< Global enable flag
        bool m_initialized;                                                ///< Initialization state
        std::string m_outputDirectory;                                     ///< Base output directory
        uint32_t m_frameCounter;                                           ///< Current frame directory
        uint32_t m_maxFramesToCapture;                                     ///< Maximum frames to capture (0 = unlimited)
        uint32_t m_captureInterval;                                        ///< Frame interval for capture
        uint32_t m_capturedFrames;                                         ///< Number of frames captured so far

        std::unordered_map<std::string, StageDebugConfig> m_stageConfigs;  ///< Per-stage debug configurations
        std::vector<std::string> m_capturedFiles;                          ///< List of captured file paths
    };

} // namespace PixelCraft::Rendering