#pragma once

#include "RenderSystem.h"
#include <glm/glm.hpp>
#include <vector>
#include <memory>

// Base class for post-processing effects is already defined in RenderSystem.h

// SSAO (Screen-Space Ambient Occlusion) post-processor
class SSAOPostProcessor : public PostProcessor
{
public:
    SSAOPostProcessor()
    {
        // Initialize default parameters
        kernelSize = 64;
        radius = 0.5;
        bias = 0.025f;
        power = 2.0f;
    }

    void initialize() override
    {
        // Create ssao shader
        ssaoShader = new Shader("shaders/SSAOVert.glsl", "shaders/SSAOFrag.glsl");
        blurShader = new Shader("shaders/SSAOBlurVert.glsl", "shaders/SSAOBlurFrag.glsl");

        // Create framebuffers
        ssaoFBO = std::make_unique<RenderTarget>(512, 512);
        blurFBO = std::make_unique<RenderTarget>(512, 512);

        // Generate sample kernel and noise texture
        generateSampleKernel();
        generateNoiseTexture();

        // Create fullscreen quad
        createFullscreenQuad();
    }

    void apply(RenderTarget* input, RenderTarget* output, RenderContext& context) override
    {
        // Ensure render targets are valid
        if (!input || !output || !ssaoShader || !blurShader) return;

        // Resize internal buffers if needed
        if (input->getWidth() != ssaoFBO->getWidth() || input->getHeight() != ssaoFBO->getHeight())
        {
            ssaoFBO->resize(input->getWidth(), input->getHeight());
            blurFBO->resize(input->getWidth(), input->getHeight());
        }

        // First pass: Calculate SSAO
        ssaoFBO->bind();
        glClear(GL_COLOR_BUFFER_BIT);

        ssaoShader->use();

        // Bind position and normal textures from deferred renderer's G-Buffer
        // (This assumes they're available; in a real implementation you'd
        // have access to these from the deferred rendering stage)
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, input->getColorTexture()); // Position texture
        ssaoShader->setInt("gPosition", 0);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, input->getDepthTexture()); // Normal texture would ideally be separate
        ssaoShader->setInt("gNormal", 1);

        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, noiseTexture);
        ssaoShader->setInt("texNoise", 2);

        // Set SSAO parameters
        ssaoShader->setInt("kernelSize", kernelSize);
        ssaoShader->setFloat("radius", radius);
        ssaoShader->setFloat("bias", bias);
        ssaoShader->setMat4("projection", context.projectionMatrix);

        // Upload sample kernel
        for (unsigned int i = 0; i < kernelSize; i++)
        {
            ssaoShader->setVec3("samples[" + std::to_string(i) + "]", sampleKernel[i]);
        }

        // Render fullscreen quad
        glBindVertexArray(quadVAO);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        // Second pass: Blur SSAO texture
        blurFBO->bind();
        glClear(GL_COLOR_BUFFER_BIT);

        blurShader->use();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, ssaoFBO->getColorTexture());
        blurShader->setInt("ssaoInput", 0);

        glBindVertexArray(quadVAO);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        // Final pass: Apply SSAO to the original scene
        output->bind();

        // This would typically be done in a lighting shader that combines
        // the SSAO result with the scene lighting. Here we're just copying
        // the blurred SSAO result to the output for demonstration.
        glBindFramebuffer(GL_READ_FRAMEBUFFER, blurFBO->getFbo());
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, output->getFbo());
        glBlitFramebuffer(0, 0, blurFBO->getWidth(), blurFBO->getHeight(),
                          0, 0, output->getWidth(), output->getHeight(),
                          GL_COLOR_BUFFER_BIT, GL_NEAREST);
    }

    // Accessors for SSAO parameters
    void setKernelSize(int size) { kernelSize = size; generateSampleKernel(); }
    void setRadius(float radius) { this->radius = radius; }
    void setBias(float bias) { this->bias = bias; }
    void setPower(float power) { this->power = power; }

private:
    // SSAO parameters
    unsigned int kernelSize;
    float radius;
    float bias;
    float power;

    // OpenGL resources
    unsigned int quadVAO = 0;
    unsigned int quadVBO = 0;
    unsigned int noiseTexture = 0;

    // Render targets
    std::unique_ptr<RenderTarget> ssaoFBO;
    std::unique_ptr<RenderTarget> blurFBO;

    // Shaders
    Shader* ssaoShader = nullptr;
    Shader* blurShader = nullptr;

    // Sample vectors for SSAO calculations
    std::vector<glm::vec3> sampleKernel;

    // Helper methods
    void createFullscreenQuad()
    {
        float quadVertices[] = {
            // positions        // texture coords
            -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
            -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
             1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
             1.0f, -1.0f, 0.0f, 1.0f, 0.0f
        };

        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    }

    void generateSampleKernel()
    {
        sampleKernel.resize(kernelSize);

        for (unsigned int i = 0; i < kernelSize; i++)
        {
            // Generate random points in tengent hemisphere
            glm::vec3 sample(
                static_cast<float>(rand()) / RAND_MAX * 2.0f - 1.0f,
                static_cast<float>(rand()) / RAND_MAX * 2.0f - 1.0f,
                static_cast<float>(rand()) / RAND_MAX
            );
            sample = glm::normalize(sample);
            sample *= static_cast<float>(rand()) / RAND_MAX;

            // Scale samples based on distance from center
            float scale = static_cast<float>(i) / static_cast<float>(kernelSize);
            scale = glm::mix(0.1f, 1.0f, scale * scale); // Emphasize closer samples
            sample *= scale;

            sampleKernel[i] = sample;
        }
    }

    void generateNoiseTexture()
    {
        // Generate random rotation vectors for SSAO
        std::vector<glm::vec3> ssaoNoise;
        for (unsigned int i = 0; i < 16; i++)
        {
            glm::vec3 noise(
                static_cast<float>(rand()) / RAND_MAX * 2.0f - 1.0f,
                static_cast<float>(rand()) / RAND_MAX * 2.0f - 1.0f,
                0.0f
            );
            ssaoNoise.push_back(noise);
        }

        // Create and populate noise texture
        glGenTextures(1, &noiseTexture);
        glBindTexture(GL_TEXTURE_2D, noiseTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, 4, 4, 0, GL_RGB, GL_FLOAT, &ssaoNoise[0]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    }
};

// Bloom post-processor
class BloomPostProcessor : public PostProcessor
{
public:
    BloomPostProcessor()
    {
        // Default parameters
        threshold = 1.0f;
        intensity = 0.5f;
        mipCount = 5;
    }

    void initialize() override
    {
        // Create bloom shaders
        bloomThresholdShader = new Shader("shaders/BloomThresholdVert.glsl", "shaders/BloomThresholdFrag.glsl");
        bloomBlurShader = new Shader("shaders/BloomBlurVert.glsl", "shaders/BloomBlurFrag.glsl");
        bloomCombineShader = new Shader("shaders/BloomCombineVert.glsl", "shaders/BloomCombineFrag.glsl");

        // Create framebuffers for each mip level
        for (int i = 0; i < mipCount; i++)
        {
            bloomMips.push_back(std::make_unique<RenderTarget>(512 >> i, 512 >> i));
            bloomBlur.push_back(std::make_unique<RenderTarget>(512 >> i, 512 >> i));
        }

        // Create fullscreen quad
        createFullscreenQuad();
    }

    void apply(RenderTarget* input, RenderTarget* output, RenderContext& context) override
    {
        if (!input || !output) return;

        // Resize mip chain if needed
        int width = input->getWidth();
        int height = input->getHeight();

        for (int i = 0; i < mipCount; i++)
        {
            int mipWidth = width >> i;
            int mipHeight = height >> i;
            if (mipWidth < 1) mipWidth = 1;
            if (mipHeight < 1) mipHeight = 1;

            bloomMips[i]->resize(mipWidth, mipHeight);
            bloomBlur[i]->resize(mipWidth, mipHeight);
        }

        // 1. Threshold pass
        bloomMips[0]->bind();
        glClear(GL_COLOR_BUFFER_BIT);

        bloomThresholdShader->use();
        bloomThresholdShader->setFloat("threshold", threshold);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, input->getColorTexture());
        bloomThresholdShader->setInt("inputTexture", 0);

        glBindVertexArray(quadVAO);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        // 2. Generate mip chain (downsample)
        for (int i = 1; i < mipCount; i++)
        {
            bloomMips[i]->bind();
            glClear(GL_COLOR_BUFFER_BIT);

            bloomBlurShader->use();
            bloomBlurShader->setBool("horizontal", false);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, bloomMips[i - 1]->getColorTexture());
            bloomBlurShader->setInt("image", 0);

            glBindVertexArray(quadVAO);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        }

        // 3. Blur passes (ping-pong)
        for (int i = 0; i < mipCount; i++)
        {
            // Horizontal blur
            bloomBlur[i]->bind();
            glClear(GL_COLOR_BUFFER_BIT);

            bloomBlurShader->use();
            bloomBlurShader->setBool("horizontal", true);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, bloomMips[i]->getColorTexture());
            bloomBlurShader->setInt("image", 0);

            glBindVertexArray(quadVAO);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

            // Vertical blur
            bloomMips[i]->bind();
            glClear(GL_COLOR_BUFFER_BIT);

            bloomBlurShader->use();
            bloomBlurShader->setBool("horizontal", false);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, bloomBlur[i]->getColorTexture());
            bloomBlurShader->setInt("image", 0);

            glBindVertexArray(quadVAO);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        }

        // 4. Final combine pass
        output->bind();
        glClear(GL_COLOR_BUFFER_BIT);

        bloomCombineShader->use();
        bloomCombineShader->setFloat("bloomIntensity", intensity);

        // Bind original scene
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, input->getColorTexture());
        bloomCombineShader->setInt("sceneTexture", 0);

        // Bind all bloom mips
        for (int i = 0; i < mipCount; i++)
        {
            glActiveTexture(GL_TEXTURE1 + i);
            glBindTexture(GL_TEXTURE_2D, bloomMips[i]->getColorTexture());
            bloomCombineShader->setInt("bloomMip" + std::to_string(i), 1 + i);
        }

        bloomCombineShader->setInt("mipCount", mipCount);

        glBindVertexArray(quadVAO);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }

    // Accessors
    void setThreshold(float threshold) { this->threshold = threshold; }
    void setIntensity(float intensity) { this->intensity = intensity; }
    void setMipCount(int count)
    {
        mipCount = count;
        // Resize mip arrays
        bloomMips.resize(mipCount);
        bloomBlur.resize(mipCount);
    }

private:
    // Bloom parameters
    float threshold;
    float intensity;
    int mipCount;

    // OpenGL resources
    unsigned int quadVAO = 0;
    unsigned int quadVBO = 0;

    // Render targets for mip chain
    std::vector<std::unique_ptr<RenderTarget>> bloomMips;
    std::vector<std::unique_ptr<RenderTarget>> bloomBlur;

    // Shaders
    Shader* bloomThresholdShader = nullptr;
    Shader* bloomBlurShader = nullptr;
    Shader* bloomCombineShader = nullptr;

    void createFullscreenQuad()
    {
        float quadVertices[] = {
            // positions        // texture Coords
            -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
            -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
             1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
             1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
        };

        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    }
};

// Basic tone mapping and gamma correction post-processor
class TonemapPostProcessor : public PostProcessor
{
public:
    TonemapPostProcessor()
    {
        // Default parameters
        exposure = 1.0f;
        gamma = 2.2f;
    }

    void initialize() override
    {
        // Create tonemap shader
        tonemapShader = new Shader("shaders/TonemapVert.glsl", "shaders/TonemapFrag.glsl");

        // Create fullscreen quad
        createFullscreenQuad();
    }

    void apply(RenderTarget* input, RenderTarget* output, RenderContext& context) override
    {
        if (!input || !output || !tonemapShader) return;

        // Bind output framebuffer
        output->bind();
        glClear(GL_COLOR_BUFFER_BIT);

        // Set up shader
        tonemapShader->use();
        tonemapShader->setFloat("exposure", exposure);
        tonemapShader->setFloat("gamma", gamma);

        // Bind input texture
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, input->getColorTexture());
        tonemapShader->setInt("hdrTexture", 0);

        // Render fullscreen quad
        glBindVertexArray(quadVAO);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }

    // Accessors
    void setExposure(float exposure) { this->exposure = exposure; }
    void setGamma(float gamma) { this->gamma = gamma; }

private:
    // Tonemap parameters
    float exposure;
    float gamma;

    // OpenGL resources
    unsigned int quadVAO = 0;
    unsigned int quadVBO = 0;

    // Shader
    Shader* tonemapShader = nullptr;

    void createFullscreenQuad()
    {
        float quadVertices[] = {
            // positions        // texture Coords
            -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
            -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
             1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
             1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
        };

        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    }
};