// automatically generated by the FlatBuffers compiler, do not modify


// @generated

use crate::common_types_generated::*;
use core::mem;
use core::cmp::Ordering;

extern crate flatbuffers;
use self::flatbuffers::{EndianScalar, Follow};

#[allow(unused_imports, dead_code)]
pub mod pixel_craft {

  use crate::common_types_generated::*;
  use core::mem;
  use core::cmp::Ordering;

  extern crate flatbuffers;
  use self::flatbuffers::{EndianScalar, Follow};
#[allow(unused_imports, dead_code)]
pub mod ecs {

  use crate::common_types_generated::*;
  use core::mem;
  use core::cmp::Ordering;

  extern crate flatbuffers;
  use self::flatbuffers::{EndianScalar, Follow};

#[deprecated(since = "2.0.0", note = "Use associated constants instead. This will no longer be generated in 2021.")]
pub const ENUM_MIN_COLLIDER_TYPE: i8 = 0;
#[deprecated(since = "2.0.0", note = "Use associated constants instead. This will no longer be generated in 2021.")]
pub const ENUM_MAX_COLLIDER_TYPE: i8 = 3;
#[deprecated(since = "2.0.0", note = "Use associated constants instead. This will no longer be generated in 2021.")]
#[allow(non_camel_case_types)]
pub const ENUM_VALUES_COLLIDER_TYPE: [ColliderType; 4] = [
  ColliderType::Box,
  ColliderType::Sphere,
  ColliderType::Capsule,
  ColliderType::Mesh,
];

#[derive(Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Hash, Default)]
#[repr(transparent)]
pub struct ColliderType(pub i8);
#[allow(non_upper_case_globals)]
impl ColliderType {
  pub const Box: Self = Self(0);
  pub const Sphere: Self = Self(1);
  pub const Capsule: Self = Self(2);
  pub const Mesh: Self = Self(3);

  pub const ENUM_MIN: i8 = 0;
  pub const ENUM_MAX: i8 = 3;
  pub const ENUM_VALUES: &'static [Self] = &[
    Self::Box,
    Self::Sphere,
    Self::Capsule,
    Self::Mesh,
  ];
  /// Returns the variant's name or "" if unknown.
  pub fn variant_name(self) -> Option<&'static str> {
    match self {
      Self::Box => Some("Box"),
      Self::Sphere => Some("Sphere"),
      Self::Capsule => Some("Capsule"),
      Self::Mesh => Some("Mesh"),
      _ => None,
    }
  }
}
impl core::fmt::Debug for ColliderType {
  fn fmt(&self, f: &mut core::fmt::Formatter) -> core::fmt::Result {
    if let Some(name) = self.variant_name() {
      f.write_str(name)
    } else {
      f.write_fmt(format_args!("<UNKNOWN {:?}>", self.0))
    }
  }
}
impl<'a> flatbuffers::Follow<'a> for ColliderType {
  type Inner = Self;
  #[inline]
  unsafe fn follow(buf: &'a [u8], loc: usize) -> Self::Inner {
    let b = flatbuffers::read_scalar_at::<i8>(buf, loc);
    Self(b)
  }
}

impl flatbuffers::Push for ColliderType {
    type Output = ColliderType;
    #[inline]
    unsafe fn push(&self, dst: &mut [u8], _written_len: usize) {
        flatbuffers::emplace_scalar::<i8>(dst, self.0);
    }
}

impl flatbuffers::EndianScalar for ColliderType {
  type Scalar = i8;
  #[inline]
  fn to_little_endian(self) -> i8 {
    self.0.to_le()
  }
  #[inline]
  #[allow(clippy::wrong_self_convention)]
  fn from_little_endian(v: i8) -> Self {
    let b = i8::from_le(v);
    Self(b)
  }
}

impl<'a> flatbuffers::Verifiable for ColliderType {
  #[inline]
  fn run_verifier(
    v: &mut flatbuffers::Verifier, pos: usize
  ) -> Result<(), flatbuffers::InvalidFlatbuffer> {
    use self::flatbuffers::Verifiable;
    i8::run_verifier(v, pos)
  }
}

impl flatbuffers::SimpleToVerifyInSlice for ColliderType {}
pub enum ColliderComponentDataOffset {}
#[derive(Copy, Clone, PartialEq)]

pub struct ColliderComponentData<'a> {
  pub _tab: flatbuffers::Table<'a>,
}

impl<'a> flatbuffers::Follow<'a> for ColliderComponentData<'a> {
  type Inner = ColliderComponentData<'a>;
  #[inline]
  unsafe fn follow(buf: &'a [u8], loc: usize) -> Self::Inner {
    Self { _tab: flatbuffers::Table::new(buf, loc) }
  }
}

impl<'a> ColliderComponentData<'a> {
  pub const VT_COLLIDER_TYPE: flatbuffers::VOffsetT = 4;
  pub const VT_IS_TRIGGER: flatbuffers::VOffsetT = 6;
  pub const VT_SIZE: flatbuffers::VOffsetT = 8;
  pub const VT_RADIUS: flatbuffers::VOffsetT = 10;
  pub const VT_HEIGHT: flatbuffers::VOffsetT = 12;
  pub const VT_MATERIAL_NAME: flatbuffers::VOffsetT = 14;
  pub const VT_MESH_PATH: flatbuffers::VOffsetT = 16;

  #[inline]
  pub unsafe fn init_from_table(table: flatbuffers::Table<'a>) -> Self {
    ColliderComponentData { _tab: table }
  }
  #[allow(unused_mut)]
  pub fn create<'bldr: 'args, 'args: 'mut_bldr, 'mut_bldr, A: flatbuffers::Allocator + 'bldr>(
    _fbb: &'mut_bldr mut flatbuffers::FlatBufferBuilder<'bldr, A>,
    args: &'args ColliderComponentDataArgs<'args>
  ) -> flatbuffers::WIPOffset<ColliderComponentData<'bldr>> {
    let mut builder = ColliderComponentDataBuilder::new(_fbb);
    if let Some(x) = args.mesh_path { builder.add_mesh_path(x); }
    if let Some(x) = args.material_name { builder.add_material_name(x); }
    builder.add_height(args.height);
    builder.add_radius(args.radius);
    if let Some(x) = args.size { builder.add_size(x); }
    builder.add_is_trigger(args.is_trigger);
    builder.add_collider_type(args.collider_type);
    builder.finish()
  }


  #[inline]
  pub fn collider_type(&self) -> ColliderType {
    // Safety:
    // Created from valid Table for this object
    // which contains a valid value in this slot
    unsafe { self._tab.get::<ColliderType>(ColliderComponentData::VT_COLLIDER_TYPE, Some(ColliderType::Box)).unwrap()}
  }
  #[inline]
  pub fn is_trigger(&self) -> bool {
    // Safety:
    // Created from valid Table for this object
    // which contains a valid value in this slot
    unsafe { self._tab.get::<bool>(ColliderComponentData::VT_IS_TRIGGER, Some(false)).unwrap()}
  }
  #[inline]
  pub fn size(&self) -> Option<&'a Vec3> {
    // Safety:
    // Created from valid Table for this object
    // which contains a valid value in this slot
    unsafe { self._tab.get::<Vec3>(ColliderComponentData::VT_SIZE, None)}
  }
  #[inline]
  pub fn radius(&self) -> f32 {
    // Safety:
    // Created from valid Table for this object
    // which contains a valid value in this slot
    unsafe { self._tab.get::<f32>(ColliderComponentData::VT_RADIUS, Some(0.0)).unwrap()}
  }
  #[inline]
  pub fn height(&self) -> f32 {
    // Safety:
    // Created from valid Table for this object
    // which contains a valid value in this slot
    unsafe { self._tab.get::<f32>(ColliderComponentData::VT_HEIGHT, Some(0.0)).unwrap()}
  }
  #[inline]
  pub fn material_name(&self) -> Option<&'a str> {
    // Safety:
    // Created from valid Table for this object
    // which contains a valid value in this slot
    unsafe { self._tab.get::<flatbuffers::ForwardsUOffset<&str>>(ColliderComponentData::VT_MATERIAL_NAME, None)}
  }
  #[inline]
  pub fn mesh_path(&self) -> Option<&'a str> {
    // Safety:
    // Created from valid Table for this object
    // which contains a valid value in this slot
    unsafe { self._tab.get::<flatbuffers::ForwardsUOffset<&str>>(ColliderComponentData::VT_MESH_PATH, None)}
  }
}

impl flatbuffers::Verifiable for ColliderComponentData<'_> {
  #[inline]
  fn run_verifier(
    v: &mut flatbuffers::Verifier, pos: usize
  ) -> Result<(), flatbuffers::InvalidFlatbuffer> {
    use self::flatbuffers::Verifiable;
    v.visit_table(pos)?
     .visit_field::<ColliderType>("collider_type", Self::VT_COLLIDER_TYPE, false)?
     .visit_field::<bool>("is_trigger", Self::VT_IS_TRIGGER, false)?
     .visit_field::<Vec3>("size", Self::VT_SIZE, false)?
     .visit_field::<f32>("radius", Self::VT_RADIUS, false)?
     .visit_field::<f32>("height", Self::VT_HEIGHT, false)?
     .visit_field::<flatbuffers::ForwardsUOffset<&str>>("material_name", Self::VT_MATERIAL_NAME, false)?
     .visit_field::<flatbuffers::ForwardsUOffset<&str>>("mesh_path", Self::VT_MESH_PATH, false)?
     .finish();
    Ok(())
  }
}
pub struct ColliderComponentDataArgs<'a> {
    pub collider_type: ColliderType,
    pub is_trigger: bool,
    pub size: Option<&'a Vec3>,
    pub radius: f32,
    pub height: f32,
    pub material_name: Option<flatbuffers::WIPOffset<&'a str>>,
    pub mesh_path: Option<flatbuffers::WIPOffset<&'a str>>,
}
impl<'a> Default for ColliderComponentDataArgs<'a> {
  #[inline]
  fn default() -> Self {
    ColliderComponentDataArgs {
      collider_type: ColliderType::Box,
      is_trigger: false,
      size: None,
      radius: 0.0,
      height: 0.0,
      material_name: None,
      mesh_path: None,
    }
  }
}

pub struct ColliderComponentDataBuilder<'a: 'b, 'b, A: flatbuffers::Allocator + 'a> {
  fbb_: &'b mut flatbuffers::FlatBufferBuilder<'a, A>,
  start_: flatbuffers::WIPOffset<flatbuffers::TableUnfinishedWIPOffset>,
}
impl<'a: 'b, 'b, A: flatbuffers::Allocator + 'a> ColliderComponentDataBuilder<'a, 'b, A> {
  #[inline]
  pub fn add_collider_type(&mut self, collider_type: ColliderType) {
    self.fbb_.push_slot::<ColliderType>(ColliderComponentData::VT_COLLIDER_TYPE, collider_type, ColliderType::Box);
  }
  #[inline]
  pub fn add_is_trigger(&mut self, is_trigger: bool) {
    self.fbb_.push_slot::<bool>(ColliderComponentData::VT_IS_TRIGGER, is_trigger, false);
  }
  #[inline]
  pub fn add_size(&mut self, size: &Vec3) {
    self.fbb_.push_slot_always::<&Vec3>(ColliderComponentData::VT_SIZE, size);
  }
  #[inline]
  pub fn add_radius(&mut self, radius: f32) {
    self.fbb_.push_slot::<f32>(ColliderComponentData::VT_RADIUS, radius, 0.0);
  }
  #[inline]
  pub fn add_height(&mut self, height: f32) {
    self.fbb_.push_slot::<f32>(ColliderComponentData::VT_HEIGHT, height, 0.0);
  }
  #[inline]
  pub fn add_material_name(&mut self, material_name: flatbuffers::WIPOffset<&'b  str>) {
    self.fbb_.push_slot_always::<flatbuffers::WIPOffset<_>>(ColliderComponentData::VT_MATERIAL_NAME, material_name);
  }
  #[inline]
  pub fn add_mesh_path(&mut self, mesh_path: flatbuffers::WIPOffset<&'b  str>) {
    self.fbb_.push_slot_always::<flatbuffers::WIPOffset<_>>(ColliderComponentData::VT_MESH_PATH, mesh_path);
  }
  #[inline]
  pub fn new(_fbb: &'b mut flatbuffers::FlatBufferBuilder<'a, A>) -> ColliderComponentDataBuilder<'a, 'b, A> {
    let start = _fbb.start_table();
    ColliderComponentDataBuilder {
      fbb_: _fbb,
      start_: start,
    }
  }
  #[inline]
  pub fn finish(self) -> flatbuffers::WIPOffset<ColliderComponentData<'a>> {
    let o = self.fbb_.end_table(self.start_);
    flatbuffers::WIPOffset::new(o.value())
  }
}

impl core::fmt::Debug for ColliderComponentData<'_> {
  fn fmt(&self, f: &mut core::fmt::Formatter<'_>) -> core::fmt::Result {
    let mut ds = f.debug_struct("ColliderComponentData");
      ds.field("collider_type", &self.collider_type());
      ds.field("is_trigger", &self.is_trigger());
      ds.field("size", &self.size());
      ds.field("radius", &self.radius());
      ds.field("height", &self.height());
      ds.field("material_name", &self.material_name());
      ds.field("mesh_path", &self.mesh_path());
      ds.finish()
  }
}
#[inline]
/// Verifies that a buffer of bytes contains a `ColliderComponentData`
/// and returns it.
/// Note that verification is still experimental and may not
/// catch every error, or be maximally performant. For the
/// previous, unchecked, behavior use
/// `root_as_collider_component_data_unchecked`.
pub fn root_as_collider_component_data(buf: &[u8]) -> Result<ColliderComponentData, flatbuffers::InvalidFlatbuffer> {
  flatbuffers::root::<ColliderComponentData>(buf)
}
#[inline]
/// Verifies that a buffer of bytes contains a size prefixed
/// `ColliderComponentData` and returns it.
/// Note that verification is still experimental and may not
/// catch every error, or be maximally performant. For the
/// previous, unchecked, behavior use
/// `size_prefixed_root_as_collider_component_data_unchecked`.
pub fn size_prefixed_root_as_collider_component_data(buf: &[u8]) -> Result<ColliderComponentData, flatbuffers::InvalidFlatbuffer> {
  flatbuffers::size_prefixed_root::<ColliderComponentData>(buf)
}
#[inline]
/// Verifies, with the given options, that a buffer of bytes
/// contains a `ColliderComponentData` and returns it.
/// Note that verification is still experimental and may not
/// catch every error, or be maximally performant. For the
/// previous, unchecked, behavior use
/// `root_as_collider_component_data_unchecked`.
pub fn root_as_collider_component_data_with_opts<'b, 'o>(
  opts: &'o flatbuffers::VerifierOptions,
  buf: &'b [u8],
) -> Result<ColliderComponentData<'b>, flatbuffers::InvalidFlatbuffer> {
  flatbuffers::root_with_opts::<ColliderComponentData<'b>>(opts, buf)
}
#[inline]
/// Verifies, with the given verifier options, that a buffer of
/// bytes contains a size prefixed `ColliderComponentData` and returns
/// it. Note that verification is still experimental and may not
/// catch every error, or be maximally performant. For the
/// previous, unchecked, behavior use
/// `root_as_collider_component_data_unchecked`.
pub fn size_prefixed_root_as_collider_component_data_with_opts<'b, 'o>(
  opts: &'o flatbuffers::VerifierOptions,
  buf: &'b [u8],
) -> Result<ColliderComponentData<'b>, flatbuffers::InvalidFlatbuffer> {
  flatbuffers::size_prefixed_root_with_opts::<ColliderComponentData<'b>>(opts, buf)
}
#[inline]
/// Assumes, without verification, that a buffer of bytes contains a ColliderComponentData and returns it.
/// # Safety
/// Callers must trust the given bytes do indeed contain a valid `ColliderComponentData`.
pub unsafe fn root_as_collider_component_data_unchecked(buf: &[u8]) -> ColliderComponentData {
  flatbuffers::root_unchecked::<ColliderComponentData>(buf)
}
#[inline]
/// Assumes, without verification, that a buffer of bytes contains a size prefixed ColliderComponentData and returns it.
/// # Safety
/// Callers must trust the given bytes do indeed contain a valid size prefixed `ColliderComponentData`.
pub unsafe fn size_prefixed_root_as_collider_component_data_unchecked(buf: &[u8]) -> ColliderComponentData {
  flatbuffers::size_prefixed_root_unchecked::<ColliderComponentData>(buf)
}
pub const COLLIDER_COMPONENT_DATA_IDENTIFIER: &str = "CLDR";

#[inline]
pub fn collider_component_data_buffer_has_identifier(buf: &[u8]) -> bool {
  flatbuffers::buffer_has_identifier(buf, COLLIDER_COMPONENT_DATA_IDENTIFIER, false)
}

#[inline]
pub fn collider_component_data_size_prefixed_buffer_has_identifier(buf: &[u8]) -> bool {
  flatbuffers::buffer_has_identifier(buf, COLLIDER_COMPONENT_DATA_IDENTIFIER, true)
}

#[inline]
pub fn finish_collider_component_data_buffer<'a, 'b, A: flatbuffers::Allocator + 'a>(
    fbb: &'b mut flatbuffers::FlatBufferBuilder<'a, A>,
    root: flatbuffers::WIPOffset<ColliderComponentData<'a>>) {
  fbb.finish(root, Some(COLLIDER_COMPONENT_DATA_IDENTIFIER));
}

#[inline]
pub fn finish_size_prefixed_collider_component_data_buffer<'a, 'b, A: flatbuffers::Allocator + 'a>(fbb: &'b mut flatbuffers::FlatBufferBuilder<'a, A>, root: flatbuffers::WIPOffset<ColliderComponentData<'a>>) {
  fbb.finish_size_prefixed(root, Some(COLLIDER_COMPONENT_DATA_IDENTIFIER));
}
}  // pub mod ECS
}  // pub mod PixelCraft

