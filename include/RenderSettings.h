#pragma once

#include <glm/glm.hpp>

// Performance setting for fine-tuning rendering
struct RenderSettings
{
    // Shadow settings
    bool enableShadows = true;
    unsigned int shadowMapResolution = 1024;
    bool usePCF = true;

    // Lighting settings
    float ambientStrength = 0.2f;
    float specularStrength = 0.5f;
    float shininess = 32.0f;
    float shadowBias = 0.005f;

    // Performance settings
    bool enableFrustumCulling = true;
    bool useInstancing = true;
    bool enableVSync = false;
    bool enableLOD = false;

    // Post-processing
    bool enablePostProcessing = false;
    bool enableBloom = false;
    bool enableSSAO = false;

    // Debug settings
    bool showDebugView = false;
    bool showFrustumWireframe = false;
    int currentDebugView = 0;
    bool showPerformanceOverlay = true;
    bool showChunkBoundaries = false;
    bool showBoundingBoxes = false;
    bool showGridLines = false;
    bool colorByDistance = false;
    bool showChunkStats = false;

    // Graphics quality preset
    enum QualityPreset {
        LOW,
        MEDIUM,
        HIGH,
        ULTRA
    };

    void applyPreset(QualityPreset preset)
    {
        switch (preset)
        {
        case LOW:
            enableShadows = false;
            shadowMapResolution = 512;
            usePCF = false;
            enablePostProcessing = false;
            enableBloom = false;
            enableSSAO = false;
            break;

        case MEDIUM:
            enableShadows = true;
            shadowMapResolution = 1024;
            usePCF = false;
            enablePostProcessing = false;
            enableBloom = false;
            enableSSAO = false;
            break;

        case HIGH:
            enableShadows = true;
            shadowMapResolution = 2048;
            usePCF = true;
            enablePostProcessing = true;
            enableBloom = false;
            enableSSAO = false;
            break;

        case ULTRA:
            enableShadows = true;
            shadowMapResolution = 4096;
            usePCF = true;
            enablePostProcessing = true;
            enableBloom = true;
            enableSSAO = true;
            break;
        }
    }
};