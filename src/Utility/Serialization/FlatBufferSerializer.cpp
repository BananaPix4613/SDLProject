// -------------------------------------------------------------------------
// FlatBufferSerializer.cpp
// -------------------------------------------------------------------------
#include "ECS/FlatBufferSerializer.h"
#include "Core/Logger.h"

// Include component headers
#include "ECS/Components/TransformComponent.h"
#include "ECS/Components/CameraComponent.h"

// Include generated FlatBuffer headers
#include "ECS/Schemas/transform_component_generated.h"
#include "ECS/Schemas/camera_component_generated.h"
#include "ECS/Schemas/mesh_renderer_component_generated.h"
#include "ECS/Schemas/light_component_generated.h"
#include "ECS/Schemas/rigid_body_component_generated.h"
#include "ECS/Schemas/collider_component_generated.h"
#include "ECS/Schemas/audio_source_component_generated.h"
#include "ECS/Schemas/particle_system_component_generated.h"

namespace Log = PixelCraft::Core;

namespace PixelCraft::ECS
{

    FlatBufferSerializer::FlatBufferSerializer() : m_builder(1024)
    {
        Log::info("FlatBufferSerializer: Initialized with 1KB initial capacity");
    }

    FlatBufferSerializer::~FlatBufferSerializer()
    {
        // Smart pointers handle cleanup
        Log::debug("FlatBufferSerializer: Destroyed");
    }

    flatbuffers::FlatBufferBuilder& FlatBufferSerializer::createBuilder()
    {
        return m_builder;
    }

    void FlatBufferSerializer::resetBuilder()
    {
        m_builder.Clear();
        Log::debug("FlatBufferSerializer: Builder reset");
    }

    std::vector<uint8_t> FlatBufferSerializer::finishBuffer(const std::string& identifier)
    {
        // Ensure identifier is exactly 4 characters (FlatBuffer requirement)
        std::string fileId = identifier;
        if (fileId.length() != 4)
        {
            Log::warn("FlatBufferSerializer: File identifier must be exactly 4 characters, padding or truncating");
            fileId.resize(4, ' '); // Pad with spaces or truncating
        }

        Log::debug("FlatBufferSerializer: Finished buffer with identifier '" + fileId +
                   "', size: " + std::to_string(m_builder.GetSize()) + " bytes");

        return std::vector<uint8_t>(m_builder.GetBufferPointer(),
                                    m_builder.GetBufferPointer() + m_builder.GetSize());
    }

    bool FlatBufferSerializer::verifyBuffer(const uint8_t* buffer, size_t size, const std::string& expectedIdentifier)
    {
        // Create a FlatBuffers verifier
        flatbuffers::Verifier verifier(buffer, size);

        // If we have an expected identifier, verify it
        if (!expectedIdentifier.empty())
        {
            // Verify minimum size for a buffer with identifier
            if (size < 8)
            {
                Log::error("FlatBufferSerializer: Buffer too small to contain an identifier");
                return false;
            }

            // Check file identifier using FlatBuffers API
            const char* file_identifier = flatbuffers::GetBufferIdentifier(buffer);
            if (strncmp(file_identifier, expectedIdentifier.c_str(), 4) != 0)
            {
                Log::error("FlatBufferSerializer: Buffer has incorrect identifier");
                return false;
            }
        }

        return verifier.VerifyBuffer<TransformComponentData>(nullptr);
    }

    flatbuffers::Offset<void> FlatBufferSerializer::serializeTransformComponent(const TransformComponent& component)
    {
        Log::debug("FlatBufferSerialize: Serializing TransformComponent");

        // Create the position, rotation, and scale structures
        Vec3 position(
            component.getLocalPosition().x,
            component.getLocalPosition().y,
            component.getLocalPosition().z
        );

        Quat rotation(
            component.getLocalRotation().x,
            component.getLocalRotation().y,
            component.getLocalRotation().z,
            component.getLocalRotation().w
        );

        Vec3 scale(
            component.getLocalScale().x,
            component.getLocalScale().y,
            component.getLocalScale().z
        );

        // Get children and convert to a vector of uint32_t
        std::vector<uint32_t> children;
        children.reserve(component.getChildren().size());
        for (EntityID child : component.getChildren())
        {
            children.push_back(child);
        }

        // Create the vector of children
        auto childrenVector = m_builder.CreateVector(children);

        // Create the TransformComponent
        auto transformComponent = CreateTransformComponentData(
            m_builder,
            &position,
            &rotation,
            &scale,
            component.getParent(),
            childrenVector,
            component.isWorldTransformDirty()
        );

        return transformComponent.Union();
    }

    bool FlatBufferSerializer::deserializeTransformComponent(const void* bufferData, TransformComponent& component)
    {
        auto transformData = GetTransformComponentData(bufferData);
        if (!transformData)
        {
            Log::error("FlatBufferSerializer: Null buffer data for TransformComponent deserialization");
            return false;
        }

        Log::debug("FlatBufferSerializer: Deserializing TransformComponent");

        // Extract the data and update the component
        if (auto pos = transformData->local_position())
        {
            component.setLocalPosition(glm::vec3(pos->x(), pos->y(), pos->z()));
        }

        if (auto rot = transformData->local_rotation())
        {
            component.setLocalRotation(glm::quat(rot->w(), rot->x(), rot->y(), rot->z()));
        }

        if (auto scl = transformData->local_scale())
        {
            component.setLocalScale(glm::vec3(scl->x(), scl->y(), scl->z()));
        }

        // Set parent
        component.setParent(transformData->parent());

        // Clear existing children and add new ones
        component.clearChildren();
        if (auto children = transformData->children())
        {
            for (auto child : *children)
            {
                component.addChild(child);
            }
        }

        return true;
    }

    flatbuffers::Offset<void> FlatBufferSerializer::serializeCameraComponent(const CameraComponent& component)
    {
        Log::debug("FlatBufferSerialize: Serializing CameraComponent");

        auto cameraComponent = CreateCameraComponentData(
            m_builder,
            static_cast<CameraType>(component.getCameraType()),
            component.getFieldOfView(),
            component.getOrthographicSize(),
            component.getNearPlane(),
            component.getFarPlane(),
            component.getAspectRatio(),
            component.isMain()
        );

        return cameraComponent.Union();
    }

    bool FlatBufferSerializer::deserializeCameraComponent(const void* bufferData, CameraComponent& component)
    {
        auto cameraData = GetCameraComponentData(bufferData);
        if (!cameraData)
        {
            Log::error("FlatBufferSerializer: Null buffer data for CameraComponent deserialization");
            return false;
        }

        Log::debug("FlatBufferSerializer: Deserializing CameraComponent");

        component.setCameraType(static_cast<CameraComponent::CameraType>(cameraData->camera_type()));
        component.setFieldOfView(cameraData->fov());
        component.setOrthographicSize(cameraData->ortho_size());
        component.setNearPlane(cameraData->near_plane());
        component.setFarPlane(cameraData->far_plane());
        component.setAspectRatio(cameraData->aspect_ratio());
        component.setMain(cameraData->is_main());

        return true;
    }

    /*flatbuffers::Offset<void> FlatBufferSerializer::serializeMeshRendererComponent(const MeshRendererComponent& component)
    {
        Log::debug("FlatBufferSerialize: Serializing MeshRendererComponent");

        auto meshPath = m_builder.CreateString(component.getMeshPath());
        auto materialPath = m_builder.CreateString(component.getMaterialPath());

        auto meshRendererComponent = CreateMeshRendererComponentData(
            m_builder,
            meshPath,
            materialPath,
            component.getCastShadows(),
            component.getReceiveShadows(),
            component.isEnabled()
        );

        return meshRendererComponent.Union();
    }

    bool FlatBufferSerializer::deserializeMeshRendererComponent(const void* bufferData, MeshRendererComponent& component)
    {
        auto meshRendererData = GetMeshRendererComponentData(bufferData);
        if (!meshRendererData)
        {
            Log::error("FlatBufferSerializer: Null buffer data for MeshRendererComponent deserialization");
            return false;
        }

        Log::debug("FlatBufferSerializer: Deserializing MeshRendererComponent");

        if (meshRendererData->mesh_path())
        {
            component.setMeshPath(meshRendererData->mesh_path()->str());
        }

        if (meshRendererData->material_path())
        {
            component.setMaterialPath(meshRendererData->material_path()->str());
        }

        component.setCastShadows(meshRendererData->cast_shadows());
        component.setReceiveShadows(meshRendererData->receive_shadows());
        component.setEnabled(meshRendererData->enabled());

        return true;
    }

    flatbuffers::Offset<void> FlatBufferSerializer::serializeLightComponent(const LightComponent& component)
    {
        Log::debug("FlatBufferSerialize: Serializing LightComponent");

        auto color = Vec3(
            component.getColor().x,
            component.getColor().y,
            component.getColor().z
        );

        auto lightComponent = CreateLightComponentData(
            m_builder,
            static_cast<LightType>(component.getLightType()),
            &color,
            component.getIntensity(),
            component.getRange(),
            component.getInnerAngle(),
            component.getOuterAngle(),
            component.getCastShadows(),
            component.getShadowBias(),
            component.getShadowResolution()
        );

        return lightComponent.Union();
    }

    bool FlatBufferSerializer::deserializeLightComponent(const void* bufferData, LightComponent& component)
    {
        auto lightData = GetLightComponentData(bufferData);
        if (!lightData)
        {
            Log::error("FlatBufferSerializer: Null buffer data for LightComponent deserialization");
            return false;
        }

        Log::debug("FlatBufferSerializer: Deserializing LightComponent");

        component.setLightType(static_cast<LightComponent::LightType>(lightData->light_type()));

        if (auto color = lightData->color())
        {
            component.setColor(glm::vec3(color->x(), color->y(), color->z()));
        }

        component.setIntensity(lightData->intensity());
        component.setRange(lightData->range());
        component.setInnerAngle(lightData->inner_angle());
        component.setOuterAngle(lightData->outer_angle());
        component.setCastShadows(lightData->cast_shadows());
        component.setShadowBias(lightData->shadow_bias());
        component.setShadowResolution(lightData->shadow_resolution());

        return true;
    }

    flatbuffers::Offset<void> FlatBufferSerializer::serializeRigidBodyComponent(const RigidBodyComponent& component)
    {
        Log::debug("FlatBufferSerialize: Serializing RigidBodyComponent");

        auto linearVelocity = Vec3(
            component.getLinearVelocity().x,
            component.getLinearVelocity().y,
            component.getLinearVelocity().z
        );

        auto angularVelocity = Vec3(
            component.getAngularVelocity().x,
            component.getAngularVelocity().y,
            component.getAngularVelocity().z
        );

        auto rigidBodyComponent = CreateRigidBodyComponentData(
            m_builder,
            static_cast<BodyType>(component.getBodyType()),
            component.getMass(),
            component.getDrag(),
            component.getAngularDrag(),
            component.getUseGravity(),
            component.isKinematic(),
            &linearVelocity,
            &angularVelocity
        );

        return rigidBodyComponent.Union();
    }

    bool FlatBufferSerializer::deserializeRigidBodyComponent(const void* bufferData, RigidBodyComponent& component)
    {
        auto rigidBodyData = GetRigidBodyComponentData(bufferData);
        if (!rigidBodyData)
        {
            Log::error("FlatBufferSerializer: Null buffer data for RigidBodyComponent deserialization");
            return false;
        }

        Log::debug("FlatBufferSerializer: Deserializing RigidBodyComponent");

        component.setBodyType(static_cast<RigidBodyComponent::BodyType>(rigidBodyData->body_type()));
        component.setMass(rigidBodyData->mass());
        component.setDrag(rigidBodyData->drag());
        component.setAngularDrag(rigidBodyData->angular_drag());
        component.setUseGravity(rigidBodyData->use_gravity());
        component.setKinematic(rigidBodyData->is_kinematic());

        if (auto linVel = rigidBodyData->linear_velocity())
        {
            component.setLinearVelocity(glm::vec3(linVel->x(), linVel->y(), linVel->z()));
        }

        if (auto angVel = rigidBodyData->angular_velocity())
        {
            component.setAngularVelocity(glm::vec3(angVel->x(), angVel->y(), angVel->z()));
        }

        return true;
    }

    flatbuffers::Offset<void> FlatBufferSerializer::serializeColliderComponent(const ColliderComponent& component)
    {
        Log::debug("FlatBufferSerialize: Serializing ColliderComponent");

        auto size = Vec3(
            component.getSize().x,
            component.getSize().y,
            component.getSize().z
        );

        auto materialName = m_builder.CreateString(component.getMaterialName());
        auto meshPath = m_builder.CreateString(component.getMeshPath());

        auto colliderComponent = CreateColliderComponentData(
            m_builder,
            static_cast<ColliderType>(component.getColliderType()),
            component.isTrigger(),
            &size,
            component.getRadius(),
            component.getHeight(),
            materialName,
            meshPath
        );

        return colliderComponent.Union();
    }

    bool FlatBufferSerializer::deserializeColliderComponent(const void* bufferData, ColliderComponent& component)
    {
        auto colliderData = GetColliderComponentData(bufferData);
        if (!colliderData)
        {
            Log::error("FlatBufferSerializer: Null buffer data for ColliderComponent deserialization");
            return false;
        }

        Log::debug("FlatBufferSerializer: Deserializing ColliderComponent");

        component.setColliderType(static_cast<ColliderComponent::ColliderType>(colliderData->collider_type()));
        component.setTrigger(colliderData->is_trigger());

        if (auto size = colliderData->size())
        {
            component.setSize(glm::vec3(size->x(), size->y(), size->z()));
        }

        component.setRadius(colliderData->radius());
        component.setHeight(colliderData->height());

        if (colliderData->material_name())
        {
            component.setMaterialName(colliderData->material_name()->str());
        }

        if (colliderData->mesh_path())
        {
            component.setMeshPath(colliderData->mesh_path()->str());
        }

        return true;
    }

    flatbuffers::Offset<void> FlatBufferSerializer::serializeAudioSourceComponent(const AudioSourceComponent& component)
    {
        Log::debug("FlatBufferSerialize: Serializing AudioSourceComponent");

        auto audioClipPath = m_builder.CreateString(component.getAudioClipPath());

        auto audioSourceComponent = CreateAudioSourceComponentData(
            m_builder,
            audioClipPath,
            component.getVolume(),
            component.getPitch(),
            component.isLooping(),
            component.getSpatialBlend(),
            component.getMinDistance(),
            component.getMaxDistance(),
            component.isPlaying()
        );

        return audioSourceComponent.Union();
    }

    bool FlatBufferSerializer::deserializeAudioSourceComponent(const void* bufferData, AudioSourceComponent& component)
    {
        auto audioData = GetAudioSourceComponentData(bufferData);
        if (!audioData)
        {
            Log::error("FlatBufferSerializer: Null buffer data for AudioSourceComponent deserialization");
            return false;
        }

        Log::debug("FlatBufferSerializer: Deserializing AudioSourceComponent");

        if (audioData->audio_clip_path())
        {
            component.setAudioClipPath(audioData->audio_clip_path()->str());
        }

        component.setVolume(audioData->volume());
        component.setPitch(audioData->pitch());
        component.setLoop(audioData->loop());
        component.setSpatialBlend(audioData->spatial_blend());
        component.setMinDistance(audioData->min_distance());
        component.setMaxDistance(audioData->max_distance());

        if (audioData->playing())
        {
            component.play();
        }
        else
        {
            component.stop();
        }

        return true;
    }

    flatbuffers::Offset<void> FlatBufferSerializer::serializeParticleSystemComponent(const ParticleSystemComponent& component)
    {
        Log::debug("FlatBufferSerialize: Serializing ParticleSystemComponent");

        auto startColor = Vec3(
            component.getStartColor().x,
            component.getStartColor().y,
            component.getStartColor().z
        );

        auto texturePath = m_builder.CreateString(component.getTexturePath());

        auto particleSystemComponent = CreateParticleSystemComponentData(
            m_builder,
            component.getMaxParticles(),
            component.getEmissionRate(),
            component.getLifetime(),
            component.getStartSpeed(),
            component.getStartSize(),
            &startColor,
            component.getGravityModifier(),
            component.getSimulationSpace() == ParticleSystemComponent::SimulationSpace::World,
            texturePath
        );

        return particleSystemComponent.Union();
    }

    bool FlatBufferSerializer::deserializeParticleSystemComponent(const void* bufferData, ParticleSystemComponent& component)
    {
        auto particleData = GetParticleSystemComponentData(bufferData);
        if (!particleData)
        {
            Log::error("FlatBufferSerializer: Null buffer data for ParticleSystemComponent deserialization");
            return false;
        }

        Log::debug("FlatBufferSerializer: Deserializing ParticleSystemComponent");

        component.setMaxParticles(particleData->max_particles());
        component.setEmissionRate(particleData->emission_rate());
        component.setLifetime(particleData->lifetime());
        component.setStartSpeed(particleData->start_speed());
        component.setStartSize(particleData->start_size());

        if (auto color = particleData->start_color())
        {
            component.setStartColor(glm::vec3(color->x(), color->y(), color->z()));
        }

        component.setGravityModifier(particleData->gravity_modifier());
        component.setSimulationSpace(particleData->simulation_space() ?
                                     ParticleSystemComponent::SimulationSpace::World :
                                     ParticleSystemComponent::SimulationSpace::Local);

        if (particleData->texture_path())
        {
            component.setTexturePath(particleData->texture_path()->str());
        }

        return true;
    }*/

    /*flatbuffers::Offset<void> FlatBufferSerializer::serializeNavigationAgentComponent(const NavigationAgentComponent& component)
    {
        return flatbuffers::Offset<void>();
    }

    bool FlatBufferSerializer::deserializeNavigationAgentComponent(const void* bufferData, NavigationAgentComponent& component)
    {
        return false;
    }

    flatbuffers::Offset<void> FlatBufferSerializer::serializeAnimatorComponent(const AnimatorComponent& component)
    {
        return flatbuffers::Offset<void>();
    }

    bool FlatBufferSerializer::deserializeAnimatorComponent(const void* bufferData, AnimatorComponent& component)
    {
        return false;
    }*/

} // namespace PixelCraft::ECS