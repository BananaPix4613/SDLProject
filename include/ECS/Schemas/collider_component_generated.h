// automatically generated by the FlatBuffers compiler, do not modify


#ifndef FLATBUFFERS_GENERATED_COLLIDERCOMPONENT_PIXELCRAFT_ECS_H_
#define FLATBUFFERS_GENERATED_COLLIDERCOMPONENT_PIXELCRAFT_ECS_H_

#include "flatbuffers/flatbuffers.h"

// Ensure the included flatbuffers.h is the same version as when this file was
// generated, otherwise it may not be compatible.
static_assert(FLATBUFFERS_VERSION_MAJOR == 25 &&
              FLATBUFFERS_VERSION_MINOR == 2 &&
              FLATBUFFERS_VERSION_REVISION == 10,
             "Non-compatible flatbuffers version included");

#include "common_types_generated.h"

namespace PixelCraft {
namespace ECS {

struct ColliderComponentData;
struct ColliderComponentDataBuilder;

enum ColliderType : int8_t {
  ColliderType_Box = 0,
  ColliderType_Sphere = 1,
  ColliderType_Capsule = 2,
  ColliderType_Mesh = 3,
  ColliderType_MIN = ColliderType_Box,
  ColliderType_MAX = ColliderType_Mesh
};

inline const ColliderType (&EnumValuesColliderType())[4] {
  static const ColliderType values[] = {
    ColliderType_Box,
    ColliderType_Sphere,
    ColliderType_Capsule,
    ColliderType_Mesh
  };
  return values;
}

inline const char * const *EnumNamesColliderType() {
  static const char * const names[5] = {
    "Box",
    "Sphere",
    "Capsule",
    "Mesh",
    nullptr
  };
  return names;
}

inline const char *EnumNameColliderType(ColliderType e) {
  if (::flatbuffers::IsOutRange(e, ColliderType_Box, ColliderType_Mesh)) return "";
  const size_t index = static_cast<size_t>(e);
  return EnumNamesColliderType()[index];
}

struct ColliderComponentData FLATBUFFERS_FINAL_CLASS : private ::flatbuffers::Table {
  typedef ColliderComponentDataBuilder Builder;
  enum FlatBuffersVTableOffset FLATBUFFERS_VTABLE_UNDERLYING_TYPE {
    VT_COLLIDER_TYPE = 4,
    VT_IS_TRIGGER = 6,
    VT_SIZE = 8,
    VT_RADIUS = 10,
    VT_HEIGHT = 12,
    VT_MATERIAL_NAME = 14,
    VT_MESH_PATH = 16
  };
  PixelCraft::ECS::ColliderType collider_type() const {
    return static_cast<PixelCraft::ECS::ColliderType>(GetField<int8_t>(VT_COLLIDER_TYPE, 0));
  }
  bool is_trigger() const {
    return GetField<uint8_t>(VT_IS_TRIGGER, 0) != 0;
  }
  const PixelCraft::ECS::Vec3 *size() const {
    return GetStruct<const PixelCraft::ECS::Vec3 *>(VT_SIZE);
  }
  float radius() const {
    return GetField<float>(VT_RADIUS, 0.0f);
  }
  float height() const {
    return GetField<float>(VT_HEIGHT, 0.0f);
  }
  const ::flatbuffers::String *material_name() const {
    return GetPointer<const ::flatbuffers::String *>(VT_MATERIAL_NAME);
  }
  const ::flatbuffers::String *mesh_path() const {
    return GetPointer<const ::flatbuffers::String *>(VT_MESH_PATH);
  }
  bool Verify(::flatbuffers::Verifier &verifier) const {
    return VerifyTableStart(verifier) &&
           VerifyField<int8_t>(verifier, VT_COLLIDER_TYPE, 1) &&
           VerifyField<uint8_t>(verifier, VT_IS_TRIGGER, 1) &&
           VerifyField<PixelCraft::ECS::Vec3>(verifier, VT_SIZE, 4) &&
           VerifyField<float>(verifier, VT_RADIUS, 4) &&
           VerifyField<float>(verifier, VT_HEIGHT, 4) &&
           VerifyOffset(verifier, VT_MATERIAL_NAME) &&
           verifier.VerifyString(material_name()) &&
           VerifyOffset(verifier, VT_MESH_PATH) &&
           verifier.VerifyString(mesh_path()) &&
           verifier.EndTable();
  }
};

struct ColliderComponentDataBuilder {
  typedef ColliderComponentData Table;
  ::flatbuffers::FlatBufferBuilder &fbb_;
  ::flatbuffers::uoffset_t start_;
  void add_collider_type(PixelCraft::ECS::ColliderType collider_type) {
    fbb_.AddElement<int8_t>(ColliderComponentData::VT_COLLIDER_TYPE, static_cast<int8_t>(collider_type), 0);
  }
  void add_is_trigger(bool is_trigger) {
    fbb_.AddElement<uint8_t>(ColliderComponentData::VT_IS_TRIGGER, static_cast<uint8_t>(is_trigger), 0);
  }
  void add_size(const PixelCraft::ECS::Vec3 *size) {
    fbb_.AddStruct(ColliderComponentData::VT_SIZE, size);
  }
  void add_radius(float radius) {
    fbb_.AddElement<float>(ColliderComponentData::VT_RADIUS, radius, 0.0f);
  }
  void add_height(float height) {
    fbb_.AddElement<float>(ColliderComponentData::VT_HEIGHT, height, 0.0f);
  }
  void add_material_name(::flatbuffers::Offset<::flatbuffers::String> material_name) {
    fbb_.AddOffset(ColliderComponentData::VT_MATERIAL_NAME, material_name);
  }
  void add_mesh_path(::flatbuffers::Offset<::flatbuffers::String> mesh_path) {
    fbb_.AddOffset(ColliderComponentData::VT_MESH_PATH, mesh_path);
  }
  explicit ColliderComponentDataBuilder(::flatbuffers::FlatBufferBuilder &_fbb)
        : fbb_(_fbb) {
    start_ = fbb_.StartTable();
  }
  ::flatbuffers::Offset<ColliderComponentData> Finish() {
    const auto end = fbb_.EndTable(start_);
    auto o = ::flatbuffers::Offset<ColliderComponentData>(end);
    return o;
  }
};

inline ::flatbuffers::Offset<ColliderComponentData> CreateColliderComponentData(
    ::flatbuffers::FlatBufferBuilder &_fbb,
    PixelCraft::ECS::ColliderType collider_type = PixelCraft::ECS::ColliderType_Box,
    bool is_trigger = false,
    const PixelCraft::ECS::Vec3 *size = nullptr,
    float radius = 0.0f,
    float height = 0.0f,
    ::flatbuffers::Offset<::flatbuffers::String> material_name = 0,
    ::flatbuffers::Offset<::flatbuffers::String> mesh_path = 0) {
  ColliderComponentDataBuilder builder_(_fbb);
  builder_.add_mesh_path(mesh_path);
  builder_.add_material_name(material_name);
  builder_.add_height(height);
  builder_.add_radius(radius);
  builder_.add_size(size);
  builder_.add_is_trigger(is_trigger);
  builder_.add_collider_type(collider_type);
  return builder_.Finish();
}

inline ::flatbuffers::Offset<ColliderComponentData> CreateColliderComponentDataDirect(
    ::flatbuffers::FlatBufferBuilder &_fbb,
    PixelCraft::ECS::ColliderType collider_type = PixelCraft::ECS::ColliderType_Box,
    bool is_trigger = false,
    const PixelCraft::ECS::Vec3 *size = nullptr,
    float radius = 0.0f,
    float height = 0.0f,
    const char *material_name = nullptr,
    const char *mesh_path = nullptr) {
  auto material_name__ = material_name ? _fbb.CreateString(material_name) : 0;
  auto mesh_path__ = mesh_path ? _fbb.CreateString(mesh_path) : 0;
  return PixelCraft::ECS::CreateColliderComponentData(
      _fbb,
      collider_type,
      is_trigger,
      size,
      radius,
      height,
      material_name__,
      mesh_path__);
}

inline const PixelCraft::ECS::ColliderComponentData *GetColliderComponentData(const void *buf) {
  return ::flatbuffers::GetRoot<PixelCraft::ECS::ColliderComponentData>(buf);
}

inline const PixelCraft::ECS::ColliderComponentData *GetSizePrefixedColliderComponentData(const void *buf) {
  return ::flatbuffers::GetSizePrefixedRoot<PixelCraft::ECS::ColliderComponentData>(buf);
}

inline const char *ColliderComponentDataIdentifier() {
  return "CLDR";
}

inline bool ColliderComponentDataBufferHasIdentifier(const void *buf) {
  return ::flatbuffers::BufferHasIdentifier(
      buf, ColliderComponentDataIdentifier());
}

inline bool SizePrefixedColliderComponentDataBufferHasIdentifier(const void *buf) {
  return ::flatbuffers::BufferHasIdentifier(
      buf, ColliderComponentDataIdentifier(), true);
}

inline bool VerifyColliderComponentDataBuffer(
    ::flatbuffers::Verifier &verifier) {
  return verifier.VerifyBuffer<PixelCraft::ECS::ColliderComponentData>(ColliderComponentDataIdentifier());
}

inline bool VerifySizePrefixedColliderComponentDataBuffer(
    ::flatbuffers::Verifier &verifier) {
  return verifier.VerifySizePrefixedBuffer<PixelCraft::ECS::ColliderComponentData>(ColliderComponentDataIdentifier());
}

inline void FinishColliderComponentDataBuffer(
    ::flatbuffers::FlatBufferBuilder &fbb,
    ::flatbuffers::Offset<PixelCraft::ECS::ColliderComponentData> root) {
  fbb.Finish(root, ColliderComponentDataIdentifier());
}

inline void FinishSizePrefixedColliderComponentDataBuffer(
    ::flatbuffers::FlatBufferBuilder &fbb,
    ::flatbuffers::Offset<PixelCraft::ECS::ColliderComponentData> root) {
  fbb.FinishSizePrefixed(root, ColliderComponentDataIdentifier());
}

}  // namespace ECS
}  // namespace PixelCraft

#endif  // FLATBUFFERS_GENERATED_COLLIDERCOMPONENT_PIXELCRAFT_ECS_H_
