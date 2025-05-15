// -------------------------------------------------------------------------
// FlatBufferSerializer.h
// -------------------------------------------------------------------------
#pragma once

#include <flatbuffers/flatbuffers.h>
#include <flatbuffers/reflection.h>
#include <unordered_map>
#include <memory>
#include <string>
#include <vector>

namespace PixelCraft::ECS
{

    // Forward declarations
    class TransformComponent;
    class CameraComponent;
    class MeshRendererComponent;
    class LightComponent;
    class RigidBodyComponent;
    class ColliderComponent;
    class AudioSourceComponent;
    class ParticleSystemComponent;
    //class NavigationAgentComponent;
    //class AnimatorComponent;

    /**
     * @brief Zero-copy serialization using FlatBuffers for ECS components
     * 
     * FlatBufferSerializer provides an efficient serialization mechanism using FlatBuffers
     * library, enabling zero-copy deserialization for optimal performance. It manages
     * buffer creation, schema registration, and component type registration.
     */
    class FlatBufferSerializer
    {
    public:
        /**
         * @brief Constructor that initializes the serializer with default settings
         */
        FlatBufferSerializer();

        /**
         * @brief Destructor that cleans up resources
         */
        ~FlatBufferSerializer();

        /**
         * @brief Get a reference to the FlatBufferBuilder for creating new serialized data
         * @return Reference to the internal FlatBufferBuilder
         */
        flatbuffers::FlatBufferBuilder& createBuilder();

        /**
         * @brief Reset the FlatBufferBuilder to clear any existing data
         */
        void resetBuilder();

        /**
         * @brief Finalize the buffer with the given file identifier
         * @param identifier 4-character identifier string for buffer verification
         * @return Serialized data as a byte vector
         */
        std::vector<uint8_t> finishBuffer(const std::string& identifier);

        /**
         * @brief Verify a buffer's integrity and type
         * @param buffer Pointer to the buffer data
         * @param size Size of the buffer in bytes
         * @param expectedIdentifier Optional expected file identifier for type checking
         * @return True if the buffer is valid
         */
        bool verifyBuffer(const uint8_t* buffer, size_t size, const std::string& expectedIdentifier = "");

        // Specialized serialization methods for each component type

        /**
         * @brief Specialized method to serialize a TransformComponent
         * @param component TransformComponent to serialize
         * @return Offset to the serialized data in the buffer
         */
        flatbuffers::Offset<void> serializeTransformComponent(const TransformComponent& component);

        /**
         * @brief Specialized method to serialize a CameraComponent
         * @param component CameraComponent to serialize
         * @return Offset to the serialized data in the buffer
         */
        flatbuffers::Offset<void> serializeCameraComponent(const CameraComponent& component);

        /**
         * @brief Specialized method to serialize a MeshRendererComponent
         * @param component MeshRendererComponent to serialize
         * @return Offset to the serialized data in the buffer
         */
        flatbuffers::Offset<void> serializeMeshRendererComponent(const MeshRendererComponent& component);

        /**
         * @brief Specialized method to serialize a LightComponent
         * @param component LightComponent to serialize
         * @return Offset to the serialized data in the buffer
         */
        flatbuffers::Offset<void> serializeLightComponent(const LightComponent& component);

        /**
         * @brief Specialized method to serialize a RigidBodyComponent
         * @param component RigidBodyComponent to serialize
         * @return Offset to the serialized data in the buffer
         */
        flatbuffers::Offset<void> serializeRigidBodyComponent(const RigidBodyComponent& component);

        /**
         * @brief Specialized method to serialize a ColliderComponent
         * @param component ColliderComponent to serialize
         * @return Offset to the serialized data in the buffer
         */
        flatbuffers::Offset<void> serializeColliderComponent(const ColliderComponent& component);

        /**
         * @brief Specialized method to serialize an AudioSourceComponent
         * @param component AudioSourceComponent to serialize
         * @return Offset to the serialized data in the buffer
         */
        flatbuffers::Offset<void> serializeAudioSourceComponent(const AudioSourceComponent& component);

        /**
         * @brief Specialized method to serialize an ParticleSystemComponent
         * @param component ParticleSystemComponent to serialize
         * @return Offset to the serialized data in the buffer
         */
        flatbuffers::Offset<void> serializeParticleSystemComponent(const ParticleSystemComponent& component);

        /**
         * @brief Specialized method to serialize an AnimatorComponent
         * @param component AnimatorComponent to serialize
         * @return Offset to the serialized data in the buffer
         */
        //flatbuffers::Offset<void> serializeAnimatorComponent(const AnimatorComponent& component);

        /**
         * @brief Specialized method to serialize a NavigationAgentComponent
         * @param component NavigationAgentComponent to serialize
         * @return Offset to the serialized data in the buffer
         */
        //flatbuffers::Offset<void> serializeNavigationAgentComponent(const NavigationAgentComponent& component);

        // Specialized deserialization methods for each component type
        /**
         * @brief Specialized method to deserialize a TransformComponent
         * @param bufferData Pointer to the serialized data
         * @param component TransformComponent to populate with deserialized data
         * @return True if deserialization was successful
         */
        bool deserializeTransformComponent(const void* bufferData, TransformComponent& component);

        /**
         * @brief Specialized method to deserialize a CameraComponent
         * @param bufferData Pointer to the serialized data
         * @param component CameraComponent to populate with deserialized data
         * @return True if deserialization was successful
         */
        bool deserializeCameraComponent(const void* bufferData, CameraComponent& component);

        /**
         * @brief Specialized method to deserialize a MeshRendererComponent
         * @param bufferData Pointer to the serialized data
         * @param component MeshRendererComponent to populate with deserialized data
         * @return True if deserialization was successful
         */
        bool deserializeMeshRendererComponent(const void* bufferData, MeshRendererComponent& component);

        /**
         * @brief Specialized method to deserialize a LightComponent
         * @param bufferData Pointer to the serialized data
         * @param component LightComponent to populate with deserialized data
         * @return True if deserialization was successful
         */
        bool deserializeLightComponent(const void* bufferData, LightComponent& component);

        /**
         * @brief Specialized method to deserialize a RigidBodyComponent
         * @param bufferData Pointer to the serialized data
         * @param component RigidBodyComponent to populate with deserialized data
         * @return True if deserialization was successful
         */
        bool deserializeRigidBodyComponent(const void* bufferData, RigidBodyComponent& component);

        /**
         * @brief Specialized method to deserialize a ColliderComponent
         * @param bufferData Pointer to the serialized data
         * @param component ColliderComponent to populate with deserialized data
         * @return True if deserialization was successful
         */
        bool deserializeColliderComponent(const void* bufferData, ColliderComponent& component);

        /**
         * @brief Specialized method to deserialize an AudioSourceComponent
         * @param bufferData Pointer to the serialized data
         * @param component AudioSourceComponent to populate with deserialized data
         * @return True if deserialization was successful
         */
        bool deserializeAudioSourceComponent(const void* bufferData, AudioSourceComponent& component);

        /**
         * @brief Specialized method to deserialize an ParticleSystemComponent
         * @param bufferData Pointer to the serialized data
         * @param component ParticleSystemComponent to populate with deserialized data
         * @return True if deserialization was successful
         */
        bool deserializeParticleSystemComponent(const void* bufferData, ParticleSystemComponent& component);

        /**
         * @brief Specialized method to deserialize an AnimatorComponent
         * @param bufferData Pointer to the serialized data
         * @param component AnimatorComponent to populate with deserialized data
         * @return True if deserialization was successful
         */
        //bool deserializeAnimatorComponent(const void* bufferData, AnimatorComponent& component);

        /**
         * @brief Specialized method to deserialize a NavigationAgentComponent
         * @param bufferData Pointer to the serialized data
         * @param component NavigationAgentComponent to populate with deserialized data
         * @return True if deserialization was successful
         */
        //bool deserializeNavigationAgentComponent(const void* bufferData, NavigationAgentComponent& component);

        // Generic template methods
        /**
         * @brief Serialize a component to a FlatBuffer
         * @tparam T Component type to serialize
         * @param component Component instance to serialize
         * @return Offset to the serialized data in the buffer
         */
        template<typename T>
        flatbuffers::Offset<void> serializeComponent(const T& component);

        /**
         * @brief Deserialize a component from a FlatBuffer
         * @tparam T Component type to deserialize
         * @param bufferData Pointer to the serialized data
         * @param component Component instance to populate with deserialized data
         * @return True if deserialization was successful
         */
        template<typename T>
        bool deserializeComponent(const void* bufferData, T& component);

    private:
        flatbuffers::FlatBufferBuilder m_builder;
    };

    // Template method implementations must be in the header file

    template<typename T>
    flatbuffers::Offset<void> FlatBufferSerializer::serializeComponent(const T& component)
    {
        auto typeName = typeid(T).name();
        auto it = m_serializeFuncs.find(typeName);
        if (it != m_serializeFuncs.end())
        {
            return it->second(&component, *m_builder);
        }
    }

    template<typename T>
    bool FlatBufferSerializer::deserializeComponent(const void* bufferData, T& component)
    {
        // Verify the buffer first
        if (!bufferData)
        {
            return false;
        }

        auto typeName = typeid(T).name();
        auto it = m_deserializeFuncs.find(typeName);
        if (it != m_deserializeFuncs.end())
        {
            return it->second(bufferData, &component);
        }
    }

} // namespace PixelCraft::ECS