#include "LightManager.h"
#include <iostream>

void LightManager::initialize()
{
    // Create a default directional light
    auto defaultLight = createDirectionalLight("MainLight");
    defaultLight->setDirection(glm::vec3(-0.5f, -1.0f, -0.3f));
    defaultLight->setColor(glm::vec3(1.0f, 1.0f, 1.0f));
    defaultLight->setIntensity(1.0f);
    defaultLight->setCastShadows(true);

    // Set default ambient settings
    ambientSettings.color = glm::vec3(0.1f, 0.1f, 0.2f); // Slight blue tint
    ambientSettings.intensity = 0.15f;

    std::cout << "LightManager initialized with default directional light" << std::endl;
}

std::shared_ptr<DirectionalLight> LightManager::createDirectionalLight(const std::string& name)
{
    // Check if name already exists
    for (const auto& light : lights)
    {
        if (light->getName() == name)
        {
            std::cerr << "Light with name '" << name << "' already exists" << std::endl;
            return nullptr;
        }
    }

    auto light = std::make_shared<DirectionalLight>(name);
    lights.push_back(light);

    // Create shadow map for this light
    auto shadowMap = std::make_shared<RenderTarget>(shadowMapResolution, shadowMapResolution);
    shadowMaps[name] = shadowMap;

    return light;
}

std::shared_ptr<PointLight> LightManager::createPointLight(const std::string& name)
{
    // Check if name already exists
    for (const auto& light : lights)
    {
        if (light->getName() == name)
        {
            std::cerr << "Light with name '" << name << "' already exists" << std::endl;
            return nullptr;
        }
    }

    auto light = std::make_shared<PointLight>(name);
    lights.push_back(light);

    // Point lights would need cubemaps for shadows - not implemented in this version

    return light;
}

std::shared_ptr<SpotLight> LightManager::createSpotLight(const std::string& name)
{
    // Check if name already exists
    for (const auto& light : lights)
    {
        if (light->getName() == name)
        {
            std::cerr << "Light with name '" << name << "' already exists" << std::endl;
            return nullptr;
        }
    }

    auto light = std::make_shared<SpotLight>(name);
    lights.push_back(light);

    // Create shadow map for this light
    auto shadowMap = std::make_shared<RenderTarget>(shadowMapResolution, shadowMapResolution);
    shadowMaps[name] = shadowMap;

    return light;
}

std::shared_ptr<Light> LightManager::getLight(const std::string& name)
{
    for (const auto& light : lights)
    {
        if (light->getName() == name)
        {
            return light;
        }
    }
    return nullptr;
}

void LightManager::removeLight(const std::string& name)
{
    for (auto it = lights.begin(); it != lights.end(); it++)
    {
        if ((*it)->getName() == name)
        {
            // Remove from lights vector
            lights.erase(it);

            // Remove shadow map if it exists
            shadowMaps.erase(name);
            lightSpaceMatrices.erase(name);

            return;
        }
    }
}

std::vector<std::shared_ptr<Light>> LightManager::getLightsByType(LightType type) const
{
    std::vector<std::shared_ptr<Light>> result;
    for (const auto& light : lights)
    {
        if (light->getType() == type && light->isEnabled())
        {
            result.push_back(light);
        }
    }
    return result;
}

const std::vector<std::shared_ptr<Light>>& LightManager::getAllLights() const
{
    return lights;
}

void LightManager::applyLightsToShader(Shader* shader) const
{
    if (!shader) return;

    // Set ambient light
    shader->setVec3("ambientLight.color", ambientSettings.color);
    shader->setFloat("ambientLight.intensity", ambientSettings.intensity);

    // Set light count
    int enabledLightCount = 0;
    for (const auto& light : lights)
    {
        if (light->isEnabled())
        {
            enabledLightCount++;
        }
    }
    shader->setInt("lightCount", enabledLightCount);

    // Apply all enabled lights
    int lightIndex = 0;
    for (const auto& light : lights)
    {
        if (light->isEnabled())
        {
            light->setShaderParameters(shader, lightIndex);

            // If this light casts shadows, set its shadow map and matrix
            if (light->doesCastShadows())
            {
                auto it = shadowMaps.find(light->getName());
                if (it != shadowMaps.end())
                {
                    std::string prefix = "lights[" + std::to_string(lightIndex) + "].";

                    // Set shadow matrix for this light
                    auto matrixIt = lightSpaceMatrices.find(light->getName());
                    if (matrixIt != lightSpaceMatrices.end())
                    {
                        shader->setMat4(prefix + "lightSpaceMatrix", matrixIt->second);
                    }

                    // Bind shadow map texture (assumes textures are bound in sequence after other textures)
                    int textureUnit = 2 + lightIndex; // Start at texture unit 2 (assuming 0-1 are used for other purposes)
                    glActiveTexture(GL_TEXTURE0 + textureUnit);
                    glBindTexture(GL_TEXTURE_2D, it->second->getDepthTexture());
                    shader->setInt(prefix + "shadowMap", textureUnit);
                }
            }

            lightIndex++;
        }
    }
}

void LightManager::renderShadowMaps(RenderContext& context)
{
    // Save original state
    GLint previousViewport[4];
    glGetIntegerv(GL_VIEWPORT, previousViewport);

    // For each shadow-casting light
    for (const auto& light : lights)
    {
        if (!light->isEnabled() || !light->doesCastShadows()) continue;

        auto shadowMapIt = shadowMaps.find(light->getName());
        if (shadowMapIt == shadowMaps.end()) continue;

        auto& shadowMap = shadowMapIt->second;

        // Calculate light space matrix
        glm::mat4 lightSpaceMatrix = light->calculateLightSpaceMatrix();

        // Store for later use
        lightSpaceMatrices[light->getName()] = lightSpaceMatrix;

        // Bind shadow map framebuffer
        shadowMap->bind();

        // Set viewport to shadow map size
        glViewport(0, 0, shadowMap->getWidth(), shadowMap->getHeight());

        // Clear depth buffer
        glClear(GL_DEPTH_BUFFER_BIT);

        // Set up depth-only rendering
        glCullFace(GL_FRONT); // Helps prevent shadow acne
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(2.0f, 4.0f);

        // Create a new context for shadow rendering
        RenderContext shadowContext;
        shadowContext.lightSpaceMatrix = lightSpaceMatrix;
        shadowContext.enableShadowDepthPass = true;

        // Let each renderable object handle its own shadow rendering
        for (auto* obj : context.renderableObjects)
        {
            if (obj && obj->getActive() && obj->getVisible())
            {
                obj->renderShadow(shadowContext);
            }
        }

        // Reset state
        glDisable(GL_POLYGON_OFFSET_FILL);
        glCullFace(GL_BACK);

        // Unbind shadow map framebuffer
        shadowMap->unbind();
    }

    // Restore viewport
    glViewport(previousViewport[0], previousViewport[1],
               previousViewport[2], previousViewport[3]);
}

unsigned int LightManager::getShadowMapTexture(const std::string& lightName) const
{
    auto it = shadowMaps.find(lightName);
    if (it != shadowMaps.end())
    {
        return it->second->getDepthTexture();
    }
    return 0;
}

glm::mat4 LightManager::getLightSpaceMatrix(const std::string& lightName) const
{
    auto it = lightSpaceMatrices.find(lightName);
    if (it != lightSpaceMatrices.end())
    {
        return it->second;
    }
    return glm::mat4(1.0f);
}

void LightManager::shutdown()
{
    // Clear all lights
    lights.clear();

    // Clear shadow maps
    shadowMaps.clear();

    // Clear light space matrices
    lightSpaceMatrices.clear();
}
