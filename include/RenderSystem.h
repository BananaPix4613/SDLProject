#pragma once

#include <glad/glad.h>
#include <glm.hpp>
#include <vector>
#include <memory>
#include <unordered_map>
#include <string>
#include "Shader.h"
#include "Camera.h"
#include "CubeGrid.h"
#include "Frustum.h"

// Forward declarations
class Camera;
class RenderableObject;
class Material;
class RenderCommand;
class RenderContext;
class RenderStage;
class RenderTarget;
class PostProcessor;

// Base class for renderables
class RenderableObject
{
public:
    virtual ~RenderableObject() = default;
    virtual void prepare(RenderContext& context) = 0;
    virtual void render(RenderContext& context) = 0;

    void setActive(bool active) { isActive = active; }
    bool getActive() const { return isActive; }

    void setVisible(bool visible) { isVisible = visible; }
    bool getVisible() const { return isVisible; }

    virtual glm::vec3 getPosition() const { return position; }
    virtual void setPosition(const glm::vec3& pos) { position = pos; }

    virtual glm::vec3 getScale() const { return scale; }
    virtual void setScale(const glm::vec3& scl) { scale = scl; }

    virtual glm::mat4 getModelMatrix() const
    {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, position);
        model = glm::scale(model, scale);
        return model;
    }

protected:
    bool isActive = true;
    bool isVisible = true;
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 scale = glm::vec3(1.0f);
};

// Material definition
class Material
{
public:
    Material(Shader* shader) : shader(shader) {}

    void bind()
    {
        if (shader)
        {
            shader->use();
        }
    }

    void setShader(Shader* newShader) { shader = newShader; }
    Shader* getShader() const { return shader; }

    void setParameter(const std::string& name, int value)
    {
        if (shader) shader->setInt(name, value);
    }

    void setParameter(const std::string& name, float value)
    {
        if (shader) shader->setFloat(name, value);
    }

    void setParameter(const std::string& name, glm::vec2& value)
    {
        if (shader) shader->setVec2(name, value);
    }

    void setParameter(const std::string& name, glm::vec3& value)
    {
        if (shader) shader->setVec3(name, value);
    }

    void setParameter(const std::string& name, glm::vec4& value)
    {
        if (shader) shader->setVec4(name, value);
    }

    void setParameter(const std::string& name, glm::mat4& value)
    {
        if (shader) shader->setMat4(name, value);
    }

private:
    Shader* shader = nullptr;
};

// Encapsulates render state
class RenderContext
{
public:
    Camera* camera = nullptr;
    glm::mat4 viewMatrix;
    glm::mat4 projectionMatrix;
    Frustum frustum;
    bool enableFrustumCulling = true;
    bool enableShadows = true;
    bool showWireframe = false;
    Material* overrideMaterial = nullptr;

    // Shadow mapping data
    unsigned int shadowMapTexture = 0;
    glm::mat4 lightSpaceMatrix;
};

// Render target for output
class RenderTarget
{
public:
    RenderTarget(int width, int height);
    ~RenderTarget();

    void resize(int width, int height);
    void bind();
    void unbind();

    unsigned int getColorTexture() const { return colorTexture; }
    unsigned int getDepthTexture() const { return depthTexture; }

    int getWidth() const { return width; }
    int getHeight() const { return height; }

    unsigned int getFbo() const { return fbo; }

private:
    int width = 0;
    int height = 0;
    unsigned int fbo = 0;
    unsigned int colorTexture = 0;
    unsigned int depthTexture = 0;
};

// A render command encapsulates a single draw operation
class RenderCommand
{
public:
    virtual ~RenderCommand() = default;
    virtual void execute(RenderContext& context) = 0;
};

// A stage in the render pipeline
class RenderStage
{
public:
    RenderStage(const std::string& name) : name(name) {}
    virtual ~RenderStage() = default;

    virtual void initialize() = 0;
    virtual void execute(RenderContext& context) = 0;

    void setActive(bool active) { isActive = active; }
    bool getActive() const { return isActive; }

    const std::string& getName() const { return name; }

protected:
    std::string name;
    bool isActive = true;
};

// Post-processing effect
class PostProcessor
{
public:
    virtual ~PostProcessor() = default;
    virtual void initialize() = 0;
    virtual void apply(RenderTarget* input, RenderTarget* output, RenderContext* context) = 0;
};

// Main render system
class RenderSystem
{
public:
    RenderSystem();
    ~RenderSystem();

    void initialize();
    void shutdown();

    void resize(int width, int height);
    void render(Camera* camera);

    void addRenderableObject(RenderableObject* object);
    void removeRenderableObject(RenderableObject* object);

    void addRenderStage(std::shared_ptr<RenderStage> stage);
    void removeRenderStage(const std::string& stageName);
    std::shared_ptr<RenderStage> getRenderStage(const std::string& stageName);

    void addPostProcessor(std::shared_ptr<PostProcessor> processor);
    void removePostProcessor(std::shared_ptr<PostProcessor> processor);

    void setEnableShadows(bool enable) { shadowsEnabled = enable; }
    bool getShadowsEnabled() const { return shadowsEnabled; }

    void setEnablePostProcessing(bool enable) { postProcessingEnabled = enable; }
    bool getPostProcessingEnabled() const { return postProcessingEnabled; }

    // Resource management
    Shader* createShader(const std::string& name, const std::string& vertexPath, const std::string& fragmentPath);
    Shader* getShader(const std::string& name);

    RenderTarget* getMainRenderTarget() { return mainRenderTarget.get(); }
    RenderTarget* getFinalRenderTarget() { return finalRenderTarget.get(); }

private:
    int viewportWidth = 0;
    int viewportHeight = 0;

    std::vector<RenderableObject*> renderableObjects;
    std::vector<std::shared_ptr<RenderStage>> renderStages;
    std::vector<std::shared_ptr<PostProcessor>> postProcessors;

    std::unordered_map<std::string, std::unique_ptr<Shader>> shaders;

    std::unique_ptr<RenderTarget> mainRenderTarget;
    std::unique_ptr<RenderTarget> finalRenderTarget;
    std::unique_ptr<RenderTarget> shadowMapTarget;

    bool shadowsEnabled = true;
    bool postProcessingEnabled = true;
};