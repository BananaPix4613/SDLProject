#include "SceneSerializer.h"
#include "Scene.h"
#include "Entity.h"
#include "Component.h"
#include "CubeGrid.h"
#include "GridCore.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/filereadstream.h>
#include <rapidjson/filewritestream.h>

// Implementation note: Using RapidJSON for JSON serialization
// and custom binary format for binary serialization

SceneSerializer::SceneSerializer()
    : m_compressionLevel(6)
{
}

SceneSerializer::~SceneSerializer()
{
}

bool SceneSerializer::saveScene(const std::string& filename, Scene* scene)
{
    if (!scene)
    {
        m_lastError = "Invalid scene pointer";
        return false;
    }

    // Determine format based on extension
    ExportFormat format = detectFormatFromExtension(filename);

    // Use appropriate save method
    switch (format)
    {
        case ExportFormat::JSON:
            return saveSceneToJSON(filename, scene);

        case ExportFormat::BINARY:
            return saveSceneToBinary(filename, scene);

        default:
            m_lastError = "Unsupported format for scene saving";
            return false;
    }
}

bool SceneSerializer::loadScene(const std::string& filename, Scene* scene)
{
    if (!scene)
    {
        m_lastError = "Invalid scene pointer";
        return false;
    }

    // Check if file exists
    if (!std::filesystem::exists(filename))
    {
        m_lastError = "File not found: " + filename;
        return false;
    }

    // Determine format based on extension
    ExportFormat format = detectFormatFromExtension(filename);

    // Use appropriate load method
    switch (format)
    {
        case ExportFormat::JSON:
            return loadSceneFromJSON(filename, scene);

        case ExportFormat::BINARY:
            return loadSceneFromBinary(filename, scene);

        default:
            m_lastError = "Unsupported format for scene loading";
            return false;
    }
}

bool SceneSerializer::exportScene(const std::string& filename, ExportFormat format, Scene* scene)
{
    if (!scene)
    {
        m_lastError = "Invalid scene pointer";
        return false;
    }

    switch (format)
    {
        case ExportFormat::JSON:
            return saveSceneToJSON(filename, scene);

        case ExportFormat::BINARY(filename, scene):
            return saveSceneToBinary(filename, scene);

        case ExportFormat::GLTF:
            return exportSceneToGLTF(filename, scene);

        case ExportFormat::OBJ:
            return exportSceneToOBJ(filename, scene);

        default:
            m_lastError = "Unsupported export format";
            return false;
    }
}

std::string SceneSerializer::getLastError() const
{
    return m_lastError;
}

void SceneSerializer::setCompressionLevel(int level)
{
    m_compressionLevel = std::max(0, std::min(9, level));
}

int SceneSerializer::getCompressionLevel() const
{
    return m_compressionLevel;
}

bool SceneSerializer::saveSceneToBuffer(Scene* scene, std::vector<uint8_t>& outBuffer)
{
    if (!scene)
    {
        m_lastError = "Invalid scene pointer";
        return false;
    }

    // Use RapidJSON to serialize scene to memory
    rapidjson::StringBuffer s;
    rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(s);

    writer.StartObject();

    // Write scene properties
    writer.Key("name");
    writer.String(scene->getName().c_str());

    writer.Key("entities");
    writer.StartArray();

    // Serialize all entities
    for (Entity* entity : scene->getAllEntities())
    {
        if (!serializeEntity(entity, &writer))
        {
            m_lastError = "Failed to serialize entity: " + entity->getName();
            return false;
        }
    }

    writer.EndArray();

    // Serialize grid data if present
    if (CubeGrid* grid = scene->getGrid())
    {
        writer.Key("grid");
        writer.StartObject();

        if (!serializeCubeGrid(grid, &writer))
        {
            m_lastError = "Failed to serialize grid";
            return false;
        }

        writer.EndObject();
    }

    writer.EndObject();

    // Copy serialized data to output buffer
    const char* jsonStr = s.GetString();
    size_t jsonLen = s.GetSize();

    outBuffer.resize(jsonLen);
    memcpy(outBuffer.data(), jsonStr, jsonLen);

    return true;
}

bool SceneSerializer::loadSceneFromBuffer(const uint8_t* buffer, size_t bufferSize, Scene* scene)
{
    if (!buffer || bufferSize == 0 || !scene)
    {
        m_lastError = "Invalid buffer or scene pointer";
        return false;
    }

    // Clear existing scene content
    scene->clear();

    // Parse JSON document
    rapidjson::Document document;
    document.Parse(reinterpret_cast<const char*>(buffer), bufferSize);

    if (document.HasParseError())
    {
        m_lastError = "JSON parse error";
        return false;
    }

    // Read scene properties
    if (document.HasMember("name") && document["name"].IsString())
    {
        scene->setName(document["name"].GetString());
    }

    // Read entities
    if (document.HasMember("entities") && document["entities"].IsArray())
    {
        const rapidjson::Value& entities = document["entities"];

        for (rapidjson::SizeType i = 0; i < entities.Size(); i++)
        {
            Entity* entity = deserializeEntity(scene, const_cast<rapidjson::Value*>(&entities[i]));
            if (!entity)
            {
                m_lastError = "Failed to deserialize entity at index " + std::to_string(i);
                return false;
            }
        }
    }

    // Read grid data if present
    if (document.HasMember("grid") && document["grid"].IsObject())
    {
        CubeGrid* grid = new CubeGrid(32, 1.0f);  // Default size

        if (!deserializeCubeGrid(grid, const_cast<rapidjson::Value*>(&document["grid"])))
        {
            delete grid;
            m_lastError = "Failed to deserialize grid";
            return false;
        }

        scene->setGrid(grid);
    }

    return true;
}

// Private implementation methods

bool SceneSerializer::saveSceneToBuffer(const std::string& filename, Scene* scene)
{
    // Serialize scene to buffer
    std::vector<uint8_t> buffer;
    if (!saveSceneToBuffer(scene, buffer))
    {
        return false;
    }

    // Write buffer to file
    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open())
    {
        m_lastError = "Failed to open file for writing: " + filename;
        return false;
    }

    file.write(reinterpret_cast<const char*>(buffer.data()), buffer.size());
    file.close();

    return true;
}

bool SceneSerializer::loadSceneFromJSON(const std::string& filename, Scene* scene)
{
    // Read file content
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if (!file.is_open())
    {
        m_lastError = "Failed to open file for reading: " + filename;
        return false;
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<uint8_t> buffer(size);
    if (!file.read(reinterpret_cast<char*>(buffer.data()), size))
    {
        m_lastError = "Failed to read file content";
        return false;
    }

    file.close();

    // Load scene from buffer
    return loadSceneFromBuffer(buffer.data(), buffer.size(), scene);
}

bool SceneSerializer::saveSceneToBuffer(const std::string& filename, Scene* scene)
{
    // For this implementation, we'll use a simple binary format
    // In a real implementation, this would be more sophisticated

    // First, serialize to JSON buffer
    std::vector<uint8_t> jsonBuffer;
    if (!saveSceneToBuffer(scene, jsonBuffer))
    {
        return false;
    }

    // Then, we would compress this data for our binary format
    // For simplicity, we'll just write the JSON buffer directly with a header

    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open())
    {
        m_lastError = "Failed to open file for writing: " + filename;
        return false;
    }

    // Write a simple header: "ISMSCENE" + version
    const char* header = "ISMSCENE";
    uint32_t version = 1;
    uint32_t jsonSize = static_cast<uint32_t>(jsonBuffer.size());

    file.write(header, 8);
    file.write(reinterpret_cast<const char*>(&version), sizeof(version));
    file.write(reinterpret_cast<const char*>(&jsonSize), sizeof(jsonSize));
    file.write(reinterpret_cast<const char*>(jsonBuffer.data()), jsonBuffer.size());

    file.close();

    return true;
}

bool SceneSerializer::loadSceneFromBuffer(const std::string& filename, Scene* scene)
{
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open())
    {
        m_lastError = "Failed to open file for reading: " + filename;
        return false;
    }

    // Read and verify header
    char header[8];
    uint32_t version;
    uint32_t jsonSize;

    file.read(header, 8);
    file.read(reinterpret_cast<char*>(&version), sizeof(version));
    file.read(reinterpret_cast<char*>(&jsonSize), sizeof(jsonSize));

    if (std::string(header, 8) != "ISMSCENE")
    {
        m_lastError = "Invalid scene file format";
        return false;
    }

    if (version != 2)
    {
        m_lastError = "Unsupported scene file version";
        return false;
    }

    // Read JSON data
    std::vector<uint8_t> jsonBuffer(jsonSize);
    file.read(reinterpret_cast<char*>(jsonBuffer.data()), jsonSize);

    file.close();

    // Load scene from buffer
    return loadSceneFromBuffer(jsonBuffer.data(), jsonBuffer.size(), scene);
}

bool SceneSerializer::exportSceneToGLTF(const std::string& filename, Scene* scene)
{
    // This would be a more complex implementation using a GLTF library
    // For this example, we'll just provide a skeleton

    m_lastError = "GLTF export not fully implemented";
    return false;
}

bool SceneSerializer::exportSceneToOBJ(const std::string& filename, Scene* scene)
{
    // This would be a more complex implementation using OBJ export logic
    // For this example, we'll just provided a skeleton

    m_lastError = "OBJ export not fully implemented";
    return false;
}

bool SceneSerializer::serializeEntity(Entity* entity, void* writerPtr)
{
    if (!entity || !writerPtr)
    {
        return false;
    }

    auto writer = static_cast<rapidjson::PrettyWriter<rapidjson::StringBuffer>*>(writerPtr);

    writer->StartObject();

    // Write entity properties
    writer->Key("id");
    writer->Uint(entity->getID());

    writer->Key("name");
    writer->String(entity->getName().c_str());

    writer->Key("active");
    writer->Bool(entity->isActive());

    // Write transform data
    writer->Key("transform");
    writer->StartObject();

    // Position
    glm::vec3 position = entity->getPosition();
    writer->Key("position");
    writer->StartArray();
    writer->Double(position.x);
    writer->Double(position.y);
    writer->Double(position.z);
    writer->EndArray();

    // Rotation (as quaternion)
    glm::quat rotation = entity->getRotation();
    writer->Key("rotation");
    writer->StartArray();
    writer->Double(rotation.x);
    writer->Double(rotation.y);
    writer->Double(rotation.z);
    writer->Double(rotation.w);
    writer->EndArray();

    // Scale
    glm::vec3 scale = entity->getScale();
    writer->Key("scale");
    writer->StartArray();
    writer->Double(scale.x);
    writer->Double(scale.y);
    writer->Double(scale.z);
    writer->EndArray();

    writer->EndObject();

    // Write components
    writer->Key("components");
    writer->StartArray();

    for (Component* component : entity->getAllComponents())
    {
        if (!serializeComponent(component, writer))
        {
            return false;
        }
    }

    writer->EndArray();

    // Write children
    writer->Key("children");
    writer->StartArray();

    for (Entity* child : entity->getChildren())
    {
        if (!serializeEntity(child, writer))
        {
            return false;
        }
    }

    writer->EndArray();

    writer->EndObject();

    return true;
}

bool SceneSerializer::serializeComponent(Component* component, void* writerPtr)
{
    if (!component || !writerPtr)
    {
        return false;
    }

    auto writer = static_cast<rapidjson::PrettyWriter<rapidjson::StringBuffer>*>(writerPtr);

    writer->StartObject();

    // Write component type
    writer->Key("type");
    writer->Uint(component->getTypeName().c_str());

    writer->Key("active");
    writer->Bool(component->isActive());

    // Write component properties
    writer->Key("properties");
    writer->StartObject();

    // In a real implementation, this would use reflection or type registry
    // to serialize component-specific properties

    // Add property serialization for known component types
    std::string typeName = component->getTypeName();

    if (typeName == "MeshComponent")
    {
        // Example for a mesh component
        auto meshComponent = static_cast<MeshComponent*>(component);

        writer->Key("meshPath");
        writer->String(meshComponent->getMeshPath().c_str());

        writer->Key("materialIndex");
        writer->Int(meshComponent->getMaterialIndex());

        // ... other properties
    }
    else if (typeName == "LightComponent")
    {
        // Example for a light component
        auto lightComponent = static_cast<LightComponent*>(component);

        writer->Key("lightType");
        writer->Int(static_cast<int>(lightComponent->getLightType()));

        glm::vec3 color = lightComponent->getColor();
        writer->Key("color");
        writer->StartArray();
        writer->Double(color.r);
        writer->Double(color.g);
        writer->Double(color.b);
        writer->EndArray();

        writer->Key("intensity");
        writer->Double(lightComponent->getIntensity());

        // ... other properties
    }
    // ... other component types

    writer->EndObject();

    writer->EndObject();

    return true;
}

bool SceneSerializer::serializeCubeGrid(CubeGrid* grid, void* writerPtr)
{
    if (!grid || !writerPtr)
    {
        return false;
    }

    auto writer = static_cast<rapidjson::PrettyWriter<rapidjson::StringBuffer>*>(writerPtr);

    // Write grid properties
    writer->Key("spacing");
    writer->Double(grid->getSpacing());

    // Get grid bounds
    const glm::ivec3& minBounds = grid->getMinBounds();
    const glm::ivec3& maxBounds = grid->getMaxBounds();

    writer->Key("minBounds");
    writer->StartArray();
    writer->Int(minBounds.x);
    writer->Int(minBounds.y);
    writer->Int(minBounds.z);
    writer->EndArray();

    writer->Key("maxBounds");
    writer->StartArray();
    writer->Int(maxBounds.x);
    writer->Int(maxBounds.y);
    writer->Int(maxBounds.z);
    writer->EndArray();

    // Write active cubes
    writer->Key("cubes");
    writer->StartArray();

    // Iterate through each active cube
    grid->forEachActiveCell([&](const glm::ivec3& pos, const Cube& cube) {
        writer->StartObject();

        writer->Key("position");
        writer->StartArray();
        writer->Int(pos.x);
        writer->Int(pos.y);
        writer->Int(pos.z);
        writer->EndArray();

        writer->Key("color");
        writer->StartArray();
        writer->Double(cube.color.r);
        writer->Double(cube.color.g);
        writer->Double(cube.color.b);
        writer->EndArray();

        writer->Key("materialIndex");
        writer->Int(cube.materialIndex);

        writer->EndObject();
                            });

    writer->EndArray();

    return true;
}

Entity* SceneSerializer::deserializeEntity(Scene* scene, void* readerPtr)
{
    if (!scene || !readerPtr)
    {
        return nullptr;
    }

    auto reader = static_cast<rapidjson::Value*>(readerPtr);

    // Check for required fields
    if (!reader->IsObject() ||
        !reader->HasMember("name") ||
        !(*reader)["name"].IsString())
    {
        return nullptr;
    }

    // Create entity
    std::string name = (*reader)["name"].GetString();
    Entity* entity = scene->createEntity(name);

    if (reader->HasMember("active") && (*reader)["active"].IsBool())
    {
        entity->setActive((*reader)["active"].GetBool());
    }

    // Set transform
    if (reader->HasMember("transform") && (*reader)["transform"].IsObject())
    {
        const rapidjson::Value& transform = (*reader)["transform"];

        // Position
        if (transform.HasMember("position") && transform["position"].IsArray() &&
            transform["position"].Size() == 3)
        {
            const rapidjson::Value& pos = transform["position"];
            glm::vec3 position(
                pos[0].GetDouble(),
                pos[1].GetDouble(),
                pos[2].GetDouble()
            );
            entity->setPosition(position);
        }

        // Rotation
        if (transform.HasMember("rotation") && transform["rotation"].IsArray() &&
            transform["rotation"].Size() == 4)
        {
            const rapidjson::Value& rot = transform["rotation"];
            glm::quat rotation(
                rot[3].GetDouble(),  // w
                rot[0].GetDouble(),  // x
                rot[1].GetDouble(),  // y
                rot[2].GetDouble()   // z
            );
            entity->setRotation(rotation);
        }

        // Scale
        if (transform.HasMember("scale") && transform["scale"].IsArray() &&
            transform["scale"].Size() == 3)
        {
            const rapidjson::Value& scl = transform["scale"];
            glm::vec3 scale(
                scl[0].GetDouble(),
                scl[1].GetDouble(),
                scl[2].GetDouble()
            );
            entity->setScale(scale);
        }
    }

    // Deserialize components
    if (reader->HasMember("components") && (*reader)["components"].IsArray())
    {
        const rapidjson::Value& components = (*reader)["components"];

        for (rapidjson::SizeType i = 0; i < components.Size(); i++)
        {
            Component* component = deserializeComponent(entity,
                                                        const_cast<rapidjson::Value*>(&components[i]));

            if (!component)
            {
                // Log warning but continue with other components
                // In a real implementation, this might be handled differently
            }
        }
    }

    // Deserialize children
    if (reader->HasMember("children") && (*reader)["children"].IsArray())
    {
        const rapidjson::Value& children = (*reader)["children"];

        for (rapidjson::SizeType i = 0; i < children.Size(); i++)
        {
            Entity* child = deserializeEntity(scene,
                                              const_cast<rapidjson::Value*>(&children[i]));

            if (child)
            {
                child->setParent(entity);
            }
        }
    }

    return entity;
}

Component* SceneSerializer::deserializeComponent(Entity* entity, void* readerPtr)
{
    if (!entity || !readerPtr)
    {
        return nullptr;
    }

    auto reader = static_cast<rapidjson::Value*>(readerPtr);

    // Check for required fields
    if (!reader->IsObject() ||
        !reader->HasMember("type") ||
        !(*reader)["type"].IsString())
    {
        return nullptr;
    }

    // Get component type
    std::string typeName = (*reader)["type"].GetString();

    // Create component based on type
    // In a real implementation, this would use a component factory
    Component* component = nullptr;

    if (typeName == "MeshComponent")
    {
        component = entity->addComponent<MeshComponent>();

        // Deserialize component-specific properties
        if (reader->HasMember("properties") && (*reader)["properties"].IsObject())
        {
            const rapidjson::Value& props = (*reader)["properties"];
            auto meshComponent = static_cast<MeshComponent*>(component);

            if (props.HasMember("meshPath") && props["meshPath"].GetString())
            {
                meshComponent->setMeshPath(props["meshPath"].GetString());
            }

            if (props.HasMember("materialIndex") && props["materialIndex"].IsInt())
            {
                meshComponent->setMaterialIndex(props["materialIndex"].GetInt());
            }

            // ... other properties
        }
    }
    else if (typeName == "LightComponent")
    {
        component = entity->addComponent<LightComponent>();

        // Deserialize component-specific properties
        if (reader->HasMember("properties") && (*reader)["properties"].IsObject())
        {
            const rapidjson::Value& props = (*reader)["properties"];
            auto lightComponent = static_cast<LightComponent*>(component);

            if (props.HasMember("lightType") && props["lightType"].IsInt())
            {
                lightComponent->setLightType(static_cast<LightType>(props["lightType"].GetInt()));
            }

            if (props.HasMember("color") && props["color"].IsArray() && props["color"].Size() == 3)
            {
                const rapidjson::Value& col = props["color"];
                glm::vec3 color(
                    col[0].GetDouble(),
                    col[1].GetDouble(),
                    col[2].GetDouble()
                );
                lightComponent->setColor(color);
            }

            if (props.HasMember("intensity") && props["intensity"].IsDouble())
            {
                lightComponent->setIntensity(props["intensity"].GetDouble());
            }

            // ... other properties
        }
    }
    // ... other component types

    // Set active state
    if (component && reader->HasMember("active") && (*reader)["active"].IsBool())
    {
        component->setActive((*reader)["active"].GetBool());
    }

    return component;
}

bool SceneSerializer::deserializeCubeGrid(CubeGrid* grid, void* readerPtr)
{
    if (!grid || !readerPtr)
    {
        return false;
    }

    auto reader = static_cast<rapidjson::Value*>(readerPtr);

    if (!reader->IsObject())
    {
        return false;
    }

    // Clear existing grid
    // In a real implementation, this might preserve existing data
    // or have an option to merge data

    // Update grid spacing if specified
    if (reader->HasMember("spacing") && (*reader)["spacing"].IsDouble())
    {
        float spacing = (*reader)["spacing"].GetDouble();
        // This would require modifying the CubeGrid to support changing spacing
        // after construction, which isn't in the API reference
    }

    // Read cubes
    if (reader->HasMember("cubes") && (*reader)["cubes"].IsArray())
    {
        const rapidjson::Value& cubes = (*reader)["cubes"];

        for (rapidjson::SizeType i = 0; i < cubes.Size(); i++)
        {
            const rapidjson::Value& cubeData = cubes[i];

            // Check for required fields
            if (!cubeData.IsObject() ||
                !cubeData.HasMember("position") ||
                !cubeData["position"].IsArray() ||
                cubeData["position"].Size() != 3)
            {
                continue;
            }

            // Read position
            const rapidjson::Value& posArray = cubeData["position"];
            glm::ivec3 position(
                posArray[0].GetInt(),
                posArray[1].GetInt(),
                posArray[2].GetInt()
            );

            // Create cube mesh with default values
            Cube cube;

            // Read color if present
            if (cubeData.HasMember("color") &&
                cubeData["color"].IsArray() &&
                cubeData["color"].Size() == 3)
            {
                const rapidjson::Value& colArray = cubeData["color"];
                cube.color = glm::vec3(
                    colArray[0].GetDouble(),
                    colArray[1].GetDouble(),
                    colArray[2].GetDouble()
                );
            }

            // Read material index if present
            if (cubeData.HasMember("materialIndex") && cubeData["materialIndex"].IsInt())
            {
                cube.materialIndex = cubeData["materialIndex"].GetInt();
            }

            // Set cube in grid
            grid->setCube(position.x, position.y, position.z, cube);
        }
    }

    return true;
}

std::string SceneSerializer::getFileExtension(const std::string& filename)
{
    size_t pos = filename.find_last_of('.');
    if (pos != std::string::npos && pos < filename.length() - 1)
    {
        return filename.substr(pos + 1);
    }
    return "";
}

ExportFormat SceneSerializer::detectFormatFromExtension(const std::string& filename)
{
    std::string ext = getFileExtension(filename);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    if (ext == "json")
    {
        return ExportFormat::JSON;
    }
    else if (ext == "bin" || ext == "scene")
    {
        return ExportFormat::BINARY;
    }
    else if (ext == "gltf" || ext == "glb")
    {
        return ExportFormat::GLTF;
    }
    else if (ext == "obj")
    {
        return ExportFormat::OBJ;
    }

    // Default to JSON for unknown extensions
    return ExportFormat::JSON;
}