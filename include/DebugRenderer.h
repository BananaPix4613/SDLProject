#ifndef DEBUG_RENDERER_H
#define DEBUG_RENDERER_H

#include <glad/glad.h>
#include "Shader.h"

class DebugRenderer
{
public:
    DebugRenderer()
    {
        // Setup buffers for the quad
        float quadVertices[] = {
            // positions        // texture coords
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

        // Initialize debug shader
        debugShader = new Shader("shaders/DebugVertexShader.glsl", "shaders/DebugFragmentShader.glsl");
    }

    ~DebugRenderer()
    {
        glDeleteVertexArrays(1, &quadVAO);
        glDeleteBuffers(1, &quadVBO);
        delete debugShader;
    }

    // Render a texture to the screen (for debug purposes)
    void renderDebugTexture(GLuint textureID, float x, float y, float width, float height, bool isDepthTexture = true)
    {
        debugShader->use();
        debugShader->setBool("isDepthTexture", isDepthTexture);

        // Setup viewport transform to render in a corner of the screen
        glViewport(x, y, width, height);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureID);
        debugShader->setInt("debugTexture", 0);

        glBindVertexArray(quadVAO);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        glBindVertexArray(0);
    }

private:
    unsigned int quadVAO, quadVBO;
    Shader* debugShader;
};

#endif // DEBUG_RENDERER_H