// -------------------------------------------------------------------------
// PostProcessStage.h
// -------------------------------------------------------------------------
#pragma once

#include "Rendering/RenderStage.h"

#include <memory>
#include <vector>

namespace PixelCraft::Rendering
{
    class Shader;
    class Mesh;
    class PaletteManager;
}

namespace PixelCraft::Rendering::Stages
{

    /**
     * @brief Render stage for post-processing effects
     * 
     * The post-process stage applies various visual effects to the rendered scene
     * including pixel art dithering, palette mapping, tone mapping, and other
     * screen-space effects.
     */
    class PostProcessStage : public RenderStage
    {
    public:
        /**
         * @brief Post-processing effect types
         */
        enum class EffectType
        {
            PaletteMapping,     ///< Map colors to a pixel art palette
            Dithering,          ///< Apply dithering for color reduction
            ToneMapping,        ///< HDR tone mapping
            PixelPerfect,       ///< Pixel-perfect scaling
            Bloom,              ///< Bloom effect
            FXAA,               ///< Fast approximate anti-aliasing
            Vignette,           ///< Vignette effect
            ColorGrading        ///< Color grading/correction
        };

        /**
         * @brief Constructor
         */
        PostProcessStage();

        /**
         * @brief Destructor
         */
        virtual ~PostProcessStage();

        /**
         * @brief Initialize the post-process stage
         * @return True if initialization was successful
         */
        bool initialize() override;

        /**
         * @brief Clean up resources
         */
        void shutdown() override;

        /**
         * @brief Execute post-processing effects
         * @param context Current render context
         */
        void execute(const std::shared_ptr<RenderContext> context) override;

        /**
         * @brief Get the stage type name
         * @return Type name string
         */
        std::string getTypeName() const override
        {
            return "PostProcessStage";
        }

        /**
         * @brief Enable a post-processing effect
         * @param effect Effect type to enable
         */
        void enableEffect(EffectType effect);

        /**
         * @brief Disable a post-processing effect
         * @param effect Effect type to disable
         */
        void disableEffect(EffectType effect);

        /**
         * @brief Check if an effect is enabled
         * @param effect Effect type to check
         * @return True if the effect is enabled
         */
        bool isEffectEnabled(EffectType effect) const;

        /**
         * @brief Set the palette manager for palette mapping
         * @param paletteManager Palette manager instance
         */
        void setPaletteManager(std::shared_ptr<PaletteManager> paletteManager);

        /**
         * @brief Set the palette mapping shader
         * @param shader Palette mapping shader
         */
        void setPaletteMappingShader(std::shared_ptr<Shader> shader);

        /**
         * @brief Set the dithering shader
         * @param shader Dithering shader
         */
        void setDitheringShader(std::shared_ptr<Shader> shader);

        /**
         * @brief Set the tone mapping shader
         * @param shader Tone mapping shader
         */
        void setToneMappingShader(std::shared_ptr<Shader> shader);

        /**
         * @brief Set the bloom shader
         * @param shader Bloom shader
         */
        void setBloomShader(std::shared_ptr<Shader> shader);

        /**
         * @brief Set the FXAA shader
         * @param shader FXAA shader
         */
        void setFXAAShader(std::shared_ptr<Shader> shader);

        /**
         * @brief Set the pixel perfect shader
         * @param shader Pixel perfect scaling shader
         */
        void setPixelPerfectShader(std::shared_ptr<Shader> shader);

        /**
         * @brief Set the dithering strength
         * @param strength Dithering strength (0.0-1.0)
         */
        void setDitheringStrength(float strength);

        /**
         * @brief Get the current dithering strength
         * @return Dithering strength value
         */
        float getDitheringStrength() const;

        /**
         * @brief Set the bloom threshold
         * @param threshold Brightness threshold for bloom
         */
        void setBloomThreshold(float threshold);

        /**
         * @brief Get the current bloom threshold
         * @return Bloom threshold value
         */
        float getBloomThreshold() const;

        /**
         * @brief Set the bloom intensity
         * @param intensity Bloom effect intensity
         */
        void setBloomIntensity(float intensity);

        /**
         * @brief Get the current bloom intensity
         * @return Bloom intensity value
         */
        float getBloomIntensity() const;

        /**
         * @brief Set the exposure for tone mapping
         * @param exposure Exposure value
         */
        void setExposure(float exposure);

        /**
         * @brief Get the current exposure
         * @return Exposure value
         */
        float getExposure() const;

        /**
         * @brief Set pixel perfect scaling factor
         * @param scale Scaling factor for pixel art
         */
        void setPixelScale(int scale);

        /**
         * @brief Get the current pixel scale
         * @return Pixel scaling factor
         */
        int getPixelScale() const;

    private:
        /**
         * @brief Apply palette mapping effect
         * @param input Input render target
         * @param output Output render target
         */
        void applyPaletteMapping(std::shared_ptr<RenderTarget> input, std::shared_ptr<RenderTarget> output);

        /**
         * @brief Apply dithering effect
         * @param input Input render target
         * @param output Output render target
         */
        void applyDithering(std::shared_ptr<RenderTarget> input, std::shared_ptr<RenderTarget> output);

        /**
         * @brief Apply tone mapping
         * @param input Input render target
         * @param output Output render target
         */
        void applyToneMapping(std::shared_ptr<RenderTarget> input, std::shared_ptr<RenderTarget> output);

        /**
         * @brief Apply bloom effect
         * @param input Input render target
         * @param output Output render target
         */
        void applyBloom(std::shared_ptr<RenderTarget> input, std::shared_ptr<RenderTarget> output);

        /**
         * @brief Apply FXAA anti-aliasing
         * @param input Input render target
         * @param output Output render target
         */
        void applyFXAA(std::shared_ptr<RenderTarget> input, std::shared_ptr<RenderTarget> output);

        /**
         * @brief Apply pixel perfect scaling
         * @param input Input render target
         */
        void applyPixelPerfectToOutput(std::shared_ptr<RenderTarget> input);

        /**
         * @brief Render a fullscreen quad
         */
        void renderFullscreenQuad();

        /**
         * @brief Blit the input to the source render target
         * @param source Render target to blit to
         */
        void blitFromInput(std::shared_ptr<RenderTarget> source);

        /**
         * @brief Create intermediate render targets
         */
        void createIntermediateTargets();

        /**
         * @brief Helper to ensure intermediate targets are correct size
         * @param width Target width
         * @param height Target height
         */
        void ensureIntermediateTargetsSize(int width, int height);

        /** Enabled effects mask */
        uint32_t m_enabledEffects;

        /** Post-processing shaders */
        std::shared_ptr<Shader> m_paletteMappingShader;
        std::shared_ptr<Shader> m_ditheringShader;
        std::shared_ptr<Shader> m_toneMappingShader;
        std::shared_ptr<Shader> m_bloomShader;
        std::shared_ptr<Shader> m_fxaaShader;
        std::shared_ptr<Shader> m_pixelPerfectShader;
        std::shared_ptr<Shader> m_copyShader;

        /** Fullscreen quad mesh */
        std::shared_ptr<Mesh> m_fullscreenQuad;

        /** Palette manager for palette mapping */
        std::shared_ptr<PaletteManager> m_paletteManager;

        /** Effect parameters */
        float m_ditheringStrength;
        float m_bloomThreshold;
        float m_bloomIntensity;
        float m_exposure;
        int m_pixelScale;

        /** Intermediate render targets */
        std::shared_ptr<RenderTarget> m_tempTarget1;
        std::shared_ptr<RenderTarget> m_tempTarget2;
        std::shared_ptr<RenderTarget> m_bloomTarget;

        /** Profiling marker */
        std::string m_profilingMarker;
    };

} // namespace PixelCraft::Rendering::Stages
