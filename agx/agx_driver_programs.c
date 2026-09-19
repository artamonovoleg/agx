#include <stdio.h>
#include "agx_driver_programs.h"
#include "agx_shader.h"

#include <string.h>

#define AGX_VERTEX_CALL_TAIL_COVERAGE_MASK_BYTE 0xf2u

#define AGX_VERTEX_CALL_CLIP_DISTANCE_BYTE 0x02u
#define AGX_PIPELINE_FRAGMENT_RETURN_SMALL 0x0180u
#define AGX_PIPELINE_FRAGMENT_RETURN_LARGE 0x0200u
#define AGX_COMPUTE_POOL_ENTRY_TABLE       0x100u
#define AGX_COMPUTE_POOL_ENTRY_ROUTINE     0x1a0u
#define AGX_COMPUTE_POOL_SLOT              0x10u
#define AGX_COMPUTE_RECORD_SLOT            0x40u

#define AGX_VARYING_SHADE_MODEL_SHIFT 2u
#define AGX_VARYING_SOURCE_SHIFT      5u

static const uint8_t k_agx_tile_start_tail_cut[2] = {0x49, 0x04};
static const uint8_t k_agx_tile_start_tail_x7[6] = {
  0x07,
  0x02,
  0x54,
  0x00,
  0x02,
  0x00,
};
static const uint8_t k_agx_tile_start_tail_xc8[8] = {
  0x2c,
  0x88,
  0x09,
  0x27,
  0x60,
  0x00,
  0x00,
  0x80,
};
static const uint8_t k_agx_tile_start_tail_xc_a[4] = {0x3c, 0x8a, 0x08, 0x27};
static const uint8_t k_agx_tile_start_tail_xc_b[4] = {0x0c, 0x8c, 0x08, 0x43};
static const uint8_t k_agx_tile_start_tail_xc_c[4] = {0x1c, 0x8e, 0x08, 0x07};

static uint32_t
agx_tile_start_tail_build(uint8_t* bytes, uint32_t capacity)
{
  static const struct
  {
    uint8_t destination, source;
  } k_release[] = {
    {18, 31},
    {19, 32},
    {20, 33},
    {13, 26},
    {14, 27},
    {15, 28},
    {16, 29},
  };

  Agx_Shader_Instruction  storage[48] = {0};
  Agx_Shader_Builder      builder = {0};
  Agx_Shader_Instruction* instruction = NULL;
  uint32_t                i = 0;

  if (bytes == NULL || capacity < AGX_TILE_START_TAIL_BYTES)
  {
    agx_refuse("agx_tile_start_tail_build: bytes == NULL || capacity < AGX_TILE_START_TAIL_BYTES");
    return 0;
  }

  agx_shader_builder_init(&builder, storage, sizeof(storage) / sizeof(storage[0]));

  if (agx_shader_raw(&builder, k_agx_tile_start_tail_cut, sizeof(k_agx_tile_start_tail_cut)) == NULL)
  {
    return 0;
  }

  for (i = 0; i < sizeof(k_release) / sizeof(k_release[0]); i += 1)
  {
    instruction = agx_shader_discard(&builder, k_release[i].source);
    if (instruction == NULL)
    {
      return 0;
    }
    instruction->destination = k_release[i].destination;
  }

  if (agx_shader_raw(&builder, k_agx_tile_start_tail_x7, sizeof(k_agx_tile_start_tail_x7)) == NULL ||
      agx_shader_raw(&builder, k_agx_tile_start_tail_xc8, sizeof(k_agx_tile_start_tail_xc8)) == NULL ||
      agx_shader_raw(&builder, k_agx_tile_start_tail_xc_a, sizeof(k_agx_tile_start_tail_xc_a)) == NULL ||
      agx_shader_raw(&builder, k_agx_tile_start_tail_xc_b, sizeof(k_agx_tile_start_tail_xc_b)) == NULL ||
      agx_shader_raw(&builder, k_agx_tile_start_tail_xc_c, sizeof(k_agx_tile_start_tail_xc_c)) == NULL)
  {
    return 0;
  }

  instruction = agx_shader_load(&builder, 3, 0, 2, 1, 0x188u, 1, AGX_SHADER_LOAD_FORM_ORDINARY, 1);
  if (instruction == NULL)
  {
    return 0;
  }
  instruction->first_load = 0x20u;
  instruction->base_discard = true;
  instruction->raw_bits = AGX_SHADER_LOAD_BYTE_2_BIT_4_CLEAR;

  instruction = agx_shader_move_nibble_3(&builder, 2, 0);
  if (instruction == NULL)
  {
    return 0;
  }
  instruction->move_raw = 0x6021u;

  instruction = agx_shader_move_nibble_3(&builder, 2, 0);
  if (instruction == NULL)
  {
    return 0;
  }
  instruction->destination_half = true;
  instruction->move_raw = 0x0020u;

  instruction = agx_shader_discard(&builder, 2);
  if (instruction == NULL)
  {
    return 0;
  }
  instruction->destination = 10;
  instruction->move_byte_1_bit_7 = true;
  instruction->move_keeps_source = true;

  instruction = agx_shader_move_nibble_3(&builder, 2, 0);
  if (instruction == NULL)
  {
    return 0;
  }
  instruction->move_raw = 0x0001u | AGX_SHADER_MOVE_NIBBLE_3_SOURCE_HALF_UNMEASURED;

  instruction = agx_shader_bit_op(&builder, 8, 0x7fu, 3, AGX_SHADER_BIT_AND);
  if (instruction == NULL)
  {
    return 0;
  }
  instruction->source_immediate = true;
  instruction->source_half = true;
  instruction->source_b_half = true;
  instruction->keep_source_b = false;
  instruction->source_form = AGX_SHADER_SOURCE_FORM_DISCARD;
  instruction->raw_bits = AGX_SHADER_BIT_OP_BYTE_4_BIT_6 | ((uint32_t)0x42u << AGX_SHADER_BIT_OP_BYTE_5_SHIFT);

  instruction = agx_shader_shift_right(&builder, 9, 0, 24);
  if (instruction == NULL)
  {
    return 0;
  }
  instruction->raw_bits = AGX_SHADER_SHIFT_BYTE_4_IS_TWO | AGX_SHADER_SHIFT_BYTE_4_BIT_1_CLEAR |
                          ((uint32_t)1u << AGX_SHADER_SHIFT_BYTE_9_LOW_SHIFT);

  instruction = agx_shader_discard(&builder, 2);
  if (instruction == NULL)
  {
    return 0;
  }
  instruction->destination = 11;

  instruction = agx_shader_bit_op(&builder, 12, 0x7fu, 1, AGX_SHADER_BIT_AND);
  if (instruction == NULL)
  {
    return 0;
  }
  instruction->source_immediate = true;
  instruction->source_half = true;
  instruction->source_b_half = true;
  instruction->keep_source_b = false;
  instruction->source_form = AGX_SHADER_SOURCE_FORM_DISCARD;
  instruction->raw_bits = AGX_SHADER_BIT_OP_BYTE_4_BIT_6 | ((uint32_t)0x22u << AGX_SHADER_BIT_OP_BYTE_5_SHIFT);

  if (agx_shader_stop(&builder) == NULL || agx_shader_pad(&builder, 2) == NULL)
  {
    return 0;
  }
  return agx_shader_encode(&builder, bytes, capacity);
}

static bool
agx_driver_state_add(Agx_Shader_Builder* builder, uint8_t destination, uint8_t source, uint8_t immediate, bool subtract, uint32_t carried)
{
  Agx_Shader_Instruction* instruction =
    agx_shader_add_index(builder, destination, source, immediate, AGX_SHADER_INDEX_SOURCE_FORM_DISCARD, false);

  if (instruction == NULL)
  {
    return false;
  }
  instruction->subtract = subtract;

  instruction->source_is_16_bit = !subtract;
  instruction->index_second_operand_is_uniform = true;
  instruction->raw_bits = carried;
  return true;
}

static bool
agx_driver_state_branch_compare(Agx_Shader_Builder* builder, uint8_t source, bool condition_bit_3)
{
  Agx_Shader_Instruction* instruction = agx_shader_compare_immediate(builder, source, AGX_SHADER_COMPARE_GREATER, 0);

  if (instruction == NULL)
  {
    return false;
  }
  instruction->long_form = true;
  instruction->compare_source_is_half = true;
  instruction->raw_bits = AGX_SHADER_COMPARE_BYTE_4_BIT_7 | AGX_SHADER_COMPARE_TYPE_BITS_CLEAR |
                          AGX_SHADER_COMPARE_WIDE_BYTE_3_BIT_0 |
                          (condition_bit_3 ? AGX_SHADER_COMPARE_CONDITION_BIT_3 : 0u);
  return true;
}

static bool
agx_driver_texture_read(Agx_Shader_Builder* builder, uint8_t texture_index, bool three_d)
{
  Agx_Shader_Instruction* instruction = agx_shader_texture_sample(builder, 4, 0);

  if (instruction == NULL)
  {
    return false;
  }
  instruction->texture_index = texture_index;
  instruction->sampler_index = 1;
  instruction->texture_access = three_d ? AGX_SHADER_TEXTURE_ACCESS_READ_3D : AGX_SHADER_TEXTURE_ACCESS_READ_2D;
  instruction->sample_channels_minus_one = 3u;
  instruction->raw_bits =
    AGX_SHADER_TEXTURE_BYTE_12_IS_0X24 |
    (three_d ? (AGX_SHADER_TEXTURE_BYTE_4_BIT_7_CLEAR | (1u << AGX_SHADER_TEXTURE_BYTE_5_SHIFT))
             : (AGX_SHADER_TEXTURE_BYTE_3_BIT_7_CLEAR | (4u << AGX_SHADER_TEXTURE_BYTE_1_LOW_SHIFT)));
  return true;
}

static bool
agx_driver_store_position_add(Agx_Shader_Builder* builder)
{
  Agx_Shader_Instruction* instruction =
    agx_shader_add_index(builder, 0, 1, 0, AGX_SHADER_INDEX_SOURCE_FORM_UNMARKED, true);

  if (instruction == NULL)
  {
    return false;
  }
  instruction->add_second_source_is_register = true;
  instruction->source_b = 0;
  instruction->index_scaled = true;
  instruction->second_form = AGX_SHADER_OPERAND_FORM_KEEP;
  return true;
}

static const uint8_t k_agx_store_released[] = {
  20, 21, 22, 18, 19, 51, 52, 53, 54, 47, 48, 49, 50, 43, 44, 45, 46, 39, 40,
  41, 42, 35, 36, 37, 38, 31, 32, 33, 34, 27, 28, 29, 30, 23, 24, 25, 26,
};

static const uint8_t k_agx_vertex_call_tail[6] = {0x07, 0x02, 0x54, 0xf3, 0x02, 0x00};

static const uint8_t k_agx_background_tail[8] = {0x73, 0x32, 0x0f, 0x00, 0x22, 0x00, 0x00, 0x14};

static const uint8_t k_agx_bo_pixel_2[4] = {0x32, 0xd2, 0x07, 0x02};

static const uint8_t k_agx_bo_pixel_6[8] = {0x3b, 0xce, 0x06, 0x00, 0x17, 0x02, 0x00, 0x00};

static const uint8_t k_agx_bo_pixel_tail_1[6] = {0x0f, 0x01, 0x54, 0x9c, 0x00, 0x00};

static bool
agx_driver_tail_clamp(Agx_Shader_Builder* builder, uint8_t destination, uint8_t source, uint8_t source_b)
{
  Agx_Shader_Instruction* instruction =
    agx_shader_min_max(builder, destination, source, source_b, true, AGX_SHADER_MIN_MAX_TYPE_UNSIGNED);

  if (instruction == NULL)
  {
    return false;
  }
  instruction->source_half = true;
  instruction->source_b_half = true;
  instruction->source_form = AGX_SHADER_SOURCE_FORM_KEEP;
  instruction->second_form = AGX_SHADER_OPERAND_FORM_DISCARD;
  instruction->raw_bits = AGX_SHADER_MIN_MAX_BYTE_4_BIT_7 | AGX_SHADER_MIN_MAX_BYTE_5_BASE_CLEAR;
  return true;
}

static bool
agx_driver_call_setup(Agx_Shader_Builder* builder, uint32_t pool_offset, uint64_t program_address)
{
  Agx_Shader_Instruction* instruction = agx_shader_call_pool_shader(builder, pool_offset, program_address);

  if (instruction == NULL)
  {
    return false;
  }
  instruction->raw_bits = AGX_SHADER_CALL_BYTE_1_CLEAR | AGX_SHADER_CALL_BYTE_7_CLEAR;
  return true;
}

static bool
agx_driver_call_after(Agx_Shader_Builder* builder, uint32_t pool_offset, uint64_t program_address)
{
  Agx_Shader_Instruction* instruction = agx_shader_call_pool_shader(builder, pool_offset, program_address);

  if (instruction == NULL)
  {
    return false;
  }
  instruction->raw_bits = AGX_SHADER_CALL_BYTE_1_CLEAR | AGX_SHADER_CALL_BYTE_7_CLEAR | AGX_SHADER_CALL_BYTE_0_BIT_7;
  return true;
}

static bool
agx_driver_bo_convert(
  Agx_Shader_Builder*       builder,
  uint8_t                   destination,
  Agx_Shader_Convert_Form   form,
  uint8_t                   source_a,
  uint8_t                   source_b,
  Agx_Shader_Numeric_Format format,
  bool                      waits
)
{
  Agx_Shader_Instruction* instruction =
    agx_shader_convert_to_surface(builder, destination, false, form, source_a, source_b, format);

  if (instruction == NULL)
  {
    return false;
  }
  instruction->wait = waits;
  return true;
}

uint32_t
agx_background_object_pixel_range_build(uint8_t tile_destination, uint8_t* bytes, uint32_t capacity)
{
  Agx_Shader_Instruction storage[24] = {0};
  Agx_Shader_Builder     builder = {0};

  agx_shader_builder_init(&builder, storage, sizeof(storage) / sizeof(storage[0]));

  agx_shader_pad(&builder, 4);
  agx_shader_end_thread(&builder, 0x81);
  agx_shader_raw(&builder, k_agx_bo_pixel_2, sizeof(k_agx_bo_pixel_2));
  {
    Agx_Shader_Instruction* made = agx_shader_push_exec(&builder, false);

    if (made != NULL)
    {
      made->mask_source = AGX_SHADER_MASK_SOURCE_NONE;
    }
  }
  agx_shader_jump_exec_none_bytes(&builder, 0x3e);
  agx_shader_end_thread(&builder, 0x01);
  agx_shader_raw(&builder, k_agx_bo_pixel_6, sizeof(k_agx_bo_pixel_6));
  {
    Agx_Shader_Instruction* made = agx_shader_push_exec(&builder, false);

    if (made != NULL)
    {
      made->mask_source = AGX_SHADER_MASK_SOURCE_NONE;
    }
  }

  agx_driver_bo_convert(&builder, 0, AGX_SHADER_CONVERT_FORM_PAIR, 9, 10, AGX_SHADER_NUMERIC_FORMAT_SRGB_BOTH, true);
  agx_driver_bo_convert(&builder, 1, AGX_SHADER_CONVERT_FORM_SINGLE, 8, 0, AGX_SHADER_NUMERIC_FORMAT_SRGB_LOW_ONLY, false);
  agx_shader_store_tile_pixel(&builder, 0, 0xda);

  agx_shader_pop_exec(&builder, 1);

  agx_shader_pop_exec_short(&builder, 0x19);
  agx_driver_bo_convert(&builder, 0, AGX_SHADER_CONVERT_FORM_PAIR, 8, 9, AGX_SHADER_NUMERIC_FORMAT_UNORM8, true);
  agx_driver_bo_convert(&builder, 1, AGX_SHADER_CONVERT_FORM_PAIR, 10, 11, AGX_SHADER_NUMERIC_FORMAT_UNORM8, false);
  agx_shader_store_tile_pixel(&builder, 0, tile_destination);
  agx_shader_pop_exec(&builder, 1);
  agx_shader_pop_exec_short(&builder, 0x19);

  agx_shader_raw(&builder, k_agx_bo_pixel_tail_1, sizeof(k_agx_bo_pixel_tail_1));

  return agx_shader_encode(&builder, bytes, capacity);
}

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
)
{
  Agx_Shader_Instruction  storage[48] = {0};
  Agx_Shader_Builder      builder = {0};
  Agx_Shader_Instruction* instruction = NULL;
  uint32_t                i = 0;

  agx_shader_builder_init(&builder, storage, sizeof(storage) / sizeof(storage[0]));

  agx_shader_move_immediate(&builder, 2, (uint32_t)argument_block);
  agx_shader_move_immediate(&builder, 3, (uint32_t)(argument_block >> 32));
  agx_shader_move_immediate(&builder, 26, (uint32_t)tile_arguments);
  agx_shader_move_immediate(&builder, 27, (uint32_t)(tile_arguments >> 32));

  agx_shader_load(&builder, 22, 0, 2, 4, 0x160, 1, AGX_SHADER_LOAD_FORM_ORDINARY, 4);
  agx_shader_load(&builder, 18, 0, 2, 4, 0x000, 1, AGX_SHADER_LOAD_FORM_ORDINARY, 4);
  instruction = agx_shader_load(&builder, 58, 0, 26, 3, 0x080, 1, AGX_SHADER_LOAD_FORM_ORDINARY, 4);
  for (i = 0; i < 8; ++i)
  {
    instruction = agx_shader_load(
      &builder, (uint8_t)(54u - 4u * i), 0, 26, 4, (uint32_t)(0x70u - 0x10u * i), 1, AGX_SHADER_LOAD_FORM_ORDINARY, 4
    );
    if (instruction && i == 7)
    {
      instruction->base_discard = true;
    }
  }

  agx_driver_call_setup(&builder, 0x2180, 0x4bull << 15);
  agx_shader_call_pool_shader(&builder, shader_pool_offset, background_object_program);
  agx_shader_nop(&builder);
  agx_driver_call_after(&builder, 0x4000, 0x07ull << 15);
  agx_shader_pad(&builder, 2);
  {
    Agx_Shader_Instruction* made = agx_shader_move_immediate(&builder, 1, 0);

    if (made != NULL)
    {
      made->long_form = true;
    }
  }

  agx_shader_get_special(&builder, 1, AGX_SHADER_SPECIAL_UNKNOWN_81);

  agx_shader_move_immediate(&builder, 0, (uint32_t)(shader_pool + 0x100));
  agx_driver_store_position_add(&builder);
  agx_shader_move_immediate(&builder, 1, (uint32_t)((shader_pool + 0x100) >> 32));
  agx_shader_end_thread_long(&builder, 0x4c);

  {
    static const uint8_t k_color_destination[3] = {4, 5, 6};
    static const uint8_t k_color_source[3] = {22, 23, 24};

    instruction = agx_shader_discard(&builder, 0);
    if (instruction)
    {
      instruction->destination = 12;
      instruction->raw_bits = 0u;
      instruction->move_keeps_source = true;
    }

    for (i = 0; words && i < 4; ++i)
    {
      if (!(word_mask & (1u << i)))
      {
        continue;
      }
      instruction = agx_shader_move_immediate(&builder, (uint8_t)(22u + i), words[i]);
      if (instruction)
      {
        instruction->long_form = true;
      }
    }
    for (i = 0; i < 3; ++i)
    {
      instruction = agx_shader_move_to_tile(&builder, k_color_destination[i], k_color_source[i]);
      if (instruction && i == 0)
      {
        instruction->raw_bits = AGX_SHADER_MOVE_UNKNOWN_BIT;
      }
    }
    instruction = agx_shader_discard(&builder, 25);
    if (instruction)
    {
      instruction->destination = 7;
      instruction->raw_bits = 0u;
      instruction->move_keeps_source = true;
    }
  }
  agx_shader_raw(&builder, k_agx_background_tail, sizeof(k_agx_background_tail));

  return agx_shader_encode(&builder, bytes, capacity);
}

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
)
{
  Agx_Shader_Instruction  storage[64] = {0};
  Agx_Shader_Builder      builder = {0};
  Agx_Shader_Instruction* instruction = NULL;
  uint32_t                i = 0;

  if (binding_count == 0 || 2u * binding_count + (((binding_count & 1u) != 0) ? 2u : 0u) > 16u)
  {
    return 0;
  }

  agx_shader_builder_init(&builder, storage, sizeof(storage) / sizeof(storage[0]));

  uint8_t  table_register = (uint8_t)(18u + 2u * binding_count);
  uint32_t group = 0;
  uint32_t group_count = (binding_count + 1u) / 2u;

  agx_shader_move_immediate(&builder, 2, (uint32_t)binding_table);
  agx_shader_move_immediate(&builder, 3, (uint32_t)(binding_table >> 32));
  if ((binding_count & 1u) != 0)
  {
    agx_shader_move_immediate(&builder, table_register, (uint32_t)shader_table_entry);
    agx_shader_move_immediate(&builder, (uint8_t)(table_register + 1u), (uint32_t)(shader_table_entry >> 32));
  }

  for (group = group_count; group-- > 0;)
  {
    uint32_t first = 2u * group;
    uint8_t  words = (uint8_t)((binding_count - first >= 2u) ? 4u : 2u);

    agx_shader_load(&builder, (uint8_t)(18u + 2u * first), 0, 2, words, 8u * first, 1, AGX_SHADER_LOAD_FORM_ORDINARY, 4);
  }
  if ((binding_count & 1u) != 0)
  {
    instruction = agx_shader_load(&builder, table_register, 0, table_register, 2, 0, 1, AGX_SHADER_LOAD_FORM_ORDINARY, 4);
    if (instruction)
    {
      instruction->base_discard = true;
    }
  }

  agx_driver_call_setup(&builder, 0x2080, 0);

  agx_shader_call_pool_shader(&builder, shader_pool_offset, shader_pool);
  {
    Agx_Shader_Instruction* made = agx_shader_move_immediate(&builder, 0, 0);

    if (made != NULL)
    {
      made->destination_half = true;
    }
  }

  {
    Agx_Shader_Instruction* made =
      agx_shader_call_pool_shader(&builder, 0, kernel_reads_atomic_result ? (0x10ull << 23) : 0ull);

    if (made != NULL)
    {
      made->raw_bits = AGX_SHADER_CALL_BYTE_0_BIT_7 | AGX_SHADER_CALL_BYTE_1_CLEAR | AGX_SHADER_CALL_BYTE_7_CLEAR |
                       (kernel_reads_atomic_result ? AGX_SHADER_CALL_BYTE_7_BIT_4 : 0u);
    }
  }
  agx_shader_pad(&builder, 2);

  instruction = agx_shader_move_immediate(&builder, 1, 0);
  if (instruction)
  {
    instruction->long_form = true;
  }
  agx_shader_get_special(&builder, 1, AGX_SHADER_SPECIAL_UNKNOWN_81);

  agx_shader_move_immediate(&builder, 0, (uint32_t)(shader_pool + 0x100));
  agx_driver_store_position_add(&builder);
  agx_shader_move_immediate(&builder, 1, (uint32_t)((shader_pool + 0x100) >> 32));
  agx_shader_end_thread_long(&builder, 0x4c);

  {
    uint8_t  released[16] = {0};
    uint32_t release_count = 0;

    for (group = group_count; group-- > 0;)
    {
      uint32_t first = 2u * group;
      uint32_t words = (binding_count - first >= 2u) ? 4u : 2u;
      uint32_t word = 0;

      for (word = 0; word < words; ++word)
      {
        released[release_count++] = (uint8_t)(18u + 2u * first + word);
      }
    }
    if ((binding_count & 1u) != 0)
    {
      released[release_count++] = table_register;
      released[release_count++] = (uint8_t)(table_register + 1u);
    }

    for (i = 0; i < release_count; ++i)
    {
      instruction = agx_shader_discard(&builder, released[i]);
      if (instruction)
      {
        instruction->destination = (uint8_t)(released[i] - 18u);
        if (i == 0)
        {
          instruction->raw_bits = AGX_SHADER_MOVE_UNKNOWN_BIT;
        }
      }
    }
  }

  agx_shader_stop(&builder);
  return agx_shader_encode(&builder, bytes, capacity);
}

static const uint8_t k_agx_pipeline_release_wide[10] = {0x0f, 0x00, 0x22, 0x00, 0x00, 0x14, 0x00, 0x00};

static uint32_t
agx_pipeline_fragment_return_field(uint32_t varying_components)
{
  return varying_components <= 2u ? AGX_PIPELINE_FRAGMENT_RETURN_SMALL : AGX_PIPELINE_FRAGMENT_RETURN_LARGE;
}

static void
agx_pipeline_move_keeping_source(Agx_Shader_Builder* builder, uint8_t destination, uint8_t source, uint32_t extra_bits)
{
  Agx_Shader_Instruction* instruction = agx_shader_discard(builder, source);
  if (instruction)
  {
    instruction->destination = destination;
    instruction->raw_bits = extra_bits;
    instruction->move_keeps_source = true;
  }
}

static void
agx_pipeline_release_wide(Agx_Shader_Builder* builder, uint8_t reg, uint8_t destination)
{
  uint8_t run[10] = {0};
  run[0] = (uint8_t)(0x0bu | ((destination & 0x0fu) << 4));
  run[1] = (uint8_t)((reg << 1) | 1u);
  memcpy(run + 2, k_agx_pipeline_release_wide, 8);
  agx_shader_raw(builder, run, sizeof(run));
}

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
)
{
  static const uint8_t k_plain[14][2] = {
    {26, 8},
    {27, 9},
    {28, 10},
    {29, 11},
    {22, 4},
    {23, 5},
    {24, 6},
    {25, 7},
  };
  static const uint8_t k_late[14][2] = {
    {40, 34},
    {41, 35},
    {42, 36},
    {43, 37},
    {36, 30},
    {37, 31},
    {38, 32},
    {39, 33},
    {32, 26},
    {33, 27},
    {34, 28},
    {35, 29},
    {44, 38},
    {45, 39},
  };

  static const uint8_t k_move_then_wide[4][3] = {
    {0, 18, 4},
    {1, 19, 5},
    {2, 20, 6},
    {3, 21, 7},
  };

  Agx_Shader_Instruction  storage[80] = {0};
  Agx_Shader_Builder      builder = {0};
  Agx_Shader_Instruction* instruction = NULL;
  uint32_t                i = 0;

  agx_shader_builder_init(&builder, storage, sizeof(storage) / sizeof(storage[0]));

  if (shape == AGX_CALLER_SHAPE_MINIMAL)
  {
    agx_shader_move_immediate(&builder, 0, 0x10);
    agx_shader_jump_absolute(&builder, shader_pool + state_loader_offset);
    return agx_shader_encode(&builder, bytes, capacity);
  }

  agx_shader_move_immediate(&builder, 2, (uint32_t)argument_block);
  agx_shader_move_immediate(&builder, 3, (uint32_t)(argument_block >> 32));
  for (i = 0; i < 4; ++i)
  {
    uint8_t  reg = (uint8_t)((i < 2) ? i : 12u + (i - 2u));
    uint64_t value = (i < 2) ? uniform_block : table;
    instruction = agx_shader_move_immediate(&builder, reg, (i & 1u) ? (uint32_t)(value >> 32) : (uint32_t)value);
    if (instruction)
    {
      instruction->raw_bits = AGX_SHADER_MOVE_IMMEDIATE_DESTINATION_BIT_5;
    }
  }

  agx_shader_load(&builder, 30, 0, 2, 2, 0x30, 1, AGX_SHADER_LOAD_FORM_ORDINARY, 4);
  agx_shader_load(&builder, 26, 0, 2, 4, 0x20, 1, AGX_SHADER_LOAD_FORM_ORDINARY, 4);
  agx_shader_load(&builder, 22, 0, 2, 4, 0x10, 1, AGX_SHADER_LOAD_FORM_ORDINARY, 4);
  agx_shader_load(&builder, 18, 0, 2, 4, 0x00, 1, AGX_SHADER_LOAD_FORM_ORDINARY, 4);
  agx_shader_load(&builder, 40, 0, 32, 4, 0x8038, 1, AGX_SHADER_LOAD_FORM_ORDINARY, 4);
  agx_shader_load(&builder, 36, 0, 32, 4, 0x8028, 1, AGX_SHADER_LOAD_FORM_ORDINARY, 4);
  instruction = agx_shader_load(&builder, 32, 0, 32, 4, 0x8018, 1, AGX_SHADER_LOAD_FORM_ORDINARY, 4);
  if (instruction)
  {
    instruction->base_discard = true;
  }
  instruction = agx_shader_load(&builder, 44, 0, 44, 2, 0x00, 1, AGX_SHADER_LOAD_FORM_ORDINARY, 4);
  if (instruction)
  {
    instruction->base_discard = true;
  }

  agx_driver_call_setup(&builder, 0x2280, 0x4bull << 15);

  agx_shader_call_pool_shader(&builder, shader_pool_offset, shader_pool);
  agx_shader_nop(&builder);
  agx_driver_call_after(&builder, 0, 0x06ull << 15);
  agx_shader_pad(&builder, 2);

  instruction = agx_shader_move_immediate(&builder, 1, 0);
  if (instruction)
  {
    instruction->long_form = true;
  }
  agx_shader_get_special(&builder, 1, AGX_SHADER_SPECIAL_UNKNOWN_81);

  agx_shader_move_immediate(&builder, 0, (uint32_t)(shader_pool + 0x100));
  agx_driver_store_position_add(&builder);
  agx_shader_move_immediate(&builder, 1, (uint32_t)((shader_pool + 0x100) >> 32));
  agx_shader_end_thread_long(&builder, 0x4c);

  agx_pipeline_move_keeping_source(&builder, 14, 0, 0);
  agx_pipeline_move_keeping_source(&builder, 15, 0, 0);
  agx_pipeline_move_keeping_source(&builder, 24, 0, 0);
  agx_pipeline_move_keeping_source(&builder, 25, 0, 0);

  agx_pipeline_move_keeping_source(&builder, 12, 30, AGX_SHADER_MOVE_UNKNOWN_BIT);
  agx_pipeline_release_wide(&builder, 30, 8);
  agx_pipeline_move_keeping_source(&builder, 13, 31, 0);
  agx_pipeline_release_wide(&builder, 31, 9);

  for (i = 0; i < 8; ++i)
  {
    instruction = agx_shader_discard(&builder, k_plain[i][0]);
    if (instruction)
    {
      instruction->destination = k_plain[i][1];
    }
  }

  for (i = 0; i < 4; ++i)
  {
    agx_pipeline_move_keeping_source(&builder, k_move_then_wide[i][0], k_move_then_wide[i][1], 0);
    agx_pipeline_release_wide(&builder, k_move_then_wide[i][1], k_move_then_wide[i][2]);
  }

  for (i = 0; i < 14; ++i)
  {
    instruction = agx_shader_discard(&builder, k_late[i][0]);
    if (instruction)
    {
      instruction->destination = k_late[i][1];
    }
  }

  agx_shader_move_immediate(&builder, 0, 0x10);
  agx_shader_jump_absolute(&builder, shader_pool + state_loader_offset);
  return agx_shader_encode(&builder, bytes, capacity);
}

static const uint8_t k_agx_pool_uniform_opening_unknown[4] = {0x60, 0x00, 0x00, 0x80};

uint32_t
agx_pool_uniform_program_build(uint8_t* bytes, uint32_t capacity)
{
  static const struct
  {
    uint8_t  destination;
    bool     destination_half;
    uint8_t  base, word_count, slot;
    uint32_t offset;
    uint8_t  first_load;
    uint8_t  access_width;
    uint32_t unknown;
    bool     base_discard;
  } k_loads[] = {
    {0, false, 4, 4, 0, 0x08, 0x80, AGX_SHADER_ACCESS_WIDTH_32, 0x000, false},
    {10, false, 4, 1, 0, 0x28, 0x00, AGX_SHADER_ACCESS_WIDTH_16, 0x000, false},
    {13, true, 4, 1, 4, 0x48, 0x00, AGX_SHADER_ACCESS_WIDTH_8, 0x040, false},
    {13, false, 4, 1, 4, 0x48, 0x00, AGX_SHADER_ACCESS_WIDTH_8, 0x000, false},
    {6, false, 6, 2, 1, 0x00, 0x20, AGX_SHADER_ACCESS_WIDTH_32, 0x000, true},
    {10, true, 4, 1, 5, 0x48, 0x00, AGX_SHADER_ACCESS_WIDTH_8, 0x060, false},
    {14, false, 4, 1, 5, 0x48, 0x00, AGX_SHADER_ACCESS_WIDTH_8, 0x020, false},
    {12, false, 4, 1, 1, 0x38, 0x00, AGX_SHADER_ACCESS_WIDTH_32, 0x000, false},
    {8, false, 8, 2, 1, 0x00, 0x40, AGX_SHADER_ACCESS_WIDTH_32, 0x000, true},
    {11, false, 4, 1, 2, 0x2c, 0x00, AGX_SHADER_ACCESS_WIDTH_16, 0x000, false},
    {4, false, 4, 2, 3, 0x00, 0x00, AGX_SHADER_ACCESS_WIDTH_32, 0x000, true},
  };

  static const uint8_t k_tail_releases[7][3] = {
    {11, 19, AGX_SHADER_MOVE_UNKNOWN_BIT | AGX_SHADER_MOVE_UNKNOWN_BIT_5},
    {0, 20, 0},
    {3, 21, 0},
    {4, 22, 0},
    {10, 23, 0},
  };

  Agx_Shader_Instruction  storage[64] = {0};
  Agx_Shader_Builder      builder = {0};
  Agx_Shader_Instruction* instruction = NULL;
  uint32_t                i = 0;

  agx_shader_builder_init(&builder, storage, sizeof(storage) / sizeof(storage[0]));

  {
    static const struct
    {
      uint8_t  destination;
      uint8_t  special;
      uint16_t unknown;
    } k_opening[] = {
      {6, 0x8c, 0x2709},
      {7, 0x8e, 0x2708},
      {8, 0x88, 0x4708},
      {9, 0x8a, 0x4708},
      {4, 0x90, 0x6708},
      {5, 0x92, 0x6708},
    };
    uint32_t k = 0;

    for (k = 0; k < sizeof(k_opening) / sizeof(k_opening[0]); ++k)
    {
      instruction = agx_shader_get_special(&builder, k_opening[k].destination, (Agx_Shader_Special)k_opening[k].special);
      if (instruction)
      {
        instruction->long_form = false;
        instruction->destination_half = false;
        instruction->raw_bits = k_opening[k].unknown;
      }
      if (k == 0)
      {
        agx_shader_raw(&builder, k_agx_pool_uniform_opening_unknown, sizeof(k_agx_pool_uniform_opening_unknown));
      }
    }
  }

  for (i = 0; i < sizeof(k_loads) / sizeof(k_loads[0]); ++i)
  {
    instruction = agx_shader_load(
      &builder,
      k_loads[i].destination,
      0,
      k_loads[i].base,
      k_loads[i].word_count,
      k_loads[i].offset,
      k_loads[i].slot,
      AGX_SHADER_LOAD_FORM_ORDINARY,
      1
    );
    if (instruction)
    {
      instruction->first_load = k_loads[i].first_load;
      instruction->destination_half = k_loads[i].destination_half;
      instruction->base_discard = k_loads[i].base_discard;
      instruction->access_width = k_loads[i].access_width;
      instruction->raw_bits = k_loads[i].unknown;
    }
  }

  instruction = agx_shader_discard(&builder, 2);
  if (instruction)
  {
    instruction->destination = 14;
    instruction->raw_bits = AGX_SHADER_MOVE_UNKNOWN_BIT_5;
  }

  instruction = agx_shader_convert_to_float(&builder, 2, 10);
  if (instruction)
  {
    instruction->destination_is_word = true;
    instruction->raw_bits = 1;
  }

  instruction = agx_shader_discard(&builder, 3);
  if (instruction)
  {
    instruction->destination = 15;
  }

  instruction = agx_shader_move_nibble_3(&builder, 11, 0);
  if (instruction)
  {
    instruction->destination_half = true;
    instruction->move_raw = 0x0020;
  }

  instruction = agx_shader_half_compare_select(&builder, 3, 13);
  if (instruction)
  {
    instruction->raw_bits = AGX_SHADER_HALF_SELECT_ZERO_A;
  }
  instruction = agx_shader_half_compare_select(&builder, 3, 13);
  if (instruction)
  {
    instruction->destination_half = true;
    instruction->raw_bits = AGX_SHADER_HALF_SELECT_ONE;
  }

  instruction = agx_shader_sum(&builder, 13, 2, 0);
  if (instruction)
  {
    instruction->second_form = AGX_SHADER_OPERAND_FORM_IMMEDIATE;
    instruction->second_immediate = 0.5f;

    instruction->source_form = AGX_SHADER_SOURCE_FORM_DISCARD;
  }

  instruction = agx_shader_half_compare_select(&builder, 10, 10);
  if (instruction)
  {
    instruction->destination_half = true;
    instruction->raw_bits = AGX_SHADER_HALF_SELECT_ZERO_B;
  }
  instruction = agx_shader_half_compare_select(&builder, 2, 14);
  if (instruction)
  {
    instruction->raw_bits = AGX_SHADER_HALF_SELECT_ONE;
  }

  instruction = agx_shader_product(&builder, 16, 13, 12);
  if (instruction)
  {
    instruction->second_form = AGX_SHADER_OPERAND_FORM_DISCARD;

    instruction->source_form = AGX_SHADER_SOURCE_FORM_DISCARD;
    instruction->wait = true;
    instruction->wait_slot = 1;
    instruction->reaches_export = false;
    instruction->result_exported = true;
  }

  instruction = agx_shader_scale_index(
    &builder, 17, 8, 0, (Agx_Shader_Index_Form)0xd, AGX_SHADER_INDEX_SOURCE_FORM_DISCARD, false, false
  );
  if (instruction)
  {
    instruction->destination_suppressed = true;
    instruction->second_is_register = true;
    instruction->source_b = 6;

    instruction->second_form = AGX_SHADER_OPERAND_FORM_DISCARD;
  }

  instruction = agx_shader_move_nibble_3(&builder, 0, 1);
  if (instruction)
  {
    instruction->destination_half = true;
    instruction->move_raw = 0x0029;
  }

  instruction = agx_shader_scale_index(
    &builder, 18, 9, 0, (Agx_Shader_Index_Form)0xd, AGX_SHADER_INDEX_SOURCE_FORM_DISCARD, false, false
  );
  if (instruction)
  {
    instruction->destination_suppressed = true;
    instruction->second_is_register = true;
    instruction->source_b = 7;

    instruction->second_form = AGX_SHADER_OPERAND_FORM_DISCARD;
  }

  instruction = agx_shader_discard(&builder, k_tail_releases[0][0]);
  if (instruction)
  {
    instruction->destination = k_tail_releases[0][1];
    instruction->raw_bits = k_tail_releases[0][2];
  }
  instruction = agx_shader_discard(&builder, k_tail_releases[1][0]);
  if (instruction)
  {
    instruction->destination = k_tail_releases[1][1];
  }
  instruction = agx_shader_move_nibble_3(&builder, 4, 5);
  if (instruction)
  {
    instruction->destination_half = true;
    instruction->move_raw = 0x8029;
  }
  instruction = agx_shader_discard(&builder, k_tail_releases[2][0]);
  if (instruction)
  {
    instruction->destination = k_tail_releases[2][1];
  }
  instruction = agx_shader_move_nibble_3(&builder, 2, 0);
  if (instruction)
  {
    instruction->destination_half = true;
    instruction->move_raw = 0x0020;
  }
  for (i = 3; i < 5; ++i)
  {
    instruction = agx_shader_discard(&builder, k_tail_releases[i][0]);
    if (instruction)
    {
      instruction->destination = k_tail_releases[i][1];
    }
  }

  {
    Agx_Shader_Instruction* made = agx_shader_output_move(&builder, 8, 2, false);

    if (made != NULL)
    {
      made->long_form = true;
      made->raw_bits = AGX_SHADER_OUTPUT_MOVE_BYTE_8_IS_0X20 | AGX_SHADER_OUTPUT_MOVE_BYTE_2_BIT_6 |
                       AGX_SHADER_OUTPUT_MOVE_BYTE_2_BIT_5_CLEAR;
    }
  }
  agx_shader_stop(&builder);
  return agx_shader_encode(&builder, bytes, capacity);
}

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
)
{
  Agx_Shader_Instruction  storage[32] = {0};
  Agx_Shader_Builder      builder = {0};
  Agx_Shader_Instruction* instruction = NULL;
  uint32_t                i = 0;

  agx_shader_builder_init(&builder, storage, sizeof(storage) / sizeof(storage[0]));

  agx_shader_move_immediate(&builder, 2, (uint32_t)argument_block);
  agx_shader_move_immediate(&builder, 3, (uint32_t)(argument_block >> 32));
  agx_shader_move_immediate(&builder, 20, (uint32_t)table);
  agx_shader_move_immediate(&builder, 21, (uint32_t)(table >> 32));

  agx_shader_load(&builder, 18, 0, 2, 2, 0, 1, AGX_SHADER_LOAD_FORM_ORDINARY, 4);
  instruction = agx_shader_load(&builder, 20, 0, 20, 2, 0, 1, AGX_SHADER_LOAD_FORM_ORDINARY, 4);
  if (instruction)
  {
    instruction->base_discard = true;
  }

  agx_driver_call_setup(&builder, 0x2080, 0);

  agx_shader_call_pool_shader(&builder, shader_pool_offset, shader_pool);
  agx_shader_pad(&builder, 2);
  {
    uint8_t  ret[8] = {0xf7, 0x00, 0x2a, 0x00, 0x00, 0x00, 0x00, 0x00};
    uint32_t field = agx_pipeline_fragment_return_field(varying_components);

    ret[3] = (uint8_t)(field & 0xffu);
    ret[4] = (uint8_t)(field >> 8);
    agx_shader_raw(&builder, ret, sizeof(ret));
  }
  agx_shader_pad(&builder, 2);

  if (shape == AGX_CALLER_SHAPE_DRIVER)
  {
    instruction = agx_shader_move_immediate(&builder, 1, 0);
    if (instruction)
    {
      instruction->long_form = true;
    }
    agx_shader_get_special(&builder, 1, AGX_SHADER_SPECIAL_UNKNOWN_81);

    agx_shader_move_immediate(&builder, 0, (uint32_t)(shader_pool + 0x200));
    agx_driver_store_position_add(&builder);
    agx_shader_move_immediate(&builder, 1, (uint32_t)((shader_pool + 0x200) >> 32));
    agx_shader_end_thread_long(&builder, 0x4c);
  }

  for (i = 18; i <= 21; ++i)
  {
    instruction = agx_shader_discard(&builder, (uint8_t)i);
    if (instruction)
    {
      instruction->destination = (uint8_t)(i - 18u);
      if (i == 18)
      {
        instruction->raw_bits = AGX_SHADER_MOVE_UNKNOWN_BIT;
      }
    }
  }

  if (shape == AGX_CALLER_SHAPE_DRIVER)
  {
    Agx_Shader_Instruction* made = agx_shader_move_nibble_3(&builder, 0, 0);

    if (made != NULL)
    {
      made->long_form = true;

      made->destination_suppressed = true;
      made->move_raw = 0x05u | (2u << AGX_SHADER_MOVE_NIBBLE_3_RAW_NIBBLE_SHIFT);
    }
  }
  agx_shader_stop(&builder);
  return agx_shader_encode(&builder, bytes, capacity);
}

void
agx_varying_group_init(Agx_Varying_Group* group, uint8_t components)
{
  if (group == NULL)
  {
    return;
  }
  group->components = components;
  group->shade_model = AGX_VARYING_SHADE_MODEL_PERSPECTIVE;
  group->source = AGX_COEFFICIENT_SOURCE_VARYING;
}

void
agx_varying_group_set_shade_model(Agx_Varying_Group* group, Agx_Varying_Shade_Model model)
{
  if (group == NULL)
  {
    return;
  }
  group->shade_model = model;
}

void
agx_varying_group_set_source(Agx_Varying_Group* group, Agx_Coefficient_Source source)
{
  if (group == NULL)
  {
    return;
  }
  group->source = source;
}

static bool
agx_varying_shade_model_known(Agx_Varying_Shade_Model model)
{
  return model == AGX_VARYING_SHADE_MODEL_FLAT_VERTEX_0 || model == AGX_VARYING_SHADE_MODEL_FLAT_VERTEX_1 ||
         model == AGX_VARYING_SHADE_MODEL_FLAT_VERTEX_2 || model == AGX_VARYING_SHADE_MODEL_LINEAR ||
         model == AGX_VARYING_SHADE_MODEL_PERSPECTIVE;
}

static bool
agx_coefficient_source_known(Agx_Coefficient_Source source)
{
  return source == AGX_COEFFICIENT_SOURCE_VARYING || source == AGX_COEFFICIENT_SOURCE_FRAGCOORD_Z ||
         source == AGX_COEFFICIENT_SOURCE_POINT_COORD || source == AGX_COEFFICIENT_SOURCE_PRIMITIVE_ID ||
         source == AGX_COEFFICIENT_SOURCE_BARYCENTRIC;
}

static bool
agx_varying_source_takes_a_slot(Agx_Coefficient_Source source)
{
  return source == AGX_COEFFICIENT_SOURCE_VARYING || source == AGX_COEFFICIENT_SOURCE_FRAGCOORD_Z;
}

static uint32_t
agx_varying_base_slot(Agx_Coefficient_Source source, uint32_t slot)
{
  return agx_varying_source_takes_a_slot(source) ? slot : 0u;
}

static uint32_t
agx_varying_slot_count(const Agx_Varying_Group* groups, uint32_t group_count)
{
  uint32_t slots = 1;
  uint32_t group = 0;

  for (group = 0; group < group_count; group += 1)
  {
    if (agx_varying_source_takes_a_slot(groups[group].source))
    {
      slots += groups[group].components;
    }
  }
  return slots;
}

uint32_t
agx_varying_linkage_build(const Agx_Varying_Group* groups, uint32_t group_count, uint8_t* bytes, uint32_t capacity)
{
  const uint32_t row = 8u + 4u * group_count;
  const uint32_t length = 3u * row;
  uint32_t       components = 0;
  uint32_t       at = 0;
  uint32_t       group = 0;

  if (groups == NULL || group_count == 0 || group_count > 16u)
  {
    return 0;
  }

  for (group = 0; group < group_count; group += 1)
  {
    if (groups[group].components == 0 || groups[group].components > 4u)
    {
      return 0;
    }
    if (!agx_varying_shade_model_known(groups[group].shade_model) || !agx_coefficient_source_known(groups[group].source))
    {
      agx_refuse("agx_varying_linkage_build: unknown shade model or coefficient source");
      return 0;
    }
    components += groups[group].components;
  }

  if (components > 64u || length - 1u > capacity)
  {
    agx_refuse("agx_varying_linkage_build: components > 64u || length - 1u > capacity");
    return 0;
  }

  for (at = 0; at < 3u; at += 1)
  {
    uint8_t* out = bytes + at * row;
    uint32_t coefficient = 1;
    uint32_t slot = 1;

    out[0] = (uint8_t)agx_varying_slot_count(groups, group_count);
    out[1] = (uint8_t)(components + 1u);
    out[2] = 0x00;
    out[3] = 0x00;
    out[4] = 0x0c;
    out[5] = 0x00;
    out[6] = 0x00;
    out[7] = 0x00;

    for (group = 0; group < group_count; group += 1)
    {
      const uint32_t width = groups[group].components;

      out[8 + group * 4 + 0] =
        (uint8_t)((width - 1u) | ((uint32_t)groups[group].shade_model << AGX_VARYING_SHADE_MODEL_SHIFT) |
                  ((uint32_t)groups[group].source << AGX_VARYING_SOURCE_SHIFT));

      out[8 + group * 4 + 1] = (uint8_t)agx_varying_base_slot(groups[group].source, slot);
      out[8 + group * 4 + 2] = (uint8_t)coefficient;
      out[8 + group * 4 + 3] = 0x00;

      coefficient += width;
      if (agx_varying_source_takes_a_slot(groups[group].source))
      {
        slot += width;
      }
    }
  }

  return length - 1u;
}

uint32_t
agx_vertex_shader_call_program_build(
  uint64_t argument_block,
  uint64_t shader_pool,
  uint32_t shader_pool_offset,
  bool     writes_coverage_mask,
  uint32_t clip_distance_count,
  uint8_t* bytes,
  uint32_t capacity
)
{
  uint8_t                 tail[6] = {0};
  Agx_Shader_Instruction  storage[32] = {0};
  Agx_Shader_Builder      builder = {0};
  Agx_Shader_Instruction* instruction = NULL;
  uint32_t                i = 0;

  agx_shader_builder_init(&builder, storage, sizeof(storage) / sizeof(storage[0]));

  agx_shader_move_immediate(&builder, 2, (uint32_t)argument_block);
  agx_shader_move_immediate(&builder, 3, (uint32_t)(argument_block >> 32));
  agx_shader_load(&builder, 18, 0, 2, 4, 0, 1, AGX_SHADER_LOAD_FORM_ORDINARY, 4);

  agx_driver_call_setup(&builder, 0x2180, 0x4bull << 15);

  agx_shader_call_pool_shader(&builder, shader_pool_offset, shader_pool);

  {
    const uint8_t after_call[2] = {(uint8_t)(clip_distance_count != 0u ? AGX_VERTEX_CALL_CLIP_DISTANCE_BYTE : 0u), 0u};

    agx_shader_raw(&builder, after_call, sizeof(after_call));
  }
  agx_driver_call_after(&builder, 0x4000, 0);
  agx_shader_pad(&builder, 2);

  instruction = agx_shader_move_immediate(&builder, 1, 0);
  if (instruction)
  {
    instruction->long_form = true;
  }
  agx_shader_get_special(&builder, 1, AGX_SHADER_SPECIAL_UNKNOWN_81);

  agx_shader_move_immediate(&builder, 0, (uint32_t)(shader_pool + 0x100));
  agx_driver_store_position_add(&builder);
  agx_shader_move_immediate(&builder, 1, (uint32_t)((shader_pool + 0x100) >> 32));
  agx_shader_end_thread_long(&builder, 0x4c);

  for (i = 18; i <= 21; ++i)
  {
    instruction = agx_shader_discard(&builder, (uint8_t)i);
    if (instruction)
    {
      instruction->destination = (uint8_t)(i - 18u);
      if (i == 18)
      {
        instruction->raw_bits = AGX_SHADER_MOVE_UNKNOWN_BIT;
      }
    }
  }

  memcpy(tail, k_agx_vertex_call_tail, sizeof(tail));
  if (writes_coverage_mask)
  {
    tail[3] = AGX_VERTEX_CALL_TAIL_COVERAGE_MASK_BYTE;
  }
  agx_shader_raw(&builder, tail, sizeof(tail));

  {
    Agx_Shader_Instruction* made = agx_shader_move_nibble_3(&builder, 0, 0);

    if (made != NULL)
    {
      made->long_form = true;

      made->destination_suppressed = true;
      made->move_raw = 0x05u | (2u << AGX_SHADER_MOVE_NIBBLE_3_RAW_NIBBLE_SHIFT) | AGX_SHADER_MOVE_NIBBLE_3_RAW_TAIL_SHORT;
    }
  }
  agx_shader_stop(&builder);
  return agx_shader_encode(&builder, bytes, capacity);
}

uint32_t
agx_store_program_build(
  uint64_t argument_block,
  uint64_t tile_arguments,
  uint64_t shader_pool,
  uint32_t shader_pool_offset,
  uint8_t* bytes,
  uint32_t capacity
)
{
  Agx_Shader_Instruction  storage[80] = {0};
  Agx_Shader_Builder      builder = {0};
  Agx_Shader_Instruction* instruction = NULL;
  uint32_t                i = 0;

  agx_shader_builder_init(&builder, storage, sizeof(storage) / sizeof(storage[0]));

  agx_shader_move_immediate(&builder, 2, (uint32_t)argument_block);
  agx_shader_move_immediate(&builder, 3, (uint32_t)(argument_block >> 32));
  agx_shader_move_immediate(&builder, 23, (uint32_t)tile_arguments);
  agx_shader_move_immediate(&builder, 24, (uint32_t)(tile_arguments >> 32));

  agx_shader_load(&builder, 20, 0, 2, 3, 0x220, 1, AGX_SHADER_LOAD_FORM_ORDINARY, 4);
  agx_shader_load(&builder, 18, 0, 2, 2, 0x000, 1, AGX_SHADER_LOAD_FORM_ORDINARY, 4);
  for (i = 0; i < 8; ++i)
  {
    instruction = agx_shader_load(
      &builder, (uint8_t)(51u - 4u * i), 0, 23, 4, (uint32_t)(0x70u - 0x10u * i), 1, AGX_SHADER_LOAD_FORM_ORDINARY, 4
    );

    if (instruction && i == 7)
    {
      instruction->base_discard = true;
    }
  }

  agx_driver_call_setup(&builder, 0x2180, 0x4bull << 15);

  agx_shader_call_pool_shader(&builder, shader_pool_offset, shader_pool);
  agx_shader_nop(&builder);
  agx_driver_call_after(&builder, 0, 0x05ull << 15);
  agx_shader_pad(&builder, 2);
  {
    Agx_Shader_Instruction* made = agx_shader_move_immediate(&builder, 1, 0);

    if (made != NULL)
    {
      made->long_form = true;
    }
  }

  agx_shader_get_special(&builder, 1, AGX_SHADER_SPECIAL_UNKNOWN_81);

  agx_shader_move_immediate(&builder, 0, (uint32_t)(shader_pool + 0x100));
  agx_driver_store_position_add(&builder);
  agx_shader_move_immediate(&builder, 1, (uint32_t)((shader_pool + 0x100) >> 32));
  agx_shader_end_thread_long(&builder, 0x4c);

  for (i = 0; i < 3; ++i)
  {
    instruction = agx_shader_discard(&builder, 0);
    if (instruction)
    {
      instruction->destination = (uint8_t)(37u + i);
      instruction->raw_bits = 0u;
      instruction->move_keeps_source = true;
    }
  }

  for (i = 0; i < sizeof(k_agx_store_released) / sizeof(k_agx_store_released[0]); ++i)
  {
    uint8_t reg = k_agx_store_released[i];

    instruction = agx_shader_discard(&builder, reg);
    if (instruction)
    {
      instruction->destination = (uint8_t)(reg - 18u);
      if (reg == 20)
      {
        instruction->raw_bits = AGX_SHADER_MOVE_UNKNOWN_BIT;
      }
    }
  }

  agx_shader_stop(&builder);
  return agx_shader_encode(&builder, bytes, capacity);
}

typedef struct Agx_Program_Immediate
{
  size_t   offset;
  unsigned shift;
} Agx_Program_Immediate;

static void
agx_program_immediate_write(uint8_t* program, Agx_Program_Immediate immediate, const Agx_Bo* target)
{
  if (!target->gpu_va)
  {
    return;
  }

  uint16_t value = (uint16_t)((target->gpu_va & 0xffffffull) >> immediate.shift);
  memcpy(program + immediate.offset, &value, sizeof(value));
}

typedef enum Agx_Program_Shader_Reference
{
  AGX_STORE_PROGRAM_SHADER_REFERENCE = 0xb6,
} Agx_Program_Shader_Reference;

static void
agx_program_shader_reference_write(uint8_t* program, size_t offset, uint32_t shader_pool_offset)
{
  uint16_t value = (uint16_t)(2u * shader_pool_offset + 0x2au);
  memcpy(program + offset, &value, sizeof(value));
}

static uint32_t
agx_background_object_program_66280_build(uint8_t* bytes, uint32_t capacity)
{
  static const uint8_t k_tail_cut[4] = {0x2a, 0x81, 0x32, 0xc4};
  static const struct
  {
    uint8_t  immediate;
    uint32_t distance;
  } k_arms[] = {
    {0xcbu, 0x1e6u},
    {0xc7u, 0x0eeu},
    {0xc5u, 0x072u},
  };

  Agx_Shader_Instruction  storage[32] = {0};
  Agx_Shader_Builder      builder = {0};
  Agx_Shader_Instruction* instruction = NULL;
  uint32_t                i = 0;

  agx_shader_builder_init(&builder, storage, sizeof(storage) / sizeof(storage[0]));

  if (agx_shader_pop_exec_short(&builder, 0x19) == NULL)
  {
    return 0;
  }

  instruction =
    agx_shader_convert_to_surface(&builder, 0, false, AGX_SHADER_CONVERT_FORM_PAIR, 9, 8, AGX_SHADER_NUMERIC_FORMAT_UNORM8);
  if (instruction == NULL)
  {
    return 0;
  }
  instruction->wait = true;

  instruction = agx_shader_store_surface(&builder, 1, AGX_SHADER_CHANNEL_WIDTH_8, 0);
  if (instruction == NULL)
  {
    return 0;
  }
  instruction->store_selector = 0xd6;
  instruction->raw_bits = AGX_SHADER_STORE_SURFACE_BYTE_5_BIT_0 | ((uint32_t)0x04u << 8) | ((uint32_t)0x10u << 16);

  for (i = 0; i < 4; i += 1)
  {
    if (agx_shader_pop_exec(&builder, 1) == NULL)
    {
      return 0;
    }
  }
  if (agx_shader_pop_exec_short(&builder, 0x19) == NULL || agx_shader_jump_exec_none_bytes(&builder, 0x432u) == NULL)
  {
    return 0;
  }

  for (i = 0; i < sizeof(k_arms) / sizeof(k_arms[0]); i += 1)
  {
    instruction = agx_shader_compare_immediate(&builder, 64, AGX_SHADER_COMPARE_LESS, k_arms[i].immediate);
    if (instruction == NULL)
    {
      return 0;
    }
    instruction->compare_type = AGX_SHADER_COMPARE_TYPE_SIGNED;
    instruction->raw_bits = (uint32_t)2u << AGX_SHADER_COMPARE_BYTE_0_HIGH_SHIFT;

    instruction = agx_shader_push_exec(&builder, false);
    if (instruction == NULL)
    {
      return 0;
    }
    instruction->mask_source = AGX_SHADER_MASK_SOURCE_NONE;

    if (agx_shader_jump_exec_none_bytes(&builder, k_arms[i].distance) == NULL)
    {
      return 0;
    }
  }
  if (agx_shader_raw(&builder, k_tail_cut, sizeof(k_tail_cut)) == NULL)
  {
    return 0;
  }
  return agx_shader_encode(&builder, bytes, capacity);
}

static uint32_t
agx_background_object_program_003c0_build(uint8_t* bytes, uint32_t capacity)
{
  static const uint8_t k_head_cut[2] = {0x00, 0x00};
  static const uint8_t k_unnamed_72[10] = {
    0x72,
    0xe1,
    0x07,
    0x80,
    0x26,
    0xa0,
    0x07,
    0x02,
    0x80,
    0x02,
  };
  static const uint8_t k_tail_cut[6] = {0x0f, 0x01, 0x54, 0x94, 0x50, 0x06};

  Agx_Shader_Instruction  storage[32] = {0};
  Agx_Shader_Builder      builder = {0};
  Agx_Shader_Instruction* instruction = NULL;

  agx_shader_builder_init(&builder, storage, sizeof(storage) / sizeof(storage[0]));

  if (agx_shader_raw(&builder, k_head_cut, sizeof(k_head_cut)) == NULL)
  {
    return 0;
  }

  instruction = agx_shader_move(&builder, 48, 8);
  if (instruction == NULL)
  {
    return 0;
  }
  instruction->raw_bits = AGX_SHADER_MOVE_BYTE_2_BIT_5;
  instruction->move_keeps_source = true;
  instruction->move_byte_3 = 0x08u;

  instruction = agx_shader_push_exec(&builder, false);
  if (instruction == NULL)
  {
    return 0;
  }
  instruction->mask_source = AGX_SHADER_MASK_SOURCE_KEEP;
  instruction->raw_bits = AGX_SHADER_PUSH_EXEC_BYTE_3_LOW_IS_TWO | AGX_SHADER_PUSH_EXEC_BYTE_2_BIT_5 |
                          AGX_SHADER_PUSH_EXEC_BYTE_2_BIT_4_CLEAR;

  instruction =
    agx_shader_scale_index(&builder, 1, 0, 0, (Agx_Shader_Index_Form)0, AGX_SHADER_INDEX_SOURCE_FORM_UNMARKED, false, false);
  if (instruction == NULL)
  {
    return 0;
  }
  instruction->scale_index_short_form = true;
  instruction->scale_index_reverse = true;
  instruction->second_is_register = true;
  instruction->source_b = 48;
  instruction->raw_bits = AGX_SHADER_SCALE_INDEX_SHORT_BYTE_2_IS_0X24 | AGX_SHADER_SCALE_INDEX_SHORT_BYTE_4_BIT_0 |
                          AGX_SHADER_SCALE_INDEX_SHORT_BYTE_8_BIT_4 |
                          ((uint32_t)6u << AGX_SHADER_SCALE_INDEX_SHORT_BYTE_6_LOW_SHIFT) |
                          ((uint32_t)5u << AGX_SHADER_SCALE_INDEX_SHORT_BYTE_7_MID_SHIFT);

  instruction = agx_shader_output_move(&builder, 0, 1, false);
  if (instruction == NULL)
  {
    return 0;
  }
  instruction->long_form = true;
  instruction->raw_bits =
    AGX_SHADER_OUTPUT_MOVE_BYTE_2_BIT_0_CLEAR | ((uint32_t)0xe1u << AGX_SHADER_OUTPUT_MOVE_BYTE_3_SHIFT);
  instruction->output_move_byte_4 = 0x02u;

  instruction = agx_shader_bit_unary(&builder, 1, 0, AGX_SHADER_BIT_UNARY_POPCOUNT);
  if (instruction == NULL)
  {
    return 0;
  }
  instruction->source_form = AGX_SHADER_SOURCE_FORM_DISCARD;

  if (agx_shader_raw(&builder, k_unnamed_72, sizeof(k_unnamed_72)) == NULL)
  {
    return 0;
  }

  instruction = agx_shader_load(&builder, 12, 2, 7, 1, 0x168u, 0, AGX_SHADER_LOAD_FORM_DYNAMIC_WIDE, 4);
  if (instruction == NULL)
  {
    return 0;
  }
  instruction->raw_bits = AGX_SHADER_LOAD_BYTE_2_BIT_4_CLEAR;

  instruction = agx_shader_output_move(&builder, 0, 71, false);
  if (instruction == NULL)
  {
    return 0;
  }
  instruction->long_form = true;
  instruction->raw_bits = AGX_SHADER_OUTPUT_MOVE_BYTE_2_BIT_3_CLEAR |
                          ((uint32_t)0x9fu << AGX_SHADER_OUTPUT_MOVE_BYTE_3_SHIFT) |
                          ((uint32_t)0x02u << AGX_SHADER_OUTPUT_MOVE_BYTE_5_SHIFT);
  instruction->output_move_byte_4 = 0x10u;

  instruction = agx_shader_shift_left(&builder, 10, 0, 0);
  if (instruction == NULL)
  {
    return 0;
  }
  instruction->shift_amount_is_register = true;
  instruction->raw_bits = AGX_SHADER_SHIFT_BYTE_5_BIT_1 | AGX_SHADER_SHIFT_BYTE_8_BIT_5_CLEAR |
                          AGX_SHADER_SHIFT_BYTE_10_BIT_0_CLEAR | ((uint32_t)2u << AGX_SHADER_SHIFT_BYTE_9_LOW_SHIFT);

  instruction = agx_shader_output_move(&builder, 0, 74, false);
  if (instruction == NULL)
  {
    return 0;
  }
  instruction->long_form = true;
  instruction->raw_bits = AGX_SHADER_OUTPUT_MOVE_BYTE_2_BIT_3_CLEAR | AGX_SHADER_OUTPUT_MOVE_BYTE_4_CLEAR |
                          ((uint32_t)0x13u << AGX_SHADER_OUTPUT_MOVE_BYTE_3_SHIFT) |
                          ((uint32_t)0x02u << AGX_SHADER_OUTPUT_MOVE_BYTE_5_SHIFT);

  instruction = agx_shader_move(&builder, 48, 10);
  if (instruction == NULL)
  {
    return 0;
  }
  instruction->move_byte_1_bit_7 = true;
  instruction->move_keeps_source = true;
  instruction->raw_bits = AGX_SHADER_MOVE_BYTE_1_BIT_0 | AGX_SHADER_MOVE_BYTE_2_BIT_1 | AGX_SHADER_MOVE_BYTE_2_BIT_4 |
                          AGX_SHADER_MOVE_BYTE_2_BIT_5;
  instruction->move_byte_3 = 0x61u;

  instruction = agx_shader_compare_immediate(&builder, 0, AGX_SHADER_COMPARE_GREATER, 0);
  if (instruction == NULL)
  {
    return 0;
  }
  instruction->long_form = true;
  instruction->compare_type = AGX_SHADER_COMPARE_TYPE_SIGNED;
  instruction->discard_source = true;
  instruction->raw_bits = ((uint32_t)2u << AGX_SHADER_COMPARE_BYTE_0_HIGH_SHIFT) | AGX_SHADER_COMPARE_BYTE_6_BIT_0;

  instruction = agx_shader_compare_immediate(&builder, 48, AGX_SHADER_COMPARE_GREATER, 0);
  if (instruction == NULL)
  {
    return 0;
  }
  instruction->long_form = true;
  instruction->compare_type = AGX_SHADER_COMPARE_TYPE_SIGNED;
  instruction->raw_bits = AGX_SHADER_COMPARE_BYTE_6_BIT_0;

  instruction = agx_shader_push_exec(&builder, false);
  if (instruction == NULL)
  {
    return 0;
  }
  instruction->mask_source = AGX_SHADER_MASK_SOURCE_NONE;

  if (agx_shader_raw(&builder, k_tail_cut, sizeof(k_tail_cut)) == NULL)
  {
    return 0;
  }
  return agx_shader_encode(&builder, bytes, capacity);
}

static uint32_t
agx_background_object_program_654c0_build(uint8_t* bytes, uint32_t capacity)
{
  static const uint8_t k_head_cut[2] = {0x00, 0x00};
  static const uint8_t k_tail_cut[2] = {0x0f, 0x05};

  static const struct
  {
    bool     second_is_register;
    uint8_t  value;
    uint32_t distance;
    bool     cut;
  } k_arms[] = {
    {true, 63, 0x653cu, false},
    {true, 56, 0x2f5cu, false},
    {false, 0xd6u, 0x116eu, false},
    {false, 0xb1u, 0x09deu, false},
    {false, 0x94u, 0x052au, false},
    {false, 0x82u, 0x0000u, true},
  };

  Agx_Shader_Instruction  storage[48] = {0};
  Agx_Shader_Builder      builder = {0};
  Agx_Shader_Instruction* instruction = NULL;
  uint32_t                i = 0;

  agx_shader_builder_init(&builder, storage, sizeof(storage) / sizeof(storage[0]));

  if (agx_shader_raw(&builder, k_head_cut, sizeof(k_head_cut)) == NULL)
  {
    return 0;
  }
  for (i = 0; i < 2; i += 1)
  {
    instruction = agx_shader_pop_exec(&builder, 1);
    if (instruction == NULL)
    {
      return 0;
    }
    instruction->raw_bits = 0x20u;
  }
  instruction = agx_shader_pop_exec_short(&builder, 0x19);
  if (instruction == NULL)
  {
    return 0;
  }
  instruction->raw_bits = 0x20u;

  if (agx_shader_jump_exec_none_bytes(&builder, 0xd432u) == NULL)
  {
    return 0;
  }

  instruction = agx_shader_load(&builder, 8, 2, 7, 4, 0x08u, 5, AGX_SHADER_LOAD_FORM_DYNAMIC_WIDE, 16);
  if (instruction == NULL)
  {
    return 0;
  }
  instruction->index_is_pair = true;
  instruction->raw_bits = AGX_SHADER_LOAD_BYTE_2_BIT_4_CLEAR;

  instruction = agx_shader_shift_right(&builder, 0, 12, 8);
  if (instruction == NULL)
  {
    return 0;
  }
  instruction->first_in_program = true;
  instruction->raw_bits = 0x02u;
  instruction->shift_source_mask_bits = 1;

  instruction = agx_shader_output_move(&builder, 1, 76, false);
  if (instruction == NULL)
  {
    return 0;
  }
  instruction->long_form = true;
  instruction->raw_bits = AGX_SHADER_OUTPUT_MOVE_BYTE_2_BIT_3_CLEAR |
                          ((uint32_t)0x9fu << AGX_SHADER_OUTPUT_MOVE_BYTE_3_SHIFT) |
                          ((uint32_t)0x02u << AGX_SHADER_OUTPUT_MOVE_BYTE_5_SHIFT);
  instruction->output_move_byte_4 = 0x10u;

  instruction = agx_shader_scale_index(
    &builder, 2, 1, 0x38u + (0x70u << 6), (Agx_Shader_Index_Form)1, AGX_SHADER_INDEX_SOURCE_FORM_DISCARD, false, false
  );
  if (instruction == NULL)
  {
    return 0;
  }
  instruction->scale_index_reverse = true;
  instruction->scale_index_addend = 0x0eu;
  instruction->raw_bits = AGX_SHADER_SCALE_INDEX_BYTE_4_BIT_0;

  instruction = agx_shader_shift_right(&builder, 1, 12, 8);
  if (instruction == NULL)
  {
    return 0;
  }
  instruction->raw_bits = 0x02u;

  instruction = agx_shader_move(&builder, 2, 2);
  if (instruction == NULL)
  {
    return 0;
  }
  instruction->move_writes_zero = true;
  instruction->raw_bits = AGX_SHADER_MOVE_BYTE_1_BIT_0 | AGX_SHADER_MOVE_BYTE_2_BIT_2 | AGX_SHADER_MOVE_BYTE_2_BIT_4 |
                          AGX_SHADER_MOVE_BYTE_2_BIT_5;
  instruction->move_byte_3 = 0x01u;

  instruction = agx_shader_shift_right(&builder, 0, 12, 5);
  if (instruction == NULL)
  {
    return 0;
  }
  instruction->raw_bits = 0x02u;
  instruction->shift_source_mask_bits = 3;

  instruction = agx_shader_output_move(&builder, 1, 1, false);
  if (instruction == NULL)
  {
    return 0;
  }
  instruction->long_form = true;
  instruction->raw_bits = AGX_SHADER_OUTPUT_MOVE_BYTE_4_CLEAR | ((uint32_t)0x8fu << AGX_SHADER_OUTPUT_MOVE_BYTE_3_SHIFT) |
                          ((uint32_t)0x02u << AGX_SHADER_OUTPUT_MOVE_BYTE_5_SHIFT);

  instruction =
    agx_shader_scale_index(&builder, 1, 1, 0, (Agx_Shader_Index_Form)0, AGX_SHADER_INDEX_SOURCE_FORM_DISCARD, false, false);
  if (instruction == NULL)
  {
    return 0;
  }
  instruction->scale_index_short_form = true;
  instruction->second_is_register = true;
  instruction->source_b = 2;
  instruction->scale_index_source_is_register = true;
  instruction->raw_bits = AGX_SHADER_SCALE_INDEX_SHORT_BYTE_4_BIT_0 | AGX_SHADER_SCALE_INDEX_SHORT_BYTE_8_BIT_1 |
                          AGX_SHADER_SCALE_INDEX_SHORT_BYTE_8_BIT_4 |
                          ((uint32_t)5u << AGX_SHADER_SCALE_INDEX_SHORT_BYTE_7_MID_SHIFT);

  instruction = agx_shader_scale_index(
    &builder, 0, 0, 0x0eu + (0x10u << 6), (Agx_Shader_Index_Form)5, AGX_SHADER_INDEX_SOURCE_FORM_DISCARD, false, false
  );
  if (instruction == NULL)
  {
    return 0;
  }
  instruction->scale_index_addend = 0x02u;
  instruction->scale_index_add_register = true;
  instruction->raw_bits =
    AGX_SHADER_SCALE_INDEX_BYTE_9_BIT_0 | AGX_SHADER_SCALE_INDEX_BYTE_9_BIT_3 | AGX_SHADER_SCALE_INDEX_BYTE_4_BIT_0;

  for (i = 0; i < sizeof(k_arms) / sizeof(k_arms[0]); i += 1)
  {
    instruction = agx_shader_compare_immediate(
      &builder, 64, AGX_SHADER_COMPARE_LESS, k_arms[i].second_is_register ? 0u : k_arms[i].value
    );
    if (instruction == NULL)
    {
      return 0;
    }
    if (k_arms[i].second_is_register)
    {
      instruction->second_is_register = true;
      instruction->compare_second = k_arms[i].value;
      instruction->compare_second_is_half = false;
    }
    instruction->compare_type = AGX_SHADER_COMPARE_TYPE_SIGNED;
    instruction->raw_bits = (uint32_t)2u << AGX_SHADER_COMPARE_BYTE_0_HIGH_SHIFT;

    if (k_arms[i].cut)
    {
      break;
    }

    if (i == 0)
    {
      instruction = agx_shader_shift_right(&builder, 1, 12, 12);
      if (instruction == NULL)
      {
        return 0;
      }
      instruction->raw_bits = AGX_SHADER_SHIFT_BYTE_4_IS_TWO | AGX_SHADER_SHIFT_BYTE_8_BIT_4_CLEAR |
                              ((uint32_t)1u << AGX_SHADER_SHIFT_BYTE_9_LOW_SHIFT);
      instruction->shift_source_mask_bits = 8;
    }

    instruction = agx_shader_push_exec(&builder, false);
    if (instruction == NULL)
    {
      return 0;
    }
    instruction->mask_source = AGX_SHADER_MASK_SOURCE_NONE;

    if (agx_shader_jump_exec_none_bytes(&builder, k_arms[i].distance) == NULL)
    {
      return 0;
    }
  }

  if (agx_shader_raw(&builder, k_tail_cut, sizeof(k_tail_cut)) == NULL)
  {
    return 0;
  }
  return agx_shader_encode(&builder, bytes, capacity);
}

static uint32_t
agx_background_object_program_00240_build(uint8_t* bytes, uint32_t capacity)
{
  static const uint8_t k_head_cut[6] = {0x0a, 0x00, 0x0c, 0x00, 0x00, 0x00};
  static const uint8_t k_tail_cut[2] = {0xcc, 0xce};

  static const struct
  {
    uint8_t  destination;
    uint32_t immediate;
    uint32_t carried;
  } k_moves[] = {
    {24, 0x320u, AGX_SHADER_MOVE_IMMEDIATE_BYTE_2_BIT_3},
    {27, 0x2fcu, AGX_SHADER_MOVE_IMMEDIATE_DESTINATION_BIT_5},
    {9, 0x2c2u, AGX_SHADER_MOVE_IMMEDIATE_DESTINATION_BIT_5},
    {30, 0x2d4u, AGX_SHADER_MOVE_IMMEDIATE_DESTINATION_BIT_5},
    {1, 0x2b0u, AGX_SHADER_MOVE_IMMEDIATE_DESTINATION_BIT_5},
    {24, 0x166u, AGX_SHADER_MOVE_IMMEDIATE_DESTINATION_BIT_5},
    {25, 0x200u, AGX_SHADER_MOVE_IMMEDIATE_DESTINATION_BIT_5},
  };

  Agx_Shader_Instruction  storage[16] = {0};
  Agx_Shader_Builder      builder = {0};
  Agx_Shader_Instruction* instruction = NULL;
  uint32_t                i = 0;

  agx_shader_builder_init(&builder, storage, sizeof(storage) / sizeof(storage[0]));

  if (agx_shader_raw(&builder, k_head_cut, sizeof(k_head_cut)) == NULL)
  {
    return 0;
  }
  for (i = 0; i < sizeof(k_moves) / sizeof(k_moves[0]); i += 1)
  {
    instruction = agx_shader_move_immediate(&builder, k_moves[i].destination, k_moves[i].immediate);
    if (instruction == NULL)
    {
      return 0;
    }
    instruction->long_form = true;
    instruction->raw_bits |= k_moves[i].carried;
  }
  if (agx_shader_raw(&builder, k_tail_cut, sizeof(k_tail_cut)) == NULL)
  {
    return 0;
  }
  return agx_shader_encode(&builder, bytes, capacity);
}

static uint32_t
agx_background_object_program_66480_build(uint8_t* bytes, uint32_t capacity)
{
  static const uint8_t k_head_cut[6] = {0x02, 0x20, 0x48, 0xd0, 0x45, 0xc2};
  static const uint8_t k_tail_cut[2] = {0x2a, 0x81};

  Agx_Shader_Instruction  storage[16] = {0};
  Agx_Shader_Builder      builder = {0};
  Agx_Shader_Instruction* instruction = NULL;
  uint32_t                i = 0;

  agx_shader_builder_init(&builder, storage, sizeof(storage) / sizeof(storage[0]));

  instruction = agx_shader_nop(&builder);
  if (instruction == NULL)
  {
    return 0;
  }
  instruction->raw_bits = (uint32_t)5u << AGX_SHADER_NOP_BYTE_0_HIGH_SHIFT;

  if (agx_shader_raw(&builder, k_head_cut, sizeof(k_head_cut)) == NULL)
  {
    return 0;
  }

  instruction = agx_shader_convert_to_surface(
    &builder, 1, false, AGX_SHADER_CONVERT_FORM_SINGLE, 10, 0, AGX_SHADER_NUMERIC_FORMAT_UNORM8
  );
  if (instruction == NULL)
  {
    return 0;
  }

  instruction = agx_shader_store_surface(&builder, 1, AGX_SHADER_CHANNEL_WIDTH_8, 0);
  if (instruction == NULL)
  {
    return 0;
  }
  instruction->store_selector = 0xca;
  instruction->raw_bits = AGX_SHADER_STORE_SURFACE_BYTE_5_BIT_0 | ((uint32_t)0x04u << 8) | ((uint32_t)0x10u << 16);

  for (i = 0; i < 3; i += 1)
  {
    if (agx_shader_pop_exec(&builder, 1) == NULL)
    {
      return 0;
    }
  }
  if (agx_shader_pop_exec_short(&builder, 0x19) == NULL || agx_shader_jump_exec_none_bytes(&builder, 0x22e) == NULL ||
      agx_shader_raw(&builder, k_tail_cut, sizeof(k_tail_cut)) == NULL)
  {
    return 0;
  }
  return agx_shader_encode(&builder, bytes, capacity);
}

static uint32_t
agx_background_object_program_65f60_build(uint8_t* bytes, uint32_t capacity)
{
  static const struct
  {
    uint8_t  immediate;
    uint32_t distance;
    bool     cut;
  } k_arms[] = {
    {0xc1, 0x322u, false},
    {0xb9, 0x000u, true},
  };

  Agx_Shader_Instruction  storage[16] = {0};
  Agx_Shader_Builder      builder = {0};
  Agx_Shader_Instruction* instruction = NULL;
  uint32_t                i = 0;

  agx_shader_builder_init(&builder, storage, sizeof(storage) / sizeof(storage[0]));

  for (i = 0; i < 4; i += 1)
  {
    if (agx_shader_pop_exec(&builder, 1) == NULL)
    {
      return 0;
    }
  }
  if (agx_shader_pop_exec_short(&builder, 0x19) == NULL || agx_shader_jump_exec_none_bytes(&builder, 0x772) == NULL)
  {
    return 0;
  }

  for (i = 0; i < sizeof(k_arms) / sizeof(k_arms[0]); i += 1)
  {
    instruction = agx_shader_compare_immediate(&builder, 64, AGX_SHADER_COMPARE_LESS, k_arms[i].immediate);
    if (instruction == NULL)
    {
      return 0;
    }
    instruction->compare_type = AGX_SHADER_COMPARE_TYPE_SIGNED;
    instruction->raw_bits = (uint32_t)2u << AGX_SHADER_COMPARE_BYTE_0_HIGH_SHIFT;

    if (k_arms[i].cut)
    {
      break;
    }

    instruction = agx_shader_push_exec(&builder, false);
    if (instruction == NULL)
    {
      return 0;
    }
    instruction->mask_source = AGX_SHADER_MASK_SOURCE_NONE;

    if (agx_shader_jump_exec_none_bytes(&builder, k_arms[i].distance) == NULL)
    {
      return 0;
    }
  }
  return agx_shader_encode(&builder, bytes, capacity);
}

static uint32_t
agx_background_object_program_72900_build(uint8_t* bytes, uint32_t capacity)
{
  Agx_Shader_Instruction  storage[12] = {0};
  Agx_Shader_Builder      builder = {0};
  Agx_Shader_Instruction* instruction = NULL;

  agx_shader_builder_init(&builder, storage, sizeof(storage) / sizeof(storage[0]));

  instruction = agx_shader_move_immediate(&builder, 0, 1);
  if (instruction == NULL)
  {
    return 0;
  }
  instruction->destination_half = true;

  if (agx_shader_pad(&builder, 2) == NULL || agx_shader_pop_exec(&builder, 1) == NULL)
  {
    return 0;
  }

  instruction = agx_shader_jump_exec_any(&builder, 0);
  if (instruction == NULL)
  {
    return 0;
  }
  instruction->back_distance = -468292;
  instruction->raw_bits = 0x22u | AGX_SHADER_JUMP_ANY_BYTE_1_HIGH | (3u << AGX_SHADER_JUMP_ANY_BYTE_2_SHIFT);

  if (agx_shader_pop_exec(&builder, 2) == NULL || agx_shader_pop_exec(&builder, 1) == NULL ||
      agx_shader_stop(&builder) == NULL)
  {
    return 0;
  }

  return agx_shader_encode(&builder, bytes, capacity);
}

static uint32_t
agx_background_object_program_00100_build(uint8_t* bytes, uint32_t capacity)
{
  static const struct
  {
    uint8_t  destination;
    uint32_t immediate;
    uint32_t carried;
  } k_moves[] = {
    {31, 0x29c, AGX_SHADER_MOVE_IMMEDIATE_DESTINATION_BIT_5},
    {14, 0x3d0, AGX_SHADER_MOVE_IMMEDIATE_BYTE_2_BIT_3},
    {15, 0x46a, 0u},
    {17, 0x4b8, 0u},
    {19, 0x4e0, 0u},
  };

  Agx_Shader_Instruction  storage[16] = {0};
  Agx_Shader_Builder      builder = {0};
  Agx_Shader_Instruction* instruction = NULL;
  uint32_t                i = 0;

  agx_shader_builder_init(&builder, storage, sizeof(storage) / sizeof(storage[0]));

  instruction = agx_shader_compare_immediate(&builder, 8, AGX_SHADER_COMPARE_LESS, 0);
  if (instruction == NULL)
  {
    return 0;
  }
  instruction->long_form = true;
  instruction->compare_type = AGX_SHADER_COMPARE_TYPE_SIGNED;
  instruction->raw_bits = AGX_SHADER_COMPARE_BYTE_4_BIT_7;

  if (agx_shader_push_exec(&builder, true) == NULL || agx_shader_jump_exec_none_bytes(&builder, 0x72810) == NULL)
  {
    return 0;
  }

  for (i = 0; i < sizeof(k_moves) / sizeof(k_moves[0]); i += 1)
  {
    instruction = agx_shader_move_immediate(&builder, k_moves[i].destination, k_moves[i].immediate);
    if (instruction == NULL)
    {
      return 0;
    }
    instruction->long_form = true;
    instruction->raw_bits = k_moves[i].carried;
  }

  return agx_shader_encode(&builder, bytes, capacity);
}

static uint32_t
agx_code_heap_block_bytes(uint32_t payload)
{
  return (AGX_CODE_HEAP_BLOCK_HEADER + payload + 0x3fu) & ~0x3fu;
}

static void
agx_code_heap_block_header_write(uint8_t* block, uint32_t block_bytes)
{
  memset(block, 0, AGX_CODE_HEAP_BLOCK_HEADER);
  memcpy(block, &block_bytes, sizeof(block_bytes));
}

bool
agx_background_object_program_create(Agx_Device device, Agx_Bo* program)
{
  enum
  {
    AGX_CLEAR_TILE_DESTINATION = 0xce
  };

  enum
  {
    AGX_BACKGROUND_OBJECT_PROGRAM_BYTES = 0x72928,
    AGX_BACKGROUND_OBJECT_ALLOCATION_BYTES = 0x74000
  };

  static const struct
  {
    uint32_t       offset;
    const uint8_t* bytes;
    uint32_t       size;
  } k_sections[] = {
    {0x00100, NULL, 0x40},
    {0x00240, NULL, 0x40},
    {0x003c0, NULL, 0x80},
    {0x654c0, NULL, 0x100},
    {0x65f60, NULL, 0x40},
    {0x66280, NULL, 0x80},
    {0x66480, NULL, 0x40},
    {0x665c0, NULL, 0x80},
    {0x72900, NULL, 0x40},
  };

  Agx_Bo_Desc desc = {0};
  uint8_t*    at = NULL;
  uint8_t*    payload = NULL;
  uint32_t    n = 0;

  agx_bo_desc_init(&desc);
  agx_bo_desc_set_size(&desc, AGX_BACKGROUND_OBJECT_ALLOCATION_BYTES);
  *program = agx_bo_alloc(device, &desc);

  if (!program->cpu)
  {
    agx_refuse("agx_background_object_program_create: !program->cpu");
    return false;
  }

  at = (uint8_t*)program->cpu;
  agx_code_heap_block_header_write(at, agx_code_heap_block_bytes(AGX_BACKGROUND_OBJECT_PROGRAM_BYTES));
  payload = at + AGX_CODE_HEAP_BLOCK_HEADER;

  for (n = 0; n < sizeof(k_sections) / sizeof(k_sections[0]); ++n)
  {
    uint32_t wrote = 0;

    if (k_sections[n].bytes)
    {
      memcpy(payload + k_sections[n].offset, k_sections[n].bytes, k_sections[n].size);
      wrote = k_sections[n].size;
    }
    else if (k_sections[n].offset == 0x00100)
    {
      wrote = agx_background_object_program_00100_build(payload + k_sections[n].offset, k_sections[n].size);
    }
    else if (k_sections[n].offset == 0x66280)
    {
      wrote = agx_background_object_program_66280_build(payload + k_sections[n].offset, k_sections[n].size);
    }
    else if (k_sections[n].offset == 0x003c0)
    {
      wrote = agx_background_object_program_003c0_build(payload + k_sections[n].offset, k_sections[n].size);
    }
    else if (k_sections[n].offset == 0x654c0)
    {
      wrote = agx_background_object_program_654c0_build(payload + k_sections[n].offset, k_sections[n].size);
    }
    else if (k_sections[n].offset == 0x00240)
    {
      wrote = agx_background_object_program_00240_build(payload + k_sections[n].offset, k_sections[n].size);
    }
    else if (k_sections[n].offset == 0x66480)
    {
      wrote = agx_background_object_program_66480_build(payload + k_sections[n].offset, k_sections[n].size);
    }
    else if (k_sections[n].offset == 0x65f60)
    {
      wrote = agx_background_object_program_65f60_build(payload + k_sections[n].offset, k_sections[n].size);
    }
    else if (k_sections[n].offset == 0x72900)
    {
      wrote = agx_background_object_program_72900_build(payload + k_sections[n].offset, k_sections[n].size);
    }
    else
    {
      wrote = agx_background_object_pixel_range_build(
        AGX_CLEAR_TILE_DESTINATION, payload + k_sections[n].offset, k_sections[n].size
      );
    }

    if (wrote == 0)
    {
      agx_refuse("agx_background_object_program_create: wrote == 0");
      return false;
    }
  }
  return true;
}

uint32_t
agx_usc_global_configuration_init_build(uint8_t* bytes, uint32_t capacity)
{
  static const uint8_t k_stub[8] = {0xf7, 0x03, 0xaa, 0x00, 0x8f, 0x02, 0x54, 0x01};

  enum
  {
    AGX_DISPATCH_TABLE_ENTRIES = 10,
    AGX_DISPATCH_TABLE_STRIDE = 0x10,
    AGX_DISPATCH_TABLE_STUB = 0xa0
  };

  uint32_t table, i;

  if (capacity < AGX_USC_GLOBAL_CONFIGURATION_INIT_BYTES)
  {
    agx_refuse("agx_usc_global_configuration_init_build: capacity < AGX_USC_GLOBAL_CONFIGURATION_INIT_BYTES");
    return 0;
  }
  for (i = 0; i < AGX_USC_GLOBAL_CONFIGURATION_INIT_BYTES; i += 2)
  {
    bytes[i] = 0x06;
    bytes[i + 1] = 0x00;
  }
  for (table = 0xc0; table <= 0x1c0; table += 0x100)
  {
    for (i = 0; i < AGX_DISPATCH_TABLE_ENTRIES; ++i)
    {
      uint8_t* jump = bytes + table + i * AGX_DISPATCH_TABLE_STRIDE;
      memset(jump, 0, 10);
      jump[0] = 0x0f;
      jump[1] = 0x00;
      jump[2] = 0x54;
      jump[3] = (uint8_t)(AGX_DISPATCH_TABLE_STUB - i * AGX_DISPATCH_TABLE_STRIDE);
    }
    memcpy(bytes + table + AGX_DISPATCH_TABLE_STUB, k_stub, sizeof(k_stub));
  }
  return AGX_USC_GLOBAL_CONFIGURATION_INIT_BYTES;
}

bool
agx_shader_pool_create(Agx_Device device, uint32_t bytes, Agx_Bo* pool)
{
  Agx_Bo_Desc desc = {0};
  uint8_t*    at = NULL;
  uint32_t    block = 0;

  agx_bo_desc_init(&desc);
  agx_bo_desc_set_size(&desc, bytes);
  *pool = agx_bo_alloc(device, &desc);

  if (!pool->cpu)
  {
    agx_refuse("agx_shader_pool_create: !pool->cpu");
    return false;
  }

  at = (uint8_t*)pool->cpu;
  block = agx_code_heap_block_bytes(AGX_USC_GLOBAL_CONFIGURATION_INIT_BYTES);
  if (bytes < block + AGX_CODE_HEAP_BLOCK_HEADER)
  {
    agx_refuse("agx_shader_pool_create: bytes < block + AGX_CODE_HEAP_BLOCK_HEADER");
    return false;
  }
  agx_code_heap_block_header_write(at, block);
  if (agx_usc_global_configuration_init_build(at + AGX_CODE_HEAP_BLOCK_HEADER, AGX_USC_GLOBAL_CONFIGURATION_INIT_BYTES) == 0)
  {
    agx_refuse(
      "agx_shader_pool_create: the USC global configuration init did not fit its %u byte block",
      (uint32_t)AGX_USC_GLOBAL_CONFIGURATION_INIT_BYTES
    );
    return false;
  }
  agx_code_heap_block_header_write(at + block, (bytes - block) & ~0x3fu);
  return true;
}

static void
agx_pool_layout_take(Agx_Pool_Layout* layout, uint32_t at, uint32_t bytes)
{
  if (layout->taken_count < AGX_POOL_LAYOUT_RESERVED)
  {
    layout->taken[layout->taken_count].at = at;
    layout->taken[layout->taken_count].bytes = bytes;
    layout->taken_count += 1;
  }
}

void
agx_pool_layout_init(Agx_Pool_Layout* layout, const Agx_Bo* pool, uint32_t first)
{
  uint32_t block0 = 0;

  if (layout == NULL)
  {
    return;
  }
  memset(layout, 0, sizeof(*layout));
  if (pool == NULL || pool->cpu == NULL)
  {
    return;
  }
  layout->cpu = (uint8_t*)pool->cpu;
  layout->gpu_va = pool->gpu_va;
  layout->size = (uint32_t)pool->size;
  layout->vertex_stage_entry = AGX_POOL_NOWHERE;
  layout->state_loader = AGX_POOL_NOWHERE;
  layout->end_of_tile_second_program = AGX_POOL_NOWHERE;

  memcpy(&block0, layout->cpu, sizeof(block0));
  if (block0 == 0 || block0 > layout->size)
  {
    block0 = agx_code_heap_block_bytes(AGX_USC_GLOBAL_CONFIGURATION_INIT_BYTES);
  }
  agx_pool_layout_take(layout, 0, block0);
  layout->cursor = first > block0 ? first : block0;
}

uint32_t
agx_pool_append(Agx_Pool_Layout* layout, uint32_t bytes, uint32_t alignment)
{
  uint32_t at, i;
  bool     moved = false;

  if (layout == NULL || layout->cpu == NULL || bytes == 0)
  {
    return AGX_POOL_NOWHERE;
  }
  if (alignment == 0)
  {
    alignment = 1;
  }

  at = (layout->cursor + alignment - 1u) & ~(alignment - 1u);
  do
  {
    moved = false;
    for (i = 0; i < layout->taken_count; ++i)
    {
      uint32_t start = layout->taken[i].at;
      uint32_t end = start + layout->taken[i].bytes;

      if (at < end && start < at + bytes)
      {
        at = (end + alignment - 1u) & ~(alignment - 1u);
        moved = true;
      }
    }
  } while (moved);

  if ((uint64_t)at + bytes > layout->size)
  {
    return AGX_POOL_NOWHERE;
  }
  agx_pool_layout_take(layout, at, bytes);
  layout->cursor = at + bytes;
  return at;
}

uint32_t
agx_pool_append_program(Agx_Pool_Layout* layout, const uint8_t* program, uint32_t bytes)
{
  uint32_t block_bytes, block, remainder;

  if (layout == NULL || layout->cpu == NULL || program == NULL || bytes == 0)
  {
    return AGX_POOL_NOWHERE;
  }
  block_bytes = agx_code_heap_block_bytes(bytes);
  block = agx_pool_append(layout, block_bytes, AGX_CODE_HEAP_BLOCK_HEADER);
  if (block == AGX_POOL_NOWHERE)
  {
    return AGX_POOL_NOWHERE;
  }
  agx_code_heap_block_header_write(layout->cpu + block, block_bytes);
  memcpy(layout->cpu + block + AGX_CODE_HEAP_BLOCK_HEADER, program, bytes);

  if (layout->cursor + AGX_CODE_HEAP_BLOCK_HEADER <= layout->size)
  {
    remainder = (layout->size - layout->cursor) & ~0x3fu;
    agx_code_heap_block_header_write(layout->cpu + layout->cursor, remainder);
  }
  return block + AGX_CODE_HEAP_BLOCK_HEADER;
}

bool
agx_pool_append_end_of_tile_program(Agx_Pool_Layout* layout)
{
  uint8_t  second_program[AGX_END_OF_TILE_SECOND_PROGRAM_BYTES] = {0};
  uint32_t second = 0;
  uint32_t i = 0;

  if (layout == NULL || layout->cpu == NULL)
  {
    agx_refuse("agx_pool_append_end_of_tile_program: layout == NULL || layout->cpu == NULL");
    return false;
  }

  memset(second_program, 0, sizeof(second_program));
  for (i = 0; i < AGX_END_OF_TILE_SECOND_RANGE_COUNT; ++i)
  {
    memcpy(
      second_program + agx_end_of_tile_second_ranges[i].offset,
      agx_end_of_tile_second_ranges[i].code,
      agx_end_of_tile_second_ranges[i].bytes
    );
  }

  second = agx_pool_append_program(layout, second_program, AGX_END_OF_TILE_SECOND_PROGRAM_BYTES);
  if (second == AGX_POOL_NOWHERE)
  {
    agx_refuse("agx_pool_append_end_of_tile_program: second == AGX_POOL_NOWHERE");
    return false;
  }
  layout->end_of_tile_second_program = second;
  return true;
}

bool
agx_pool_append_driver_programs(Agx_Pool_Layout* layout)
{
  uint32_t program, tail, epilogue, gap, loader, tile;
  uint32_t program_end, next_occupant, i;

  if (layout == NULL || layout->cpu == NULL)
  {
    agx_refuse("agx_pool_append_driver_programs: layout == NULL || layout->cpu == NULL");
    return false;
  }

  if (layout->cursor + AGX_POOL_VERTEX_STAGE_ENTRY_PREFIX < AGX_POOL_VERTEX_STAGE_ENTRY_LOWEST)
  {
    layout->cursor = AGX_POOL_VERTEX_STAGE_ENTRY_LOWEST - AGX_POOL_VERTEX_STAGE_ENTRY_PREFIX;
  }

  layout->cursor =
    (((layout->cursor + AGX_POOL_VERTEX_STAGE_ENTRY_PREFIX + AGX_POOL_VERTEX_STAGE_ENTRY_ALIGNMENT - 1u) &
      ~(AGX_POOL_VERTEX_STAGE_ENTRY_ALIGNMENT - 1u)) -
     AGX_POOL_VERTEX_STAGE_ENTRY_PREFIX);

  program = agx_pool_append(layout, AGX_POOL_VERTEX_STAGE_PROGRAM_BYTES, 4u);
  if (program == AGX_POOL_NOWHERE || program + AGX_POOL_VERTEX_STAGE_ENTRY_PREFIX < AGX_POOL_VERTEX_STAGE_ENTRY_LOWEST ||
      ((program + AGX_POOL_VERTEX_STAGE_ENTRY_PREFIX) & (AGX_POOL_VERTEX_STAGE_ENTRY_ALIGNMENT - 1u)) != 0u)
  {
    agx_refuse(
      "agx_pool_append_driver_programs: the vertex stage program did not fit, landed below its measured floor, or its "
      "entry is not 64-byte aligned"
    );
    return false;
  }
  layout->vertex_stage_entry = program + AGX_POOL_VERTEX_STAGE_ENTRY_PREFIX;

  tail = agx_pool_append(layout, AGX_SHADER_POOL_VERTEX_STAGE_TAIL_BYTES, 4u);
  epilogue = agx_pool_append(layout, 0xdau, 4u);
  gap = agx_pool_append(layout, 0x40u, 4u);
  loader = agx_pool_append(layout, 0x144u, 4u);
  tile = agx_pool_append(layout, 0x100u, 4u);
  if (tail == AGX_POOL_NOWHERE || epilogue == AGX_POOL_NOWHERE || gap == AGX_POOL_NOWHERE ||
      loader == AGX_POOL_NOWHERE || tile == AGX_POOL_NOWHERE)
  {
    agx_refuse(
      "agx_pool_append_driver_programs: one of the tail, epilogue, gap, loader or tile block did not fit the pool"
    );
    return false;
  }
  layout->state_loader = loader;

  program_end = layout->vertex_stage_entry - AGX_POOL_VERTEX_STAGE_ENTRY_PREFIX + AGX_POOL_VERTEX_STAGE_PROGRAM_BYTES;
  next_occupant = layout->size;
  for (i = 0; i < layout->taken_count; ++i)
  {
    uint32_t start = layout->taken[i].at;

    if (start >= program_end && start < next_occupant && start != tail)
    {
      next_occupant = start;
    }
  }

  if (agx_pool_vertex_stage_program_place(layout->cpu, layout->vertex_stage_entry, layout->size) == 0 ||
      agx_pool_vertex_stage_tail_run_build(
        layout->cpu, tail, layout->size, layout->vertex_stage_entry, epilogue, next_occupant
      ) == 0 ||
      agx_pool_texture_epilogue_place(layout->cpu, epilogue, layout->size) == 0 ||
      agx_pool_uniform_program_gap_place(layout->cpu, gap, layout->size) == 0 ||
      agx_pool_uniform_program_place(layout->cpu, loader, layout->size) == 0 ||
      agx_pool_tile_block_place(layout->cpu, tile, layout->size) == 0)
  {
    agx_refuse("agx_pool_append_driver_programs: one of the six driver windows refused to place");
    return false;
  }
  return true;
}

bool
agx_tile_programs_create(
  Agx_Device         device,
  const Agx_Bo*      driver_arguments,
  const Agx_Bo*      background_object_program,
  const Agx_Bo*      shader_pool,
  uint32_t           store_shader,
  const Agx_Bo*      tile_arguments,
  Agx_Tile_Programs* programs
)
{
  enum
  {
    AGX_PARTIAL_BACKGROUND_PROGRAM_AT = 0x000,
    AGX_PARTIAL_BACKGROUND_TAIL_AT = 0x1c0,
    AGX_BACKGROUND_PROGRAM_AT = 0x240,
    AGX_TILE_START_TAIL_AT = 0x400,
    AGX_STORE_PROGRAM_ENTRY = 0x480,
    AGX_STORE_PROGRAM_ARGUMENT_BLOCK = 0x600,
    AGX_BACKGROUND_PROGRAM_ARGUMENT_BLOCK = 0x300,
    AGX_CLEAR_BACKGROUND_SHADER_POOL_OFFSET = 0x140,
  };

  const uint64_t AGX_BACKGROUND_PROGRAM_TILE_ARGUMENTS = tile_arguments->gpu_va + 0x3e0;

  const uint64_t AGX_STORE_PROGRAM_TILE_ARGUMENTS = tile_arguments->gpu_va + 0x4e0;

  Agx_Bo_Desc desc = {0};
  agx_bo_desc_init(&desc);
  agx_bo_desc_set_space(&desc, AGX_BO_SPACE_CMDBUF);
  agx_bo_desc_set_size(&desc, 0x8000);
  programs->heap = agx_bo_alloc(device, &desc);

  if (!programs->heap.cpu)
  {
    agx_refuse("agx_tile_programs_create: !programs->heap.cpu");
    return false;
  }

  uint8_t* at = (uint8_t*)programs->heap.cpu;

  if (agx_background_program_build(
        driver_arguments->gpu_va ? driver_arguments->gpu_va : 0x100001b0000ull,
        AGX_BACKGROUND_PROGRAM_TILE_ARGUMENTS,
        background_object_program->gpu_va ? background_object_program->gpu_va : 0x240000ull,
        shader_pool->gpu_va,
        AGX_CLEAR_BACKGROUND_SHADER_POOL_OFFSET,
        NULL,
        0,
        at + AGX_PARTIAL_BACKGROUND_PROGRAM_AT,
        0x140
      ) == 0 ||
      agx_background_program_build(
        driver_arguments->gpu_va ? driver_arguments->gpu_va + AGX_BACKGROUND_PROGRAM_ARGUMENT_BLOCK : 0x100001b0300ull,
        AGX_BACKGROUND_PROGRAM_TILE_ARGUMENTS,
        background_object_program->gpu_va ? background_object_program->gpu_va : 0x240000ull,
        shader_pool->gpu_va,
        AGX_CLEAR_BACKGROUND_SHADER_POOL_OFFSET,
        NULL,
        0,
        at + AGX_BACKGROUND_PROGRAM_AT,
        0x140
      ) == 0 ||
      agx_tile_start_tail_build(at + AGX_TILE_START_TAIL_AT, AGX_TILE_START_TAIL_BYTES) == 0)
  {
    agx_refuse("agx_tile_programs_create: a background program or the tile start tail did not fit");
    return false;
  }

  if (agx_tile_start_tail_build(at + AGX_PARTIAL_BACKGROUND_TAIL_AT, AGX_TILE_START_TAIL_BYTES) == 0)
  {
    agx_refuse("agx_tile_programs_create: the partial background program's tail did not fit");
    return false;
  }

  if (agx_store_program_build(
        driver_arguments->gpu_va ? driver_arguments->gpu_va + AGX_STORE_PROGRAM_ARGUMENT_BLOCK : 0x100001b0600ull,
        AGX_STORE_PROGRAM_TILE_ARGUMENTS,
        shader_pool->gpu_va,
        store_shader,
        at + AGX_STORE_PROGRAM_ENTRY,
        0x1c0
      ) == 0)
  {
    agx_refuse("agx_tile_programs_create: the store program did not fit its 0x1c0 bytes");
    return false;
  }

  programs->background = programs->heap.gpu_va + AGX_BACKGROUND_PROGRAM_AT;
  programs->store = programs->heap.gpu_va + AGX_STORE_PROGRAM_ENTRY;
  programs->shader_pool = shader_pool->gpu_va;

  programs->tile_arguments = tile_arguments->gpu_va;
  return true;
}

bool
agx_tile_programs_compile_background_words(
  Agx_Tile_Programs* programs,
  const Agx_Bo*      driver_arguments,
  const Agx_Bo*      background_object_program,
  const uint32_t     words[4],
  uint32_t           word_mask
)
{
  enum
  {
    AGX_BACKGROUND_PROGRAM_AT = 0x240,
    AGX_BACKGROUND_PROGRAM_ARGUMENT_BLOCK = 0x300,
    AGX_CLEAR_BACKGROUND_SHADER_POOL_OFFSET = 0x140,
    AGX_TILE_START_TAIL_AT = 0x400
  };

  if (!programs || !programs->heap.cpu || programs->heap.size < AGX_TILE_START_TAIL_AT)
  {
    return false;
  }

  return agx_background_program_build(
           driver_arguments->gpu_va ? driver_arguments->gpu_va + AGX_BACKGROUND_PROGRAM_ARGUMENT_BLOCK : 0x100001b0300ull,
           programs->tile_arguments + 0x3e0,
           background_object_program->gpu_va ? background_object_program->gpu_va : 0x240000ull,
           programs->shader_pool,
           AGX_CLEAR_BACKGROUND_SHADER_POOL_OFFSET,
           words,
           word_mask,
           (uint8_t*)programs->heap.cpu + AGX_BACKGROUND_PROGRAM_AT,
           AGX_TILE_START_TAIL_AT - AGX_BACKGROUND_PROGRAM_AT
         ) != 0;
}

void
agx_tile_programs_destroy(Agx_Device device, Agx_Tile_Programs* programs)
{
  agx_free_mem(device, &programs->heap);
  programs->background = 0;
  programs->store = 0;
}

void
agx_tile_programs_write_register_release(Agx_Tile_Programs* programs)
{
  enum
  {
    AGX_REGISTER_RELEASE_AT = 0x600,
    AGX_REGISTER_RELEASE_SIZE = 28
  };

  static const uint8_t k_released[] = {29, 30, 23, 24, 25, 26};

  Agx_Shader_Instruction  storage[8] = {0};
  Agx_Shader_Builder      builder = {0};
  Agx_Shader_Instruction* instruction = NULL;
  uint32_t                i = 0;

  if (!programs->heap.cpu || programs->heap.size < AGX_REGISTER_RELEASE_AT + AGX_REGISTER_RELEASE_SIZE)
  {
    return;
  }

  agx_shader_builder_init(&builder, storage, sizeof(storage) / sizeof(storage[0]));
  for (i = 0; i < sizeof(k_released) / sizeof(k_released[0]); ++i)
  {
    instruction = agx_shader_discard(&builder, k_released[i]);
    if (instruction)
    {
      instruction->destination = (uint8_t)(k_released[i] - 18u);
    }
  }
  agx_shader_stop(&builder);

  if (agx_shader_encode(&builder, (uint8_t*)programs->heap.cpu + AGX_REGISTER_RELEASE_AT, AGX_REGISTER_RELEASE_SIZE) == 0)
  {
    fprintf(
      stderr,
      "agx_tile_programs_write_register_release: the release did not fit its %u "
      "byte(s); the tile heap keeps whatever was there\n",
      (uint32_t)AGX_REGISTER_RELEASE_SIZE
    );
  }
}

void
agx_tile_programs_set_vertex_shader(Agx_Tile_Programs* programs, uint32_t shader_pool_offset)
{
  enum
  {
    AGX_VERTEX_SHADER_REFERENCE_AT = 0x668
  };

  if (!programs->heap.cpu || programs->heap.size < AGX_VERTEX_SHADER_REFERENCE_AT + 2)
  {
    return;
  }

  uint16_t reference = (uint16_t)(2u * shader_pool_offset + 0x2au);
  memcpy((uint8_t*)programs->heap.cpu + AGX_VERTEX_SHADER_REFERENCE_AT, &reference, sizeof(reference));
}

void
agx_render_pass_desc_set_tile_programs(Agx_Render_Pass_Desc* desc, Agx_Tile_Programs programs)
{
  desc->background_program = programs.background;
  desc->store_program = programs.store;
  desc->tile_program_heap = programs.heap.gpu_va;
}

static const uint8_t k_agx_buffer_copy_unnamed_c8[8] = {
  0x2c,
  0x88,
  0x09,
  0x27,
  0x60,
  0x00,
  0x00,
  0x80,
};
static const uint8_t k_agx_buffer_copy_unnamed_c4[4] = {0x3c, 0x8a, 0x08, 0x27};
static const uint8_t k_agx_buffer_copy_unnamed_78[8] = {
  0xa7,
  0x17,
  0x54,
  0x06,
  0x03,
  0x04,
  0x8e,
  0x20,
};
static const uint8_t k_agx_buffer_copy_unnamed_f10[10] = {
  0xaf,
  0x00,
  0x54,
  0x1a,
  0x00,
  0x0c,
  0x10,
  0x48,
  0x20,
  0x00,
};

uint32_t
agx_buffer_copy_program_build(uint8_t* bytes, uint32_t capacity, uint32_t operand_slot)
{
  static const struct
  {
    uint8_t  destination;
    uint32_t immediate;
    uint32_t carried;
  } k_setup[] = {
    {2, 0u, 0u},
    {3, 0x100u, 0u},
    {28, 0x892e0u, 0u},
    {29, 0x100u, 0u},
    {2, 0x402a0u, AGX_SHADER_MOVE_IMMEDIATE_DESTINATION_BIT_5},
    {3, 0x100u, AGX_SHADER_MOVE_IMMEDIATE_DESTINATION_BIT_5},
  };
  static const struct
  {
    uint8_t  destination;
    uint8_t  index;
    uint8_t  word_count;
    uint32_t offset;
    bool     release_base;
  } k_loads[] = {
    {26, 2, 2, 0x20u, false},
    {22, 2, 4, 0x10u, false},
    {18, 2, 4, 0x00u, false},
    {32, 28, 2, 0x8010u, false},
    {28, 28, 4, 0x8000u, true},
    {38, 34, 2, 0x10u, false},
    {34, 34, 4, 0x00u, true},
  };

  static const struct
  {
    uint8_t destination, source;
  } k_release[] = {
    {4, 22},  {5, 23},  {6, 24},  {7, 25},  {0, 18},  {1, 19},  {2, 20},  {3, 21},  {20, 32}, {21, 33},
    {16, 28}, {17, 29}, {18, 30}, {19, 31}, {26, 38}, {27, 39}, {22, 34}, {23, 35}, {24, 36}, {25, 37},
  };

  Agx_Shader_Instruction  storage[96] = {0};
  Agx_Shader_Builder      builder = {0};
  Agx_Shader_Instruction* instruction = NULL;
  uint32_t                i = 0;

  if (bytes == NULL || capacity < AGX_BUFFER_COPY_PROGRAM_BYTES)
  {
    agx_refuse("agx_buffer_copy_program_build: bytes == NULL || capacity < AGX_BUFFER_COPY_PROGRAM_BYTES");
    return 0;
  }

  agx_shader_builder_init(&builder, storage, sizeof(storage) / sizeof(storage[0]));

  for (i = 0; i < sizeof(k_setup) / sizeof(k_setup[0]); i += 1)
  {
    instruction =
      agx_shader_move_immediate(&builder, k_setup[i].destination, i == 0 ? operand_slot : k_setup[i].immediate);
    if (instruction == NULL)
    {
      return 0;
    }
    instruction->long_form = true;
    instruction->raw_bits |= k_setup[i].carried;
  }

  for (i = 0; i < sizeof(k_loads) / sizeof(k_loads[0]); i += 1)
  {
    instruction = agx_shader_load(
      &builder, k_loads[i].destination, 0, k_loads[i].index, k_loads[i].word_count, k_loads[i].offset, 1, AGX_SHADER_LOAD_FORM_ORDINARY, 4
    );
    if (instruction == NULL)
    {
      return 0;
    }
    instruction->base_discard = k_loads[i].release_base;
  }

  instruction = agx_shader_call_pool_shader(&builder, 0x2080u, 0);
  if (instruction == NULL)
  {
    return 0;
  }
  instruction->raw_bits = AGX_SHADER_CALL_BYTE_1_CLEAR | AGX_SHADER_CALL_BYTE_7_CLEAR;

  if (agx_shader_call_pool_shader(&builder, 0x5580u, 0) == NULL || agx_shader_nop(&builder) == NULL)
  {
    return 0;
  }

  instruction = agx_shader_call_pool_shader(&builder, 0, (uint64_t)0x18u << 15);
  if (instruction == NULL)
  {
    return 0;
  }
  instruction->raw_bits = AGX_SHADER_CALL_BYTE_0_BIT_7 | AGX_SHADER_CALL_BYTE_1_CLEAR | AGX_SHADER_CALL_BYTE_7_CLEAR;

  if (agx_shader_pad(&builder, 2) == NULL)
  {
    return 0;
  }

  instruction = agx_shader_move_immediate(&builder, 1, 0);
  if (instruction == NULL)
  {
    return 0;
  }
  instruction->long_form = true;

  instruction = agx_shader_get_special(&builder, 1, 0x81u);
  if (instruction == NULL)
  {
    return 0;
  }
  instruction->long_form = true;

  instruction = agx_shader_move_immediate(&builder, 0, 0x100u);
  if (instruction == NULL)
  {
    return 0;
  }
  instruction->long_form = true;

  instruction = agx_shader_add_index(&builder, 0, 1, 0, AGX_SHADER_INDEX_SOURCE_FORM_UNMARKED, true);
  if (instruction == NULL)
  {
    return 0;
  }
  instruction->add_second_source_is_register = true;
  instruction->source_b = 0;
  instruction->index_scaled = true;
  instruction->second_form = AGX_SHADER_OPERAND_FORM_KEEP;

  instruction = agx_shader_move_immediate(&builder, 1, 0x100u);
  if (instruction == NULL)
  {
    return 0;
  }
  instruction->long_form = true;

  if (agx_shader_end_thread_long(&builder, 0x4cu) == NULL)
  {
    return 0;
  }

  instruction = agx_shader_discard(&builder, 0);
  if (instruction == NULL)
  {
    return 0;
  }
  instruction->destination = 10;
  instruction->move_keeps_source = true;

  instruction = agx_shader_discard(&builder, 0);
  if (instruction == NULL)
  {
    return 0;
  }
  instruction->destination = 11;
  instruction->move_keeps_source = true;

  instruction = agx_shader_discard(&builder, 26);
  if (instruction == NULL)
  {
    return 0;
  }
  instruction->destination = 8;
  instruction->raw_bits |= AGX_SHADER_MOVE_UNKNOWN_BIT;
  instruction->move_keeps_source = true;

  instruction = agx_shader_output_move(&builder, 4, 26, false);
  if (instruction == NULL)
  {
    return 0;
  }
  instruction->long_form = true;
  instruction->raw_bits = AGX_SHADER_OUTPUT_MOVE_BYTE_2_BIT_5_CLEAR | 0x14u;
  instruction->output_move_byte_4 = 0x22u;

  instruction = agx_shader_discard(&builder, 27);
  if (instruction == NULL)
  {
    return 0;
  }
  instruction->destination = 9;
  instruction->move_keeps_source = true;

  instruction = agx_shader_output_move(&builder, 5, 27, false);
  if (instruction == NULL)
  {
    return 0;
  }
  instruction->long_form = true;
  instruction->raw_bits = AGX_SHADER_OUTPUT_MOVE_BYTE_2_BIT_5_CLEAR | 0x14u;
  instruction->output_move_byte_4 = 0x22u;

  for (i = 0; i < sizeof(k_release) / sizeof(k_release[0]); i += 1)
  {
    instruction = agx_shader_discard(&builder, k_release[i].source);
    if (instruction == NULL)
    {
      return 0;
    }
    instruction->destination = k_release[i].destination;
  }

  if (agx_shader_raw(&builder, k_agx_buffer_copy_unnamed_c8, sizeof(k_agx_buffer_copy_unnamed_c8)) == NULL ||
      agx_shader_raw(&builder, k_agx_buffer_copy_unnamed_c4, sizeof(k_agx_buffer_copy_unnamed_c4)) == NULL)
  {
    return 0;
  }

  instruction = agx_shader_load(&builder, 0, 0, 2, 2, 0x0cu, 0, AGX_SHADER_LOAD_FORM_ORDINARY, 1);
  if (instruction == NULL)
  {
    return 0;
  }
  instruction->first_load = 0x20u;

  instruction = agx_shader_load(&builder, 4, 0, 2, 1, 0x18u, 1, AGX_SHADER_LOAD_FORM_ORDINARY, 1);
  if (instruction == NULL)
  {
    return 0;
  }
  instruction->access_width = AGX_SHADER_ACCESS_WIDTH_8;

  instruction = agx_shader_load(&builder, 2, 0, 2, 1, 0x18u, 2, AGX_SHADER_LOAD_FORM_ORDINARY, 1);
  if (instruction == NULL)
  {
    return 0;
  }
  instruction->access_width = AGX_SHADER_ACCESS_WIDTH_8;
  instruction->destination_half = true;
  instruction->base_discard = true;
  instruction->raw_bits = AGX_SHADER_LOAD_BYTE_2_BIT_4_CLEAR | 0x20u;

  if (agx_shader_raw(&builder, k_agx_buffer_copy_unnamed_78, sizeof(k_agx_buffer_copy_unnamed_78)) == NULL)
  {
    return 0;
  }

  instruction = agx_shader_half_compare_select(&builder, 2, 4);
  if (instruction == NULL)
  {
    return 0;
  }
  instruction->raw_bits = 0x4207u;

  instruction = agx_shader_discard(&builder, 0);
  if (instruction == NULL)
  {
    return 0;
  }
  instruction->destination = 10;

  instruction = agx_shader_discard(&builder, 1);
  if (instruction == NULL)
  {
    return 0;
  }
  instruction->destination = 11;

  instruction = agx_shader_half_compare_select(&builder, 2, 2);
  if (instruction == NULL)
  {
    return 0;
  }
  instruction->destination_half = true;
  instruction->raw_bits = 0x620fu;

  instruction = agx_shader_discard(&builder, 3);
  if (instruction == NULL)
  {
    return 0;
  }
  instruction->destination = 12;
  instruction->move_byte_1_bit_7 = true;
  instruction->move_keeps_source = true;

  if (agx_shader_raw(&builder, k_agx_buffer_copy_unnamed_f10, sizeof(k_agx_buffer_copy_unnamed_f10)) == NULL)
  {
    return 0;
  }

  instruction = agx_shader_output_move(&builder, 14, 2, false);
  if (instruction == NULL)
  {
    return 0;
  }
  instruction->long_form = true;
  instruction->raw_bits = AGX_SHADER_OUTPUT_MOVE_BYTE_2_BIT_5_CLEAR | AGX_SHADER_OUTPUT_MOVE_BYTE_8_IS_0X20;

  if (agx_shader_stop(&builder) == NULL)
  {
    return 0;
  }
  return agx_shader_encode(&builder, bytes, capacity);
}
void
agx_buffer_copy_program_write(void* programs_cpu, uint32_t cursor, uint64_t operands_gpu)
{
  if (!programs_cpu)
  {
    return;
  }

  if (agx_buffer_copy_program_build(
        (uint8_t*)programs_cpu + (size_t)cursor * 0x40u,
        AGX_BUFFER_COPY_PROGRAM_BYTES,
        (uint32_t)((operands_gpu + AGX_BUFFER_COPY_OPERAND_SLOT) & 0xffffffu)
      ) == 0)
  {
    fprintf(
      stderr,
      "agx_buffer_copy_program_write: the copy program did not fit its %u byte(s) "
      "at cursor %u\n",
      (uint32_t)AGX_BUFFER_COPY_PROGRAM_BYTES,
      cursor
    );
  }
}

static void
agx_driver_arguments_write_block_attachments(const Agx_Bo* arguments, uint32_t block, uint32_t count)
{
  size_t   base = (size_t)block * 0x300u;
  uint8_t* at = NULL;
  uint32_t pairs = 0;
  uint32_t mask = 0;
  uint32_t live = 0;
  uint32_t k = 0;

  if (base + 0x2f4u > arguments->size)
  {
    return;
  }

  at = (uint8_t*)arguments->cpu + base;

  for (k = 0; k < count; k += 1)
  {
    pairs |= 2u << (2u * k);
  }
  mask = (1u << count) - 1u;

  if (block == 0)
  {
    memcpy(at + 0x168, &pairs, sizeof(pairs));
    memcpy(at + 0x16c, &mask, sizeof(mask));
  }
  else
  {
    uint32_t packed = pairs | (mask << 24);

    memcpy(at + 0x168, &packed, sizeof(packed));
  }

  for (k = 0; k < count; k += 1)
  {
    uint32_t entry = 0x00100083u | (k << 14);

    memcpy(at + 0x2d0 + 4u * (size_t)k, &entry, sizeof(entry));
  }

  live = mask | 0x400000u | (((count + 1u) / 2u) << 15);
  memcpy(at + 0x2f0, &live, sizeof(live));
}

void
agx_driver_arguments_set_attachment_count(const Agx_Bo* arguments, uint32_t count)
{
  if (!arguments->cpu || count == 0 || count > 8)
  {
    return;
  }

  agx_driver_arguments_write_block_attachments(arguments, 0, count);
  agx_driver_arguments_write_block_attachments(arguments, 1, count);
}

void
agx_tile_programs_set_store_shader(Agx_Tile_Programs* programs, uint32_t shader_pool_offset)
{
  uint64_t at = 0;

  if (!programs->heap.cpu || !programs->store || !programs->heap.gpu_va || programs->store < programs->heap.gpu_va)
  {
    return;
  }

  at = programs->store - programs->heap.gpu_va;
  if (at + AGX_STORE_PROGRAM_SHADER_REFERENCE + sizeof(uint16_t) > programs->heap.size)
  {
    return;
  }
  agx_program_shader_reference_write(
    (uint8_t*)programs->heap.cpu + at, AGX_STORE_PROGRAM_SHADER_REFERENCE, shader_pool_offset
  );
}

static void
agx_pool_vertex_stage_nibble_3(Agx_Shader_Builder* builder, uint8_t destination, uint8_t source)
{
  Agx_Shader_Instruction* instruction = agx_shader_move_nibble_3(builder, destination, source);

  if (instruction)
  {
    instruction->destination_half = true;
    instruction->move_raw = 0x0020;
  }
}

static void
agx_pool_vertex_stage_tail_release(Agx_Shader_Builder* builder, uint8_t destination, uint8_t source, bool byte_2_bit_3_clear)
{
  Agx_Shader_Instruction* instruction = agx_shader_discard(builder, source);

  if (instruction)
  {
    instruction->destination = destination;
    instruction->raw_bits = AGX_SHADER_MOVE_BYTE_2_BIT_5;
    instruction->move_keeps_source = byte_2_bit_3_clear;
  }
}

static void
agx_pool_vertex_stage_tail_output(
  Agx_Shader_Builder* builder,
  uint8_t             output_index,
  uint8_t             source,
  bool                byte_2_bit_3_clear,
  bool                byte_4_bit_7
)
{
  Agx_Shader_Instruction* instruction = agx_shader_output_move(builder, output_index, source, false);

  if (instruction)
  {
    instruction->long_form = true;
    instruction->raw_bits = (uint32_t)(0x04u | (byte_2_bit_3_clear ? AGX_SHADER_OUTPUT_MOVE_BYTE_2_BIT_3_CLEAR : 0u));
    instruction->output_move_byte_4 = byte_4_bit_7 ? 0xc2u : 0x00u;
  }
}

static Agx_Shader_Instruction*
agx_driver_store(Agx_Shader_Builder* builder)
{
  Agx_Shader_Instruction* instruction = agx_shader_store_surface(builder, 4, AGX_SHADER_CHANNEL_WIDTH_32, 0);

  if (instruction)
  {
    instruction->store_selector = 0xae;
    instruction->raw_bits = 0x040205u;
  }
  return instruction;
}

static void
agx_pool_vertex_stage_special(Agx_Shader_Builder* builder, uint8_t destination, uint8_t selector, bool byte_2_is_0x14)
{
  Agx_Shader_Instruction* instruction = agx_shader_get_special(builder, destination, (Agx_Shader_Special)selector);

  if (instruction)
  {
    instruction->long_form = false;
    instruction->destination_half = true;
    instruction->raw_bits = byte_2_is_0x14 ? 0x0614u : 0u;
  }
}

uint32_t
agx_pool_vertex_stage_program_build(uint8_t* bytes, uint32_t capacity)
{
  static const uint8_t k_cluster[5][10] = {
    {0x22, 0x81, 0x25, 0x80, 0x22, 0x80, 0x07, 0x02, 0x20, 0x81},
    {0x2a, 0x83, 0x25, 0x80, 0x21, 0x80, 0x07, 0x02, 0x22, 0x83},
    {0x25, 0x35, 0x22, 0x81, 0x05, 0x02, 0x20, 0x80, 0x2a, 0x04},
    {0x2d, 0x80, 0x26, 0x80, 0x0f, 0x02, 0x82, 0x04, 0x22, 0x81},
    {0x25, 0x37, 0x22, 0x81, 0x05, 0x02, 0x20, 0x80, 0x1a, 0x04},
  };
  static const uint8_t k_release_word[8] = {0x0b, 0x80, 0x26, 0x80, 0x0f, 0x02, 0x82, 0x04};

  Agx_Shader_Instruction  storage[128] = {0};
  Agx_Shader_Builder      builder = {0};
  Agx_Shader_Instruction* instruction = NULL;
  uint32_t                i = 0;

  agx_shader_builder_init(&builder, storage, sizeof(storage) / sizeof(storage[0]));

  for (i = 0; i < 30; ++i)
  {
    agx_shader_nop(&builder);
  }

  instruction = agx_shader_get_special(&builder, 11, (Agx_Shader_Special)0xa1);
  if (instruction)
  {
    instruction->long_form = false;
  }
  instruction = agx_shader_get_special(&builder, 12, (Agx_Shader_Special)0xa0);
  if (instruction)
  {
    instruction->long_form = false;
    instruction->raw_bits = 0x2600;
  }

  {
    static const struct
    {
      uint8_t destination;
      uint8_t source;
      uint8_t second;
      uint8_t slot;
    } k_rows[] = {{0, 15, 11, 0}, {1, 14, 12, 1}};
    uint32_t row = 0;

    for (row = 0; row < sizeof(k_rows) / sizeof(k_rows[0]); row += 1)
    {
      Agx_Shader_Instruction* made = agx_shader_add_index(
        &builder, k_rows[row].destination, k_rows[row].source, 0, AGX_SHADER_INDEX_SOURCE_FORM_DISCARD, true
      );

      if (made != NULL)
      {
        made->subtract = true;
        made->add_second_source_is_register = true;
        made->source_b = k_rows[row].second;
        made->source_is_uniform = true;
        made->second_form = AGX_SHADER_OPERAND_FORM_KEEP;
        made->wait_slot = k_rows[row].slot;
        made->raw_bits = AGX_SHADER_ADD_INDEX_BYTE_4_SWAP;
      }
    }
  }
  for (i = 0; i < 5; ++i)
  {
    agx_shader_raw(&builder, k_cluster[i], sizeof(k_cluster[i]));
  }

  agx_shader_raw(&builder, k_release_word, sizeof(k_release_word));
  {
    Agx_Shader_Instruction* made = agx_shader_add_index(&builder, 10, 1, 0x28, AGX_SHADER_INDEX_SOURCE_FORM_KEEP, false);

    if (made != NULL)
    {
      made->source_is_16_bit = true;
      made->index_second_operand_is_uniform = true;
      made->raw_bits = AGX_SHADER_ADD_INDEX_BYTE_7_BIT_3_CLEAR | AGX_SHADER_ADD_INDEX_BYTE_4_SWAP;
    }
  }
  {
    Agx_Shader_Instruction* made = agx_shader_add_index(&builder, 9, 0, 0x29, AGX_SHADER_INDEX_SOURCE_FORM_KEEP, false);

    if (made != NULL)
    {
      made->source_is_16_bit = true;
      made->index_second_operand_is_uniform = true;
      made->raw_bits = AGX_SHADER_ADD_INDEX_BYTE_7_BIT_3_CLEAR | AGX_SHADER_ADD_INDEX_BYTE_4_SWAP;
    }
  }

  agx_shader_push_exec(&builder, false);

  agx_shader_jump_exec_none_bytes(&builder, 0x9e2);
  agx_driver_state_branch_compare(&builder, 21, true);

  agx_shader_push_exec(&builder, false);
  agx_shader_jump_exec_none_bytes(&builder, 0x17e);
  agx_driver_state_branch_compare(&builder, 23, true);
  agx_driver_state_add(&builder, 1, 1, 0x2c, false, AGX_SHADER_ADD_INDEX_BYTE_7_BIT_3_CLEAR);
  agx_driver_state_add(&builder, 0, 0, 0x2d, false, AGX_SHADER_ADD_INDEX_BYTE_7_BIT_3_CLEAR);

  agx_shader_push_exec(&builder, false);
  agx_shader_jump_exec_none_bytes(&builder, 0x0a8);
  agx_driver_state_branch_compare(&builder, 24, false);

  agx_shader_push_exec(&builder, false);
  agx_shader_jump_exec_none_bytes(&builder, 0x028);

  agx_pool_vertex_stage_nibble_3(&builder, 1, 0);
  agx_pool_vertex_stage_nibble_3(&builder, 0, 0);
  instruction = agx_shader_discard(&builder, 1);
  if (instruction)
  {
    instruction->destination = 4;
    instruction->raw_bits = AGX_SHADER_MOVE_BYTE_2_BIT_5;
  }
  instruction = agx_shader_discard(&builder, 0);
  if (instruction)
  {
    instruction->destination = 5;
    instruction->raw_bits = AGX_SHADER_MOVE_BYTE_2_BIT_5;
  }
  agx_driver_texture_read(&builder, 2, false);

  instruction = agx_shader_pop_exec_short(&builder, 0x19);
  if (instruction)
  {
    instruction->raw_bits |= 0x80u;
  }
  agx_shader_jump_exec_none_bytes(&builder, 0x046);
  instruction = agx_shader_move_nibble_3(&builder, 2, 23);
  if (instruction)
  {
    instruction->move_raw = 0x0821;
  }

  agx_pool_vertex_stage_nibble_3(&builder, 1, 0);
  agx_pool_vertex_stage_nibble_3(&builder, 0, 0);
  agx_pool_vertex_stage_nibble_3(&builder, 2, 0);

  agx_pool_vertex_stage_tail_output(&builder, 0, 1, false, false);
  agx_pool_vertex_stage_tail_output(&builder, 1, 0, false, false);
  agx_pool_vertex_stage_tail_output(&builder, 2, 2, false, false);
  agx_driver_texture_read(&builder, 3, true);

  agx_shader_pop_exec(&builder, 1);
  instruction = agx_shader_get_special(&builder, 5, (Agx_Shader_Special)0xa5);
  if (instruction)
  {
    instruction->long_form = false;
    instruction->destination_half = true;
    instruction->raw_bits = 0x0614;
  }
  instruction = agx_shader_get_special(&builder, 5, (Agx_Shader_Special)0xa4);
  if (instruction)
  {
    instruction->raw_bits = 0x20;
  }
  (void)agx_driver_store(&builder);

  instruction = agx_shader_pop_exec_short(&builder, 0x19);
  if (instruction)
  {
    instruction->raw_bits |= 0x80u;
  }

  agx_shader_jump_exec_none_bytes(&builder, 0x0a0);

  return agx_shader_encode(&builder, bytes, capacity);
}

static bool
agx_driver_tail_export(Agx_Shader_Builder* builder, uint8_t component, uint8_t source, bool byte_5_bit_4, uint8_t byte_7)
{
  Agx_Shader_Instruction* instruction = agx_shader_vertex_export(builder, 3, source, component);

  if (instruction == NULL)
  {
    return false;
  }
  instruction->raw_bits = AGX_SHADER_VERTEX_EXPORT_BYTE_1_LOW_CLEAR | AGX_SHADER_VERTEX_EXPORT_BYTE_5_BASE_CLEAR |
                          AGX_SHADER_VERTEX_EXPORT_BYTE_6_BASE_CLEAR |
                          (byte_5_bit_4 ? AGX_SHADER_VERTEX_EXPORT_BYTE_5_BIT_4 : 0u) |
                          ((uint32_t)byte_7 << AGX_SHADER_VERTEX_EXPORT_BYTE_7_SHIFT);
  return true;
}

uint32_t
agx_pool_vertex_stage_tail_build(uint8_t* bytes, uint32_t capacity)
{
  static const uint8_t k_atomic_tail[8] = {0x23, 0x81, 0x06, 0x00, 0x80, 0x02, 0x00, 0x00};
  static const uint8_t k_pop_split[2] = {0x0f, 0x06};

  static const uint8_t k_long_x7[6][12] = {
    {0x97, 0x05, 0x54, 0x04, 0x03, 0x00, 0x00, 0x00, 0x50, 0x08, 0x20, 0x00},
    {0x97, 0x05, 0x54, 0x14, 0x00, 0x00, 0x00, 0x00, 0x50, 0x40, 0x04, 0x00},
    {0x97, 0x05, 0x54, 0x16, 0x00, 0x00, 0x08, 0x20, 0xf0, 0xc2, 0x07, 0x00},
    {0x97, 0x05, 0x54, 0x04, 0x03, 0x00, 0x00, 0x00, 0x50, 0x08, 0x20, 0x00},
    {0x97, 0x05, 0x54, 0x08, 0x00, 0x00, 0x00, 0x00, 0x50, 0x44, 0x04, 0x00},
    {0x97, 0x05, 0x54, 0x0a, 0x00, 0x00, 0x08, 0x20, 0xf0, 0xc6, 0x07, 0x00},
  };

  static const uint8_t k_tail_a[4] = {0xa8, 0x3d, 0x01, 0x00};
  static const uint8_t k_tail_b[4] = {0x28, 0x3e, 0x01, 0x00};

  Agx_Shader_Instruction  storage[128] = {0};
  Agx_Shader_Builder      builder = {0};
  Agx_Shader_Instruction* instruction = NULL;
  uint32_t                i = 0;

  agx_shader_builder_init(&builder, storage, sizeof(storage) / sizeof(storage[0]));

  agx_shader_raw(&builder, k_atomic_tail, sizeof(k_atomic_tail));
  agx_shader_push_exec(&builder, false);
  instruction = NULL;
  if (instruction)
  {
    instruction->destination_half = true;
  }
  agx_shader_jump_exec_none_bytes(&builder, 0x028);
  agx_pool_vertex_stage_nibble_3(&builder, 10, 0);
  agx_pool_vertex_stage_nibble_3(&builder, 9, 0);
  agx_pool_vertex_stage_tail_release(&builder, 4, 10, true);
  agx_pool_vertex_stage_tail_release(&builder, 5, 9, true);
  agx_driver_texture_read(&builder, 0, false);

  instruction = agx_shader_pop_exec_short(&builder, 0x19);
  if (instruction)
  {
    instruction->raw_bits |= 0x80u;
  }
  agx_shader_jump_exec_none_bytes(&builder, 0x046);
  instruction = agx_shader_move_nibble_3(&builder, 0, 31);
  if (instruction)
  {
    instruction->move_raw = 0x0821;
  }
  agx_pool_vertex_stage_nibble_3(&builder, 10, 0);
  agx_pool_vertex_stage_nibble_3(&builder, 9, 0);
  agx_pool_vertex_stage_nibble_3(&builder, 0, 0);
  agx_pool_vertex_stage_tail_output(&builder, 0, 10, true, false);
  agx_pool_vertex_stage_tail_output(&builder, 1, 9, true, false);
  agx_pool_vertex_stage_tail_output(&builder, 2, 0, false, false);
  agx_driver_texture_read(&builder, 1, true);

  agx_shader_pop_exec(&builder, 1);
  agx_pool_vertex_stage_special(&builder, 5, 0xa5, true);
  agx_pool_vertex_stage_special(&builder, 5, 0xa4, false);
  (void)agx_driver_store(&builder);

  agx_shader_pop_exec(&builder, 1);
  agx_pool_vertex_stage_special(&builder, 0, 0xa5, false);
  agx_pool_vertex_stage_special(&builder, 0, 0xa4, true);

  {
    Agx_Shader_Instruction* compare = agx_shader_compare_immediate(&builder, 0, AGX_SHADER_COMPARE_LESS, 0);

    if (compare != NULL)
    {
      compare->long_form = true;
      compare->discard_source = true;
      compare->wait = true;
      compare->compare_type = AGX_SHADER_COMPARE_TYPE_SIGNED;
    }
  }
  {
    Agx_Shader_Instruction* made = agx_shader_barrier(&builder, AGX_SHADER_MEMORY_SCOPE_NONE);

    if (made != NULL)
    {
      made->raw_bits = AGX_SHADER_BARRIER_BYTE_3_BIT_1 | 0x10u;
    }
  }

  agx_shader_push_exec(&builder, false);
  agx_shader_jump_exec_none_bytes(&builder, 0x0e6);
  agx_driver_state_branch_compare(&builder, 21, false);
  agx_driver_state_add(&builder, 0, 12, 0x22, true, AGX_SHADER_ADD_INDEX_BYTE_7_BIT_5);
  agx_driver_state_add(&builder, 1, 11, 0x24, true, AGX_SHADER_ADD_INDEX_BYTE_7_BIT_5);
  agx_driver_tail_clamp(&builder, 0, 28, 0);
  agx_driver_tail_clamp(&builder, 1, 29, 1);

  agx_shader_push_exec(&builder, false);
  agx_shader_jump_exec_none_bytes(&builder, 0x04a);
  agx_pool_vertex_stage_nibble_3(&builder, 10, 0);
  agx_pool_vertex_stage_nibble_3(&builder, 9, 0);
  for (i = 0; i < 3; ++i)
  {
    agx_shader_raw(&builder, k_long_x7[i], sizeof(k_long_x7[i]));
  }
  agx_pool_vertex_stage_tail_release(&builder, 8, 10, false);
  agx_pool_vertex_stage_tail_release(&builder, 9, 9, false);
  agx_driver_tail_export(&builder, 1, 8, false, 0x80);
  agx_shader_raw(&builder, k_tail_a, sizeof(k_tail_a));

  (void)agx_shader_pop_exec_short(&builder, 0x19);
  agx_shader_jump_exec_none_bytes(&builder, 0x060);
  agx_pool_vertex_stage_nibble_3(&builder, 10, 0);
  agx_pool_vertex_stage_nibble_3(&builder, 9, 0);
  for (i = 3; i < 6; ++i)
  {
    agx_shader_raw(&builder, k_long_x7[i], sizeof(k_long_x7[i]));
  }
  agx_pool_vertex_stage_tail_output(&builder, 0, 10, false, false);
  agx_pool_vertex_stage_tail_output(&builder, 1, 9, false, false);
  agx_pool_vertex_stage_tail_output(&builder, 2, 19, true, true);
  agx_driver_tail_export(&builder, 2, 0, true, 0x90);
  agx_shader_raw(&builder, k_tail_b, sizeof(k_tail_b));

  agx_shader_raw(&builder, k_pop_split, sizeof(k_pop_split));
  instruction = agx_shader_move_immediate(&builder, 0, 0x02);
  if (instruction)
  {
    instruction->destination_half = true;
  }

  return agx_shader_encode(&builder, bytes, capacity);
}

uint32_t
agx_pool_uniform_program_gap_build(uint8_t* bytes, uint32_t capacity)
{
  if (capacity < 2)
  {
    agx_refuse("agx_pool_uniform_program_gap_build: capacity < 2");
    return 0;
  }
  memset(bytes, 0, capacity);
  bytes[1] = 0x0e;
  return capacity;
}

uint64_t
agx_pool_arguments_vertex_uniforms(uint64_t pool_arguments)
{
  return pool_arguments + 0x8000u + 0x12e0u;
}

uint64_t
agx_pool_arguments_fragment_block(uint64_t pool_arguments)
{
  return pool_arguments + 0x13e0u;
}

uint64_t
agx_driver_arguments_vertex_table(uint64_t driver_arguments)
{
  return driver_arguments + 0x0260u;
}

uint64_t
agx_driver_arguments_fragment_table(uint64_t driver_arguments)
{
  return driver_arguments + 0x0060u;
}

uint64_t
agx_driver_arguments_vertex_shader_block(uint64_t driver_arguments)
{
  return driver_arguments + 0x0600u;
}

uint32_t
agx_pool_uniform_program_place(uint8_t* pool, uint32_t at, uint32_t pool_bytes)
{
  if (pool == NULL || at >= pool_bytes)
  {
    agx_refuse("agx_pool_uniform_program_place: pool == NULL || at >= pool_bytes");
    return 0;
  }
  return agx_pool_uniform_program_build(pool + at, pool_bytes - at);
}

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
)
{
  uint32_t at = AGX_PIPELINE_PROGRAMS_VERTEX_CALLER_OFFSET;

  if (pipeline_programs == NULL || at >= pipeline_programs_bytes)
  {
    agx_refuse("agx_pipeline_vertex_call_program_place: pipeline_programs == NULL || at >= pipeline_programs_bytes");
    return 0;
  }
  return agx_pipeline_vertex_call_program_build(
    shape,
    argument_block,
    uniform_block,
    table,
    shader_pool,
    shader_pool_offset,
    state_loader_offset,
    pipeline_programs + at,
    pipeline_programs_bytes - at
  );
}

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
)
{
  if (pipeline_programs == NULL || at >= pipeline_programs_bytes)
  {
    agx_refuse(
      "agx_pipeline_fragment_call_program_place: offset 0x%x is outside a %u byte buffer", at, pipeline_programs_bytes
    );
    return 0;
  }
  if ((at % AGX_PIPELINE_PROGRAMS_CALLER_ALIGN) != 0u)
  {
    agx_refuse(
      "agx_pipeline_fragment_call_program_place: offset 0x%x is not a multiple of %u -- "
      "a misaligned fragment caller draws the clear and no triangle",
      at,
      (uint32_t)AGX_PIPELINE_PROGRAMS_CALLER_ALIGN
    );
    return 0;
  }
  if (at < AGX_PIPELINE_PROGRAMS_VERTEX_CALLER_OFFSET + vertex_caller_bytes)
  {
    agx_refuse(
      "agx_pipeline_fragment_call_program_place: offset 0x%x overlaps the vertex caller, "
      "which ends at 0x%x",
      at,
      AGX_PIPELINE_PROGRAMS_VERTEX_CALLER_OFFSET + vertex_caller_bytes
    );
    return 0;
  }
  return agx_pipeline_fragment_call_program_build(
    shape, argument_block, table, shader_pool, shader_pool_offset, varying_components, pipeline_programs + at, pipeline_programs_bytes - at
  );
}

uint32_t
agx_tile_programs_build_vertex_shader_caller(
  Agx_Tile_Programs* programs,
  uint64_t           argument_block,
  uint32_t           shader_pool_offset,
  bool               writes_coverage_mask,
  uint32_t           clip_distance_count
)
{
  uint32_t at = AGX_TILE_PROGRAMS_VERTEX_SHADER_CALLER_OFFSET;

  if (programs == NULL || programs->heap.cpu == NULL || at >= programs->heap.size)
  {
    return 0;
  }
  return agx_vertex_shader_call_program_build(
    argument_block,
    programs->shader_pool,
    shader_pool_offset,
    writes_coverage_mask,
    clip_distance_count,
    (uint8_t*)programs->heap.cpu + at,
    (uint32_t)(programs->heap.size - at)
  );
}

uint32_t
agx_pool_uniform_program_gap_place(uint8_t* pool, uint32_t at, uint32_t pool_bytes)
{
  if (pool == NULL || (uint64_t)at + 0x40u > pool_bytes)
  {
    agx_refuse("agx_pool_uniform_program_gap_place: pool == NULL || (uint64_t)at + 0x40u > pool_bytes");
    return 0;
  }
  return agx_pool_uniform_program_gap_build(pool + at, 0x40);
}

uint32_t
agx_pool_texture_epilogue_place(uint8_t* pool, uint32_t at, uint32_t pool_bytes)
{
  if (pool == NULL || (uint64_t)at + 0xdau > pool_bytes)
  {
    agx_refuse("agx_pool_texture_epilogue_place: pool == NULL || (uint64_t)at + 0xdau > pool_bytes");
    return 0;
  }
  return agx_pool_texture_epilogue_build(pool + at, 0xda);
}

uint32_t
agx_pool_tile_block_place(uint8_t* pool, uint32_t at, uint32_t pool_bytes)
{
  if (pool == NULL || (uint64_t)at + 0x100u > pool_bytes)
  {
    agx_refuse("agx_pool_tile_block_place: pool == NULL || (uint64_t)at + 0x100u > pool_bytes");
    return 0;
  }

  return agx_pool_tile_block_build(pool + at, 0x100);
}

uint32_t
agx_pool_vertex_stage_program_place(uint8_t* pool, uint32_t entry_offset, uint32_t pool_bytes)
{
  uint32_t at = 0;

  if (pool == NULL || entry_offset < AGX_POOL_VERTEX_STAGE_ENTRY_LOWEST ||
      entry_offset > AGX_POOL_VERTEX_STAGE_ENTRY_HIGHEST || entry_offset < AGX_POOL_VERTEX_STAGE_ENTRY_PREFIX)
  {
    return 0;
  }

  at = entry_offset - AGX_POOL_VERTEX_STAGE_ENTRY_PREFIX;
  if ((uint64_t)at + AGX_POOL_VERTEX_STAGE_PROGRAM_ENCODED > pool_bytes)
  {
    agx_refuse("agx_pool_vertex_stage_program_place: (uint64_t)at + AGX_POOL_VERTEX_STAGE_PROGRAM_ENCODED > pool_bytes");
    return 0;
  }
  return agx_pool_vertex_stage_program_build(pool + at, AGX_POOL_VERTEX_STAGE_PROGRAM_ENCODED);
}

uint32_t
agx_pool_vertex_stage_tail_run_build(
  uint8_t* pool,
  uint32_t at,
  uint32_t pool_bytes,
  uint32_t vertex_stage_entry,
  uint32_t texture_epilogue_offset,
  uint32_t next_occupant
)
{
  uint32_t written = 0;
  uint32_t program_end = 0;
  uint32_t ceiling = 0;
  bool     in_capture_band = false;
  bool     follows_the_program = false;

  if (pool == NULL || (uint64_t)at + AGX_SHADER_POOL_VERTEX_STAGE_TAIL_BYTES > pool_bytes ||
      vertex_stage_entry < AGX_POOL_VERTEX_STAGE_ENTRY_PREFIX)
  {
    return 0;
  }

  program_end = vertex_stage_entry - AGX_POOL_VERTEX_STAGE_ENTRY_PREFIX + AGX_POOL_VERTEX_STAGE_PROGRAM_BYTES;

  ceiling = next_occupant;
  if (texture_epilogue_offset >= program_end && texture_epilogue_offset < ceiling)
  {
    ceiling = texture_epilogue_offset;
  }

  follows_the_program = at >= program_end && (uint64_t)at + AGX_SHADER_POOL_VERTEX_STAGE_TAIL_BYTES <= ceiling;

  in_capture_band = texture_epilogue_offset == AGX_SHADER_POOL_TEXTURE_EPILOGUE_OFFSET &&
                    at >= AGX_SHADER_POOL_VERTEX_STAGE_TAIL_LOWEST && at <= AGX_SHADER_POOL_VERTEX_STAGE_TAIL_HIGHEST;

  if (!in_capture_band && !follows_the_program)
  {
    return 0;
  }

  written = agx_pool_vertex_stage_prologue_build(pool + at, 0x20);
  if (written == 0)
  {
    return 0;
  }
  if (agx_pool_vertex_stage_tail_build(pool + at + 0x20, 0x1a4) == 0)
  {
    return 0;
  }
  if (agx_pool_vertex_stage_tail_end_build(pool + at + 0x1c4, 0x3c) == 0)
  {
    return 0;
  }
  return AGX_SHADER_POOL_VERTEX_STAGE_TAIL_BYTES;
}

uint32_t
agx_pool_vertex_stage_prologue_build(uint8_t* bytes, uint32_t capacity)
{
  static const uint8_t k_jump[4] = {0x0f, 0x01, 0x54, 0xa4};
  static const uint8_t k_atomic_head[2] = {0x0a, 0x2a};

  Agx_Shader_Instruction  storage[16] = {0};
  Agx_Shader_Builder      builder = {0};
  Agx_Shader_Instruction* instruction = NULL;
  uint32_t                i = 0;

  agx_shader_builder_init(&builder, storage, sizeof(storage) / sizeof(storage[0]));

  instruction = agx_shader_move_immediate(&builder, 0, 0x01);
  if (instruction)
  {
    instruction->destination_half = true;
  }
  agx_shader_pad(&builder, 2);
  agx_shader_pop_exec(&builder, 1);
  instruction = agx_shader_pop_exec(&builder, 1);
  if (instruction)
  {
    instruction->raw_bits = 0x80;
  }

  instruction = agx_shader_pop_exec_short(&builder, 0x19);
  if (instruction)
  {
    instruction->raw_bits |= 0x80u;
  }
  agx_shader_raw(&builder, k_jump, sizeof(k_jump));
  for (i = 0; i < 3; ++i)
  {
    agx_shader_pad(&builder, 2);
  }
  agx_shader_raw(&builder, k_atomic_head, sizeof(k_atomic_head));
  return agx_shader_encode(&builder, bytes, capacity);
}

uint32_t
agx_pool_vertex_stage_tail_end_build(uint8_t* bytes, uint32_t capacity)
{
  Agx_Shader_Instruction storage[4] = {0};
  Agx_Shader_Builder     builder = {0};

  if (capacity < 6)
  {
    agx_refuse("agx_pool_vertex_stage_tail_end_build: capacity < 6");
    return 0;
  }
  memset(bytes, 0, capacity);
  agx_shader_builder_init(&builder, storage, sizeof(storage) / sizeof(storage[0]));
  agx_shader_stop(&builder);
  if (!agx_shader_encode(&builder, bytes + 2, capacity - 2))
  {
    agx_refuse("agx_pool_vertex_stage_tail_end_build: !agx_shader_encode(&builder, bytes + 2, capacity - 2)");
    return 0;
  }
  return capacity;
}

static void
epilogue_special(Agx_Shader_Builder* b, uint8_t reg, uint8_t which, bool odd)
{
  Agx_Shader_Instruction* at = agx_shader_get_special(b, reg, (Agx_Shader_Special)which);
  if (at)
  {
    at->long_form = false;
    at->destination_half = true;
    if (odd)
    {
      at->raw_bits = 0x14;
    }
  }
}

static void
epilogue_immediate(Agx_Shader_Builder* b, bool product, uint8_t reg, float value)
{
  Agx_Shader_Instruction* at = product ? agx_shader_product(b, reg, reg, 0) : agx_shader_sum(b, reg, reg, 0);
  if (at)
  {
    at->second_form = AGX_SHADER_OPERAND_FORM_IMMEDIATE;
    at->second_immediate = value;

    at->source_form = AGX_SHADER_SOURCE_FORM_DISCARD;
  }
}

static void
epilogue_release(Agx_Shader_Builder* b, uint8_t destination, uint8_t source)
{
  Agx_Shader_Instruction* at = agx_shader_move(b, destination, source);
  if (at)
  {
    at->op = AGX_SHADER_OP_DISCARD;
    at->destination = destination;
    at->raw_bits = AGX_SHADER_MOVE_BYTE_2_BIT_5;
  }
}

static void
epilogue_output_release(Agx_Shader_Builder* b, uint8_t output, uint8_t source, bool late)
{
  Agx_Shader_Instruction* at = agx_shader_output_move(b, output, source, false);
  if (at)
  {
    at->raw_bits = 0x04u | (late ? AGX_SHADER_OUTPUT_MOVE_BYTE_2_BIT_3_CLEAR : 0u);
    at->output_move_byte_4 = late ? 0xc2u : 0x00u;
  }
}

static void
epilogue_convert(Agx_Shader_Builder* b, uint8_t reg)
{
  Agx_Shader_Instruction* at = agx_shader_convert_to_float(b, reg, reg);
  if (at)
  {
    at->raw_bits = 1;
    at->destination_is_word = true;
    at->convert_source_form = AGX_SHADER_CONVERT_SOURCE_FORM_RELEASE;
  }
}

uint32_t
agx_pool_texture_epilogue_build(uint8_t* bytes, uint32_t capacity)
{
  static const uint8_t k_pad2[2] = {0x00, 0x00};
  static const uint8_t k_jump[4] = {0x0f, 0x01, 0x54, 0x42};
  static const uint8_t k_pad8[4] = {0x08, 0x00, 0x00, 0x00};
  static const uint8_t k_tail[4] = {0x9f, 0x00, 0x54, 0x00};

  Agx_Shader_Instruction  storage[64] = {0};
  Agx_Shader_Builder      b = {0};
  Agx_Shader_Instruction* instruction = NULL;

  agx_shader_builder_init(&b, storage, sizeof(storage) / sizeof(storage[0]));
  agx_shader_raw(&b, k_pad2, 2);
  epilogue_convert(&b, 1);

  instruction = agx_shader_compare_immediate(&b, 24, AGX_SHADER_COMPARE_NEVER, 0);
  if (instruction == NULL)
  {
    return 0;
  }
  instruction->long_form = true;
  instruction->compare_source_is_half = true;
  instruction->raw_bits = AGX_SHADER_COMPARE_WIDE_BYTE_3_BIT_0 | AGX_SHADER_COMPARE_BYTE_4_BIT_7;
  epilogue_convert(&b, 0);

  epilogue_immediate(&b, false, 1, 0.5f);
  epilogue_immediate(&b, false, 0, 0.5f);
  epilogue_immediate(&b, true, 1, 0.0078125f);

  epilogue_immediate(&b, true, 0, 0.0087890625f);
  agx_shader_push_exec(&b, false);

  epilogue_release(&b, 4, 1);
  epilogue_release(&b, 5, 0);
  instruction = agx_shader_texture_sample(&b, 0, 0);
  if (instruction == NULL)
  {
    return 0;
  }
  instruction->texture_index = 2;
  instruction->sampler_index = 0;
  instruction->texture_access = AGX_SHADER_TEXTURE_ACCESS_SAMPLE_2D;
  instruction->sample_channels_minus_one = 3u;
  instruction->raw_bits = AGX_SHADER_TEXTURE_BYTE_3_BIT_7_CLEAR | AGX_SHADER_TEXTURE_BYTE_12_IS_0X24 |
                          ((uint32_t)4u << AGX_SHADER_TEXTURE_BYTE_1_LOW_SHIFT);
  instruction = agx_shader_pop_exec_short(&b, 0x19);
  if (instruction == NULL)
  {
    return 0;
  }
  instruction->raw_bits = 0x80u;

  epilogue_output_release(&b, 0, 1, false);
  epilogue_output_release(&b, 1, 0, false);
  epilogue_output_release(&b, 2, 16, true);

  instruction = agx_shader_texture_sample(&b, 0, 0);
  if (instruction == NULL)
  {
    return 0;
  }
  instruction->texture_index = 3;
  instruction->sampler_index = 0;
  instruction->texture_access = AGX_SHADER_TEXTURE_ACCESS_SAMPLE_2D;
  instruction->sample_channels_minus_one = 3u;
  instruction->raw_bits = AGX_SHADER_TEXTURE_BYTE_4_BIT_7_CLEAR | AGX_SHADER_TEXTURE_BYTE_12_IS_0X24 |
                          AGX_SHADER_TEXTURE_BYTE_6_HIGH_BITS | ((uint32_t)1u << AGX_SHADER_TEXTURE_BYTE_5_SHIFT);
  agx_shader_pop_exec(&b, 1);
  epilogue_special(&b, 5, 0xa5, true);
  epilogue_special(&b, 5, 0xa4, false);
  (void)agx_driver_store(&b);
  agx_shader_pop_exec(&b, 1);
  if (agx_shader_pop_exec_short(&b, 0x19) == NULL)
  {
    return 0;
  }
  agx_shader_raw(&b, k_jump, 4);
  agx_shader_raw(&b, k_pad8, 4);
  agx_shader_raw(&b, k_pad2, 2);
  epilogue_special(&b, 13, 0xa5, true);
  epilogue_special(&b, 13, 0xa4, false);

  instruction = agx_shader_move(&b, 5, 0);
  if (instruction == NULL)
  {
    return 0;
  }
  instruction->move_writes_zero = true;
  instruction->raw_bits = 0u;
  instruction->move_keeps_source = true;
  instruction = agx_shader_move(&b, 6, 0);
  if (instruction == NULL)
  {
    return 0;
  }
  instruction->move_writes_zero = true;
  instruction->raw_bits = 0u;
  instruction->move_keeps_source = true;
  instruction = agx_shader_move(&b, 7, 0);
  if (instruction == NULL)
  {
    return 0;
  }
  instruction->move_writes_zero = true;
  instruction->raw_bits = 0u;
  instruction->move_keeps_source = true;
  instruction = agx_shader_move(&b, 8, 0);
  if (instruction == NULL)
  {
    return 0;
  }
  instruction->move_writes_zero = true;
  instruction->raw_bits = 0u;
  instruction->move_keeps_source = true;

  instruction = agx_shader_compare_immediate(&b, 37, AGX_SHADER_COMPARE_LESS, 12);
  if (instruction == NULL)
  {
    return 0;
  }
  instruction->compare_type = AGX_SHADER_COMPARE_TYPE_SIGNED;
  instruction->raw_bits = AGX_SHADER_COMPARE_BYTE_4_BIT_7;

  instruction =
    agx_shader_scale_index(&b, 1, 0x23, 0, (Agx_Shader_Index_Form)13, AGX_SHADER_INDEX_SOURCE_FORM_DISCARD, false, false);
  if (instruction == NULL)
  {
    return 0;
  }
  instruction->second_is_register = true;
  instruction->source_b = 1;
  instruction->second_form = AGX_SHADER_OPERAND_FORM_DISCARD;
  instruction->raw_bits = AGX_SHADER_SCALE_INDEX_BYTE_10_BIT_1_CLEAR | AGX_SHADER_SCALE_INDEX_BYTE_4_BIT_0;
  agx_shader_raw(&b, k_tail, 4);
  return agx_shader_encode(&b, bytes, capacity);
}

static const uint8_t k_agx_tile_block_end[2] = {0x2a, 0x08};
static const uint8_t k_agx_tile_block_xb8[8] = {
  0x3b,
  0x06,
  0x06,
  0x00,
  0x17,
  0x08,
  0x02,
  0x00,
};

static const uint8_t k_agx_tile_block_value[4][12] = {
  {0x9f, 0x20, 0x54, 0x00, 0x00, 0x20, 0x38, 0x00, 0x90, 0x96, 0x0b, 0x00},
  {0x9f, 0x00, 0x54, 0x00, 0x00, 0x10, 0x28, 0x00, 0x90, 0x96, 0x0b, 0x00},
  {0x97, 0x05, 0x54, 0x00, 0x00, 0x0e, 0x10, 0x00, 0xf0, 0xe4, 0x00, 0x00},
  {0x97, 0x05, 0x54, 0x00, 0x00, 0x0c, 0x00, 0x00, 0xd0, 0xe4, 0x10, 0x00},
};
static const uint8_t k_agx_tile_block_x57[8] = {
  0x57,
  0x00,
  0x56,
  0x08,
  0x00,
  0xc0,
  0xa0,
  0xd0,
};
static const uint8_t k_agx_tile_block_x8[4] = {0x28, 0x30, 0x04, 0x00};

static uint32_t
agx_pool_tile_block_head_build(uint8_t* bytes, uint32_t capacity)
{
  static const uint8_t k_open[8] = {0x54, 0x22, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00};
  static const uint8_t k_compare[10] = {
    0x2a,
    0x88,
    0x23,
    0x86,
    0x02,
    0x00,
    0x17,
    0x08,
    0x02,
    0x00,
  };
  static const uint32_t k_constant[2] = {0x328u, 0x324u};

  Agx_Shader_Instruction  storage[16] = {0};
  Agx_Shader_Builder      builder = {0};
  Agx_Shader_Instruction* instruction = NULL;

  agx_shader_builder_init(&builder, storage, sizeof(storage) / sizeof(storage[0]));

  if (agx_shader_raw(&builder, k_open, sizeof(k_open)) == NULL)
  {
    return 0;
  }

  instruction = agx_shader_move_immediate(&builder, 3, k_constant[0]);
  if (instruction == NULL)
  {
    return 0;
  }
  instruction->long_form = true;
  instruction->destination_half = true;
  instruction->raw_bits |= AGX_SHADER_MOVE_IMMEDIATE_BYTE_2_BIT_5 | AGX_SHADER_MOVE_IMMEDIATE_BYTE_2_BIT_2;

  if (agx_shader_raw(&builder, k_compare, sizeof(k_compare)) == NULL)
  {
    return 0;
  }

  instruction = agx_shader_push_exec(&builder, false);
  if (instruction == NULL)
  {
    return 0;
  }
  instruction->mask_source = AGX_SHADER_MASK_SOURCE_NONE;

  if (agx_shader_jump_exec_none_bytes(&builder, 0x80u) == NULL)
  {
    return 0;
  }

  instruction = agx_shader_move_immediate(&builder, 3, k_constant[1]);
  if (instruction == NULL)
  {
    return 0;
  }
  instruction->long_form = true;
  instruction->destination_half = true;
  instruction->raw_bits |= AGX_SHADER_MOVE_IMMEDIATE_BYTE_2_BIT_5 | AGX_SHADER_MOVE_IMMEDIATE_BYTE_2_BIT_2;

  return agx_shader_encode(&builder, bytes, capacity);
}

static uint32_t
agx_pool_tile_block_copy_build(uint8_t* bytes, uint32_t capacity, uint8_t base_register)
{
  static const struct
  {
    uint8_t  destination;
    uint8_t  selector;
    bool     half;
    uint32_t unknown;
  } k_specials[5] = {
    {7, 0x9c, false, 0x2610},
    {8, 0xa8, true, 0x2610},
    {5, 0x9d, false, 0x2610},
    {4, 0xa9, true, 0x2610},
    {3, 0xc3, true, 0x2614},
  };

  Agx_Shader_Instruction  storage[24] = {0};
  Agx_Shader_Builder      builder = {0};
  Agx_Shader_Instruction* instruction = NULL;
  uint8_t                 value[12] = {0};
  uint32_t                i = 0;
  uint32_t                j = 0;

  if (bytes == NULL)
  {
    return 0;
  }

  agx_shader_builder_init(&builder, storage, sizeof(storage) / sizeof(storage[0]));

  if (agx_shader_raw(&builder, k_agx_tile_block_end, sizeof(k_agx_tile_block_end)) == NULL ||
      agx_shader_raw(&builder, k_agx_tile_block_xb8, sizeof(k_agx_tile_block_xb8)) == NULL)
  {
    return 0;
  }

  instruction = agx_shader_push_exec(&builder, false);
  if (instruction == NULL)
  {
    return 0;
  }
  instruction->mask_source = AGX_SHADER_MASK_SOURCE_NONE;

  if (agx_shader_jump_exec_none_bytes(&builder, 0x5a) == NULL)
  {
    return 0;
  }

  for (i = 0; i < 5; ++i)
  {
    instruction = agx_shader_get_special(&builder, k_specials[i].destination, (Agx_Shader_Special)k_specials[i].selector);
    if (instruction == NULL)
    {
      return 0;
    }
    instruction->long_form = false;
    instruction->destination_half = k_specials[i].half;
    instruction->raw_bits = k_specials[i].unknown;
  }

  for (i = 0; i < 4; ++i)
  {
    for (j = 0; j < sizeof(value); ++j)
    {
      value[j] = k_agx_tile_block_value[i][j];
    }
    value[3] = (uint8_t)((base_register + i) << 1);
    if (agx_shader_raw(&builder, value, sizeof(value)) == NULL)
    {
      return 0;
    }
  }

  if (agx_shader_raw(&builder, k_agx_tile_block_x57, sizeof(k_agx_tile_block_x57)) == NULL ||
      agx_shader_raw(&builder, k_agx_tile_block_x8, sizeof(k_agx_tile_block_x8)) == NULL)
  {
    return 0;
  }

  if (agx_shader_pop_exec(&builder, 1) == NULL)
  {
    return 0;
  }
  if (agx_shader_pop_exec_short(&builder, 0x19) == NULL)
  {
    return 0;
  }
  if (agx_shader_jump_exec_none_bytes(&builder, 0x78) == NULL)
  {
    return 0;
  }
  return agx_shader_encode(&builder, bytes, capacity);
}

uint32_t
agx_pool_tile_block_build(uint8_t* bytes, uint32_t capacity)
{
  static const uint8_t k_base_register[2] = {4, 0};

  uint8_t  block[AGX_POOL_TILE_BLOCK_BYTES] = {0};
  uint32_t at = 0;
  uint32_t copy = 0;

  if (capacity < AGX_POOL_TILE_BLOCK_HEAD_BYTES)
  {
    agx_refuse("agx_pool_tile_block_build: capacity < AGX_POOL_TILE_BLOCK_HEAD_BYTES");
    return 0;
  }
  if (agx_pool_tile_block_head_build(bytes, capacity) != AGX_POOL_TILE_BLOCK_HEAD_BYTES)
  {
    agx_refuse("agx_pool_tile_block_build: the tile block head did not build its expected length");
    return 0;
  }
  at = AGX_POOL_TILE_BLOCK_HEAD_BYTES;

  for (copy = 0; copy < 2; ++copy)
  {
    uint32_t n = AGX_POOL_TILE_BLOCK_BYTES;

    if (agx_pool_tile_block_copy_build(block, sizeof(block), k_base_register[copy]) != AGX_POOL_TILE_BLOCK_BYTES)
    {
      return 0;
    }
    if (at + n > capacity)
    {
      n = capacity - at;
    }
    memcpy(bytes + at, block, n);
    at += n;
  }
  return at;
}

void
agx_pool_arguments_set_geometry(void* arguments, uint64_t geometry)
{
  agx_pool_arguments_set_geometry_pair(arguments, 0, geometry);
}

void
agx_pool_arguments_set_geometry_pair(void* arguments, uint32_t pair, uint64_t address)
{
  if (arguments)
  {
    if ((address >> AGX_POOL_ARGUMENTS_GEOMETRY_BITS) != 0u)
    {
      fprintf(
        stderr,
        "agx_pool_arguments_set_geometry_pair: 0x%llx does not fit the %u bits this field "
        "is read as; the high bits are ignored by the hardware, not by this call\n",
        (unsigned long long)address,
        (unsigned)AGX_POOL_ARGUMENTS_GEOMETRY_BITS
      );
    }
    *(uint64_t*)((uint8_t*)arguments + AGX_POOL_ARGUMENTS_GEOMETRY_OFFSET + pair * sizeof(uint64_t)) = address;
  }
}

void
agx_pool_arguments_set_copy_extent(void* arguments, uint32_t width, uint32_t height)
{
  if (arguments)
  {
    *(uint32_t*)((uint8_t*)arguments + 0x12f8) = width;
    *(uint32_t*)((uint8_t*)arguments + 0x12fc) = height;
  }
}

void
agx_compute_pipeline_desc_init(Agx_Compute_Pipeline_Desc* desc)
{
  memset(desc, 0, sizeof(*desc));
  desc->binding_count = 1;
}

void
agx_compute_pipeline_desc_set_kernel(Agx_Compute_Pipeline_Desc* desc, uint32_t offset, uint32_t bytes)
{
  desc->kernel_offset = offset;
  desc->kernel_bytes = bytes;
}

void
agx_compute_pipeline_desc_set_binding_count(Agx_Compute_Pipeline_Desc* desc, uint32_t count)
{
  desc->binding_count = count;
}

void
agx_compute_pipeline_desc_set_reads_texture(Agx_Compute_Pipeline_Desc* desc, bool reads)
{
  desc->reads_a_texture = reads;
}

uint32_t
agx_compute_pipeline_write(const Agx_Compute_Pipeline_Desc* desc, void* pool_cpu, uint32_t pool_bytes)
{
  uint8_t* pool = (uint8_t*)pool_cpu;
  uint32_t at = 0;
  uint32_t length = 0;
  uint32_t declared = 0;

  if (pool == NULL || desc == NULL)
  {
    return 0;
  }

  if (desc->kernel_offset < 2u * AGX_COMPUTE_RECORD_SLOT || (desc->kernel_offset % AGX_COMPUTE_RECORD_SLOT) != 0u ||
      desc->kernel_offset > pool_bytes)
  {
    return 0;
  }

  memset(pool, 0, desc->kernel_offset);

  for (at = AGX_COMPUTE_POOL_ENTRY_TABLE; at < AGX_COMPUTE_POOL_ENTRY_ROUTINE; at += AGX_COMPUTE_POOL_SLOT)
  {
    uint32_t words[4] = {0};

    words[0] = 0x0054000fu | ((AGX_COMPUTE_POOL_ENTRY_ROUTINE - at) << 24);
    words[1] = 0;
    words[2] = 0x00060000u;
    words[3] = 0x00060006u;
    memcpy(pool + at, words, sizeof(words));
  }

  for (at = 0; at < 2u; at += 1u)
  {
    uint32_t words[4] = {0};

    words[0] = 0x00aa03f7u;
    words[1] = 0x0154028fu;
    words[2] = 0x00060006u;
    words[3] = 0x00060006u;
    memcpy(pool + AGX_COMPUTE_POOL_ENTRY_ROUTINE + at * AGX_COMPUTE_POOL_SLOT, words, sizeof(words));
  }

  length = 2u * AGX_COMPUTE_RECORD_SLOT +
           ((desc->kernel_bytes + AGX_COMPUTE_RECORD_SLOT - 1u) & ~(AGX_COMPUTE_RECORD_SLOT - 1u));
  memcpy(pool + desc->kernel_offset - 2u * AGX_COMPUTE_RECORD_SLOT, &length, sizeof(length));

  declared = (desc->reads_a_texture ? 0x20u : 0u) + (desc->binding_count > 1u ? 0x60u : 0u);
  {
    uint8_t* constant_program = pool + desc->kernel_offset - AGX_COMPUTE_RECORD_SLOT;
    uint32_t words[4] = {0};
    uint32_t filled = 0;

    if (declared != 0u)
    {
      words[0] = 0x00070003u;
      words[1] = 0x00000002u;
      words[2] = 0x000e0000u | declared;
      words[3] = 0x00060000u;
    }
    else
    {
      words[0] = 0x0000000eu;
      words[1] = 0x00060006u;
      words[2] = 0x00060006u;
      words[3] = 0x00060006u;
    }
    memcpy(constant_program, words, sizeof(words));
    for (filled = (uint32_t)sizeof(words); filled < AGX_COMPUTE_RECORD_SLOT; filled += 4u)
    {
      uint32_t pad = 0x00060006u;

      memcpy(constant_program + filled, &pad, sizeof(pad));
    }
  }

  return desc->kernel_offset;
}

static bool
agx_driver_state_store_item(Agx_Shader_Builder* builder, uint8_t source, bool waits, bool wide)
{
  Agx_Shader_Instruction* instruction =
    agx_shader_store_surface(builder, 4, wide ? AGX_SHADER_CHANNEL_WIDTH_32 : AGX_SHADER_CHANNEL_WIDTH_8, 0);

  if (instruction == NULL)
  {
    return false;
  }
  instruction->store_selector = 0xae;
  instruction->raw_bits = 0x04020du;
  instruction->store_waits = waits;
  instruction->store_source = source;
  return true;
}

static bool
agx_driver_state_pop_run(Agx_Shader_Builder* builder, uint32_t six_byte, uint32_t byte_2_bits)
{
  Agx_Shader_Instruction* instruction = NULL;
  uint32_t                i = 0;

  for (i = 0; i < six_byte; i += 1)
  {
    instruction = agx_shader_pop_exec(builder, 1);
    if (instruction == NULL)
    {
      return false;
    }
    instruction->raw_bits |= byte_2_bits;
  }

  instruction = agx_shader_pop_exec_short(builder, 0x19);
  if (instruction == NULL)
  {
    return false;
  }
  instruction->raw_bits |= byte_2_bits;
  return true;
}

static bool
agx_driver_state_arm_tail(Agx_Shader_Builder* builder, bool waits, bool wide, bool pops_twice, uint8_t store_source)
{
  return agx_driver_state_store_item(builder, store_source, waits, wide) &&
         agx_driver_state_pop_run(builder, pops_twice ? 1u : 0u, 0x20u);
}

uint32_t
agx_pool_state_arm_build(uint8_t* bytes, uint32_t capacity)
{
  static const struct
  {
    uint8_t destination;
    bool    keep_second;
    bool    byte_2_bit_5;
    bool    wait;
    uint8_t byte_5_low;
    uint8_t byte_6_low;
  } k_adds[] = {
    {3, true, false, false, 0x02, 0x02},
    {2, true, false, false, 0x02, 0x02},
    {1, false, true, false, 0x02, 0x00},
    {0, false, false, true, 0x00, 0x00},
  };

  Agx_Shader_Instruction  storage[16] = {0};
  Agx_Shader_Builder      builder = {0};
  Agx_Shader_Instruction* instruction = NULL;
  uint32_t                i = 0;

  agx_shader_builder_init(&builder, storage, sizeof(storage) / sizeof(storage[0]));

  instruction = agx_shader_load(&builder, 0, 5, 0, 1, 0, 5, AGX_SHADER_LOAD_FORM_DYNAMIC_WIDE, 1);
  if (instruction == NULL)
  {
    return 0;
  }
  instruction->index_is_pair = true;
  instruction->access_width = AGX_SHADER_ACCESS_WIDTH_16;
  instruction->raw_bits |= AGX_SHADER_LOAD_BYTE_2_BIT_4_CLEAR;

  instruction = agx_shader_move_nibble_3(&builder, 0, 0);
  if (instruction == NULL)
  {
    return 0;
  }
  instruction->destination_half = true;
  instruction->move_raw = 0x20u;

  for (i = 0; i < sizeof(k_adds) / sizeof(k_adds[0]); i += 1)
  {
    instruction =
      agx_shader_add_index(&builder, k_adds[i].destination, 0, 0, AGX_SHADER_INDEX_SOURCE_FORM_UNMARKED, false);
    if (instruction == NULL)
    {
      return 0;
    }
    instruction->add_second_source_is_register = true;
    instruction->source_is_uniform = true;
    instruction->second_form = k_adds[i].keep_second ? AGX_SHADER_OPERAND_FORM_KEEP : AGX_SHADER_OPERAND_FORM_DISCARD;
    instruction->wait = k_adds[i].wait;

    instruction->wait_slot = 5;
    instruction->raw_bits = AGX_SHADER_ADD_INDEX_BYTE_7_HIGH_CLEAR | AGX_SHADER_ADD_INDEX_BYTE_9_BIT_1 |
                            (k_adds[i].byte_2_bit_5 ? AGX_SHADER_ADD_INDEX_BYTE_2_BIT_5 : 0u) |
                            ((uint32_t)k_adds[i].byte_5_low << 10) | k_adds[i].byte_6_low;
  }

  if (!agx_driver_state_arm_tail(&builder, false, false, false, 0))
  {
    return 0;
  }

  return agx_shader_encode(&builder, bytes, capacity);
}

uint32_t
agx_pool_state_arm_place(uint8_t* pool, uint32_t at, uint32_t pool_bytes)
{
  if (pool == NULL || at >= pool_bytes)
  {
    agx_refuse("agx_pool_state_arm_place: pool == NULL || at >= pool_bytes");
    return 0;
  }
  return agx_pool_state_arm_build(pool + at, pool_bytes - at);
}

uint32_t
agx_pool_state_arm_two_build(uint8_t* bytes, uint32_t capacity)
{
  static const struct
  {
    uint8_t  destination;
    uint8_t  word_count;
    uint32_t offset;
    bool     index_is_pair;
  } k_loads[] = {
    {2, 2, 0x08, false},
    {1, 1, 0x04, false},
    {0, 1, 0x00, true},
  };

  Agx_Shader_Instruction  storage[16] = {0};
  Agx_Shader_Builder      builder = {0};
  Agx_Shader_Instruction* instruction = NULL;
  uint32_t                i = 0;

  agx_shader_builder_init(&builder, storage, sizeof(storage) / sizeof(storage[0]));

  for (i = 0; i < sizeof(k_loads) / sizeof(k_loads[0]); i += 1)
  {
    instruction = agx_shader_load(
      &builder, k_loads[i].destination, 5, 0, k_loads[i].word_count, k_loads[i].offset, 5, AGX_SHADER_LOAD_FORM_DYNAMIC_WIDE, 1
    );
    if (instruction == NULL)
    {
      return 0;
    }
    instruction->index_is_pair = k_loads[i].index_is_pair;
  }

  if (!agx_driver_state_arm_tail(&builder, true, false, false, 0))
  {
    return 0;
  }

  return agx_shader_encode(&builder, bytes, capacity);
}

uint32_t
agx_pool_state_arm_two_place(uint8_t* pool, uint32_t at, uint32_t pool_bytes)
{
  if (pool == NULL || at >= pool_bytes)
  {
    agx_refuse("agx_pool_state_arm_two_place: pool == NULL || at >= pool_bytes");
    return 0;
  }
  return agx_pool_state_arm_two_build(pool + at, pool_bytes - at);
}

uint32_t
agx_pool_state_arm_three_build(uint8_t* bytes, uint32_t capacity)
{
  Agx_Shader_Instruction  storage[16] = {0};
  Agx_Shader_Builder      builder = {0};
  Agx_Shader_Instruction* instruction = NULL;
  uint32_t                i = 0;

  static const struct
  {
    uint8_t destination;
    bool    index_is_pair;
    uint8_t byte_9_bits;
  } k_loads[] = {
    {1, false, 0x20},
    {0, true, 0x00},
  };
  static const uint8_t k_zero_moves[2] = {2, 3};
  static const uint8_t k_high_moves[2] = {1, 0};

  agx_shader_builder_init(&builder, storage, sizeof(storage) / sizeof(storage[0]));

  for (i = 0; i < sizeof(k_loads) / sizeof(k_loads[0]); i += 1)
  {
    instruction = agx_shader_load(&builder, k_loads[i].destination, 5, 0, 1, 0, 5, AGX_SHADER_LOAD_FORM_DYNAMIC_WIDE, 1);
    if (instruction == NULL)
    {
      return 0;
    }
    instruction->access_width = AGX_SHADER_ACCESS_WIDTH_8;
    instruction->index_is_pair = k_loads[i].index_is_pair;
    instruction->raw_bits |= k_loads[i].byte_9_bits;
  }

  if (agx_shader_move_zero(&builder, k_zero_moves[0]) == NULL)
  {
    return 0;
  }

  for (i = 0; i < sizeof(k_high_moves) / sizeof(k_high_moves[0]); i += 1)
  {
    instruction = agx_shader_move_nibble_3(&builder, k_high_moves[i], 0);
    if (instruction == NULL)
    {
      return 0;
    }
    instruction->destination_half = true;
  }

  if (agx_shader_move_zero(&builder, k_zero_moves[1]) == NULL)
  {
    return 0;
  }

  if (!agx_driver_state_arm_tail(&builder, true, false, false, 0))
  {
    return 0;
  }

  return agx_shader_encode(&builder, bytes, capacity);
}

uint32_t
agx_pool_state_arm_three_place(uint8_t* pool, uint32_t at, uint32_t pool_bytes)
{
  if (pool == NULL || at >= pool_bytes)
  {
    agx_refuse("agx_pool_state_arm_three_place: pool == NULL || at >= pool_bytes");
    return 0;
  }
  return agx_pool_state_arm_three_build(pool + at, pool_bytes - at);
}

uint32_t
agx_pool_state_arm_four_build(uint8_t* bytes, uint32_t capacity)
{
  static const struct
  {
    uint8_t  destination;
    uint8_t  word_count;
    uint32_t offset;
    uint8_t  slot;
    bool     index_is_pair;
    uint8_t  byte_9_bits;
  } k_loads[] = {
    {3, 2, 0x04, 5, false, 0x00},
    {1, 1, 0x00, 0, false, 0x40},
    {0, 1, 0x00, 0, true, 0x00},
  };

  static const struct
  {
    uint8_t  destination;
    uint8_t  source;
    bool     destination_half;
    uint32_t raw;
  } k_moves[] = {
    {4, 0, false, 0x0000u},
    {2, 3, false, 0xc009u},
    {2, 0, true, 0x0000u},
    {1, 0, true, 0x0000u},
    {0, 0, true, 0x0000u},
    {3, 3, false, AGX_SHADER_MOVE_NIBBLE_3_SOURCE_HALF_UNMEASURED | 0x0001u},
    {3, 0, true, 0x0000u},
  };

  Agx_Shader_Instruction  storage[24] = {0};
  Agx_Shader_Builder      builder = {0};
  Agx_Shader_Instruction* instruction = NULL;
  uint32_t                i = 0;

  agx_shader_builder_init(&builder, storage, sizeof(storage) / sizeof(storage[0]));

  for (i = 0; i < sizeof(k_loads) / sizeof(k_loads[0]); i += 1)
  {
    instruction = agx_shader_load(
      &builder,
      k_loads[i].destination,
      5,
      0,
      k_loads[i].word_count,
      k_loads[i].offset,
      k_loads[i].slot,
      AGX_SHADER_LOAD_FORM_DYNAMIC_WIDE,
      1
    );
    if (instruction == NULL)
    {
      return 0;
    }
    instruction->access_width = AGX_SHADER_ACCESS_WIDTH_16;
    instruction->index_is_pair = k_loads[i].index_is_pair;
    instruction->raw_bits |= k_loads[i].byte_9_bits;
  }

  for (i = 0; i < sizeof(k_moves) / sizeof(k_moves[0]); i += 1)
  {
    instruction = agx_shader_move_nibble_3(&builder, k_moves[i].destination, k_moves[i].source);
    if (instruction == NULL)
    {
      return 0;
    }
    instruction->destination_half = k_moves[i].destination_half;
    instruction->move_raw = k_moves[i].raw;
  }

  if (!agx_driver_state_arm_tail(&builder, false, true, false, 0))
  {
    return 0;
  }

  return agx_shader_encode(&builder, bytes, capacity);
}

uint32_t
agx_pool_state_arm_four_place(uint8_t* pool, uint32_t at, uint32_t pool_bytes)
{
  if (pool == NULL || at >= pool_bytes)
  {
    agx_refuse("agx_pool_state_arm_four_place: pool == NULL || at >= pool_bytes");
    return 0;
  }
  return agx_pool_state_arm_four_build(pool + at, pool_bytes - at);
}

uint32_t
agx_pool_state_store_build(uint8_t* bytes, uint32_t capacity)
{
  Agx_Shader_Instruction  storage[4] = {0};
  Agx_Shader_Builder      builder = {0};
  Agx_Shader_Instruction* instruction = NULL;

  agx_shader_builder_init(&builder, storage, sizeof(storage) / sizeof(storage[0]));

  instruction = agx_shader_store_surface(&builder, 4, AGX_SHADER_CHANNEL_WIDTH_32, 0);
  if (instruction == NULL)
  {
    return 0;
  }
  instruction->store_source = 5;
  instruction->store_selector = 0xae;
  instruction->raw_bits = 0x04000du;

  return agx_shader_encode(&builder, bytes, capacity);
}

uint32_t
agx_pool_state_store_place(uint8_t* pool, uint32_t at, uint32_t pool_bytes)
{
  if (pool == NULL || at >= pool_bytes)
  {
    agx_refuse("agx_pool_state_store_place: pool == NULL || at >= pool_bytes");
    return 0;
  }
  return agx_pool_state_store_build(pool + at, pool_bytes - at);
}

uint32_t
agx_pool_state_arm_five_build(uint8_t* bytes, uint32_t capacity)
{
  static const struct
  {
    uint8_t destination;
    uint8_t amount;
    bool    wait;
    uint8_t source_mask_bits;
  } k_shifts[] = {
    {1, 10, true, 0x0a},
    {2, 20, false, 0x0a},
    {3, 30, false, 0x00},
  };

  Agx_Shader_Instruction  storage[24] = {0};
  Agx_Shader_Builder      builder = {0};
  Agx_Shader_Instruction* instruction = NULL;
  uint32_t                i = 0;

  agx_shader_builder_init(&builder, storage, sizeof(storage) / sizeof(storage[0]));

  instruction = agx_shader_load(&builder, 0, 5, 0, 1, 0, 5, AGX_SHADER_LOAD_FORM_DYNAMIC_WIDE, 1);
  if (instruction == NULL)
  {
    return 0;
  }
  instruction->index_is_pair = true;
  instruction->raw_bits |= AGX_SHADER_LOAD_BYTE_2_BIT_4_CLEAR;

  for (i = 0; i < sizeof(k_shifts) / sizeof(k_shifts[0]); i += 1)
  {
    instruction = agx_shader_shift_right(&builder, k_shifts[i].destination, 0, k_shifts[i].amount);
    if (instruction == NULL)
    {
      return 0;
    }
    instruction->wait = k_shifts[i].wait;

    instruction->wait_slot = 5;
    instruction->shift_source_mask_bits = k_shifts[i].source_mask_bits;
    instruction->raw_bits = AGX_SHADER_SHIFT_BYTE_4_IS_TWO | (k_shifts[i].source_mask_bits != 0 ? 0x02u : 0x00u);
  }

  instruction = agx_shader_output_move(&builder, 0, 0, false);
  if (instruction == NULL)
  {
    return 0;
  }
  instruction->long_form = true;
  instruction->raw_bits = AGX_SHADER_OUTPUT_MOVE_BYTE_1_BIT_0 | AGX_SHADER_OUTPUT_MOVE_BYTE_2_BIT_5_CLEAR |
                          AGX_SHADER_OUTPUT_MOVE_BYTE_4_CLEAR | (0x4cu << AGX_SHADER_OUTPUT_MOVE_BYTE_3_SHIFT) |
                          (0x02u << AGX_SHADER_OUTPUT_MOVE_BYTE_5_SHIFT);

  instruction = agx_shader_store_surface(&builder, 4, AGX_SHADER_CHANNEL_WIDTH_8, 0);
  if (instruction == NULL)
  {
    return 0;
  }
  instruction->store_selector = 0xae;
  instruction->raw_bits = 0x04020du;

  for (i = 0; i < 4u; i += 1)
  {
    if (agx_shader_pop_exec(&builder, 1) == NULL)
    {
      return 0;
    }
  }

  return agx_shader_encode(&builder, bytes, capacity);
}

uint32_t
agx_pool_state_arm_eight_build(uint8_t* bytes, uint32_t capacity)
{
  static const struct
  {
    uint8_t  destination;
    uint8_t  word_count;
    uint32_t offset;
    uint8_t  slot;
    bool     index_is_pair;
    uint8_t  byte_9_bits;
  } k_loads[] = {
    {1, 2, 0x04, 5, false, 0x00},
    {2, 1, 0x00, 0, false, 0x40},
    {0, 1, 0x00, 1, true, 0x00},
  };

  static const struct
  {
    uint8_t destination;
    uint8_t second;
    bool    keep_second;
    bool    wait;
    uint8_t slot;
    uint8_t byte_5_low;
    uint8_t byte_6_low;
  } k_adds[] = {
    {8, 1, false, true, 5, 0x02, 0x00},
    {7, 1, false, false, 5, 0x00, 0x00},
    {6, 2, false, false, 0, 0x00, 0x00},
    {5, 0, false, false, 1, 0x00, 0x00},
  };

  Agx_Shader_Instruction  storage[24] = {0};
  Agx_Shader_Builder      builder = {0};
  Agx_Shader_Instruction* instruction = NULL;
  uint32_t                i = 0;

  agx_shader_builder_init(&builder, storage, sizeof(storage) / sizeof(storage[0]));

  for (i = 0; i < sizeof(k_loads) / sizeof(k_loads[0]); i += 1)
  {
    instruction = agx_shader_load(
      &builder,
      k_loads[i].destination,
      5,
      0,
      k_loads[i].word_count,
      k_loads[i].offset,
      k_loads[i].slot,
      AGX_SHADER_LOAD_FORM_DYNAMIC_WIDE,
      1
    );
    if (instruction == NULL)
    {
      return 0;
    }
    instruction->access_width = AGX_SHADER_ACCESS_WIDTH_16;
    instruction->index_is_pair = k_loads[i].index_is_pair;
    instruction->raw_bits |= k_loads[i].byte_9_bits;
  }

  for (i = 0; i < sizeof(k_adds) / sizeof(k_adds[0]); i += 1)
  {
    instruction =
      agx_shader_add_index(&builder, k_adds[i].destination, 0, 0, AGX_SHADER_INDEX_SOURCE_FORM_UNMARKED, false);
    if (instruction == NULL)
    {
      return 0;
    }
    instruction->source_b = k_adds[i].second;
    instruction->add_second_source_is_register = true;
    instruction->source_is_uniform = true;
    instruction->second_form = k_adds[i].keep_second ? AGX_SHADER_OPERAND_FORM_KEEP : AGX_SHADER_OPERAND_FORM_DISCARD;
    instruction->wait = k_adds[i].wait;
    instruction->wait_slot = k_adds[i].slot;
    instruction->first_in_program = k_adds[i].slot < 4u;
    instruction->raw_bits = AGX_SHADER_ADD_INDEX_BYTE_7_HIGH_CLEAR | AGX_SHADER_ADD_INDEX_BYTE_9_BIT_1 |
                            ((uint32_t)k_adds[i].byte_5_low << 10) | k_adds[i].byte_6_low;
  }

  if (!agx_driver_state_arm_tail(&builder, false, false, true, 5))
  {
    return 0;
  }

  return agx_shader_encode(&builder, bytes, capacity);
}

uint32_t
agx_pool_state_arm_eight_place(uint8_t* pool, uint32_t at, uint32_t pool_bytes)
{
  if (pool == NULL || at >= pool_bytes)
  {
    agx_refuse("agx_pool_state_arm_eight_place: pool == NULL || at >= pool_bytes");
    return 0;
  }
  return agx_pool_state_arm_eight_build(pool + at, pool_bytes - at);
}

uint32_t
agx_pool_state_arm_nine_build(uint8_t* bytes, uint32_t capacity)
{
  static const struct
  {
    uint8_t  destination;
    uint8_t  word_count;
    uint32_t offset;
    uint8_t  slot;
    bool     index_is_pair;
    uint8_t  byte_9_bits;
  } k_loads[] = {
    {1, 1, 0x00, 5, false, 0x40},
    {0, 1, 0x00, 0, true, 0x00},
  };
  static const struct
  {
    uint8_t destination;
    uint8_t second;
    bool    keep_second;
    bool    wait;
    uint8_t slot;
    uint8_t byte_5_low;
    uint8_t byte_6_low;
  } k_adds[] = {
    {3, 0, true, false, 5, 0x02, 0x02},
    {2, 0, false, false, 5, 0x02, 0x00},
    {1, 1, false, true, 5, 0x00, 0x00},
    {0, 0, false, false, 0, 0x00, 0x00},
  };

  Agx_Shader_Instruction  storage[24] = {0};
  Agx_Shader_Builder      builder = {0};
  Agx_Shader_Instruction* instruction = NULL;
  uint32_t                i = 0;

  agx_shader_builder_init(&builder, storage, sizeof(storage) / sizeof(storage[0]));

  for (i = 0; i < sizeof(k_loads) / sizeof(k_loads[0]); i += 1)
  {
    instruction = agx_shader_load(
      &builder,
      k_loads[i].destination,
      5,
      0,
      k_loads[i].word_count,
      k_loads[i].offset,
      k_loads[i].slot,
      AGX_SHADER_LOAD_FORM_DYNAMIC_WIDE,
      1
    );
    if (instruction == NULL)
    {
      return 0;
    }
    instruction->access_width = AGX_SHADER_ACCESS_WIDTH_16;
    instruction->index_is_pair = k_loads[i].index_is_pair;
    instruction->raw_bits |= k_loads[i].byte_9_bits;
  }

  instruction = agx_shader_move_nibble_3(&builder, 0, 0);
  if (instruction == NULL)
  {
    return 0;
  }
  instruction->destination_half = true;
  instruction->move_raw = 0x20u;

  for (i = 0; i < sizeof(k_adds) / sizeof(k_adds[0]); i += 1)
  {
    instruction =
      agx_shader_add_index(&builder, k_adds[i].destination, 0, 0, AGX_SHADER_INDEX_SOURCE_FORM_UNMARKED, false);
    if (instruction == NULL)
    {
      return 0;
    }
    instruction->source_b = k_adds[i].second;
    instruction->add_second_source_is_register = true;
    instruction->source_is_uniform = true;
    instruction->second_form = k_adds[i].keep_second ? AGX_SHADER_OPERAND_FORM_KEEP : AGX_SHADER_OPERAND_FORM_DISCARD;
    instruction->wait = k_adds[i].wait;
    instruction->wait_slot = k_adds[i].slot;
    instruction->first_in_program = k_adds[i].slot < 4u;
    instruction->raw_bits = AGX_SHADER_ADD_INDEX_BYTE_7_HIGH_CLEAR | AGX_SHADER_ADD_INDEX_BYTE_9_BIT_1 |
                            ((uint32_t)k_adds[i].byte_5_low << 10) | k_adds[i].byte_6_low;
  }

  if (!agx_driver_state_arm_tail(&builder, false, false, true, 0))
  {
    return 0;
  }

  return agx_shader_encode(&builder, bytes, capacity);
}

uint32_t
agx_pool_state_arm_nine_place(uint8_t* pool, uint32_t at, uint32_t pool_bytes)
{
  if (pool == NULL || at >= pool_bytes)
  {
    agx_refuse("agx_pool_state_arm_nine_place: pool == NULL || at >= pool_bytes");
    return 0;
  }
  return agx_pool_state_arm_nine_build(pool + at, pool_bytes - at);
}

static bool
agx_driver_state_chain_head(Agx_Shader_Builder* builder, uint8_t immediate)
{
  Agx_Shader_Instruction* instruction = agx_shader_compare_immediate(builder, 37, AGX_SHADER_COMPARE_LESS, immediate);

  if (instruction == NULL)
  {
    return false;
  }

  instruction->compare_type = AGX_SHADER_COMPARE_TYPE_SIGNED;
  instruction->raw_bits |= AGX_SHADER_COMPARE_BYTE_4_BIT_7;
  return true;
}

uint32_t
agx_pool_state_arm_six_build(uint8_t* bytes, uint32_t capacity)
{
  Agx_Shader_Instruction  storage[16] = {0};
  Agx_Shader_Builder      builder = {0};
  Agx_Shader_Instruction* instruction = NULL;
  uint32_t                i = 0;
  static const uint8_t    k_zeroed_after[] = {2, 3};

  agx_shader_builder_init(&builder, storage, sizeof(storage) / sizeof(storage[0]));

  if (!agx_driver_state_chain_head(&builder, 4))
  {
    return 0;
  }

  if (agx_shader_move_zero(&builder, 1) == NULL || agx_shader_push_exec(&builder, false) == NULL)
  {
    return 0;
  }

  instruction = agx_shader_load(&builder, 0, 5, 0, 1, 0, 5, AGX_SHADER_LOAD_FORM_DYNAMIC_WIDE, 1);
  if (instruction == NULL)
  {
    return 0;
  }

  instruction->index_is_pair = true;

  for (i = 0; i < sizeof(k_zeroed_after) / sizeof(k_zeroed_after[0]); i += 1)
  {
    if (agx_shader_move_zero(&builder, k_zeroed_after[i]) == NULL)
    {
      return 0;
    }
  }

  if (!agx_driver_state_arm_tail(&builder, true, false, false, 0))
  {
    return 0;
  }

  return agx_shader_encode(&builder, bytes, capacity);
}

uint32_t
agx_pool_state_arm_six_place(uint8_t* pool, uint32_t at, uint32_t pool_bytes)
{
  if (pool == NULL || at >= pool_bytes)
  {
    agx_refuse("agx_pool_state_arm_six_place: pool == NULL || at >= pool_bytes");
    return 0;
  }
  return agx_pool_state_arm_six_build(pool + at, pool_bytes - at);
}

uint32_t
agx_pool_state_arm_seven_build(uint8_t* bytes, uint32_t capacity)
{
  Agx_Shader_Instruction  storage[8] = {0};
  Agx_Shader_Builder      builder = {0};
  Agx_Shader_Instruction* instruction = NULL;

  agx_shader_builder_init(&builder, storage, sizeof(storage) / sizeof(storage[0]));

  if (agx_shader_jump_exec_none_bytes(&builder, 0x9e) == NULL || !agx_driver_state_chain_head(&builder, 11))
  {
    return 0;
  }

  instruction = agx_shader_move_nibble_3(&builder, 2, 0);
  if (instruction == NULL)
  {
    return 0;
  }
  instruction->move_raw = 0x20u;

  if (agx_shader_move_zero(&builder, 7) == NULL || agx_shader_push_exec(&builder, false) == NULL)
  {
    return 0;
  }

  return agx_shader_encode(&builder, bytes, capacity);
}

static bool
agx_driver_state_chain_head_wide(Agx_Shader_Builder* builder, uint8_t immediate)
{
  Agx_Shader_Instruction* instruction = NULL;

  if (!agx_driver_state_chain_head(builder, immediate))
  {
    return false;
  }
  instruction = &builder->instructions[builder->count - 1u];
  instruction->long_form = true;
  return true;
}

uint32_t
agx_pool_state_arm_ten_build(uint8_t* bytes, uint32_t capacity)
{
  Agx_Shader_Instruction  storage[24] = {0};
  Agx_Shader_Builder      builder = {0};
  Agx_Shader_Instruction* instruction = NULL;

  agx_shader_builder_init(&builder, storage, sizeof(storage) / sizeof(storage[0]));

  if (agx_shader_jump_exec_none_bytes(&builder, 0x42) == NULL)
  {
    return 0;
  }

  instruction = agx_shader_load(&builder, 0, 5, 0, 1, 0, 5, AGX_SHADER_LOAD_FORM_DYNAMIC_WIDE, 1);
  if (instruction == NULL)
  {
    return 0;
  }
  instruction->index_is_pair = true;
  instruction->access_width = AGX_SHADER_ACCESS_WIDTH_8;
  instruction->raw_bits |= AGX_SHADER_LOAD_BYTE_2_BIT_4_CLEAR;

  instruction = agx_shader_shift_left(&builder, 0, 0, 24);
  if (instruction == NULL)
  {
    return 0;
  }
  instruction->wait = true;
  instruction->wait_slot = 5;
  instruction->raw_bits |= 1u << AGX_SHADER_SHIFT_BYTE_9_LOW_SHIFT;

  instruction = agx_shader_shift_right_arithmetic(&builder, 0, 0, 24);
  if (instruction == NULL)
  {
    return 0;
  }

  instruction->source_form = AGX_SHADER_SOURCE_FORM_DISCARD;
  instruction->raw_bits |= AGX_SHADER_SHIFT_BYTE_4_IS_TWO;

  if (agx_shader_move_zero(&builder, 2) == NULL || agx_shader_move_zero(&builder, 3) == NULL ||
      !agx_driver_state_store_item(&builder, 0, false, false) || !agx_driver_state_pop_run(&builder, 2, 0x00u) ||
      agx_shader_jump_exec_none_bytes(&builder, 0x1ce) == NULL || !agx_driver_state_chain_head(&builder, 9))
  {
    return 0;
  }

  instruction = agx_shader_push_exec(&builder, false);
  if (instruction == NULL)
  {
    return 0;
  }
  instruction->raw_bits |= AGX_SHADER_PUSH_EXEC_BYTE_2_BIT_5 | AGX_SHADER_PUSH_EXEC_BYTE_2_BIT_4_CLEAR;

  if (agx_shader_jump_exec_none_bytes(&builder, 0xb2) == NULL || !agx_driver_state_chain_head(&builder, 6) ||
      agx_shader_push_exec(&builder, false) == NULL || agx_shader_jump_exec_none_bytes(&builder, 0x50) == NULL)
  {
    return 0;
  }

  return agx_shader_encode(&builder, bytes, capacity);
}

uint32_t
agx_pool_state_arm_ten_place(uint8_t* pool, uint32_t at, uint32_t pool_bytes)
{
  if (pool == NULL || at >= pool_bytes)
  {
    agx_refuse("agx_pool_state_arm_ten_place: pool == NULL || at >= pool_bytes");
    return 0;
  }
  return agx_pool_state_arm_ten_build(pool + at, pool_bytes - at);
}

uint32_t
agx_pool_state_arm_eleven_build(uint8_t* bytes, uint32_t capacity)
{
  Agx_Shader_Instruction  storage[24] = {0};
  Agx_Shader_Builder      builder = {0};
  Agx_Shader_Instruction* instruction = NULL;

  agx_shader_builder_init(&builder, storage, sizeof(storage) / sizeof(storage[0]));

  if (agx_shader_jump_exec_none_bytes(&builder, 0x44) == NULL || !agx_driver_state_chain_head_wide(&builder, 6) ||
      agx_shader_move_zero(&builder, 1) == NULL || agx_shader_push_exec(&builder, false) == NULL)
  {
    return 0;
  }

  instruction = agx_shader_load(&builder, 0, 5, 0, 1, 0, 5, AGX_SHADER_LOAD_FORM_DYNAMIC_WIDE, 1);
  if (instruction == NULL)
  {
    return 0;
  }
  instruction->index_is_pair = true;

  if (agx_shader_move_zero(&builder, 2) == NULL || agx_shader_move_zero(&builder, 3) == NULL ||
      !agx_driver_state_store_item(&builder, 0, true, false) || !agx_driver_state_pop_run(&builder, 2, 0x20u) ||
      agx_shader_jump_exec_none_bytes(&builder, 0xfe) == NULL || !agx_driver_state_chain_head(&builder, 10) ||
      agx_shader_push_exec(&builder, false) == NULL || agx_shader_jump_exec_none_bytes(&builder, 0x42) == NULL)
  {
    return 0;
  }

  return agx_shader_encode(&builder, bytes, capacity);
}

uint32_t
agx_pool_state_arm_eleven_place(uint8_t* pool, uint32_t at, uint32_t pool_bytes)
{
  if (pool == NULL || at >= pool_bytes)
  {
    agx_refuse("agx_pool_state_arm_eleven_place: pool == NULL || at >= pool_bytes");
    return 0;
  }
  return agx_pool_state_arm_eleven_build(pool + at, pool_bytes - at);
}

uint32_t
agx_pool_state_arm_fourteen_build(uint8_t* bytes, uint32_t capacity)
{
  static const struct
  {
    uint8_t  immediate;
    uint32_t distance;
  } k_heads[] = {
    {18, 0x1de},
    {14, 0x0e2},
    {13, 0x066},
  };

  Agx_Shader_Instruction  storage[48] = {0};
  Agx_Shader_Builder      builder = {0};
  Agx_Shader_Instruction* instruction = NULL;
  uint32_t                i = 0;

  agx_shader_builder_init(&builder, storage, sizeof(storage) / sizeof(storage[0]));

  if (agx_shader_jump_exec_none_bytes(&builder, 0x42) == NULL)
  {
    return 0;
  }
  for (i = 0; i < 2u; i += 1)
  {
    instruction = agx_shader_load(&builder, 1u - i, 5, 0, 1, 0, 5, AGX_SHADER_LOAD_FORM_DYNAMIC_WIDE, 1);
    if (instruction == NULL)
    {
      return 0;
    }
    instruction->access_width = AGX_SHADER_ACCESS_WIDTH_16;
    instruction->index_is_pair = i != 0;
    instruction->raw_bits |= i == 0 ? 0x40u : 0x00u;
  }
  for (i = 0; i < 3u; i += 1)
  {
    instruction = agx_shader_move_nibble_3(&builder, 2u - i, 0);
    if (instruction == NULL)
    {
      return 0;
    }
    instruction->destination_half = true;
    instruction->move_raw = i == 0 ? 0x20u : 0x00u;
  }
  instruction = agx_shader_move(&builder, 3, 2);
  if (instruction == NULL)
  {
    return 0;
  }
  instruction->move_keeps_source = true;

  if (!agx_driver_state_store_item(&builder, 0, true, false) || !agx_driver_state_pop_run(&builder, 0, 0x20u) ||
      agx_shader_jump_exec_none_bytes(&builder, 0x36) == NULL)
  {
    return 0;
  }

  for (i = 0; i < 2u; i += 1)
  {
    instruction = agx_shader_load(&builder, 6u - i, 5, 0, 1, i == 0 ? 4u : 0u, 5, AGX_SHADER_LOAD_FORM_DYNAMIC_WIDE, 1);
    if (instruction == NULL)
    {
      return 0;
    }
    instruction->index_is_pair = i != 0;
    instruction->raw_bits |= i == 0 ? 0x80u : 0x00u;
  }
  instruction = agx_shader_move(&builder, 8, 7);
  if (instruction == NULL)
  {
    return 0;
  }
  instruction->move_keeps_source = true;

  if (!agx_driver_state_store_item(&builder, 5, true, false) || !agx_driver_state_pop_run(&builder, 4, 0x20u) ||
      agx_shader_jump_exec_none_bytes(&builder, 0x4aa) == NULL)
  {
    return 0;
  }

  for (i = 0; i < sizeof(k_heads) / sizeof(k_heads[0]); i += 1)
  {
    if (!agx_driver_state_chain_head(&builder, k_heads[i].immediate) || agx_shader_push_exec(&builder, false) == NULL ||
        agx_shader_jump_exec_none_bytes(&builder, k_heads[i].distance) == NULL)
    {
      return 0;
    }
  }

  for (i = 0; i < 2u; i += 1)
  {
    instruction = agx_shader_load(&builder, 1u - i, 5, 0, 1, 0, i == 0 ? 5u : 0u, AGX_SHADER_LOAD_FORM_DYNAMIC_WIDE, 1);
    if (instruction == NULL)
    {
      return 0;
    }
    instruction->access_width = AGX_SHADER_ACCESS_WIDTH_8;
    instruction->index_is_pair = i != 0;
    instruction->raw_bits |= i == 0 ? 0x20u : 0x00u;
  }
  for (i = 0; i < 2u; i += 1)
  {
    instruction = agx_shader_shift_left(&builder, 1u - i, 1u - i, 24);
    if (instruction == NULL)
    {
      return 0;
    }
    instruction->wait = i == 0;
    instruction->wait_slot = i == 0 ? 5u : 0u;
    instruction->first_in_program = i != 0;
    instruction->raw_bits |= 1u << AGX_SHADER_SHIFT_BYTE_9_LOW_SHIFT;
  }
  for (i = 0; i < 2u; i += 1)
  {
    instruction = agx_shader_shift_right_arithmetic(&builder, 1u - i, 1u - i, 24);
    if (instruction == NULL)
    {
      return 0;
    }
    instruction->source_form = AGX_SHADER_SOURCE_FORM_DISCARD;
    instruction->wait_slot = 5;
    instruction->raw_bits |= AGX_SHADER_SHIFT_BYTE_4_IS_TWO;
  }

  if (agx_shader_move_zero(&builder, 2) == NULL || agx_shader_move_zero(&builder, 3) == NULL ||
      !agx_driver_state_store_item(&builder, 0, false, false) || !agx_driver_state_pop_run(&builder, 0, 0x20u) ||
      agx_shader_jump_exec_none_bytes(&builder, 0x5e) == NULL)
  {
    return 0;
  }

  return agx_shader_encode(&builder, bytes, capacity);
}

uint32_t
agx_pool_state_arm_fourteen_place(uint8_t* pool, uint32_t at, uint32_t pool_bytes)
{
  if (pool == NULL || at >= pool_bytes)
  {
    agx_refuse("agx_pool_state_arm_fourteen_place: pool == NULL || at >= pool_bytes");
    return 0;
  }
  return agx_pool_state_arm_fourteen_build(pool + at, pool_bytes - at);
}

uint32_t
agx_pool_state_arm_thirteen_build(uint8_t* bytes, uint32_t capacity)
{
  Agx_Shader_Instruction  storage[40] = {0};
  Agx_Shader_Builder      builder = {0};
  Agx_Shader_Instruction* instruction = NULL;
  uint32_t                i = 0;

  static const struct
  {
    uint8_t destination;
    uint8_t slot;
    bool    index_is_pair;
    uint8_t width;
    uint8_t byte_9_bits;
  } k_second_loads[] = {
    {2, 5, false, AGX_SHADER_ACCESS_WIDTH_8, 0x40},
    {1, 5, false, AGX_SHADER_ACCESS_WIDTH_8, 0x20},
    {0, 5, true, AGX_SHADER_ACCESS_WIDTH_8, 0x00},
  };

  agx_shader_builder_init(&builder, storage, sizeof(storage) / sizeof(storage[0]));

  if (agx_shader_jump_exec_none_bytes(&builder, 0xde) == NULL || !agx_driver_state_chain_head(&builder, 17) ||
      agx_shader_push_exec(&builder, false) == NULL || agx_shader_jump_exec_none_bytes(&builder, 0x58) == NULL ||
      !agx_driver_state_chain_head_wide(&builder, 14))
  {
    return 0;
  }

  instruction = agx_shader_move_zero(&builder, 2);
  if (instruction == NULL)
  {
    return 0;
  }
  instruction->raw_bits |= AGX_SHADER_MOVE_BYTE_2_BIT_5;
  instruction->move_keeps_source = true;

  if (agx_shader_push_exec(&builder, false) == NULL || agx_shader_jump_exec_none_bytes(&builder, 0x36) == NULL)
  {
    return 0;
  }

  for (i = 0; i < 2u; i += 1)
  {
    instruction = agx_shader_load(&builder, 1u - i, 5, 0, 1, i == 0 ? 4u : 0u, 5, AGX_SHADER_LOAD_FORM_DYNAMIC_WIDE, 1);
    if (instruction == NULL)
    {
      return 0;
    }
    instruction->index_is_pair = i != 0;
    instruction->raw_bits |= i == 0 ? 0x80u : 0x00u;
  }

  instruction = agx_shader_move(&builder, 3, 2);
  if (instruction == NULL)
  {
    return 0;
  }
  instruction->move_keeps_source = true;

  if (!agx_driver_state_store_item(&builder, 0, true, false) || !agx_driver_state_pop_run(&builder, 1, 0x20u) ||
      agx_shader_jump_exec_none_bytes(&builder, 0x68) == NULL)
  {
    return 0;
  }

  for (i = 0; i < sizeof(k_second_loads) / sizeof(k_second_loads[0]); i += 1)
  {
    instruction = agx_shader_load(
      &builder, k_second_loads[i].destination, 5, 0, i == 0 ? 2u : 1u, 0, k_second_loads[i].slot, AGX_SHADER_LOAD_FORM_DYNAMIC_WIDE, 1
    );
    if (instruction == NULL)
    {
      return 0;
    }
    instruction->access_width = k_second_loads[i].width;
    instruction->index_is_pair = k_second_loads[i].index_is_pair;
    instruction->raw_bits |= k_second_loads[i].byte_9_bits;

    if (i == 0)
    {
      instruction = agx_shader_shift_right(&builder, 3, 2, 8);
      if (instruction == NULL)
      {
        return 0;
      }
      instruction->wait = true;
      instruction->wait_slot = 5;
      instruction->raw_bits |= 0x02u | AGX_SHADER_SHIFT_BYTE_8_BIT_4_CLEAR | AGX_SHADER_SHIFT_BYTE_8_BIT_5_CLEAR;
    }
  }

  for (i = 0; i < 2u; i += 1)
  {
    instruction = agx_shader_bit_op(&builder, 3u - i, 0xff, 3u - i, AGX_SHADER_BIT_AND);
    if (instruction == NULL)
    {
      return 0;
    }
    instruction->source_immediate = true;
    instruction->keep_source_b = false;
    instruction->source_form = AGX_SHADER_SOURCE_FORM_DISCARD;
    instruction->raw_bits |= 2u << AGX_SHADER_BIT_OP_BYTE_5_SHIFT;
  }

  for (i = 0; i < 2u; i += 1)
  {
    instruction = agx_shader_move_nibble_3(&builder, 1u - i, 0);
    if (instruction == NULL)
    {
      return 0;
    }
    instruction->destination_half = true;
  }

  if (!agx_driver_state_store_item(&builder, 0, true, false) || !agx_driver_state_pop_run(&builder, 2, 0x20u) ||
      agx_shader_jump_exec_none_bytes(&builder, 0x2ae) == NULL || !agx_driver_state_chain_head(&builder, 20) ||
      agx_shader_push_exec(&builder, false) == NULL || agx_shader_jump_exec_none_bytes(&builder, 0x11e) == NULL ||
      !agx_driver_state_chain_head(&builder, 19) || agx_shader_push_exec(&builder, false) == NULL ||
      agx_shader_jump_exec_none_bytes(&builder, 0x5c) == NULL)
  {
    return 0;
  }

  return agx_shader_encode(&builder, bytes, capacity);
}

uint32_t
agx_pool_state_arm_thirteen_place(uint8_t* pool, uint32_t at, uint32_t pool_bytes)
{
  if (pool == NULL || at >= pool_bytes)
  {
    agx_refuse("agx_pool_state_arm_thirteen_place: pool == NULL || at >= pool_bytes");
    return 0;
  }
  return agx_pool_state_arm_thirteen_build(pool + at, pool_bytes - at);
}

uint32_t
agx_pool_state_chain_start_build(uint8_t* bytes, uint32_t capacity)
{
  static const struct
  {
    uint8_t  immediate;
    uint32_t distance;
    bool     push_variant;
    bool     wide;
  } k_heads[] = {
    {5, 0x152, false, false},
    {3, 0x0ae, false, false},
  };

  Agx_Shader_Instruction  storage[40] = {0};
  Agx_Shader_Builder      builder = {0};
  Agx_Shader_Instruction* instruction = NULL;
  uint32_t                i = 0;

  agx_shader_builder_init(&builder, storage, sizeof(storage) / sizeof(storage[0]));

  if (agx_shader_push_exec(&builder, false) == NULL || agx_shader_jump_exec_none_bytes(&builder, 0x33e) == NULL)
  {
    return 0;
  }

  for (i = 0; i < sizeof(k_heads) / sizeof(k_heads[0]); i += 1)
  {
    if (!agx_driver_state_chain_head(&builder, k_heads[i].immediate) || agx_shader_push_exec(&builder, false) == NULL ||
        agx_shader_jump_exec_none_bytes(&builder, k_heads[i].distance) == NULL)
    {
      return 0;
    }
  }

  if (!agx_driver_state_chain_head(&builder, 2))
  {
    return 0;
  }
  instruction = agx_shader_move_nibble_3(&builder, 2, 0);
  if (instruction == NULL)
  {
    return 0;
  }
  instruction->destination_half = true;
  instruction = agx_shader_push_exec(&builder, false);
  if (instruction == NULL)
  {
    return 0;
  }
  instruction->raw_bits |= AGX_SHADER_PUSH_EXEC_BYTE_2_BIT_5 | AGX_SHADER_PUSH_EXEC_BYTE_2_BIT_4_CLEAR;

  if (agx_shader_jump_exec_none_bytes(&builder, 0x58) == NULL || !agx_driver_state_chain_head_wide(&builder, 1) ||
      agx_shader_push_exec(&builder, false) == NULL || agx_shader_jump_exec_none_bytes(&builder, 0x3a) == NULL)
  {
    return 0;
  }

  instruction = agx_shader_load(&builder, 0, 5, 0, 1, 0, 5, AGX_SHADER_LOAD_FORM_DYNAMIC_WIDE, 1);
  if (instruction == NULL)
  {
    return 0;
  }
  instruction->index_is_pair = true;
  instruction->access_width = AGX_SHADER_ACCESS_WIDTH_8;
  instruction->raw_bits |= AGX_SHADER_LOAD_BYTE_2_BIT_4_CLEAR;

  if (agx_shader_move_zero(&builder, 1) == NULL || agx_shader_move_zero(&builder, 2) == NULL ||
      agx_shader_move_zero(&builder, 3) == NULL)
  {
    return 0;
  }

  instruction = agx_shader_move_nibble_3(&builder, 0, 0);
  if (instruction == NULL)
  {
    return 0;
  }
  instruction->long_form = true;
  instruction->move_raw = 0x06u | (4u << AGX_SHADER_MOVE_NIBBLE_3_RAW_NIBBLE_SHIFT);

  if (!agx_driver_state_store_item(&builder, 0, true, false) || !agx_driver_state_pop_run(&builder, 1, 0x20u) ||
      agx_shader_jump_exec_none_bytes(&builder, 0x34) == NULL)
  {
    return 0;
  }

  instruction = agx_shader_load(&builder, 0, 5, 0, 1, 0, 5, AGX_SHADER_LOAD_FORM_DYNAMIC_WIDE, 1);
  if (instruction == NULL)
  {
    return 0;
  }
  instruction->index_is_pair = true;
  instruction->access_width = AGX_SHADER_ACCESS_WIDTH_16;

  if (agx_shader_move_zero(&builder, 3) == NULL || agx_shader_move_nibble_3(&builder, 2, 0) == NULL ||
      agx_shader_move_zero(&builder, 1) == NULL)
  {
    return 0;
  }

  instruction = agx_shader_move_nibble_3(&builder, 0, 0);
  if (instruction == NULL)
  {
    return 0;
  }
  instruction->destination_half = true;

  if (!agx_driver_state_store_item(&builder, 0, true, false) || !agx_driver_state_pop_run(&builder, 1, 0x20u) ||
      agx_shader_jump_exec_none_bytes(&builder, 0x86) == NULL)
  {
    return 0;
  }

  return agx_shader_encode(&builder, bytes, capacity);
}

uint32_t
agx_pool_state_chain_start_place(uint8_t* pool, uint32_t at, uint32_t pool_bytes)
{
  if (pool == NULL || at >= pool_bytes)
  {
    agx_refuse("agx_pool_state_chain_start_place: pool == NULL || at >= pool_bytes");
    return 0;
  }
  return agx_pool_state_chain_start_build(pool + at, pool_bytes - at);
}

uint32_t
agx_pool_state_arm_twelve_build(uint8_t* bytes, uint32_t capacity)
{
  static const struct
  {
    uint8_t destination;
    uint8_t word_count;
    uint8_t slot;
    bool    index_is_pair;
    uint8_t byte_9_bits;
  } k_loads[] = {
    {1, 2, 5, false, 0x40},
    {3, 1, 5, false, 0x20},
    {0, 1, 0, true, 0x00},
  };

  static const struct
  {
    uint8_t destination;
    uint8_t source;
    bool    left;
    bool    wait;
    uint8_t slot;
    bool    byte_4_is_two;
  } k_shifts[] = {
    {2, 2, true, false, 5, false},
    {8, 2, false, false, 5, true},
    {2, 1, true, false, 5, true},
    {1, 3, true, true, 5, false},
    {0, 0, true, false, 0, false},
    {7, 2, false, false, 5, true},
    {6, 1, false, false, 5, true},
    {5, 0, false, false, 5, true},
  };

  Agx_Shader_Instruction  storage[32] = {0};
  Agx_Shader_Builder      builder = {0};
  Agx_Shader_Instruction* instruction = NULL;
  uint32_t                i = 0;

  agx_shader_builder_init(&builder, storage, sizeof(storage) / sizeof(storage[0]));

  if (agx_shader_jump_exec_none_bytes(&builder, 0xa4) == NULL)
  {
    return 0;
  }

  for (i = 0; i < sizeof(k_loads) / sizeof(k_loads[0]); i += 1)
  {
    instruction = agx_shader_load(
      &builder, k_loads[i].destination, 5, 0, k_loads[i].word_count, 0, k_loads[i].slot, AGX_SHADER_LOAD_FORM_DYNAMIC_WIDE, 1
    );
    if (instruction == NULL)
    {
      return 0;
    }
    instruction->access_width = AGX_SHADER_ACCESS_WIDTH_8;
    instruction->index_is_pair = k_loads[i].index_is_pair;
    instruction->raw_bits |= k_loads[i].byte_9_bits;

    if (i == 0)
    {
      instruction = agx_shader_shift_right(&builder, 2, 1, 8);
      if (instruction == NULL)
      {
        return 0;
      }
      instruction->wait = true;
      instruction->wait_slot = 5;
      instruction->raw_bits |= 0x02u | AGX_SHADER_SHIFT_BYTE_8_BIT_4_CLEAR | AGX_SHADER_SHIFT_BYTE_8_BIT_5_CLEAR;
    }
  }

  for (i = 0; i < sizeof(k_shifts) / sizeof(k_shifts[0]); i += 1)
  {
    instruction = k_shifts[i].left
                  ? agx_shader_shift_left(&builder, k_shifts[i].destination, k_shifts[i].source, 24)
                  : agx_shader_shift_right_arithmetic(&builder, k_shifts[i].destination, k_shifts[i].source, 24);
    if (instruction == NULL)
    {
      return 0;
    }
    instruction->wait = k_shifts[i].wait;
    instruction->wait_slot = k_shifts[i].slot;
    instruction->first_in_program = k_shifts[i].slot < 4u;
    if (k_shifts[i].left)
    {
      instruction->raw_bits |= 1u << AGX_SHADER_SHIFT_BYTE_9_LOW_SHIFT;
    }
    else
    {
      instruction->source_form = AGX_SHADER_SOURCE_FORM_DISCARD;
    }
    if (k_shifts[i].byte_4_is_two)
    {
      instruction->raw_bits |= AGX_SHADER_SHIFT_BYTE_4_IS_TWO;
    }
  }

  if (!agx_driver_state_store_item(&builder, 5, false, false) || !agx_driver_state_pop_run(&builder, 1, 0x20u) ||
      agx_shader_jump_exec_none_bytes(&builder, 0x172) == NULL || !agx_driver_state_chain_head(&builder, 23) ||
      agx_shader_push_exec(&builder, false) == NULL || agx_shader_jump_exec_none_bytes(&builder, 0x86) == NULL ||
      !agx_driver_state_chain_head_wide(&builder, 20) || agx_shader_push_exec(&builder, false) == NULL ||
      agx_shader_jump_exec_none_bytes(&builder, 0x68) == NULL)
  {
    return 0;
  }

  return agx_shader_encode(&builder, bytes, capacity);
}

uint32_t
agx_pool_state_arm_twelve_place(uint8_t* pool, uint32_t at, uint32_t pool_bytes)
{
  if (pool == NULL || at >= pool_bytes)
  {
    agx_refuse("agx_pool_state_arm_twelve_place: pool == NULL || at >= pool_bytes");
    return 0;
  }
  return agx_pool_state_arm_twelve_build(pool + at, pool_bytes - at);
}

uint32_t
agx_pool_state_chain_link_wide_build(uint8_t* bytes, uint32_t capacity)
{
  Agx_Shader_Instruction storage[8] = {0};
  Agx_Shader_Builder     builder = {0};

  agx_shader_builder_init(&builder, storage, sizeof(storage) / sizeof(storage[0]));

  if (agx_shader_jump_exec_none_bytes(&builder, 0x70) == NULL || !agx_driver_state_chain_head_wide(&builder, 25) ||
      agx_shader_push_exec(&builder, false) == NULL || agx_shader_jump_exec_none_bytes(&builder, 0x52) == NULL)
  {
    return 0;
  }

  return agx_shader_encode(&builder, bytes, capacity);
}

uint32_t
agx_pool_state_chain_link_wide_place(uint8_t* pool, uint32_t at, uint32_t pool_bytes)
{
  if (pool == NULL || at >= pool_bytes)
  {
    agx_refuse("agx_pool_state_chain_link_wide_place: pool == NULL || at >= pool_bytes");
    return 0;
  }
  return agx_pool_state_chain_link_wide_build(pool + at, pool_bytes - at);
}

uint32_t
agx_pool_state_chain_link_build(uint8_t* bytes, uint32_t capacity)
{
  Agx_Shader_Instruction storage[8] = {0};
  Agx_Shader_Builder     builder = {0};

  agx_shader_builder_init(&builder, storage, sizeof(storage) / sizeof(storage[0]));

  if (agx_shader_jump_exec_none_bytes(&builder, 0xce) == NULL || !agx_driver_state_chain_head(&builder, 25) ||
      agx_shader_push_exec(&builder, false) == NULL || agx_shader_jump_exec_none_bytes(&builder, 0x40) == NULL)
  {
    return 0;
  }

  return agx_shader_encode(&builder, bytes, capacity);
}

uint32_t
agx_pool_state_chain_link_place(uint8_t* pool, uint32_t at, uint32_t pool_bytes)
{
  if (pool == NULL || at >= pool_bytes)
  {
    agx_refuse("agx_pool_state_chain_link_place: pool == NULL || at >= pool_bytes");
    return 0;
  }
  return agx_pool_state_chain_link_build(pool + at, pool_bytes - at);
}

uint32_t
agx_pool_state_arm_seven_place(uint8_t* pool, uint32_t at, uint32_t pool_bytes)
{
  if (pool == NULL || at >= pool_bytes)
  {
    agx_refuse("agx_pool_state_arm_seven_place: pool == NULL || at >= pool_bytes");
    return 0;
  }
  return agx_pool_state_arm_seven_build(pool + at, pool_bytes - at);
}

uint32_t
agx_pool_state_arm_five_place(uint8_t* pool, uint32_t at, uint32_t pool_bytes)
{
  if (pool == NULL || at >= pool_bytes)
  {
    agx_refuse("agx_pool_state_arm_five_place: pool == NULL || at >= pool_bytes");
    return 0;
  }
  return agx_pool_state_arm_five_build(pool + at, pool_bytes - at);
}
