#pragma once

#include <string>
#include <memory>
#include <vector>
#include "Scene.h"

// Forward declarations
class Entity;
class Component;
class CubeGrid;
template <typename T> class Grid;

/**
 * @enum ExportFormat
 * @brief Defines different export formats for scenes
 */
enum class ExportFormat
{
    JSON,       // JSON format (human readable)
    BINARY,     // Custom binary format (compact)
    GLTF,       // GLTF 2.0 format (for interoperability)
    OBJ         // Wavefront OBJ format (mesh only)
};

/**
 * @class SceneSerializer
 * @brief Handles saving and loading scene data to and from files
 * 
 * The SceneSerializer provides functionality to serialize scene data, including
 * entities, components, and grid data. It supports multiple formats for flexibility.
 */
class SceneSerializer
{
public:
    /**
     * @brief Constructor
     */
    SceneSerializer();

    /**
     * @brief Destructor
     */
    ~SceneSerializer();

    /**
     * @brief Save a scene to a file
     * @param filename Path to save the scene to
     * @param scene Pointer to the scene to save
     * @return True if save succeeded
     */
    bool saveScene(const std::string& filename, Scene* scene);

    /**
     * @brief Load a scene from a file
     * @param filename Path to the scene file
     * @param scene Pointer to the scene to load into
     * @return True if load succeeded
     */
    bool loadScene(const std::string& filename, Scene* scene);

    /**
     * @brief Export a scene to a different format
     * @param filename Path to export the scene to
     * @param format Format to export to
     * @param scene Pointer to the scene to export
     * @return True if export succeeded
     */
    bool exportScene(const std::string& filename, ExportFormat format, Scene* scene);

    /**
     * @brief Get the last error message
     * @return Error message string
     */
    std::string getLastError() const;

    /**
     * @brief Set compression level for binary formats
     * @param level Compression level (0-9, where 0 is no compression, 9 is highest)
     */
    void setCompressionLevel(int level);

    /**
     * @brief Get the current compression level
     * @return Current compression level
     */
    int getCompressionLevel() const;

    /**
     * @brief Save scene to a memory buffer
     * @param scene Pointer to the scene to save
     * @param outBuffer Reference to vector to store serialized data
     * @return True if save succeeded
     */
    bool saveSceneToBuffer(Scene* scene, std::vector<uint8_t>& outBuffer);

    /**
     * @brief Load scene from a memory buffer
     * @param buffer Buffer containing serialized scene data
     * @param bufferSize Size of the buffer in bytes
     * @param scene Pointer to the scene to load into
     * @return True if load succeeded
     */
    bool loadSceneFromBuffer(const uint8_t* buffer, size_t bufferSize, Scene* scene);

private:
    std::string m_lastError;
    int m_compressionLevel;

    /**
     * @brief Save a scene to a JSON file
     * @param filename Path to save the scene to
     * @param scene Pointer to the scene to save
     * @return True if save succeeded
     */
    bool saveSceneToJSON(const std::string& filename, Scene* scene);

    /**
     * @brief Load a scene from a JSON file
     * @param filename Path to the scene file
     * @param scene Pointer to the scene to load into
     * @return True if load succeeded
     */
    bool loadSceneFromJSON(const std::string& filename, Scene* scene);

    /**
     * @brief Save a scene to a binary file
     * @param filename Path to save the scene to
     * @param scene Pointer to the scene to save
     * @return True if save succeeded
     */
    bool saveSceneToBinary(const std::string& filename, Scene* scene);

    /**
     * @brief Load a scene from a binary file
     * @param filename Path to the scene file
     * @param scene Pointer to the scene to load into
     * @return True if load succeeded
     */
    bool loadSceneFromBinary(const std::string& filename, Scene* scene);

    /**
     * @brief Export a scene to GLTF format
     * @param filename Path to export the scene to
     * @param scene Pointer to the scene to export
     * @return True if export succeeded
     */
    bool exportSceneToGLTF(const std::string& filename, Scene* scene);

    /**
     * @brief Export a scene to OBJ format
     * @param filename Path to export the scene to
     * @param scene Pointer to the scene to export
     * @return True if export succeeded
     */
    bool exportSceneToOBJ(const std::string& filename, Scene* scene);

    /**
     * @brief Serialize an entity to the given writer
     * @param entity Entity to serialize
     * @param writer Writer to serialize to
     * @return True if serialization succeeded
     */
    bool serializeEntity(Entity* entity, void* writer);

    /**
     * @brief Serialize a component to the given writer
     * @param component Component to serialize
     * @param writer Writer to serialize to
     * @return True if serialization succeeded
     */
    bool serializeComponent(Component* component, void* writer);

    /**
     * @brief Serialize a grid to the given writer
     * @param grid Grid to serialize
     * @param writer Writer to serialize to
     * @return True if serialization succeeded
     */
    bool serializeGrid(Grid<void*>* grid, void* writer);

    /**
     * @brief Serialize a cube grid to the given writer
     * @param grid CubeGrid to serialize
     * @param writer Writer to serialize to
     * @return True if serialization succeeded
     */
    bool serializeCubeGrid(CubeGrid* grid, void* writer);

    /**
     * @brief Deserialize an entity from the given reader
     * @param scene Scene to add the entity to
     * @param reader Reader to deserialize from
     * @return Created entity pointer, or nullptr on failure
     */
    Entity* deserializeEntity(Scene* scene, void* reader);

    /**
     * @brief Deserialize a component from the given reader
     * @param entity Entity to add the component to
     * @param reader Reader to deserialize from
     * @return Created component pointer, or nullptr on failure
     */
    Component* deserializeComponent(Entity* entity, void* reader);

    /**
     * @brief Deserialize a grid from the given reader
     * @param grid Grid to deserialize into
     * @param reader Reader to deserialize from
     * @return True if deserialization succeeded
     */
    bool deserializeGrid(Grid<void*>* grid, void* reader);

    /**
     * @brief Deserialize a cube grid from the given reader
     * @param grid CubeGrid to deserialize into
     * @param reader Reader to deserialize from
     * @return True if deserialization succeeded
     */
    bool deserializeCubeGrid(CubeGrid* grid, void* reader);

    /**
     * @brief Get the file extension from a filename
     * @param filename Filename to get extension from
     * @return File extension string
     */
    std::string getFileExtension(const std::string& filename);

    /**
     * @brief Detect export format from file extension
     * @param filename Filename to detect format from
     * @return Detected export format
     */
    ExportFormat detectFormatFromExtension(const std::string& filename);
};