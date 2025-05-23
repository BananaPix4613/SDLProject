// entity.fbs
// automatically generated by the FlatBuffers compiler, do not modify


#ifndef ENTITY_GENERATED_H
#define ENTITY_GENERATED_H

#include "flatbuffers/flatbuffers.h"

namespace PixelCraft
{
    namespace ECS
    {

        // Entity component types enumeration
        enum ComponentType : uint8_t
        {
            ComponentType_Transform = 0,
            ComponentType_MeshRenderer = 1,
            ComponentType_Camera = 2,
            ComponentType_Light = 3,
            ComponentType_RigidBody = 4,
            ComponentType_Collider = 5,
            ComponentType_Script = 6,
            ComponentType_MAX = ComponentType_Script
        };

        inline const char* const* EnumNamesComponentType()
        {
            static const char* const names[8] = {
                "Transform",
                "MeshRenderer",
                "Camera",
                "Light",
                "RigidBody",
                "Collider",
                "Script",
                nullptr
            };
            return names;
        }

        inline const char* EnumNameComponentType(ComponentType e)
        {
            if (e < ComponentType_Transform || e > ComponentType_Script) return "";
            const size_t index = static_cast<size_t>(e);
            return EnumNamesComponentType()[index];
        }

        // Component entry in an entity - type plus serialized data
        struct ComponentEntry;
        struct EntityData;

        struct ComponentEntry FLATBUFFERS_FINAL_CLASS : private flatbuffers::Table
        {
            typedef ComponentEntryBuilder Builder;
            enum FlatBuffersVTableOffset FLATBUFFERS_VTABLE_UNDERLYING_TYPE
            {
                VT_TYPE = 4,
                VT_DATA = 6
            };
            ComponentType type() const
            {
                return static_cast<ComponentType>(GetField<uint8_t>(VT_TYPE, 0));
            }
            const void* data() const
            {
                return GetPointer<const void*>(VT_DATA);
            }
            bool Verify(flatbuffers::Verifier& verifier) const
            {
                return VerifyTableStart(verifier) &&
                    VerifyField<uint8_t>(verifier, VT_TYPE, 1) &&
                    VerifyOffset(verifier, VT_DATA) &&
                    verifier.EndTable();
            }
        };

        struct ComponentEntryBuilder
        {
            typedef ComponentEntry Table;
            flatbuffers::FlatBufferBuilder& fbb_;
            flatbuffers::uoffset_t start_;
            void add_type(ComponentType type)
            {
                fbb_.AddElement<uint8_t>(ComponentEntry::VT_TYPE, static_cast<uint8_t>(type), 0);
            }
            void add_data(flatbuffers::Offset<void> data)
            {
                fbb_.AddOffset(ComponentEntry::VT_DATA, data);
            }
            flatbuffers::Offset<ComponentEntry> Finish()
            {
                const auto end = fbb_.EndTable(start_);
                auto o = flatbuffers::Offset<ComponentEntry>(end);
                return o;
            }
        };

        inline flatbuffers::Offset<ComponentEntry> CreateComponentEntry(
            flatbuffers::FlatBufferBuilder& _fbb,
            ComponentType type = ComponentType_Transform,
            flatbuffers::Offset<void> data = 0)
        {
            ComponentEntryBuilder builder_(_fbb);
            builder_.add_data(data);
            builder_.add_type(type);
            return builder_.Finish();
        }

        // Entity data structure for serialization
        struct EntityData FLATBUFFERS_FINAL_CLASS : private flatbuffers::Table
        {
            typedef EntityDataBuilder Builder;
            enum FlatBuffersVTableOffset FLATBUFFERS_VTABLE_UNDERLYING_TYPE
            {
                VT_ID = 4,
                VT_NAME = 6,
                VT_UUID = 8,
                VT_COMPONENTS = 10,
                VT_TAGS = 12
            };
            uint32_t id() const
            {
                return GetField<uint32_t>(VT_ID, 0);
            }
            const flatbuffers::String* name() const
            {
                return GetPointer<const flatbuffers::String*>(VT_NAME);
            }
            const flatbuffers::String* uuid() const
            {
                return GetPointer<const flatbuffers::String*>(VT_UUID);
            }
            const flatbuffers::Vector<flatbuffers::Offset<ComponentEntry>>* components() const
            {
                return GetPointer<const flatbuffers::Vector<flatbuffers::Offset<ComponentEntry>> *>(VT_COMPONENTS);
            }
            const flatbuffers::Vector<flatbuffers::Offset<flatbuffers::String>>* tags() const
            {
                return GetPointer<const flatbuffers::Vector<flatbuffers::Offset<flatbuffers::String>> *>(VT_TAGS);
            }
            bool Verify(flatbuffers::Verifier& verifier) const
            {
                return VerifyTableStart(verifier) &&
                    VerifyField<uint32_t>(verifier, VT_ID, 4) &&
                    VerifyOffset(verifier, VT_NAME) &&
                    verifier.VerifyString(name()) &&
                    VerifyOffset(verifier, VT_UUID) &&
                    verifier.VerifyString(uuid()) &&
                    VerifyOffset(verifier, VT_COMPONENTS) &&
                    verifier.VerifyVector(components()) &&
                    verifier.VerifyVectorOfTables(components()) &&
                    VerifyOffset(verifier, VT_TAGS) &&
                    verifier.VerifyVector(tags()) &&
                    verifier.VerifyVectorOfStrings(tags()) &&
                    verifier.EndTable();
            }
        };

        struct EntityDataBuilder
        {
            typedef EntityData Table;
            flatbuffers::FlatBufferBuilder& fbb_;
            flatbuffers::uoffset_t start_;
            void add_id(uint32_t id)
            {
                fbb_.AddElement<uint32_t>(EntityData::VT_ID, id, 0);
            }
            void add_name(flatbuffers::Offset<flatbuffers::String> name)
            {
                fbb_.AddOffset(EntityData::VT_NAME, name);
            }
            void add_uuid(flatbuffers::Offset<flatbuffers::String> uuid)
            {
                fbb_.AddOffset(EntityData::VT_UUID, uuid);
            }
            void add_components(flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<ComponentEntry>>> components)
            {
                fbb_.AddOffset(EntityData::VT_COMPONENTS, components);
            }
            void add_tags(flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<flatbuffers::String>>> tags)
            {
                fbb_.AddOffset(EntityData::VT_TAGS, tags);
            }
            flatbuffers::Offset<EntityData> Finish()
            {
                const auto end = fbb_.EndTable(start_);
                auto o = flatbuffers::Offset<EntityData>(end);
                return o;
            }
        };

        inline flatbuffers::Offset<EntityData> CreateEntityData(
            flatbuffers::FlatBufferBuilder& _fbb,
            uint32_t id = 0,
            flatbuffers::Offset<flatbuffers::String> name = 0,
            flatbuffers::Offset<flatbuffers::String> uuid = 0,
            flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<ComponentEntry>>> components = 0,
            flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<flatbuffers::String>>> tags = 0)
        {
            EntityDataBuilder builder_(_fbb);
            builder_.add_tags(tags);
            builder_.add_components(components);
            builder_.add_uuid(uuid);
            builder_.add_name(name);
            builder_.add_id(id);
            return builder_.Finish();
        }

        inline const EntityData* GetEntityData(const void* buf)
        {
            return flatbuffers::GetRoot<EntityData>(buf);
        }

        inline bool VerifyEntityDataBuffer(
            flatbuffers::Verifier& verifier)
        {
            return verifier.VerifyBuffer<EntityData>(nullptr);
        }

        inline bool VerifyEntityDataBufferWithIdentifier(
            flatbuffers::Verifier& verifier, const char* identifier)
        {
            return verifier.VerifyBuffer<EntityData>(identifier);
        }

        inline const char* EntityDataIdentifier()
        {
            return "ENTY";
        }

        inline bool EntityDataBufferHasIdentifier(const void* buf)
        {
            return flatbuffers::BufferHasIdentifier(
                buf, EntityDataIdentifier());
        }

    }  // namespace ECS
}  // namespace PixelCraft

#endif  // ENTITY_GENERATED_H