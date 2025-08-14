// -------------------------------------------------------------------------
// RenderPipelineDebugger.cpp
// -------------------------------------------------------------------------
#include "Rendering/RenderPipelineDebugger.h"
#include "Rendering/RenderTarget.h"
#include "Rendering/Texture.h"
#include "Rendering/RenderStage.h"
#include "Core/Logger.h"
#include "Utility/FileSystem.h"

#include <glad/glad.h>
#include <stb_image_write.h>
#include <filesystem>
#include <iomanip>
#include <sstream>
#include <algorithm>

namespace PixelCraft::Rendering
{

    RenderPipelineDebugger::RenderPipelineDebugger()
        : m_enabled(false)
        , m_initialized(false)
        , m_outputDirectory("debug_output/")
        , m_frameCounter(0)
        , m_maxFramesToCapture(0)  // 0 = unlimited
        , m_captureInterval(1)     // Every frame
        , m_capturedFrames(0)
    {
    }

    RenderPipelineDebugger::~RenderPipelineDebugger()
    {
        shutdown();
    }

    bool RenderPipelineDebugger::initialize()
    {
        if (m_initialized)
        {
            return true;
        }

        Core::info("Initializing RenderPipelineDebugger");

        // Create output directory
        if (!createOutputDirectory(m_outputDirectory))
        {
            Core::error("RenderPipelineDebugger: Failed to create output directory: " + m_outputDirectory);
            return false;
        }

        m_initialized = true;
        Core::info("RenderPipelineDebugger initialized successfully");
        return true;
    }

    void RenderPipelineDebugger::shutdown()
    {
        if (!m_initialized)
        {
            return;
        }

        Core::info("Shutting down RenderPipelineDebugger");

        // Log capture statistics
        if (m_capturedFrames > 0)
        {
            Core::info("RenderPipelineDebugger: Captured " + std::to_string(m_capturedFrames) +
                       " frames with " + std::to_string(m_capturedFiles.size()) + " total files");
        }

        m_stageConfigs.clear();
        m_capturedFiles.clear();
        m_initialized = false;
    }

    void RenderPipelineDebugger::setEnabled(bool enabled)
    {
        m_enabled = enabled;
        if (enabled && !m_initialized)
        {
            initialize();
        }

        Core::info("RenderPipelineDebugger: " + std::string(enabled ? "Enabled" : "Disabled"));
    }

    bool RenderPipelineDebugger::isEnabled() const
    {
        return m_enabled && m_initialized;
    }

    void RenderPipelineDebugger::configureStage(const std::string& stageName, const StageDebugConfig& config)
    {
        m_stageConfigs[stageName] = config;

        if (config.enabled)
        {
            // Ensure output directory exists for this stage
            std::string stageOutputPath = config.outputPath;
            if (stageOutputPath.empty())
            {
                stageOutputPath = m_outputDirectory + stageName + "/";
            }
            createOutputDirectory(stageOutputPath);

            Core::info("RenderPipelineDebugger: Configured stage '" + stageName + "' for debugging");
        }
    }

    void RenderPipelineDebugger::enableStage(const std::string& stageName, bool captureColor, bool captureDepth)
    {
        StageDebugConfig config;
        config.enabled = true;
        config.captureColor = captureColor;
        config.captureDepth = captureDepth;
        config.captureStencil = false;
        config.outputPath = m_outputDirectory + stageName + "/";
        config.filenamePrefix = stageName;

        configureStage(stageName, config);
    }

    void RenderPipelineDebugger::disableStage(const std::string& stageName)
    {
        auto it = m_stageConfigs.find(stageName);
        if (it != m_stageConfigs.end())
        {
            it->second.enabled = false;
            Core::info("RenderPipelineDebugger: Disabled debugging for stage '" + stageName + "'");
        }
    }

    bool RenderPipelineDebugger::isStageEnabled(const std::string& stageName) const
    {
        auto it = m_stageConfigs.find(stageName);
        return (it != m_stageConfigs.end()) && it->second.enabled;
    }

    void RenderPipelineDebugger::captureStage(std::shared_ptr<RenderStage> stage, uint32_t frameNumber,
                                              float nearPlane, float farPlane)
    {
        if (!isEnabled() || !stage)
        {
            return;
        }

        // Check if we should capture this frame based on interval
        if (m_captureInterval > 1 && (frameNumber % m_captureInterval) != 0)
        {
            return;
        }

        // Check if we've exceeded max capture count
        if (m_maxFramesToCapture > 0 && m_capturedFrames >= m_maxFramesToCapture)
        {
            return;
        }

        std::string stageName = stage->getName();
        if (!isStageEnabled(stageName))
        {
            return;
        }

        auto renderTarget = stage->getOutput();
        if (!renderTarget)
        {
            Core::warn("RenderPipelineDebugger: Stage '" + stageName + "' has no output render target");
            return;
        }

        captureRenderTarget(renderTarget, stageName, frameNumber, nearPlane, farPlane);
    }

    void RenderPipelineDebugger::captureRenderTarget(std::shared_ptr<RenderTarget> target,
                                                     const std::string& stageName,
                                                     uint32_t frameNumber,
                                                     float nearPlane,
                                                     float farPlane)
    {
        if (!isEnabled() || !target || !isStageEnabled(stageName))
        {
            return;
        }

        auto it = m_stageConfigs.find(stageName);
        if (it == m_stageConfigs.end())
        {
            return;
        }

        const StageDebugConfig& config = it->second;

        try
        {
            // Capture color buffer
            if (config.captureColor)
            {
                auto colorTexture = target->getColorTexture(config.colorAttachmentIndex);
                if (colorTexture)
                {
                    std::string filename = generateFilename(stageName, frameNumber, "color", config.colorAttachmentIndex);
                    std::string filepath = config.outputPath + filename;

                    if (saveTextureToFile(target, colorTexture, filepath))
                    {
                        m_capturedFiles.push_back(filepath);
                        Core::debug("RenderPipelineDebugger: Captured color buffer for " + stageName + " -> " + filepath);
                    }
                }
                else
                {
                    Core::warn("RenderPipelineDebugger: No color texture at index " +
                               std::to_string(config.colorAttachmentIndex) + " for stage " + stageName);
                }
            }

            // Capture depth buffer
            if (config.captureDepth)
            {
                auto depthTexture = target->getDepthTexture();
                if (depthTexture)
                {
                    // Convert depth to visible format
                    auto visibleDepth = convertDepthForSaving(depthTexture, nearPlane, farPlane);
                    if (visibleDepth)
                    {
                        std::string filename = generateFilename(stageName, frameNumber, "depth");
                        std::string filepath = config.outputPath + filename;

                        if (saveTextureToFile(target, visibleDepth, filepath))
                        {
                            m_capturedFiles.push_back(filename);
                            Core::debug("RenderPipelineDebugger: Captured depth buffer for " + stageName + " -> " + filepath);
                        }
                    }
                }
                else
                {
                    Core::warn("RenderPipelineDebugger: No depth texture for stage " + stageName);
                }
            }

            // Capture stencil buffer
            if (config.captureStencil)
            {
                auto stencilTexture = target->getStencilTexture();
                if (stencilTexture)
                {
                    std::string filename = generateFilename(stageName, frameNumber, "stencil");
                    std::string filepath = config.outputPath + filename;

                    if (saveTextureToFile(target, stencilTexture, filepath))
                    {
                        m_capturedFiles.push_back(filepath);
                        Core::debug("RenderPipelineDebugger: Captured stencil buffer for " + stageName + " -> " + filepath);
                    }
                }
            }

            // Update frame counter only once per frame (on first successful capture)
            static uint32_t lastCapturedFrame = UINT32_MAX;
            if (frameNumber != lastCapturedFrame)
            {
                m_capturedFrames++;
                lastCapturedFrame = frameNumber;
            }
        }
        catch (const std::exception& e)
        {
            Core::error("RenderPipelineDebugger: Exception during capture: " + std::string(e.what()));
        }
    }

    bool RenderPipelineDebugger::saveTextureToFile(std::shared_ptr<RenderTarget> target,
                                                   std::shared_ptr<Texture> texture,
                                                   const std::string& filepath,
                                                   const std::string& format)
    {
        if (!texture || !texture->isLoaded())
        {
            Core::error("RenderPipelineDebugger: Invalid texture for saving");
            return false;
        }

        int width = texture->getWidth();
        int height = texture->getHeight();
        TextureFormat texFormat = texture->getFormat();

        // Read pixel data from texture
        std::vector<uint8_t> pixelData = readPixelsFromRenderTarget(target, width, height, texFormat);
        if (pixelData.empty())
        {
            // Fallback: try to get pixel data directly from texture if available
            // This would require additional methods in Texture class to read GPU data
            Core::warn("RenderPipelineDebugger: Could not read pixel data from texture");
            return false;
        }

        // Determine number of channels based on format
        int channels = 4; // Default to RGBA
        switch (texFormat)
        {
            case TextureFormat::R8:
            case TextureFormat::R16F:
            case TextureFormat::R32F:
                channels = 1;
                break;
            case TextureFormat::RG8:
            case TextureFormat::RG16F:
            case TextureFormat::RG32F:
                channels = 2;
                break;
            case TextureFormat::RGB8:
            case TextureFormat::RGB16F:
            case TextureFormat::RGB32F:
                channels = 3;
                break;
            default:
                channels = 4;
                break;
        }

        return savePixelDataToFile(pixelData, width, height, channels, filepath, format);
    }

    void RenderPipelineDebugger::setOutputDirectory(const std::string& path)
    {
        m_outputDirectory = path;
        if (!m_outputDirectory.empty() && m_outputDirectory.back() != '/')
        {
            m_outputDirectory += "/";
        }

        if (m_initialized)
        {
            createOutputDirectory(m_outputDirectory);
        }
    }

    const std::string& RenderPipelineDebugger::getOutputDirectory() const
    {
        return m_outputDirectory;
    }

    void RenderPipelineDebugger::setMaxFramesToCapture(uint32_t maxFrames)
    {
        m_maxFramesToCapture = maxFrames;
    }

    void RenderPipelineDebugger::setCaptureInterval(uint32_t interval)
    {
        m_captureInterval = std::max(1u, interval);
    }

    void RenderPipelineDebugger::clearCapturedData()
    {
        m_capturedFiles.clear();
        m_capturedFrames = 0;
        m_frameCounter = 0;
    }

    std::string RenderPipelineDebugger::getCaptureStatistics() const
    {
        std::stringstream ss;
        ss << "RenderPipelineDebugger Statistics:\n";
        ss << "  Frames Captured: " << m_capturedFrames << "\n";
        ss << "  Files Generated: " << m_capturedFiles.size() << "\n";
        ss << "  Output Directory: " << m_outputDirectory << "\n";
        ss << "  Capture Interval: " << m_captureInterval << "\n";
        ss << "  Max Frames: " << (m_maxFramesToCapture == 0 ? "Unlimited" : std::to_string(m_maxFramesToCapture)) << "\n";
        ss << "  Enabled Stages: ";

        bool first = true;
        for (const auto& [stageName, config] : m_stageConfigs)
        {
            if (config.enabled)
            {
                if (!first) ss << ", ";
                ss << stageName;
                first = false;
            }
        }
        if (first) ss << "None";

        return ss.str();
    }

    bool RenderPipelineDebugger::createOutputDirectory(const std::string& path)
    {
        try
        {
            std::filesystem::create_directories(path);
            return true;
        }
        catch (const std::exception& e)
        {
            Core::error("RenderPipelineDebugger: Failed to create directory '" + path + "': " + e.what());
            return false;
        }
    }

    std::string RenderPipelineDebugger::generateFilename(const std::string& stageName,
                                                          uint32_t frameNumber,
                                                          const std::string& bufferType,
                                                          int attachmentIndex) const
    {
        std::stringstream ss;
        ss << stageName << "_frame_" << std::setfill('0') << std::setw(6) << frameNumber;
        ss << "_" << bufferType;

        if (bufferType == "color" && attachmentIndex > 0)
        {
            ss << "_" << attachmentIndex;
        }

        ss << ".png";
        return ss.str();
    }

    std::shared_ptr<Texture> RenderPipelineDebugger::convertDepthForSaving(std::shared_ptr<Texture> depthTexture, float nearPlane, float farPlane)
    {
        if (!depthTexture)
        {
            return nullptr;
        }

        int width = depthTexture->getWidth();
        int height = depthTexture->getHeight();

        // Create a new RGBA texture for the visible depth
        auto visibleTexture = Core::makeShared<Texture>("DepthVisualization");
        if (!visibleTexture->initialize(width, height, TextureFormat::RGBA8))
        {
            return nullptr;
        }

        try
        {
            depthTexture->bind(0);

            std::vector<float> depthData(width * height);
            glGetTexImage(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, GL_FLOAT, depthData.data());

            std::vector<uint8_t> rgbaData(width * height * 4);

            for (int i = 0; i < width * height; ++i)
            {
                float z = depthData[i];
                // Linearize depth value from non-linear depth buffer value
                float linearDepth = (2.0f * nearPlane) / (farPlane + nearPlane - z * (farPlane - nearPlane));
                linearDepth = glm::clamp(linearDepth, 0.0f, 1.0f);

                uint8_t gray = static_cast<uint8_t>(linearDepth * 255.0f);
                rgbaData[i * 4 + 0] = gray;
                rgbaData[i * 4 + 1] = gray;
                rgbaData[i * 4 + 2] = gray;
                rgbaData[i * 4 + 3] = 255;
            }

            visibleTexture->update(rgbaData.data());

            depthTexture->unbind();
            return visibleTexture;
        }
        catch (const std::exception& e)
        {
            Core::error("RenderPipelineDebugger: Failed to convert depth texture: " + std::string(e.what()));
            return nullptr;
        }
    }

    std::vector<uint8_t> RenderPipelineDebugger::readPixelsFromRenderTarget(std::shared_ptr<RenderTarget> target,
                                                                            int width, int height,
                                                                            TextureFormat format)
    {
        std::vector<uint8_t> pixelData;

        if (!target)
        {
            return pixelData;
        }

        try
        {
            // Calculate expected data size
            int channels = 4; // Default to RGBA
            switch (format)
            {
                case TextureFormat::R8: channels = 1; break;
                case TextureFormat::RG8: channels = 2; break;
                case TextureFormat::RGB8: channels = 3; break;
                default: channels = 4; break;
            }

            pixelData.resize(width * height * channels);

            // Use RenderTarget's readPixels method if available
            target->readPixels(0, 0, width, height, pixelData.data(), TextureFormat::RGBA8);

            return pixelData;
        }
        catch (const std::exception& e)
        {
            Core::error("RenderPipelineDebugger: Failed to read pixels: " + std::string(e.what()));
            return std::vector<uint8_t>();
        }
    }

    bool RenderPipelineDebugger::savePixelDataToFile(const std::vector<uint8_t>& data,
                                                     int width, int height, int channels,
                                                     const std::string& filepath,
                                                     const std::string& format)
    {
        if (data.empty())
        {
            Core::error("RenderPipelineDebugger: No pixel data to save");
            return false;
        }

        try
        {
            // Ensure directory exists
            std::filesystem::path path(filepath);
            std::filesystem::create_directories(path.parent_path());

            int result = 0;
            if (format == "png")
            {
                result = stbi_write_png(filepath.c_str(), width, height, channels, data.data(), width * channels);
            }
            else if (format == "jpg" || format == "jpeg")
            {
                result = stbi_write_jpg(filepath.c_str(), width, height, channels, data.data(), 90);
            }
            else if (format == "bmp")
            {
                result = stbi_write_bmp(filepath.c_str(), width, height, channels, data.data());
            }
            else if (format == "tga")
            {
                result = stbi_write_tga(filepath.c_str(), width, height, channels, data.data());
            }
            else
            {
                Core::error("RenderPipelineDebugger: Unsupported image format: " + format);
                return false;
            }

            if (result == 0)
            {
                Core::error("RenderPipelineDebugger: Failed to save image: " + filepath);
                return false;
            }

            return true;
        }
        catch (const std::exception& e)
        {
            Core::error("RenderPipelineDebugger: Exception saving image: " + std::string(e.what()));
            return false;
        }
    }

} // namespace PixelCraft::Rendering