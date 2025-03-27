#include "RenderSystem.h"
#include <iostream>

// RenderTarget implementation
RenderTarget::RenderTarget(int width, int height)
    : width(width), height(height)
{
    // Create framebuffer
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    // Create color texture
    glGenTextures(1, &colorTexture);
    glBindTexture(GL_TEXTURE_2D, colorTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTexture, 0);

    // Create depth texture
    glGenTextures(1, &depthTexture);
    glBindTexture(GL_TEXTURE_2D, depthTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTexture, 0);

    // Check framebuffer completeness
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        std::cerr << "Framebuffer not complete!" << std::endl;
    }

    // Unbind framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

RenderTarget::~RenderTarget()
{
    if (fbo) glDeleteFramebuffers(1, &fbo);
    if (colorTexture) glDeleteTextures(1, &colorTexture);
    if (depthTexture) glDeleteTextures(1, &depthTexture);
}

void RenderTarget::resize(int newWidth, int newHeight)
{
    // Skip if size hasn't changed
    if (width == newWidth && height == newHeight) return;

    width = newWidth;
    height = newHeight;

    // Resize color texture
    glBindTexture(GL_TEXTURE_2D, colorTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

    // Resize depth texture
    glBindTexture(GL_TEXTURE_2D, depthTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
}

void RenderTarget::bind()
{
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glViewport(0, 0, width, height);
}

void RenderTarget::unbind()
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

// RenderSystem implementation
RenderSystem::RenderSystem()
    : viewportWidth(800), viewportHeight(600)
{
    // Default contructor
}

RenderSystem::~RenderSystem()
{
    shutdown();
}

void RenderSystem::initialize()
{
    // Create main render target
    mainRenderTarget = std::make_unique<RenderTarget>(viewportWidth, viewportHeight);
    finalRenderTarget = std::make_unique<RenderTarget>(viewportWidth, viewportHeight);

    // Create shadow map target (smaller resolution)
    shadowMapTarget = std::make_unique<RenderTarget>(2048, 2048);

    // Initialize render stages
    for (auto& stage : renderStages)
    {
        stage->initialize();
    }

    // Initialize post-processors
    for (auto& processor : postProcessors)
    {
        processor->initialize();
    }

    std::cout << "Render system initialized" << std::endl;
}

void RenderSystem::shutdown()
{
    // Clear resource collections
    renderableObjects.clear();
    renderStages.clear();
    postProcessors.clear();
    shaders.clear();

    // Release render targets
    mainRenderTarget.reset();
    finalRenderTarget.reset();
    shadowMapTarget.reset();

    std::cout << "Render system shut down" << std::endl;
}

void RenderSystem::resize(int width, int height)
{
    viewportWidth = width;
    viewportHeight = height;

    // Resize render targets
    if (mainRenderTarget) mainRenderTarget->resize(width, height);
    if (finalRenderTarget) finalRenderTarget->resize(width, height);

    // No need to resize shadow map target as it's independent of screen size
}

void RenderSystem::render(Camera* camera)
{
    if (!camera) return;

    // Prepare render context
    RenderContext context;
    context.camera = camera;
    context.viewMatrix = camera->getViewMatrix();
    context.projectionMatrix = camera->getProjectionMatrix();
    context.frustum.extractFromMatrix(context.projectionMatrix * context.viewMatrix);
    context.enableFrustumCulling = true;
    context.enableShadows = shadowsEnabled;

    // Prepare shadow data if shadows are enabled
    if (shadowsEnabled && shadowMapTarget)
    {
        // Calculate light position and view (from a directional light)
        glm::vec3 lightPos = glm::vec3(20.0f, 30.0f, 20.0f);
        glm::vec3 lightTarget = glm::vec3(0.0f);

        // Calculate light view matrix
        glm::mat4 lightView = glm::lookAt(lightPos, lightTarget, glm::vec3(0.0f, 1.0f, 0.0f));

        // Calculate orthographic projection for the light
        float orthoSize = 40.0f;
        glm::mat4 lightProjection = glm::ortho(-orthoSize, orthoSize,
                                               -orthoSize, orthoSize,
                                               1.0f, 100.0f);

        // Combined light space matrix
        context.lightSpaceMatrix = lightProjection * lightView;

        // Render shadow map
        shadowMapTarget->bind();
        glClear(GL_DEPTH_BUFFER_BIT);

        // Depth-only rendering
        glCullFace(GL_FRONT); // Avoid peter-panning

        // Apply depth bias
        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(2.0f, 4.0f);

        // Prepare renderable objects
        for (auto* obj : renderableObjects)
        {
            if (obj && obj->getActive() && obj->getVisible())
            {
                obj->render(context);
            }
        }

        // Reset state
        glDisable(GL_POLYGON_OFFSET_FILL);
        glCullFace(GL_BACK);
        context.overrideMaterial = nullptr;

        // Store shadow map texture for use in main pass
        context.shadowMapTexture = shadowMapTarget->getDepthTexture();
    }

    // Execute each render stage
    for (auto& stage : renderStages)
    {
        if (stage->getActive())
        {
            stage->execute(context);
        }
    }

    // Main rendering pass
    mainRenderTarget->bind();

    // Clear with sky color
    glClearColor(0.678f, 0.847f, 0.902f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Re-prepare objects with final camera view
    for (auto* obj : renderableObjects)
    {
        if (obj && obj->getActive())
        {
            obj->prepare(context);
        }
    }

    // Render objects
    for (auto* obj : renderableObjects)
    {
        if (obj && obj->getActive() && obj->getVisible())
        {
            obj->render(context);
        }
    }

    // Apply post-processing if enabled
    if (postProcessingEnabled && !postProcessors.empty())
    {
        RenderTarget* source = mainRenderTarget.get();
        RenderTarget* target = finalRenderTarget.get();

        for (auto& processor : postProcessors)
        {
            processor->apply(source, target, &context);

            // Swap source and target for next pass
            std::swap(source, target);
        }

        // Ensure final output is in finalRenderTarget
        if (source != finalRenderTarget.get())
        {
            // Need to blit from source to finalRenderTarget
            glBindFramebuffer(GL_READ_FRAMEBUFFER, source->getFbo());
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, finalRenderTarget->getFbo());
            glBlitFramebuffer(0, 0, source->getWidth(), source->getHeight(),
                              0, 0, finalRenderTarget->getWidth(), finalRenderTarget->getHeight(),
                              GL_COLOR_BUFFER_BIT, GL_NEAREST);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }
    }

    // Clean up
    mainRenderTarget->unbind();
}

void RenderSystem::addRenderableObject(RenderableObject* object)
{
    if (object)
    {
        renderableObjects.push_back(object);
    }
}

void RenderSystem::removeRenderableObject(RenderableObject* object)
{
    auto it = std::find(renderableObjects.begin(), renderableObjects.end(), object);
    if (it != renderableObjects.end())
    {
        renderableObjects.erase(it);
    }
}

void RenderSystem::addRenderStage(std::shared_ptr<RenderStage> stage)
{
    if (stage)
    {
        renderStages.push_back(stage);
        stage->initialize();
    }
}

void RenderSystem::removeRenderStage(const std::string& stageName)
{
    auto it = std::find_if(renderStages.begin(), renderStages.end(),
                           [&stageName](const std::shared_ptr<RenderStage>& stage) {
                               return stage->getName() == stageName;
                           });

    if (it != renderStages.end())
    {
        renderStages.erase(it);
    }
}

std::shared_ptr<RenderStage> RenderSystem::getRenderStage(const std::string& stageName)
{
    auto it = std::find_if(renderStages.begin(), renderStages.end(),
                           [&stageName](const std::shared_ptr<RenderStage>& stage) {
                               return stage->getName() == stageName;
                           });

    if (it != renderStages.end())
    {
        return *it;
    }

    return nullptr;
}

void RenderSystem::addPostProcessor(std::shared_ptr<PostProcessor> processor)
{
    if (processor)
    {
        postProcessors.push_back(processor);
        processor->initialize();
    }
}

void RenderSystem::removePostProcessor(std::shared_ptr<PostProcessor> processor)
{
    auto it = std::find(postProcessors.begin(), postProcessors.end(), processor);
    if (it != postProcessors.end())
    {
        postProcessors.erase(it);
    }
}

Shader* RenderSystem::createShader(const std::string& name, const std::string& vertexPath, const std::string& fragmentPath)
{
    // Check if shader already exists
    auto it = shaders.find(name);
    if (it != shaders.end())
    {
        return it->second.get();
    }

    // Create new shader
    auto shader = std::make_unique<Shader>(vertexPath.c_str(), fragmentPath.c_str());
    Shader* result = shader.get();

    // Store in map
    shaders[name] = std::move(shader);

    return result;
}

Shader* RenderSystem::getShader(const std::string& name)
{
    auto it = shaders.find(name);
    if (it != shaders.end())
    {
        return it->second.get();
    }

    return nullptr;
}