#pragma once

#include "agx.h"

#define AGX_CODE_HEAP_BLOCK_HEADER              0x40u
#define AGX_USC_GLOBAL_CONFIGURATION_INIT_BYTES 0x300u
#define AGX_END_OF_TILE_SECOND_PROGRAM_BYTES    0x861cu
#define AGX_END_OF_TILE_ENTRY_CLEAR             0x0000u
#define AGX_END_OF_TILE_ENTRY_PASS_PROBE        0x0080u
#define AGX_END_OF_TILE_SECOND_RANGE_COUNT      4u
#define AGX_POOL_LAYOUT_RESERVED                8

#define AGX_POOL_ARGUMENTS_GEOMETRY_OFFSET 0x13e0u

#define AGX_POOL_ARGUMENTS_GEOMETRY_BITS             43u
#define AGX_POOL_NOWHERE                             0xffffffffu
#define AGX_PIPELINE_DRIVER_VERTEX_STAGE_POOL_OFFSET 0x56c0u
#define AGX_POOL_VERTEX_STAGE_ENTRY_PREFIX           0x3cu
#define AGX_POOL_VERTEX_STAGE_PROGRAM_BYTES          0x1a2u
#define AGX_POOL_VERTEX_STAGE_PROGRAM_ENCODED        0x1a4u
#define AGX_POOL_VERTEX_STAGE_ENTRY_LOWEST           0x00800u
#define AGX_POOL_VERTEX_STAGE_ENTRY_ALIGNMENT        0x40u
#define AGX_POOL_VERTEX_STAGE_ENTRY_HIGHEST          0x056c0u
#define AGX_SHADER_POOL_STATE_LOADER_OFFSET          0x05540u

#define AGX_PIPELINE_PROGRAMS_VERTEX_CALLER_OFFSET    0x100u
#define AGX_PIPELINE_PROGRAMS_FRAGMENT_CALLER_OFFSET  0x2c0u
#define AGX_PIPELINE_PROGRAMS_CALLER_ALIGN            4u
#define AGX_TILE_PROGRAMS_VERTEX_SHADER_CALLER_OFFSET 0x640u
#define AGX_BUFFER_COPY_OPERAND_SLOT                  0x14c0u
#define AGX_BUFFER_COPY_PROGRAM_BYTES                 0x1c8u
#define AGX_TILE_START_TAIL_BYTES                     0x80u
#define AGX_POOL_TILE_BLOCK_BYTES                     124u
#define AGX_POOL_TILE_BLOCK_HEAD_BYTES                48u
#define AGX_SHADER_POOL_VERTEX_STAGE_TAIL_OFFSET      0x06100u

#define AGX_SHADER_POOL_STATE_ARM_OFFSET             0x05ab4u
#define AGX_SHADER_POOL_STATE_ARM_TWO_OFFSET         0x06042u
#define AGX_SHADER_POOL_STATE_ARM_THREE_OFFSET       0x05b6au
#define AGX_SHADER_POOL_STATE_ARM_FOUR_OFFSET        0x05e82u
#define AGX_SHADER_POOL_STATE_STORE_OFFSET           0x05908u
#define AGX_SHADER_POOL_STATE_ARM_FIVE_OFFSET        0x0609eu
#define AGX_SHADER_POOL_STATE_ARM_EIGHT_OFFSET       0x05fbcu
#define AGX_SHADER_POOL_STATE_ARM_NINE_OFFSET        0x05d0au
#define AGX_SHADER_POOL_STATE_ARM_SIX_OFFSET         0x059fcu
#define AGX_SHADER_POOL_STATE_ARM_SEVEN_OFFSET       0x05ba6u
#define AGX_SHADER_POOL_STATE_ARM_TEN_OFFSET         0x05a30u
#define AGX_SHADER_POOL_STATE_ARM_ELEVEN_OFFSET      0x05afeu
#define AGX_SHADER_POOL_STATE_CHAIN_LINK_WIDE_OFFSET 0x0607cu
#define AGX_SHADER_POOL_STATE_ARM_TWELVE_OFFSET      0x05ed8u
#define AGX_SHADER_POOL_STATE_CHAIN_START_OFFSET     0x05914u
#define AGX_SHADER_POOL_STATE_ARM_THIRTEEN_OFFSET    0x05d68u
#define AGX_SHADER_POOL_STATE_ARM_FOURTEEN_OFFSET    0x05bc2u
#define AGX_SHADER_POOL_STATE_CHAIN_LINK_OFFSET      0x06024u
#define AGX_SHADER_POOL_VERTEX_STAGE_TAIL_LOWEST     0x060e8u
#define AGX_SHADER_POOL_VERTEX_STAGE_TAIL_HIGHEST    0x06300u
#define AGX_SHADER_POOL_VERTEX_STAGE_TAIL_BYTES      0x200u
#define AGX_SHADER_POOL_UNIFORM_GAP_OFFSET           0x05500u
#define AGX_SHADER_POOL_TEXTURE_EPILOGUE_OFFSET      0x05826u
#define AGX_SHADER_POOL_TILE_BLOCK_OFFSET            0x0e800u

typedef enum Agx_Caller_Shape
{
  AGX_CALLER_SHAPE_MINIMAL = 0,
  AGX_CALLER_SHAPE_DRIVER = 1,
} Agx_Caller_Shape;

typedef enum Agx_Varying_Shade_Model
{
  AGX_VARYING_SHADE_MODEL_FLAT_VERTEX_0 = 0,
  AGX_VARYING_SHADE_MODEL_FLAT_VERTEX_2 = 2,
  AGX_VARYING_SHADE_MODEL_LINEAR = 3,
  AGX_VARYING_SHADE_MODEL_FLAT_VERTEX_1 = 6,
  AGX_VARYING_SHADE_MODEL_PERSPECTIVE = 7,
} Agx_Varying_Shade_Model;

typedef enum Agx_Coefficient_Source
{
  AGX_COEFFICIENT_SOURCE_VARYING = 0,
  AGX_COEFFICIENT_SOURCE_FRAGCOORD_Z = 1,
  AGX_COEFFICIENT_SOURCE_POINT_COORD = 2,
  AGX_COEFFICIENT_SOURCE_PRIMITIVE_ID = 3,
  AGX_COEFFICIENT_SOURCE_BARYCENTRIC = 5,
} Agx_Coefficient_Source;

typedef struct Agx_End_Of_Tile_Range
{
  uint32_t       offset;
  uint32_t       bytes;
  const uint8_t* code;
} Agx_End_Of_Tile_Range;

typedef struct Agx_Pool_Layout
{
  uint8_t* cpu;
  uint64_t gpu_va;
  uint32_t size;
  uint32_t cursor;
  struct
  {
    uint32_t at, bytes;
  } taken[AGX_POOL_LAYOUT_RESERVED];
  uint32_t taken_count;
  uint32_t vertex_stage_entry;
  uint32_t state_loader;
  uint32_t end_of_tile_second_program;
} Agx_Pool_Layout;

void
agx_pool_layout_init(Agx_Pool_Layout* layout, const Agx_Bo* pool, uint32_t first);

typedef struct Agx_Tile_Programs
{
  Agx_Bo   heap;
  uint64_t background;
  uint64_t store;
  uint64_t shader_pool;
  uint64_t tile_arguments;
} Agx_Tile_Programs;

typedef struct Agx_Compute_Pipeline_Desc
{
  uint32_t kernel_offset;
  uint32_t kernel_bytes;
  uint32_t binding_count;
  bool     reads_a_texture;
} Agx_Compute_Pipeline_Desc;

void
agx_compute_pipeline_desc_init(Agx_Compute_Pipeline_Desc* desc);
void
agx_compute_pipeline_desc_set_kernel(Agx_Compute_Pipeline_Desc* desc, uint32_t offset, uint32_t bytes);
void
agx_compute_pipeline_desc_set_binding_count(Agx_Compute_Pipeline_Desc* desc, uint32_t count);
void
agx_compute_pipeline_desc_set_reads_texture(Agx_Compute_Pipeline_Desc* desc, bool reads);

typedef struct Agx_Varying_Group
{
  uint8_t                 components;
  Agx_Varying_Shade_Model shade_model;
  Agx_Coefficient_Source  source;
} Agx_Varying_Group;

void
agx_varying_group_init(Agx_Varying_Group* group, uint8_t components);
void
agx_varying_group_set_shade_model(Agx_Varying_Group* group, Agx_Varying_Shade_Model model);
void
agx_varying_group_set_source(Agx_Varying_Group* group, Agx_Coefficient_Source source);

uint32_t
agx_usc_global_configuration_init_build(uint8_t* bytes, uint32_t capacity);

extern const Agx_End_Of_Tile_Range agx_end_of_tile_second_ranges[AGX_END_OF_TILE_SECOND_RANGE_COUNT];

bool
agx_background_object_program_create(Agx_Device device, Agx_Bo* program);

bool
agx_shader_pool_create(Agx_Device device, uint32_t bytes, Agx_Bo* pool);

uint32_t
agx_pool_append(Agx_Pool_Layout* layout, uint32_t bytes, uint32_t alignment);

uint32_t
agx_pool_append_program(Agx_Pool_Layout* layout, const uint8_t* program, uint32_t bytes);

bool
agx_pool_append_end_of_tile_program(Agx_Pool_Layout* layout);

bool
agx_pool_append_driver_programs(Agx_Pool_Layout* layout);

bool
agx_tile_programs_create(
  Agx_Device         device,
  const Agx_Bo*      driver_arguments,
  const Agx_Bo*      background_object_program,
  const Agx_Bo*      shader_pool,
  uint32_t           store_shader,
  const Agx_Bo*      tile_arguments,
  Agx_Tile_Programs* programs
);

void
agx_tile_programs_destroy(Agx_Device device, Agx_Tile_Programs* programs);

uint32_t
agx_store_program_build(
  uint64_t argument_block,
  uint64_t tile_arguments,
  uint64_t shader_pool,
  uint32_t shader_pool_offset,
  uint8_t* bytes,
  uint32_t capacity
);

uint32_t
agx_pool_vertex_stage_program_place(uint8_t* pool, uint32_t entry_offset, uint32_t pool_bytes);

uint32_t
agx_pool_uniform_program_build(uint8_t* bytes, uint32_t capacity);

uint32_t
agx_pool_uniform_program_place(uint8_t* pool, uint32_t at, uint32_t pool_bytes);

uint32_t
agx_pipeline_vertex_call_program_build(
  Agx_Caller_Shape shape,
  uint64_t         argument_block,
  uint64_t         uniform_block,
  uint64_t         table,
  uint64_t         shader_pool,
  uint32_t         shader_pool_offset,
  uint32_t         state_loader_offset,
  uint8_t*         bytes,
  uint32_t         capacity
);

uint32_t
agx_pipeline_vertex_call_program_place(
  uint8_t*         pipeline_programs,
  uint32_t         pipeline_programs_bytes,
  Agx_Caller_Shape shape,
  uint64_t         argument_block,
  uint64_t         uniform_block,
  uint64_t         table,
  uint64_t         shader_pool,
  uint32_t         shader_pool_offset,
  uint32_t         state_loader_offset
);

void
agx_pool_arguments_set_geometry(void* arguments, uint64_t geometry);

void
agx_pool_arguments_set_geometry_pair(void* arguments, uint32_t pair, uint64_t address);

uint64_t
agx_pool_arguments_vertex_uniforms(uint64_t pool_arguments);
uint64_t
agx_pool_arguments_fragment_block(uint64_t pool_arguments);
uint64_t
agx_driver_arguments_vertex_table(uint64_t driver_arguments);
uint64_t
agx_driver_arguments_fragment_table(uint64_t driver_arguments);
uint64_t
agx_driver_arguments_vertex_shader_block(uint64_t driver_arguments);

void
agx_pool_arguments_set_copy_extent(void* arguments, uint32_t width, uint32_t height);

uint32_t
agx_pipeline_fragment_call_program_build(
  Agx_Caller_Shape shape,
  uint64_t         argument_block,
  uint64_t         table,
  uint64_t         shader_pool,
  uint32_t         shader_pool_offset,
  uint32_t         varying_components,
  uint8_t*         bytes,
  uint32_t         capacity
);

uint32_t
agx_pipeline_fragment_call_program_place(
  uint8_t*         pipeline_programs,
  uint32_t         at,
  uint32_t         pipeline_programs_bytes,
  uint32_t         vertex_caller_bytes,
  Agx_Caller_Shape shape,
  uint64_t         argument_block,
  uint64_t         table,
  uint64_t         shader_pool,
  uint32_t         shader_pool_offset,
  uint32_t         varying_components
);

uint32_t
agx_vertex_shader_call_program_build(
  uint64_t argument_block,
  uint64_t shader_pool,
  uint32_t shader_pool_offset,
  bool     writes_coverage_mask,
  uint32_t clip_distance_count,
  uint8_t* bytes,
  uint32_t capacity
);

uint32_t
agx_compute_pipeline_write(const Agx_Compute_Pipeline_Desc* desc, void* pool_cpu, uint32_t pool_bytes);

uint32_t
agx_tile_programs_build_vertex_shader_caller(
  Agx_Tile_Programs* programs,
  uint64_t           argument_block,
  uint32_t           shader_pool_offset,
  bool               writes_coverage_mask,
  uint32_t           clip_distance_count
);

uint32_t
agx_background_object_pixel_range_build(uint8_t tile_destination, uint8_t* bytes, uint32_t capacity);

uint32_t
agx_background_program_build(
  uint64_t        argument_block,
  uint64_t        tile_arguments,
  uint64_t        background_object_program,
  uint64_t        shader_pool,
  uint32_t        shader_pool_offset,
  const uint32_t* words,
  uint32_t        word_mask,
  uint8_t*        bytes,
  uint32_t        capacity
);

bool
agx_tile_programs_compile_background_words(
  Agx_Tile_Programs* programs,
  const Agx_Bo*      driver_arguments,
  const Agx_Bo*      background_object_program,
  const uint32_t     words[4],
  uint32_t           word_mask
);

uint32_t
agx_compute_prologue_build(
  uint64_t binding_table,
  uint32_t binding_count,
  uint64_t shader_table_entry,
  uint64_t shader_pool,
  uint32_t shader_pool_offset,
  bool     kernel_reads_atomic_result,
  uint8_t* bytes,
  uint32_t capacity
);

void
agx_tile_programs_write_register_release(Agx_Tile_Programs* programs);

void
agx_tile_programs_set_vertex_shader(Agx_Tile_Programs* programs, uint32_t shader_pool_offset);

void
agx_render_pass_desc_set_tile_programs(Agx_Render_Pass_Desc* desc, Agx_Tile_Programs programs);

void
agx_buffer_copy_program_write(void* programs_cpu, uint32_t cursor, uint64_t operands_gpu);

uint32_t
agx_buffer_copy_program_build(uint8_t* bytes, uint32_t capacity, uint32_t operand_slot);

void
agx_tile_programs_set_store_shader(Agx_Tile_Programs* programs, uint32_t shader_pool_offset);

void
agx_driver_arguments_set_attachment_count(const Agx_Bo* arguments, uint32_t count);

uint32_t
agx_pool_vertex_stage_program_build(uint8_t* bytes, uint32_t capacity);

uint32_t
agx_pool_state_arm_build(uint8_t* bytes, uint32_t capacity);
uint32_t
agx_pool_state_arm_place(uint8_t* pool, uint32_t at, uint32_t pool_bytes);

uint32_t
agx_pool_state_arm_two_build(uint8_t* bytes, uint32_t capacity);
uint32_t
agx_pool_state_arm_two_place(uint8_t* pool, uint32_t at, uint32_t pool_bytes);

uint32_t
agx_pool_state_arm_three_build(uint8_t* bytes, uint32_t capacity);
uint32_t
agx_pool_state_arm_three_place(uint8_t* pool, uint32_t at, uint32_t pool_bytes);

uint32_t
agx_pool_state_arm_four_build(uint8_t* bytes, uint32_t capacity);
uint32_t
agx_pool_state_arm_four_place(uint8_t* pool, uint32_t at, uint32_t pool_bytes);

uint32_t
agx_pool_state_store_build(uint8_t* bytes, uint32_t capacity);
uint32_t
agx_pool_state_store_place(uint8_t* pool, uint32_t at, uint32_t pool_bytes);

uint32_t
agx_pool_state_arm_eight_build(uint8_t* bytes, uint32_t capacity);
uint32_t
agx_pool_state_arm_eight_place(uint8_t* pool, uint32_t at, uint32_t pool_bytes);
uint32_t
agx_pool_state_arm_nine_build(uint8_t* bytes, uint32_t capacity);
uint32_t
agx_pool_state_arm_nine_place(uint8_t* pool, uint32_t at, uint32_t pool_bytes);

uint32_t
agx_pool_state_arm_six_build(uint8_t* bytes, uint32_t capacity);
uint32_t
agx_pool_state_arm_six_place(uint8_t* pool, uint32_t at, uint32_t pool_bytes);

uint32_t
agx_pool_state_arm_ten_build(uint8_t* bytes, uint32_t capacity);
uint32_t
agx_pool_state_arm_ten_place(uint8_t* pool, uint32_t at, uint32_t pool_bytes);
uint32_t
agx_pool_state_arm_eleven_build(uint8_t* bytes, uint32_t capacity);
uint32_t
agx_pool_state_arm_eleven_place(uint8_t* pool, uint32_t at, uint32_t pool_bytes);
uint32_t
agx_pool_state_arm_fourteen_build(uint8_t* bytes, uint32_t capacity);
uint32_t
agx_pool_state_arm_fourteen_place(uint8_t* pool, uint32_t at, uint32_t pool_bytes);
uint32_t
agx_pool_state_arm_thirteen_build(uint8_t* bytes, uint32_t capacity);
uint32_t
agx_pool_state_arm_thirteen_place(uint8_t* pool, uint32_t at, uint32_t pool_bytes);
uint32_t
agx_pool_state_chain_start_build(uint8_t* bytes, uint32_t capacity);
uint32_t
agx_pool_state_chain_start_place(uint8_t* pool, uint32_t at, uint32_t pool_bytes);
uint32_t
agx_pool_state_arm_twelve_build(uint8_t* bytes, uint32_t capacity);
uint32_t
agx_pool_state_arm_twelve_place(uint8_t* pool, uint32_t at, uint32_t pool_bytes);
uint32_t
agx_pool_state_chain_link_wide_build(uint8_t* bytes, uint32_t capacity);
uint32_t
agx_pool_state_chain_link_wide_place(uint8_t* pool, uint32_t at, uint32_t pool_bytes);

uint32_t
agx_pool_state_chain_link_build(uint8_t* bytes, uint32_t capacity);
uint32_t
agx_pool_state_chain_link_place(uint8_t* pool, uint32_t at, uint32_t pool_bytes);

uint32_t
agx_pool_state_arm_seven_build(uint8_t* bytes, uint32_t capacity);
uint32_t
agx_pool_state_arm_seven_place(uint8_t* pool, uint32_t at, uint32_t pool_bytes);

uint32_t
agx_pool_state_arm_five_build(uint8_t* bytes, uint32_t capacity);
uint32_t
agx_pool_state_arm_five_place(uint8_t* pool, uint32_t at, uint32_t pool_bytes);

uint32_t
agx_pool_vertex_stage_tail_run_build(
  uint8_t* pool,
  uint32_t at,
  uint32_t pool_bytes,
  uint32_t vertex_stage_entry,
  uint32_t texture_epilogue_offset,
  uint32_t next_occupant
);

uint32_t
agx_pool_vertex_stage_tail_build(uint8_t* bytes, uint32_t capacity);

uint32_t
agx_pool_uniform_program_gap_build(uint8_t* bytes, uint32_t capacity);

uint32_t
agx_pool_uniform_program_gap_place(uint8_t* pool, uint32_t at, uint32_t pool_bytes);
uint32_t
agx_pool_texture_epilogue_place(uint8_t* pool, uint32_t at, uint32_t pool_bytes);
uint32_t
agx_pool_tile_block_place(uint8_t* pool, uint32_t at, uint32_t pool_bytes);

uint32_t
agx_pool_vertex_stage_prologue_build(uint8_t* bytes, uint32_t capacity);

uint32_t
agx_pool_vertex_stage_tail_end_build(uint8_t* bytes, uint32_t capacity);

uint32_t
agx_pool_texture_epilogue_build(uint8_t* bytes, uint32_t capacity);

uint32_t
agx_pool_tile_block_build(uint8_t* bytes, uint32_t capacity);

uint32_t
agx_varying_linkage_build(const Agx_Varying_Group* groups, uint32_t group_count, uint8_t* bytes, uint32_t capacity);
