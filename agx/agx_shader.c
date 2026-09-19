#include "agx_refusal.h"
#include "agx_shader.h"

#include <stdio.h>
#include <string.h>

#define AGX_SHADER_WAIT_SLOT_IMPLICIT 5u

static uint32_t
agx_shader_flag(bool set, uint32_t bit)
{
  return set ? (1u << bit) : 0u;
}

static uint32_t
agx_shader_pack_bits(uint32_t value, uint32_t width, uint32_t shift)
{
  return (value & ((1u << width) - 1u)) << shift;
}

static uint32_t
agx_shader_take_bits(uint32_t value, uint32_t width, uint32_t shift)
{
  return (value >> shift) & ((1u << width) - 1u);
}

void
agx_shader_builder_init(Agx_Shader_Builder* builder, Agx_Shader_Instruction* storage, uint32_t capacity)
{
  builder->instructions = storage;
  builder->count = 0;
  builder->capacity = capacity;
  builder->overflowed = false;
  builder->wait_scratch = 0;
  builder->has_wait_scratch = false;
}

static Agx_Shader_Instruction*
agx_shader_append(Agx_Shader_Builder* builder, Agx_Shader_Op op)
{
  Agx_Shader_Instruction* instruction = NULL;

  if (builder->count >= builder->capacity)
  {
    builder->overflowed = true;
    return NULL;
  }

  instruction = &builder->instructions[builder->count++];
  memset(instruction, 0, sizeof(*instruction));
  instruction->op = op;
  return instruction;
}

Agx_Shader_Instruction*
agx_shader_nop(Agx_Shader_Builder* builder)
{
  return agx_shader_append(builder, AGX_SHADER_OP_NOP);
}

Agx_Shader_Instruction*
agx_shader_stop(Agx_Shader_Builder* builder)
{
  return agx_shader_append(builder, AGX_SHADER_OP_STOP);
}

Agx_Shader_Instruction*
agx_shader_store_buffer(
  Agx_Shader_Builder* builder,
  uint8_t             source,
  uint8_t             base,
  uint8_t             index,
  uint8_t             word_count,
  uint32_t            offset,
  bool                waits,
  bool                discards_index
)
{
  Agx_Shader_Instruction* instruction = agx_shader_append(builder, AGX_SHADER_OP_STORE_BUFFER);

  if (instruction)
  {
    instruction->store_source = source;
    instruction->base = base;
    instruction->index = index;
    instruction->word_count = word_count;
    instruction->offset = offset;
    instruction->store_waits = waits;

    instruction->index_scale = 4;
    instruction->store_discards_index = discards_index;
  }
  return instruction;
}

Agx_Shader_Instruction*
agx_shader_atomic(Agx_Shader_Builder* builder, Agx_Shader_Atomic_Op operation)
{
  Agx_Shader_Instruction* instruction = agx_shader_append(builder, AGX_SHADER_OP_ATOMIC);

  if (instruction)
  {
    instruction->atomic_operation = (uint8_t)operation;

    instruction->implicit_base = 0;

    instruction->atomic_writes_destination = true;
  }
  return instruction;
}

Agx_Shader_Instruction*
agx_shader_atomic_returning(Agx_Shader_Builder* builder, Agx_Shader_Atomic_Op operation)
{
  Agx_Shader_Instruction* instruction = agx_shader_atomic(builder, operation);

  if (instruction)
  {
    instruction->atomic_result_slot = 1;

    instruction->atomic_writes_destination = false;
  }
  return instruction;
}

Agx_Shader_Instruction*
agx_shader_atomic_result(Agx_Shader_Builder* builder, uint8_t destination)
{
  Agx_Shader_Instruction* instruction = NULL;

  if (destination > 15u)
  {
    return NULL;
  }
  instruction = agx_shader_append(builder, AGX_SHADER_OP_ATOMIC_RESULT);
  if (instruction)
  {
    instruction->destination = destination;
  }
  return instruction;
}

Agx_Shader_Instruction*
agx_shader_compare_immediate(Agx_Shader_Builder* builder, uint8_t source, Agx_Shader_Compare compare, uint8_t immediate)
{
  if (compare == AGX_SHADER_COMPARE_EQUAL)
  {
    return NULL;
  }

  Agx_Shader_Instruction* instruction = agx_shader_append(builder, AGX_SHADER_OP_COMPARE);

  if (instruction)
  {
    instruction->source = source;
    instruction->compare = compare;
    instruction->compare_type = AGX_SHADER_COMPARE_TYPE_UNSIGNED;

    instruction->compare_immediate = immediate;
  }
  return instruction;
}

Agx_Shader_Instruction*
agx_shader_compare_register(Agx_Shader_Builder* builder, uint8_t source, Agx_Shader_Compare compare, uint8_t second)
{
  if (compare == AGX_SHADER_COMPARE_EQUAL)
  {
    return NULL;
  }

  Agx_Shader_Instruction* instruction = agx_shader_append(builder, AGX_SHADER_OP_COMPARE);

  if (instruction)
  {
    instruction->source = source;
    instruction->compare = compare;
    instruction->compare_type = AGX_SHADER_COMPARE_TYPE_UNSIGNED;
    instruction->compare_second = second;
    instruction->second_is_register = true;
  }
  return instruction;
}

Agx_Shader_Instruction*
agx_shader_push_exec(Agx_Shader_Builder* builder, bool invert)
{
  Agx_Shader_Instruction* instruction = agx_shader_append(builder, AGX_SHADER_OP_PUSH_EXEC);

  if (instruction)
  {
    instruction->invert = invert;
  }
  return instruction;
}

Agx_Shader_Instruction*
agx_shader_barrier(Agx_Shader_Builder* builder, Agx_Shader_Memory_Scope scope)
{
  Agx_Shader_Instruction* instruction = agx_shader_append(builder, AGX_SHADER_OP_BARRIER);

  if (instruction)
  {
    instruction->memory_scope = scope;

    instruction->raw_bits = 0x10u;
  }
  return instruction;
}

Agx_Shader_Instruction*
agx_shader_pop_exec(Agx_Shader_Builder* builder, uint8_t nest)
{
  Agx_Shader_Instruction* instruction = agx_shader_append(builder, AGX_SHADER_OP_POP_EXEC);

  if (instruction)
  {
    instruction->nest = nest;
  }
  return instruction;
}

Agx_Shader_Instruction*
agx_shader_move_zero(Agx_Shader_Builder* builder, uint8_t destination)
{
  Agx_Shader_Instruction* instruction = agx_shader_append(builder, AGX_SHADER_OP_MOVE);

  if (instruction)
  {
    instruction->destination = destination;
    instruction->move_writes_zero = true;

    instruction->raw_bits = 0u;
    instruction->move_keeps_source = true;
  }
  return instruction;
}

Agx_Shader_Instruction*
agx_shader_pop_exec_short(Agx_Shader_Builder* builder, uint8_t byte_3)
{
  Agx_Shader_Instruction* instruction = agx_shader_append(builder, AGX_SHADER_OP_POP_EXEC);

  if (instruction)
  {
    instruction->pop_short_form = true;

    instruction->raw_bits = (uint32_t)byte_3 << 8;
  }
  return instruction;
}

Agx_Shader_Instruction*
agx_shader_jump_exec_any(Agx_Shader_Builder* builder, uint32_t instructions)
{
  Agx_Shader_Instruction* instruction = agx_shader_append(builder, AGX_SHADER_OP_JUMP_EXEC_ANY);

  if (instruction)
  {
    instruction->skip_instructions = instructions;
  }
  return instruction;
}

Agx_Shader_Instruction*
agx_shader_jump_exec_none_bytes(Agx_Shader_Builder* builder, uint32_t distance)
{
  Agx_Shader_Instruction* instruction = agx_shader_append(builder, AGX_SHADER_OP_JUMP_EXEC_NONE);

  if (instruction)
  {
    instruction->jump_distance = distance;
    instruction->jump_distance_given = true;
  }
  return instruction;
}

Agx_Shader_Instruction*
agx_shader_jump_exec_none_to_pop(Agx_Shader_Builder* builder)
{
  Agx_Shader_Instruction* instruction = agx_shader_append(builder, AGX_SHADER_OP_JUMP_EXEC_NONE);

  if (instruction)
  {
    instruction->skip_instructions = AGX_SHADER_JUMP_TO_MATCHING_POP;
  }
  return instruction;
}

Agx_Shader_Instruction*
agx_shader_jump_absolute(Agx_Shader_Builder* builder, uint64_t target)
{
  Agx_Shader_Instruction* instruction = agx_shader_append(builder, AGX_SHADER_OP_JUMP_ABSOLUTE);

  if (instruction)
  {
    instruction->jump_target = target;
  }
  return instruction;
}

Agx_Shader_Instruction*
agx_shader_move_immediate(Agx_Shader_Builder* builder, uint8_t destination, uint32_t immediate)
{
  if (destination < AGX_SHADER_VIRTUAL_REGISTER_BASE && destination > 63u)
  {
    return NULL;
  }

  Agx_Shader_Instruction* instruction = agx_shader_append(builder, AGX_SHADER_OP_MOVE_IMMEDIATE);

  if (instruction)
  {
    instruction->destination = destination;
    instruction->immediate = immediate;
  }
  return instruction;
}

Agx_Shader_Instruction*
agx_shader_move(Agx_Shader_Builder* builder, uint8_t destination, uint8_t source)
{
  Agx_Shader_Instruction* instruction = agx_shader_append(builder, AGX_SHADER_OP_MOVE);

  if (instruction)
  {
    instruction->destination = destination;
    instruction->source = source;
  }
  return instruction;
}

Agx_Shader_Instruction*
agx_shader_discard(Agx_Shader_Builder* builder, uint8_t reg)
{
  Agx_Shader_Instruction* instruction = agx_shader_append(builder, AGX_SHADER_OP_DISCARD);

  if (instruction)
  {
    instruction->source = reg;
  }
  return instruction;
}

Agx_Shader_Instruction*
agx_shader_load(
  Agx_Shader_Builder*  builder,
  uint8_t              destination,
  uint8_t              base,
  uint8_t              index,
  uint8_t              word_count,
  uint32_t             offset,
  uint8_t              slot,
  Agx_Shader_Load_Form form,
  uint8_t              index_scale
)
{
  Agx_Shader_Instruction* instruction = agx_shader_append(builder, AGX_SHADER_OP_LOAD);

  if (instruction)
  {
    instruction->destination = destination;
    instruction->base = base;
    instruction->index = index;
    instruction->word_count = word_count;
    instruction->offset = offset;
    instruction->slot = slot;
    instruction->load_form = form;
    instruction->index_scale = index_scale;
  }
  return instruction;
}

Agx_Shader_Instruction*
agx_shader_load_element(
  Agx_Shader_Builder* builder,
  uint8_t             destination,
  uint8_t             base,
  uint8_t             index,
  uint8_t             word_count,
  uint32_t            offset,
  uint8_t             slot,
  uint8_t             element_bytes
)
{
  if (element_bytes == 0u)
  {
    agx_refuse("agx_shader_load_element: an element of zero bytes has no index to scale");
    return NULL;
  }
  if (element_bytes > AGX_SHADER_LOAD_ELEMENT_BYTES_MAX)
  {
    if (agx_shader_scale_index(
          builder, index, index, element_bytes, AGX_SHADER_INDEX_FORM_FULL, AGX_SHADER_INDEX_SOURCE_FORM_DISCARD, true, false
        ) == NULL)
    {
      return NULL;
    }
    return agx_shader_load(builder, destination, base, index, word_count, offset, slot, AGX_SHADER_LOAD_FORM_ORDINARY, 1u);
  }

  return agx_shader_load(
    builder, destination, base, index, word_count, offset, slot, AGX_SHADER_LOAD_FORM_DYNAMIC_WIDE, element_bytes
  );
}

Agx_Shader_Instruction*
agx_shader_store_element(
  Agx_Shader_Builder* builder,
  uint8_t             source,
  uint8_t             base,
  uint8_t             index,
  uint8_t             word_count,
  uint32_t            offset,
  bool                waits,
  uint8_t             element_bytes
)
{
  Agx_Shader_Instruction* instruction = NULL;
  uint8_t                 scale = element_bytes;
  bool                    prescaled = false;

  if (element_bytes == 0u)
  {
    agx_refuse("agx_shader_store_element: an element of zero bytes has no index to scale");
    return NULL;
  }
  if (element_bytes > AGX_SHADER_LOAD_ELEMENT_BYTES_MAX)
  {
    if (agx_shader_scale_index(
          builder, index, index, element_bytes, AGX_SHADER_INDEX_FORM_FULL, AGX_SHADER_INDEX_SOURCE_FORM_DISCARD, true, false
        ) == NULL)
    {
      return NULL;
    }
    scale = 1u;
    prescaled = true;
  }
  instruction = agx_shader_store_buffer(builder, source, base, index, word_count, offset, waits, false);
  if (instruction != NULL)
  {
    instruction->index_scale = scale;
    instruction->store_index_prescaled = prescaled;
  }
  return instruction;
}

Agx_Shader_Instruction*
agx_shader_branch(Agx_Shader_Builder* builder, uint8_t landing_site)
{
  Agx_Shader_Instruction* instruction = agx_shader_append(builder, AGX_SHADER_OP_BRANCH);

  if (instruction)
  {
    instruction->landing_site = landing_site;
    instruction->discard_pending = true;
  }
  return instruction;
}

Agx_Shader_Instruction*
agx_shader_half_compare_select(Agx_Shader_Builder* builder, uint8_t destination, uint8_t source)
{
  Agx_Shader_Instruction* instruction = agx_shader_append(builder, AGX_SHADER_OP_HALF_COMPARE_SELECT);

  if (instruction)
  {
    instruction->destination = destination;
    instruction->source = source;
  }
  return instruction;
}

Agx_Shader_Instruction*
agx_shader_move_nibble_3(Agx_Shader_Builder* builder, uint8_t destination, uint8_t source)
{
  Agx_Shader_Instruction* instruction = NULL;

  if (destination < AGX_SHADER_VIRTUAL_REGISTER_BASE && destination > 31u)
  {
    return NULL;
  }

  instruction = agx_shader_append(builder, AGX_SHADER_OP_MOVE_NIBBLE_3);

  if (instruction)
  {
    instruction->destination = destination;
    instruction->source = source;
  }
  return instruction;
}

Agx_Shader_Instruction*
agx_shader_convert_to_float(Agx_Shader_Builder* builder, uint8_t destination, uint8_t source)
{
  Agx_Shader_Instruction* instruction = agx_shader_append(builder, AGX_SHADER_OP_CONVERT_TO_FLOAT);

  if (instruction)
  {
    instruction->destination = destination;
    instruction->source = source;
  }
  return instruction;
}

Agx_Shader_Instruction*
agx_shader_convert_to_integer(Agx_Shader_Builder* builder, uint8_t destination, uint8_t source, bool is_signed)
{
  Agx_Shader_Instruction* instruction = agx_shader_append(builder, AGX_SHADER_OP_CONVERT_TO_INTEGER);

  if (instruction)
  {
    instruction->destination = destination;
    instruction->source = source;
    instruction->source_signed = is_signed;
    instruction->destination_is_word = true;
    instruction->source_is_word = true;

    instruction->convert_source_form = AGX_SHADER_CONVERT_SOURCE_FORM_KEEP;

    instruction->convert_destination_type = is_signed ? 0x0348u : 0x0208u;

    instruction->convert_bytes =
      0x07ull | (0x54ull << 8) | (0x02ull << 16) | (0x00ull << 24) | (0x90ull << 32) | (0x00ull << 40);
  }
  return instruction;
}

Agx_Shader_Instruction*
agx_shader_get_special(Agx_Shader_Builder* builder, uint8_t destination, Agx_Shader_Special special)
{
  Agx_Shader_Instruction* instruction = NULL;

  if (destination < AGX_SHADER_VIRTUAL_REGISTER_BASE && (agx_shader_take_bits(destination, 5, 0)) != destination)
  {
    return NULL;
  }

  instruction = agx_shader_append(builder, AGX_SHADER_OP_GET_SPECIAL);

  if (instruction)
  {
    instruction->destination = destination;
    instruction->special = special;

    instruction->long_form = true;
  }
  return instruction;
}

Agx_Shader_Instruction*
agx_shader_call_pool_shader(Agx_Shader_Builder* builder, uint32_t pool_offset, uint64_t program_address)
{
  Agx_Shader_Instruction* instruction = NULL;

  if (2u * (uint64_t)pool_offset + 0x2au > 0xffffu)
  {
    return NULL;
  }
  instruction = agx_shader_append(builder, AGX_SHADER_OP_CALL_POOL_SHADER);
  if (instruction)
  {
    instruction->pool_offset = pool_offset;
    instruction->program_address = program_address;
  }
  return instruction;
}

Agx_Shader_Instruction*
agx_shader_move_to_tile(Agx_Shader_Builder* builder, uint8_t destination, uint8_t source)
{
  Agx_Shader_Instruction* instruction = agx_shader_append(builder, AGX_SHADER_OP_MOVE_TO_TILE);

  if (instruction)
  {
    instruction->destination = destination;
    instruction->source = source;
  }
  return instruction;
}

Agx_Shader_Instruction*
agx_shader_store_tile_pixel(Agx_Shader_Builder* builder, uint8_t data_register, uint8_t tile_destination)
{
  Agx_Shader_Instruction* instruction = agx_shader_append(builder, AGX_SHADER_OP_STORE_TILE_PIXEL);

  if (instruction)
  {
    instruction->source = data_register;
    instruction->tile_destination = tile_destination;
  }
  return instruction;
}

Agx_Shader_Instruction*
agx_shader_end_thread(Agx_Shader_Builder* builder, uint8_t operand)
{
  Agx_Shader_Instruction* instruction = agx_shader_append(builder, AGX_SHADER_OP_END_THREAD);

  if (instruction)
  {
    instruction->end_thread_operand = operand;
  }
  return instruction;
}

Agx_Shader_Instruction*
agx_shader_end_thread_long(Agx_Shader_Builder* builder, uint8_t operand)
{
  Agx_Shader_Instruction* instruction = agx_shader_append(builder, AGX_SHADER_OP_END_THREAD_LONG);

  if (instruction)
  {
    instruction->end_thread_operand = operand;
  }
  return instruction;
}

Agx_Shader_Instruction*
agx_shader_shift_left(Agx_Shader_Builder* builder, uint8_t destination, uint8_t source, uint8_t amount)
{
  Agx_Shader_Instruction* instruction = agx_shader_shift_right(builder, destination, source, amount);

  if (instruction)
  {
    instruction->shift_left = true;
  }
  return instruction;
}

Agx_Shader_Instruction*
agx_shader_shift_right(Agx_Shader_Builder* builder, uint8_t destination, uint8_t source, uint8_t amount)
{
  Agx_Shader_Instruction* instruction = agx_shader_append(builder, AGX_SHADER_OP_SHIFT_RIGHT);

  if (instruction)
  {
    instruction->destination = destination;
    instruction->source = source;
    instruction->shift_amount = amount;
  }
  return instruction;
}

bool
agx_shader_register_is_sampled(const Agx_Shader_Builder* builder, uint8_t reg)
{
  uint32_t at = 0;

  if (builder == NULL)
  {
    return false;
  }
  for (at = builder->count; at > 0; at -= 1)
  {
    const Agx_Shader_Instruction* instruction = &builder->instructions[at - 1u];
    Agx_Shader_Operands           operands = {0};
    uint32_t                      which = 0;

    if (!agx_shader_operands(instruction, &operands))
    {
      continue;
    }
    for (which = 0; which < operands.count; which += 1)
    {
      uint32_t base = agx_shader_operand_register(&operands, which);
      uint32_t span = operands.width[which] > 0 ? operands.width[which] : 1u;

      if (operands.role[which] != AGX_SHADER_OPERAND_ROLE_DEFINE || reg < base || reg >= base + span)
      {
        continue;
      }
      return instruction->op == AGX_SHADER_OP_TEXTURE_SAMPLE;
    }
  }
  return false;
}

Agx_Shader_Instruction*
agx_shader_pack_texel(
  Agx_Shader_Builder*      builder,
  uint8_t                  destination,
  const uint8_t            sources[4],
  bool                     sources_are_half,
  bool                     alpha_immediate,
  uint8_t                  alpha,
  Agx_Shader_Packed_Format format
)
{
  Agx_Shader_Instruction* instruction = agx_shader_append(builder, AGX_SHADER_OP_PACK_TEXEL);
  uint32_t                channel = 0;

  if (instruction == NULL)
  {
    return NULL;
  }
  instruction->destination = destination;
  for (channel = 0; channel < AGX_SHADER_PACK_CHANNELS; channel += 1)
  {
    instruction->packed_sources[channel] = sources[channel];
  }

  if (alpha_immediate)
  {
    instruction->packed_sources[3] = 0;
  }
  instruction->packed_sources_are_half = sources_are_half;
  instruction->packed_alpha_immediate = alpha_immediate;
  instruction->packed_alpha = alpha;
  instruction->packed_format = format;
  return instruction;
}

Agx_Shader_Instruction*
agx_shader_convert_to_surface(
  Agx_Shader_Builder*       builder,
  uint8_t                   destination,
  bool                      first,
  Agx_Shader_Convert_Form   form,
  uint8_t                   source_a,
  uint8_t                   source_b,
  Agx_Shader_Numeric_Format format
)
{
  Agx_Shader_Instruction* instruction = agx_shader_append(builder, AGX_SHADER_OP_CONVERT_TO_SURFACE);

  if (instruction)
  {
    instruction->destination = destination;
    instruction->first_converter = first;
    instruction->convert_form = form;
    instruction->source = source_a;
    instruction->source_b = source_b;
    instruction->numeric_format = format;
  }
  return instruction;
}

static uint16_t
agx_shader_half_from_float(float value)
{
  uint32_t bits = 0;
  uint32_t sign = 0;
  int32_t  exponent = 0;
  uint32_t mantissa = 0;

  memcpy(&bits, &value, sizeof(bits));
  sign = (bits >> 16) & 0x8000u;
  exponent = (int32_t)(agx_shader_take_bits(bits, 8, 23)) - 127 + 15;
  mantissa = agx_shader_take_bits(bits, 10, 13);

  if (exponent <= 0)
  {
    return (uint16_t)sign;
  }
  if (exponent >= 0x1f)
  {
    return (uint16_t)(sign | 0x7bffu);
  }
  return (uint16_t)(sign | ((uint32_t)exponent << 10) | mantissa);
}

static bool
agx_shader_pack_constant_exact(float value, uint16_t dropped)
{
  uint16_t half = agx_shader_half_from_float(value);

  return half == 0 || (((half & 0xf000u) == 0x3000u) && (half & dropped) == 0);
}

Agx_Shader_Instruction*
agx_shader_pack_constant(Agx_Shader_Builder* builder, uint8_t destination, float first, float second)
{
  Agx_Shader_Instruction* instruction = agx_shader_append(builder, AGX_SHADER_OP_PACK_CONSTANT);

  if (instruction)
  {
    instruction->destination = destination;
    instruction->pack_first = first;
    instruction->pack_second = second;
  }
  return instruction;
}

bool
agx_shader_pack_constant_representable(float first, float second)
{
  return agx_shader_pack_constant_exact(first, 0x00ffu) && agx_shader_pack_constant_exact(second, 0x03ffu);
}

Agx_Shader_Instruction*
agx_shader_output_move(Agx_Shader_Builder* builder, uint8_t output_index, uint8_t source, bool wait)
{
  Agx_Shader_Instruction* instruction = agx_shader_append(builder, AGX_SHADER_OP_OUTPUT_MOVE);

  if (instruction)
  {
    instruction->output_index = output_index;
    instruction->source = source;
    instruction->wait = wait;
    instruction->long_form = true;
  }
  return instruction;
}

Agx_Shader_Instruction*
agx_shader_vertex_export(Agx_Shader_Builder* builder, uint8_t output_index, uint8_t source, uint8_t position_component)
{
  Agx_Shader_Instruction* instruction = agx_shader_append(builder, AGX_SHADER_OP_VERTEX_EXPORT);

  if (instruction)
  {
    uint32_t positions = 0;
    uint32_t varyings = 0;
    uint32_t i = 0;

    instruction->output_index = output_index;
    instruction->source = source;
    instruction->position_component = position_component;

    for (i = 0; i < builder->count; ++i)
    {
      if (builder->instructions[i].op != AGX_SHADER_OP_VERTEX_EXPORT || &builder->instructions[i] == instruction)
      {
        continue;
      }
      if (builder->instructions[i].position_component)
      {
        positions++;
      }
      else
      {
        varyings++;
      }
    }
    if (position_component)
    {
      instruction->export_slot = (uint8_t)positions;
      instruction->export_varying = 0;
    }
    else
    {
      instruction->export_slot = (uint8_t)(4u + varyings);
      instruction->export_varying = (uint8_t)(varyings + 1u);
    }
  }
  return instruction;
}

Agx_Shader_Instruction*
agx_shader_varying_read(Agx_Shader_Builder* builder, uint8_t destination, uint8_t varying_slot, Agx_Shader_Varying_Form form)
{
  Agx_Shader_Instruction* instruction = agx_shader_append(builder, AGX_SHADER_OP_VARYING_READ);

  if (instruction)
  {
    instruction->destination = destination;
    instruction->varying_slot = varying_slot;
    instruction->varying_form = form;
  }
  return instruction;
}

bool
agx_shader_float_immediate(float value, uint8_t* out)
{
  uint32_t code = 0;

  for (code = 0; code < 128u; code += 1)
  {
    uint32_t exponent = agx_shader_take_bits(code, 3, 4);
    uint32_t mantissa = agx_shader_take_bits(code, 4, 0);
    float    held = 0.0f;
    int32_t  shift = 0;

    if (exponent == 0)
    {
      held = (float)mantissa / 16.0f;
      shift = -2;
    }
    else
    {
      held = 1.0f + (float)mantissa / 16.0f;
      shift = (int32_t)exponent - 3;
    }

    while (shift > 0)
    {
      held *= 2.0f;
      shift -= 1;
    }
    while (shift < 0)
    {
      held *= 0.5f;
      shift += 1;
    }

    if (held == value)
    {
      *out = (uint8_t)code;
      return true;
    }
  }
  return false;
}

Agx_Shader_Instruction*
agx_shader_compare_select(
  Agx_Shader_Builder* builder,
  uint8_t             destination,
  uint8_t             source,
  Agx_Shader_Compare  compare,
  uint8_t             immediate,
  uint8_t             true_value,
  uint8_t             false_value
)
{
  Agx_Shader_Instruction* instruction = agx_shader_append(builder, AGX_SHADER_OP_COMPARE_SELECT);

  if (instruction)
  {
    instruction->destination = destination;
    instruction->source = source;
    instruction->compare = compare;
    instruction->compare_immediate = immediate;
    instruction->true_value = true_value;
    instruction->false_value = false_value;

    instruction->compare_type = AGX_SHADER_COMPARE_TYPE_UNSIGNED;
    instruction->source_half = true;

    instruction->second_form = AGX_SHADER_OPERAND_FORM_KEEP;

    instruction->source_form = AGX_SHADER_SOURCE_FORM_KEEP;
  }
  return instruction;
}

Agx_Shader_Instruction*
agx_shader_min_max(
  Agx_Shader_Builder*     builder,
  uint8_t                 destination,
  uint8_t                 source,
  uint8_t                 source_b,
  bool                    minimum,
  Agx_Shader_Min_Max_Type type
)
{
  Agx_Shader_Instruction* instruction = agx_shader_append(builder, AGX_SHADER_OP_MIN_MAX);

  if (instruction)
  {
    instruction->destination = destination;
    instruction->source = source;
    instruction->source_b = source_b;

    instruction->source_form = AGX_SHADER_SOURCE_FORM_KEEP;
    instruction->minimum = minimum;
    instruction->min_max_type = type;

    instruction->source_half = true;
    instruction->source_b_half = true;

    instruction->second_form = AGX_SHADER_OPERAND_FORM_KEEP;
  }
  return instruction;
}

Agx_Shader_Instruction*
agx_shader_float_unary(Agx_Shader_Builder* builder, uint8_t destination, uint8_t source, Agx_Shader_Float_Unary which)
{
  Agx_Shader_Instruction* instruction = agx_shader_append(builder, AGX_SHADER_OP_FLOAT_UNARY);

  if (instruction)
  {
    instruction->destination = destination;
    instruction->source = source;
    instruction->float_unary = which;

    instruction->wait = true;

    instruction->source_form = AGX_SHADER_SOURCE_FORM_KEEP;
  }
  return instruction;
}

Agx_Shader_Instruction*
agx_shader_reciprocal(Agx_Shader_Builder* builder, uint8_t destination, uint8_t source, uint8_t unknown_byte6)
{
  Agx_Shader_Instruction* instruction = agx_shader_append(builder, AGX_SHADER_OP_RECIPROCAL);

  if (instruction)
  {
    instruction->destination = destination;
    instruction->source = source;
    instruction->raw_bits = unknown_byte6;
  }
  return instruction;
}

Agx_Shader_Instruction*
agx_shader_multiply(Agx_Shader_Builder* builder, uint8_t destination, uint8_t source_a, uint8_t source_b, uint8_t wait_slot, bool discard_source_b)
{
  Agx_Shader_Instruction* instruction = agx_shader_append(builder, AGX_SHADER_OP_MULTIPLY);

  if (instruction)
  {
    instruction->destination = destination;
    instruction->source = source_a;
    instruction->source_b = source_b;
    instruction->wait_slot = wait_slot;
    instruction->discard_source_b = discard_source_b;
  }
  return instruction;
}

Agx_Shader_Instruction*
agx_shader_top_bit(Agx_Shader_Builder* builder, uint8_t destination, uint8_t source, bool discard_source)
{
  Agx_Shader_Instruction* instruction = agx_shader_append(builder, AGX_SHADER_OP_TOP_BIT);

  if (instruction)
  {
    instruction->destination = destination;
    instruction->source = source;

    instruction->source_half = true;
    instruction->discard_source = discard_source;
  }
  return instruction;
}

Agx_Shader_Instruction*
agx_shader_sum(Agx_Shader_Builder* builder, uint8_t destination, uint8_t source, uint8_t source_b)
{
  Agx_Shader_Instruction* instruction = agx_shader_append(builder, AGX_SHADER_OP_SUM);

  if (instruction)
  {
    instruction->destination = destination;
    instruction->source = source;
    instruction->source_b = source_b;

    instruction->source_form = AGX_SHADER_SOURCE_FORM_KEEP;
    instruction->second_form = AGX_SHADER_OPERAND_FORM_KEEP;
    instruction->reaches_export = true;
  }
  return instruction;
}

Agx_Shader_Instruction*
agx_shader_widen(Agx_Shader_Builder* builder, uint8_t destination, uint8_t source)
{
  Agx_Shader_Instruction* instruction = agx_shader_append(builder, AGX_SHADER_OP_WIDEN);

  if (instruction)
  {
    instruction->destination = destination;
    instruction->source = source;

    instruction->source_form = AGX_SHADER_SOURCE_FORM_KEEP;
    instruction->reaches_export = true;
  }
  return instruction;
}

Agx_Shader_Instruction*
agx_shader_narrow(Agx_Shader_Builder* builder, uint8_t destination, uint8_t source)
{
  Agx_Shader_Instruction* instruction = agx_shader_append(builder, AGX_SHADER_OP_NARROW);

  if (instruction)
  {
    instruction->destination = destination;
    instruction->source = source;

    instruction->source_form = AGX_SHADER_SOURCE_FORM_KEEP;
    instruction->reaches_export = true;
  }
  return instruction;
}

Agx_Shader_Instruction*
agx_shader_shift_right_arithmetic(Agx_Shader_Builder* builder, uint8_t destination, uint8_t source, uint8_t amount)
{
  Agx_Shader_Instruction* instruction = agx_shader_shift_right(builder, destination, source, amount);

  if (instruction)
  {
    instruction->shift_arithmetic = true;

    instruction->source_form = AGX_SHADER_SOURCE_FORM_KEEP;
  }
  return instruction;
}

Agx_Shader_Instruction*
agx_shader_bit_unary(Agx_Shader_Builder* builder, uint8_t destination, uint8_t source, Agx_Shader_Bit_Unary which)
{
  Agx_Shader_Instruction* instruction = agx_shader_append(builder, AGX_SHADER_OP_BIT_UNARY);

  if (instruction)
  {
    instruction->destination = destination;
    instruction->source = source;
    instruction->bit_unary = which;
    instruction->source_form = AGX_SHADER_SOURCE_FORM_KEEP;
  }
  return instruction;
}

Agx_Shader_Instruction*
agx_shader_add_index(
  Agx_Shader_Builder*          builder,
  uint8_t                      destination,
  uint8_t                      source,
  uint8_t                      immediate,
  Agx_Shader_Index_Source_Form source_form,
  bool                         first_in_program
)
{
  Agx_Shader_Instruction* instruction = agx_shader_append(builder, AGX_SHADER_OP_ADD_INDEX);

  if (instruction)
  {
    instruction->destination = destination;
    instruction->source = source;
    instruction->immediate = immediate;
    instruction->index_source_form = source_form;
    instruction->first_in_program = first_in_program;
  }
  return instruction;
}

Agx_Shader_Instruction*
agx_shader_extend_16(Agx_Shader_Builder* builder, uint8_t destination, uint8_t source, bool sign_extends)
{
  Agx_Shader_Instruction* instruction =
    agx_shader_add_index(builder, destination, source, 0, AGX_SHADER_INDEX_SOURCE_FORM_KEEP, false);

  if (instruction)
  {
    instruction->source_is_16_bit = true;
    instruction->source_sign_extends = sign_extends;
  }
  return instruction;
}

Agx_Shader_Instruction*
agx_shader_product(Agx_Shader_Builder* builder, uint8_t destination, uint8_t source, uint8_t source_b)
{
  Agx_Shader_Instruction* instruction = agx_shader_append(builder, AGX_SHADER_OP_PRODUCT);

  if (instruction)
  {
    instruction->destination = destination;
    instruction->source = source;
    instruction->source_b = source_b;

    instruction->source_form = AGX_SHADER_SOURCE_FORM_KEEP;
    instruction->second_form = AGX_SHADER_OPERAND_FORM_KEEP;
    instruction->reaches_export = true;
  }
  return instruction;
}

Agx_Shader_Instruction*
agx_shader_multiply_add(Agx_Shader_Builder* builder, uint8_t destination, uint8_t source_a, uint8_t source_b, uint8_t source_c)
{
  Agx_Shader_Instruction* instruction = agx_shader_append(builder, AGX_SHADER_OP_MULTIPLY_ADD);

  if (instruction)
  {
    instruction->destination = destination;
    instruction->source = source_a;
    instruction->source_b = source_b;
    instruction->source_c = source_c;

    instruction->source_form = AGX_SHADER_SOURCE_FORM_KEEP;
    instruction->third_form = AGX_SHADER_OPERAND_FORM_KEEP;
    instruction->discard_source_b = false;
    instruction->reaches_export = true;
  }
  return instruction;
}

static bool
agx_shader_minifloat_from_float(float value, uint8_t* encoded)
{
  int32_t exponent = 0;
  int32_t mantissa = 0;

  for (exponent = -11; exponent <= 4; ++exponent)
  {
    float   scale = 1.0f;
    int32_t step = 0;

    for (step = 0; step < exponent; ++step)
    {
      scale *= 2.0f;
    }
    for (step = 0; step > exponent; --step)
    {
      scale *= 0.5f;
    }

    for (mantissa = 0; mantissa < 8; ++mantissa)
    {
      if ((1.0f + (float)mantissa / 8.0f) * scale == value)
      {
        *encoded = (uint8_t)((((uint32_t)(exponent + 11)) << 4) | ((uint32_t)mantissa << 1));
        return true;
      }
    }
  }
  *encoded = 0;
  return false;
}

bool
agx_shader_minifloat_is_exact(float value)
{
  uint8_t encoded = 0;

  return agx_shader_minifloat_from_float(value, &encoded);
}

static uint32_t
agx_shader_multiply_add_length(const Agx_Shader_Instruction* instruction)
{
  if (instruction->accumulate)
  {
    return 4;
  }

  if (instruction->absolute_source || instruction->absolute_source_b)
  {
    return 12;
  }
  if ((agx_shader_take_bits(instruction->raw_bits, 8, 0)) != 0 || instruction->result_wait || instruction->long_form)
  {
    return 10;
  }

  if (instruction->result_exported || instruction->wait || instruction->negate_product || instruction->absolute_third)
  {
    return 8;
  }
  return 6;
}

static uint32_t
agx_shader_product_length(const Agx_Shader_Instruction* instruction)
{
  bool immediate = instruction->second_form == AGX_SHADER_OPERAND_FORM_IMMEDIATE;

  if (instruction->absolute_source || instruction->absolute_source_b)
  {
    return instruction->op == AGX_SHADER_OP_PRODUCT ? 12u : 10u;
  }

  if ((agx_shader_take_bits(instruction->raw_bits, 16, 0)) != 0 || instruction->long_form ||
      (instruction->negate_source_b && !immediate))
  {
    return 8;
  }
  if (instruction->result_exported || instruction->wait || immediate || instruction->negate_source ||
      instruction->destination_high_half || instruction->source_form == AGX_SHADER_SOURCE_FORM_UNIFORM)
  {
    return 6;
  }
  return 4;
}

Agx_Shader_Instruction*
agx_shader_move_for_sample(Agx_Shader_Builder* builder, uint8_t destination, uint8_t source)
{
  Agx_Shader_Instruction* instruction = agx_shader_append(builder, AGX_SHADER_OP_MOVE_FOR_SAMPLE);

  if (instruction)
  {
    instruction->destination = destination;
    instruction->source = source;
  }
  return instruction;
}

static uint8_t
agx_shader_store_channel_code(uint8_t write_mask)
{
  static const uint8_t k_code[16] = {
    0x00u,
    0x1cu,
    0x14u,
    0x08u,
    0x10u,
    0x04u,
    0x18u,
    0x0cu,
    0x02u,
    0x1eu,
    0x16u,
    0x0au,
    0x12u,
    0x06u,
    0x1au,
    0x0eu,
  };

  return k_code[agx_shader_take_bits(write_mask, 4, 0)];
}

uint8_t
agx_shader_store_write_mask_for_channels(uint8_t channel_count)
{
  if (channel_count == 0 || channel_count > 4)
  {
    return 0x0fu;
  }
  return (uint8_t)((0x0fu << (4u - channel_count)) & 0x0fu);
}

static uint8_t
agx_shader_store_selector(uint8_t write_mask, Agx_Shader_Channel_Width channel_width)
{
  uint8_t width = (channel_width == AGX_SHADER_CHANNEL_WIDTH_8)  ? 0x40u
                : (channel_width == AGX_SHADER_CHANNEL_WIDTH_16) ? 0x00u
                                                                 : 0x20u;

  return (uint8_t)(width | agx_shader_store_channel_code(write_mask));
}

Agx_Shader_Instruction*
agx_shader_store_surface(Agx_Shader_Builder* builder, uint8_t channel_count, Agx_Shader_Channel_Width channel_width, uint8_t color_attachment)
{
  Agx_Shader_Instruction* instruction = agx_shader_append(builder, AGX_SHADER_OP_STORE_SURFACE);

  if (instruction)
  {
    instruction->store_write_mask = agx_shader_store_write_mask_for_channels(channel_count);
    instruction->store_selector = agx_shader_store_selector(instruction->store_write_mask, channel_width);
    instruction->store_channel_width = (uint8_t)channel_width;
    instruction->store_wide = (channel_width == AGX_SHADER_CHANNEL_WIDTH_32);
    instruction->color_attachment = color_attachment;

    instruction->store_register_count =
      (uint8_t)(channel_width == AGX_SHADER_CHANNEL_WIDTH_32   ? channel_count
                : channel_width == AGX_SHADER_CHANNEL_WIDTH_16 ? (channel_count + 1u) / 2u
                                                               : (channel_count + 3u) / 4u);
  }
  return instruction;
}

bool
agx_shader_writes_coverage_mask(const Agx_Shader_Builder* builder)
{
  uint32_t i = 0;

  if (builder == NULL || builder->instructions == NULL)
  {
    return false;
  }
  for (i = 0; i < builder->count; ++i)
  {
    if (builder->instructions[i].op == AGX_SHADER_OP_SAMPLE_MASK)
    {
      return true;
    }
  }
  return false;
}

bool
agx_shader_store_surface_set_write_mask(Agx_Shader_Instruction* instruction, uint8_t write_mask)
{
  if (instruction == NULL || instruction->op != AGX_SHADER_OP_STORE_SURFACE ||
      (agx_shader_take_bits(write_mask, 4, 0)) == 0)
  {
    return false;
  }
  instruction->store_write_mask = (uint8_t)(agx_shader_take_bits(write_mask, 4, 0));
  instruction->store_selector =
    agx_shader_store_selector(instruction->store_write_mask, (Agx_Shader_Channel_Width)instruction->store_channel_width);
  return true;
}

Agx_Shader_Instruction*
agx_shader_load_surface(
  Agx_Shader_Builder*      builder,
  uint8_t                  channel_count,
  Agx_Shader_Channel_Width channel_width,
  uint8_t                  color_attachment,
  uint8_t                  destination
)
{
  Agx_Shader_Instruction* instruction = agx_shader_append(builder, AGX_SHADER_OP_LOAD_SURFACE);

  if (instruction)
  {
    uint32_t i = 0;
    bool     earlier = false;

    instruction->store_selector =
      agx_shader_store_selector(agx_shader_store_write_mask_for_channels(channel_count), channel_width);
    instruction->color_attachment = color_attachment;
    instruction->load_destination = destination;

    for (i = 0; i + 1 < builder->count; ++i)
    {
      if (builder->instructions[i].op == AGX_SHADER_OP_LOAD_SURFACE)
      {
        earlier = true;
      }
    }
    if (!earlier)
    {
      instruction->raw_bits |= AGX_SHADER_LOAD_SURFACE_FIRST;
      instruction->load_byte_7_bit_7 = true;
    }
  }
  return instruction;
}

static bool
agx_shader_convert_from_surface_format(Agx_Shader_Numeric_Format format, uint8_t channel_count, uint8_t* width_byte, uint8_t* format_byte)
{
  if (channel_count != 1 && channel_count != 2)
  {
    return false;
  }
  switch (format)
  {
  case AGX_SHADER_NUMERIC_FORMAT_UNORM8:
    *width_byte = 0x14;
    *format_byte = (channel_count == 2) ? 0xeau : 0x6au;
    return true;

  case AGX_SHADER_NUMERIC_FORMAT_UNORM16:
    if (channel_count != 2)
    {
      return false;
    }
    *width_byte = 0x1c;
    *format_byte = 0xca;
    return true;

  case AGX_SHADER_NUMERIC_FORMAT_SRGB_BOTH:
    if (channel_count != 2)
    {
      return false;
    }
    *width_byte = 0x14;
    *format_byte = 0x9au;
    return true;

  case AGX_SHADER_NUMERIC_FORMAT_SRGB_LOW_ONLY:
    if (channel_count != 2)
    {
      return false;
    }
    *width_byte = 0x14;
    *format_byte = 0xdau;
    return true;

  default:
    return false;
  }
}

Agx_Shader_Instruction*
agx_shader_convert_from_surface(
  Agx_Shader_Builder*       builder,
  uint8_t                   destination,
  bool                      first,
  uint8_t                   source,
  bool                      upper_pair,
  uint8_t                   channel_count,
  Agx_Shader_Numeric_Format format
)
{
  uint8_t                 width_byte = 0;
  uint8_t                 format_byte = 0;
  Agx_Shader_Instruction* instruction = NULL;

  if (!agx_shader_convert_from_surface_format(format, channel_count, &width_byte, &format_byte))
  {
    return NULL;
  }

  instruction = agx_shader_append(builder, AGX_SHADER_OP_CONVERT_FROM_SURFACE);
  if (instruction)
  {
    instruction->destination = destination;
    instruction->first_converter = first;
    instruction->source = source;
    instruction->convert_upper_pair = upper_pair;
    instruction->convert_from_width_byte = width_byte;
    instruction->convert_from_format_byte = format_byte;
    instruction->convert_channel_count = channel_count;
  }
  return instruction;
}

Agx_Shader_Instruction*
agx_shader_multiply_integer(
  Agx_Shader_Builder*          builder,
  uint8_t                      destination,
  uint8_t                      source,
  uint8_t                      source_b,
  Agx_Shader_Index_Source_Form source_form
)
{
  Agx_Shader_Instruction* instruction = agx_shader_append(builder, AGX_SHADER_OP_SCALE_INDEX);

  if (instruction)
  {
    instruction->destination = destination;
    instruction->source = source;
    instruction->source_b = source_b;
    instruction->second_is_register = true;

    instruction->second_form = AGX_SHADER_OPERAND_FORM_KEEP;
    instruction->index_form = AGX_SHADER_INDEX_FORM_MULTIPLY;

    instruction->index_source_form = source_form;
  }
  return instruction;
}

Agx_Shader_Instruction*
agx_shader_subtract_registers(
  Agx_Shader_Builder*          builder,
  uint8_t                      destination,
  uint8_t                      minuend,
  uint8_t                      subtrahend,
  Agx_Shader_Index_Source_Form source_form
)
{
  Agx_Shader_Instruction* instruction = agx_shader_add_registers(builder, destination, subtrahend, minuend, source_form);

  if (instruction)
  {
    instruction->subtract = true;
  }
  return instruction;
}

Agx_Shader_Instruction*
agx_shader_add_registers(
  Agx_Shader_Builder*          builder,
  uint8_t                      destination,
  uint8_t                      source,
  uint8_t                      source_b,
  Agx_Shader_Index_Source_Form source_form
)
{
  Agx_Shader_Instruction* instruction = agx_shader_append(builder, AGX_SHADER_OP_ADD_INDEX);

  if (instruction)
  {
    instruction->destination = destination;
    instruction->source = source;
    instruction->source_b = source_b;
    instruction->add_second_source_is_register = true;

    instruction->index_source_form = source_form;
    instruction->second_form = AGX_SHADER_OPERAND_FORM_KEEP;
  }
  return instruction;
}

Agx_Shader_Instruction*
agx_shader_derivative(Agx_Shader_Builder* builder, uint8_t destination, uint8_t source, bool vertical, bool absolute_result, bool last)
{
  Agx_Shader_Instruction* instruction = agx_shader_append(builder, AGX_SHADER_OP_DERIVATIVE);

  if (instruction)
  {
    instruction->destination = destination;
    instruction->source = source;
    instruction->derivative_vertical = vertical;
    instruction->derivative_absolute = absolute_result;
    instruction->derivative_last = last;
  }
  return instruction;
}

Agx_Shader_Instruction*
agx_shader_bit_op(Agx_Shader_Builder* builder, uint8_t destination, uint8_t source, uint8_t source_b, uint8_t table)
{
  Agx_Shader_Instruction* instruction = agx_shader_append(builder, AGX_SHADER_OP_BIT_OP);

  if (instruction)
  {
    instruction->destination = destination;
    instruction->source = source;
    instruction->source_b = source_b;
    instruction->truth_table = (uint8_t)(agx_shader_take_bits(table, 4, 0));
    instruction->source_form = AGX_SHADER_SOURCE_FORM_KEEP;

    instruction->keep_source_b = true;

    instruction->source_half = true;
    instruction->source_b_half = true;
  }
  return instruction;
}

Agx_Shader_Instruction*
agx_shader_wait_for_store(Agx_Shader_Builder* builder, bool active)
{
  Agx_Shader_Instruction* instruction = agx_shader_append(builder, AGX_SHADER_OP_WAIT_FOR_STORE);

  if (instruction)
  {
    instruction->wait = active;

    instruction->wait_operand = AGX_SHADER_WAIT_OPERAND_DEFAULT;
  }
  return instruction;
}

Agx_Shader_Instruction*
agx_shader_scale_index(
  Agx_Shader_Builder*          builder,
  uint8_t                      destination,
  uint8_t                      source,
  uint32_t                     stride,
  Agx_Shader_Index_Form        form,
  Agx_Shader_Index_Source_Form source_form,
  bool                         first_in_program,
  bool                         signed_index
)
{
  Agx_Shader_Instruction* instruction = agx_shader_append(builder, AGX_SHADER_OP_SCALE_INDEX);

  if (instruction)
  {
    instruction->destination = destination;
    instruction->source = source;
    instruction->vertex_stride = stride;
    instruction->index_form = form;
    instruction->index_source_form = source_form;

    instruction->second_form = AGX_SHADER_OPERAND_FORM_KEEP;
    instruction->first_in_program = first_in_program;
    instruction->signed_index = signed_index;
  }
  return instruction;
}

Agx_Shader_Instruction*
agx_shader_texture_sample(Agx_Shader_Builder* builder, uint8_t channel_count, uint8_t lod)
{
  Agx_Shader_Instruction* instruction = agx_shader_append(builder, AGX_SHADER_OP_TEXTURE_SAMPLE);

  if (instruction)
  {
    instruction->sample_channels_minus_one = (uint8_t)((channel_count ? channel_count - 1u : 0u) & 0x03u);
    instruction->lod = lod;
  }
  return instruction;
}

Agx_Shader_Instruction*
agx_shader_sample_mask(Agx_Shader_Builder* builder, uint8_t source, uint8_t slot, bool kill_all)
{
  Agx_Shader_Instruction* release = NULL;
  Agx_Shader_Instruction* before = NULL;
  Agx_Shader_Instruction* second = NULL;
  Agx_Shader_Instruction* commit = NULL;
  Agx_Shader_Instruction* after = NULL;

  release = agx_shader_discard(builder, source);
  if (release == NULL)
  {
    return NULL;
  }
  release->raw_bits = AGX_SHADER_MOVE_BYTE_2_BIT_5;

  before = agx_shader_wait_for_store(builder, true);
  if (before == NULL)
  {
    return NULL;
  }
  before->wait_operand = AGX_SHADER_WAIT_OPERAND_FIRST;

  if (!kill_all)
  {
    second = agx_shader_wait_for_store(builder, true);
    if (second == NULL)
    {
      return NULL;
    }
    second->wait_operand = AGX_SHADER_WAIT_OPERAND_COVERAGE;
  }

  commit = agx_shader_append(builder, AGX_SHADER_OP_SAMPLE_MASK);
  if (commit == NULL)
  {
    return NULL;
  }
  commit->source = source;
  commit->output_index = slot;
  commit->kill_all = kill_all;

  after = agx_shader_wait_for_store(builder, false);
  if (after == NULL)
  {
    return NULL;
  }
  after->wait_operand = kill_all ? AGX_SHADER_WAIT_OPERAND_COVERAGE_KILL : AGX_SHADER_WAIT_OPERAND_COVERAGE;

  return commit;
}

Agx_Shader_Instruction*
agx_shader_pad(Agx_Shader_Builder* builder, uint32_t length)
{
  Agx_Shader_Instruction* instruction = NULL;

  if (length > AGX_SHADER_RAW_MAX)
  {
    builder->overflowed = true;
    return NULL;
  }

  instruction = agx_shader_append(builder, AGX_SHADER_OP_RAW);
  if (instruction)
  {
    memset(instruction->raw, 0, sizeof(instruction->raw));
    instruction->raw_length = length;
  }
  return instruction;
}

Agx_Shader_Instruction*
agx_shader_raw(Agx_Shader_Builder* builder, const uint8_t* bytes, uint32_t length)
{
  Agx_Shader_Instruction* instruction = NULL;

  if (length > AGX_SHADER_RAW_MAX)
  {
    builder->overflowed = true;
    return NULL;
  }

  instruction = agx_shader_append(builder, AGX_SHADER_OP_RAW);
  if (instruction)
  {
    memcpy(instruction->raw, bytes, length);
    instruction->raw_length = length;
  }
  return instruction;
}

static bool
agx_shader_move_immediate_is_short(const Agx_Shader_Instruction* instruction)
{
  return !instruction->long_form && instruction->immediate < 0x80u && instruction->destination < 16u;
}

uint32_t
agx_shader_instruction_length(const Agx_Shader_Instruction* instruction)
{
  switch (instruction->op)
  {
  case AGX_SHADER_OP_NOP:
    return 2;
  case AGX_SHADER_OP_STOP:
    return 4;
  case AGX_SHADER_OP_MOVE_IMMEDIATE:
    return agx_shader_move_immediate_is_short(instruction) ? 2 : 8;
  case AGX_SHADER_OP_MOVE:
    return 4;
  case AGX_SHADER_OP_DISCARD:
    return 4;
  case AGX_SHADER_OP_LOAD:
    return 14;
  case AGX_SHADER_OP_BRANCH:
    return 12;
  case AGX_SHADER_OP_POP_EXEC:
    return instruction->pop_short_form ? 4 : 6;
  case AGX_SHADER_OP_BARRIER:
    return 6;

  case AGX_SHADER_OP_JUMP_EXEC_NONE:
    return 10;
  case AGX_SHADER_OP_JUMP_EXEC_ANY:
    return 14;
  case AGX_SHADER_OP_JUMP_ABSOLUTE:
    return 10;
  case AGX_SHADER_OP_MIN_MAX:
    return 6;
  case AGX_SHADER_OP_COMPARE_SELECT:
    return 10;
  case AGX_SHADER_OP_FLOAT_UNARY:
    return 10;
  case AGX_SHADER_OP_PUSH_EXEC:
    return 4;
  case AGX_SHADER_OP_SAMPLE_MASK:
    return 6;
  case AGX_SHADER_OP_COMPARE:
    return instruction->long_form ? 10 : 6;
  case AGX_SHADER_OP_STORE_BUFFER:
    return 14;
  case AGX_SHADER_OP_ATOMIC:
    return 14;
  case AGX_SHADER_OP_ATOMIC_RESULT:
    return 8;
  case AGX_SHADER_OP_MOVE_NIBBLE_3:
    return instruction->long_form ? 10 : 4;
  case AGX_SHADER_OP_HALF_COMPARE_SELECT:
    return 10;
  case AGX_SHADER_OP_CONVERT_TO_FLOAT:
    return 8;
  case AGX_SHADER_OP_CONVERT_TO_INTEGER:
    return 10;
  case AGX_SHADER_OP_GET_SPECIAL:
    return instruction->long_form ? 8 : 4;
  case AGX_SHADER_OP_CALL_POOL_SHADER:
    return 8;
  case AGX_SHADER_OP_MOVE_TO_TILE:
    return 14;
  case AGX_SHADER_OP_STORE_TILE_PIXEL:
    return 12;
  case AGX_SHADER_OP_END_THREAD:
    return 2;
  case AGX_SHADER_OP_END_THREAD_LONG:
    return 6;
  case AGX_SHADER_OP_SHIFT_RIGHT:
    return instruction->shift_arithmetic ? 10 : 12;
  case AGX_SHADER_OP_CONVERT_TO_SURFACE:
    return 10;
  case AGX_SHADER_OP_PACK_TEXEL:
    return 14;
  case AGX_SHADER_OP_STORE_SURFACE:
    return 12;
  case AGX_SHADER_OP_LOAD_SURFACE:
    return 12;
  case AGX_SHADER_OP_CONVERT_FROM_SURFACE:
    return 8;
  case AGX_SHADER_OP_PACK_CONSTANT:
    return 10;
  case AGX_SHADER_OP_OUTPUT_MOVE:
    return instruction->long_form ? 10 : 4;
  case AGX_SHADER_OP_VERTEX_EXPORT:
    return 8;
  case AGX_SHADER_OP_VARYING_READ:
    return 10;
  case AGX_SHADER_OP_RECIPROCAL:
    return 10;
  case AGX_SHADER_OP_MULTIPLY:
    return 8;
  case AGX_SHADER_OP_TOP_BIT:
    return 10;
  case AGX_SHADER_OP_MULTIPLY_ADD:
    return agx_shader_multiply_add_length(instruction);
  case AGX_SHADER_OP_PRODUCT:
  case AGX_SHADER_OP_SUM:
    return agx_shader_product_length(instruction);
  case AGX_SHADER_OP_ADD_INDEX:
    return 10;
  case AGX_SHADER_OP_WIDEN:
  case AGX_SHADER_OP_NARROW:
    return ((agx_shader_take_bits(instruction->raw_bits, 8, 0)) != 0 || instruction->long_form) ? 8 : 6;
  case AGX_SHADER_OP_BIT_UNARY:
    return 8;
  case AGX_SHADER_OP_BIT_OP:
    return 10;
  case AGX_SHADER_OP_DERIVATIVE:
    return 10;
  case AGX_SHADER_OP_WAIT_FOR_STORE:
    return 6;
  case AGX_SHADER_OP_SCALE_INDEX:
    return instruction->scale_index_short_form ? 10 : 12;
  case AGX_SHADER_OP_TEXTURE_SAMPLE:
    return 14;
  case AGX_SHADER_OP_MOVE_FOR_SAMPLE:
    return 8;
  case AGX_SHADER_OP_RAW:
    return instruction->raw_length;
  case AGX_SHADER_OP_COUNT:
    break;
  }
  return 0;
}

static uint8_t
agx_shader_load_word_count_code(uint8_t word_count)
{
  switch (word_count)
  {
  case 1:
    return 0x0;
  case 2:
    return 0x4;
  case 3:
    return 0x6;
  case 4:
    return 0x3;
  default:
    return 0x0;
  }
}

static uint8_t
agx_shader_access_width_code(uint8_t width)
{
  switch (width)
  {
  case AGX_SHADER_ACCESS_WIDTH_16:
    return 0x00;
  case AGX_SHADER_ACCESS_WIDTH_8:
    return 0x20;
  default:
    return 0x10;
  }
}

static uint8_t
agx_shader_register_field(uint8_t reg)
{
  return (uint8_t)(agx_shader_pack_bits(reg, 7, 1));
}

static uint8_t
agx_shader_destination_nibble(uint8_t reg)
{
  return (uint8_t)(agx_shader_pack_bits(reg, 4, 4));
}

static uint8_t
agx_shader_index_scale_code(uint8_t scale)
{
  switch (scale)
  {
  case 1:
    return 0x1;
  case 2:
    return 0x2;
  case 4:
    return 0x3;
  case 8:
    return 0x4;
  case 16:
    return 0x0;
  default:
    return 0x1;
  }
}

static uint8_t
agx_shader_compare_byte_5(const Agx_Shader_Instruction* instruction, bool second_is_immediate)
{
  uint8_t byte = (uint8_t)agx_shader_flag(second_is_immediate, 1);

  if (instruction->wait && instruction->wait_slot < 7u)
  {
    byte |= (uint8_t)((instruction->wait_slot + 1u) << 5);
  }
  return byte;
}

typedef struct Agx_Shader_Packed_Format_Bytes
{
  uint8_t byte_10;
  uint8_t byte_11;
  uint8_t byte_13;
} Agx_Shader_Packed_Format_Bytes;

static const Agx_Shader_Packed_Format_Bytes k_packed_format_template[] = {
  [AGX_SHADER_PACKED_FORMAT_RGB10A2] = {0xea, 0x8b, 0x11},
  [AGX_SHADER_PACKED_FORMAT_RG11B10] = {0xca, 0x89, 0x08},
  [AGX_SHADER_PACKED_FORMAT_RGB9E5] = {0xca, 0x89, 0x00},
};

typedef struct Agx_Shader_Template
{
  uint16_t op;
  uint8_t  length;
  uint8_t  byte[14];
} Agx_Shader_Template;

static const Agx_Shader_Template k_agx_shader_template[] = {
  {AGX_SHADER_OP_ATOMIC, 14, {0x67, 0x11, 0x54, 0x00, 0x00, 0x00, 0x00, 0x80, 0x01, 0x00, 0x00, 0x00, 0x00, 0x02}},
  {AGX_SHADER_OP_ATOMIC_RESULT, 8, {0x0c, 0x80, 0x09, 0xa7, 0x00, 0x20, 0x00, 0x00}},
  {AGX_SHADER_OP_BARRIER, 6, {0x07, 0x04, 0x44, 0x00, 0x00, 0x00}},
  {AGX_SHADER_OP_BIT_OP, 10, {0x0b, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}},
  {AGX_SHADER_OP_BIT_UNARY, 8, {0x00, 0x00, 0x54, 0x00, 0x03, 0x00, 0x00, 0x04}},
  {AGX_SHADER_OP_BRANCH, 12, {0x2a, 0x80, 0x32, 0xd2, 0x07, 0x02, 0x00, 0x05, 0x54, 0x05, 0x0f, 0x01}},
  {AGX_SHADER_OP_CALL_POOL_SHADER, 8, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}},
  {AGX_SHADER_OP_COMPARE_SELECT, 10, {0x02}},
  {AGX_SHADER_OP_CONVERT_FROM_SURFACE, 8, {0x17, 0x00, 0x54}},
  {AGX_SHADER_OP_CONVERT_TO_FLOAT, 8, {0xa7, 0x07, 0x54, 0x00, 0x02, 0x00, 0x00, 0x20}},
  {AGX_SHADER_OP_CONVERT_TO_INTEGER, 10, {0x27}},
  {AGX_SHADER_OP_CONVERT_TO_SURFACE, 10, {0x97, 0x00, 0x54, 0x00, 0x02}},

  {AGX_SHADER_OP_PACK_TEXEL, 14, {0xa7, 0x06, 0x54, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0xa0, 0x00, 0x00, 0x24, 0x00}},
  {AGX_SHADER_OP_DERIVATIVE, 10, {0x37, 0x05, 0x54, 0x00, 0x03, 0x00, 0x90, 0x40, 0x00, 0x00}},
  {AGX_SHADER_OP_DISCARD, 4, {0x0b}},
  {AGX_SHADER_OP_END_THREAD, 2, {0x2a}},
  {AGX_SHADER_OP_END_THREAD_LONG, 6, {0x0f, 0x12, 0x54, 0x00, 0x00, 0x00}},
  {AGX_SHADER_OP_FLOAT_UNARY, 10, {0x2f, 0x00, 0x54, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00}},
  {AGX_SHADER_OP_HALF_COMPARE_SELECT, 10, {0x00, 0x00, 0x29, 0x80, 0x26, 0x80, 0x00, 0x00, 0x22, 0xb0}},
  {AGX_SHADER_OP_JUMP_ABSOLUTE, 10, {0x0f, 0x00, 0x54, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}},
  {AGX_SHADER_OP_JUMP_EXEC_ANY, 14, {0x8f, 0x04, 0x54, 0x00, 0x0f, 0x00, 0x54, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}},
  {AGX_SHADER_OP_JUMP_EXEC_NONE, 10, {0x0f, 0x01, 0x54, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}},
  {AGX_SHADER_OP_LOAD, 14, {0x67, 0x00, 0x44, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x40, 0x00, 0x00}},
  {AGX_SHADER_OP_LOAD_SURFACE, 12, {0x67, 0x00, 0x54, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00}},
  {AGX_SHADER_OP_MIN_MAX, 6, {0x02}},
  {AGX_SHADER_OP_MOVE, 4, {0x0b}},
  {AGX_SHADER_OP_MOVE_FOR_SAMPLE, 8, {0x09, 0x00, 0x2f, 0x00, 0x00, 0x00, 0x40, 0x00}},
  {AGX_SHADER_OP_MOVE_TO_TILE, 14, {0x0b, 0x00, 0x01, 0x04, 0x0b, 0x00, 0x0f, 0x00, 0x22, 0x00, 0x00, 0x14, 0x00, 0x00}},
  {AGX_SHADER_OP_MULTIPLY, 8, {0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}},
  {AGX_SHADER_OP_NOP, 2, {0x00, 0x00}},
  {AGX_SHADER_OP_PACK_CONSTANT, 10, {0x97, 0x04, 0x54, 0x00, 0x02, 0x00, 0x00, 0x50, 0x04, 0xc8}},
  {AGX_SHADER_OP_PUSH_EXEC, 4, {0x0f, 0x05}},
  {AGX_SHADER_OP_RECIPROCAL, 10, {0xaf, 0x00, 0x54, 0x00, 0x03, 0x00, 0x00, 0x48, 0x20}},
  {AGX_SHADER_OP_SAMPLE_MASK, 6, {0x57, 0x14, 0x54, 0x00, 0x00, 0x01}},
  {AGX_SHADER_OP_STOP, 4, {0x0e, 0x00, 0x00, 0x00}},
  {AGX_SHADER_OP_STORE_BUFFER, 14, {0xe7, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00}},
  {AGX_SHADER_OP_STORE_SURFACE, 12, {0xe7, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00}},
  {AGX_SHADER_OP_STORE_TILE_PIXEL, 12, {0xe7, 0x06, 0x54, 0x00, 0x00, 0x01, 0x01, 0x00, 0x04, 0x00, 0x00, 0x10}},
  {AGX_SHADER_OP_TEXTURE_SAMPLE, 14, {0x05, 0x80, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}},
  {AGX_SHADER_OP_TOP_BIT, 10, {0x02, 0x00, 0x27, 0x80, 0x22, 0x81, 0x07, 0x22, 0x20, 0x80}},
  {AGX_SHADER_OP_VARYING_READ, 10, {0x2f, 0x00, 0x54, 0x00, 0x03, 0x00, 0x00, 0x02, 0x00, 0x00}},
  {AGX_SHADER_OP_VERTEX_EXPORT, 8, {0x57}},
  {AGX_SHADER_OP_WAIT_FOR_STORE, 6, {0x00, 0x02, 0x54, 0x00, 0x00, 0x00}},
  {AGX_SHADER_OP_POP_EXEC, 4, {0x0f, 0x00, 0x04}},
  {AGX_SHADER_OP_POP_EXEC, 6, {0x0f, 0x00, 0x04}},
  {AGX_SHADER_OP_MOVE_NIBBLE_3, 10, {0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}},
  {AGX_SHADER_OP_MOVE_NIBBLE_3, 4, {0x03}},
  {AGX_SHADER_OP_SHIFT_RIGHT, 10, {0x00, 0x00, 0x54, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}},
  {AGX_SHADER_OP_OUTPUT_MOVE, 4, {0x0b, 0x00, 0x21, 0x04}},
  {AGX_SHADER_OP_OUTPUT_MOVE, 10, {0x0b}},
  {AGX_SHADER_OP_SCALE_INDEX, 10, {0x00, 0x01, 0x00, 0x00, 0x02}},
  {AGX_SHADER_OP_MOVE_IMMEDIATE, 8, {0x00, 0x80, 0x02}},
  {AGX_SHADER_OP_GET_SPECIAL, 8, {0x04, 0x00, 0x11, 0x06, 0x00, 0x00, 0x00, 0x00}},
};

static const uint8_t*
agx_shader_template(uint16_t op, uint32_t length)
{
  uint32_t at = 0;

  for (at = 0; at < sizeof(k_agx_shader_template) / sizeof(k_agx_shader_template[0]); ++at)
  {
    if (k_agx_shader_template[at].op == op && k_agx_shader_template[at].length == length)
    {
      return k_agx_shader_template[at].byte;
    }
  }
  return NULL;
}

static void
agx_shader_encode_one(const Agx_Shader_Instruction* instruction, uint8_t* at)
{
  const uint32_t length = agx_shader_instruction_length(instruction);
  const uint8_t* template = agx_shader_template((uint16_t)instruction->op, length);

  if (template != NULL)
  {
    memcpy(at, template, length);
  }

  switch (instruction->op)
  {
  case AGX_SHADER_OP_NOP:

    at[0] = (uint8_t)(0x06u | (agx_shader_pack_bits((instruction->raw_bits >> AGX_SHADER_NOP_BYTE_0_HIGH_SHIFT), 4, 4)));
    break;

  case AGX_SHADER_OP_STOP:
    break;

  case AGX_SHADER_OP_POP_EXEC:

    at[1] = (uint8_t)(instruction->pop_short_form ? 0x04u : 0x06u);

    at[2] = (uint8_t)(0x04u | (instruction->raw_bits & 0xf8u));
    if (instruction->pop_short_form)
    {
      uint8_t byte_3 = (uint8_t)(agx_shader_take_bits(instruction->raw_bits, 8, 8));

      at[3] = byte_3 != 0 ? byte_3 : 0x19u;
      break;
    }
    at[3] = instruction->nest;
    at[4] = 0x00;
    at[5] = 0x00;
    break;

  case AGX_SHADER_OP_JUMP_EXEC_NONE:
  {
    uint64_t distance = instruction->jump_distance;
    uint32_t k = 0;

    for (k = 0; k < 6; ++k)
    {
      at[3 + k] = (uint8_t)(distance >> (8 * k));
    }
    break;
  }

  case AGX_SHADER_OP_JUMP_ABSOLUTE:
  {
    uint64_t operand = instruction->jump_target | 1ull;
    uint32_t k = 0;

    for (k = 0; k < 6; ++k)
    {
      at[3 + k] = (uint8_t)(operand >> (8 * k));
    }
    break;
  }

  case AGX_SHADER_OP_JUMP_EXEC_ANY:

    at[1] = (uint8_t)(0x04u | ((instruction->raw_bits & AGX_SHADER_JUMP_ANY_BYTE_1_HIGH) ? 0xf0u : 0x00u));
    at[2] = (uint8_t)(0x54u | ((instruction->raw_bits >> AGX_SHADER_JUMP_ANY_BYTE_2_SHIFT) & 0x03u));
    at[3] =
      (uint8_t)(agx_shader_take_bits(instruction->raw_bits, 8, 0) ? (agx_shader_take_bits(instruction->raw_bits, 8, 0))
                                                                  : 0x02u);

    at[5] = (uint8_t)agx_shader_flag(instruction->jump_forward, 0);

    {
      int64_t  distance = instruction->back_distance;
      uint32_t k = 0;

      for (k = 0; k < 6; ++k)
      {
        at[7 + k] = (uint8_t)((uint64_t)distance >> (8u * k));
      }
    }
    break;

  case AGX_SHADER_OP_BARRIER:
  {
    uint32_t             which = (uint32_t)instruction->memory_scope % (uint32_t)AGX_SHADER_MEMORY_SCOPE_COUNT;
    static const uint8_t k_scope[AGX_SHADER_MEMORY_SCOPE_COUNT][2] = {
      {0x41, 0x09},
      {0x61, 0x09},
      {0x85, 0x08},
      {0x51, 0x0e},
      {0xd1, 0x0e},
    };

    at[2] = (uint8_t)(0x44u | (instruction->raw_bits & 0x30u));
    at[3] = (uint8_t)(k_scope[which][0] | agx_shader_flag((instruction->raw_bits & AGX_SHADER_BARRIER_BYTE_3_BIT_1), 1));
    at[4] = k_scope[which][1];
    break;
  }

  case AGX_SHADER_OP_PUSH_EXEC:

    at[2] = (uint8_t)((0x54u | agx_shader_flag((instruction->raw_bits & AGX_SHADER_PUSH_EXEC_BYTE_2_BIT_5), 5)) &
                      (uint8_t)((instruction->raw_bits & AGX_SHADER_PUSH_EXEC_BYTE_2_BIT_4_CLEAR) ? 0xefu : 0xffu));

    at[3] =
      (uint8_t)((instruction->invert ? 0x21u : 0x01u) | (agx_shader_pack_bits((uint32_t)instruction->mask_source, 3, 2)));
    if (instruction->raw_bits & AGX_SHADER_PUSH_EXEC_BYTE_3_LOW_IS_TWO)
    {
      at[3] = (uint8_t)((at[3] & 0xfcu) | 0x02u);
    }
    break;

  case AGX_SHADER_OP_ATOMIC:

    at[6] = (uint8_t)agx_shader_flag(!(instruction->atomic_operation == AGX_SHADER_ATOMIC_OP_COMPARE_EXCHANGE), 7);

    at[7] = (uint8_t)(0x80u | (agx_shader_take_bits(instruction->destination, 7, 0)) |
                      agx_shader_flag(instruction->atomic_operation == AGX_SHADER_ATOMIC_OP_COMPARE_EXCHANGE, 0));

    at[9] = (uint8_t)(agx_shader_flag(instruction->atomic_writes_destination, 6) |
                      (agx_shader_pack_bits(instruction->atomic_result_slot, 3, 1)));
    at[12] = instruction->atomic_operation;

    break;

  case AGX_SHADER_OP_ATOMIC_RESULT:
    at[0] = (uint8_t)(0x0cu | agx_shader_destination_nibble(instruction->destination));
    break;

  case AGX_SHADER_OP_STORE_BUFFER:

    at[1] = (uint8_t)(((instruction->store_waits || instruction->wait) && instruction->wait_slot < 4u)
                        ? (uint8_t)(0x10u << instruction->wait_slot)
                        : 0x00u);

    at[2] = (uint8_t)((instruction->store_waits || instruction->wait) ? 0x56u : 0x54u);
    at[3] = (uint8_t)(agx_shader_register_field(instruction->store_source) | (instruction->source_half ? 1u : 0u));
    at[4] = (uint8_t)(agx_shader_take_bits(instruction->base, 7, 0));
    at[5] = (uint8_t)(agx_shader_take_bits(instruction->index, 7, 0));

    at[6] =
      (uint8_t)(instruction->store_index_prescaled
                  ? AGX_SHADER_STORE_INDEX_PRESCALED
                  : (AGX_SHADER_STORE_INDEX_HARDWARE_SCALED | agx_shader_flag(instruction->store_discards_index, 0)));

    at[8] = (uint8_t)(0x01u | agx_shader_access_width_code(instruction->access_width) |
                      (agx_shader_load_word_count_code(instruction->word_count) << 1));
    at[9] = (uint8_t)(agx_shader_pack_bits((instruction->offset >> 2), 3, 5));
    at[10] = (uint8_t)(agx_shader_take_bits(instruction->offset, 8, 5));

    at[11] = (uint8_t)((agx_shader_pack_bits(agx_shader_index_scale_code(instruction->index_scale), 1, 7)) | 0x10u |
                       (agx_shader_take_bits(instruction->offset, 3, 13)));

    at[12] = (uint8_t)(0x10u | (agx_shader_take_bits(agx_shader_index_scale_code(instruction->index_scale), 2, 1)) |
                       agx_shader_flag(instruction->load_form == AGX_SHADER_LOAD_FORM_REGISTER_BASE, 3));
    break;

  case AGX_SHADER_OP_COMPARE:

    at[0] =
      (uint8_t)(0x0au | (agx_shader_pack_bits((instruction->raw_bits >> AGX_SHADER_COMPARE_BYTE_0_HIGH_SHIFT), 4, 4)));
    at[1] = (uint8_t)((instruction->source << 1) | (instruction->compare_source_is_half ? 0u : 1u));

    at[2] = (uint8_t)(0x22u | agx_shader_flag(instruction->discard_source, 3));

    at[4] = (uint8_t)(instruction->compare == AGX_SHADER_COMPARE_ALWAYS ? 0x01u
                      : instruction->compare == AGX_SHADER_COMPARE_NEVER
                        ? 0x00u
                        : ((uint32_t)instruction->compare_type |
                           agx_shader_flag(instruction->compare == AGX_SHADER_COMPARE_LESS, 0)));

    at[4] |= (uint8_t)agx_shader_flag((instruction->raw_bits & AGX_SHADER_COMPARE_BYTE_4_BIT_7), 7);
    at[4] |= (uint8_t)agx_shader_flag((instruction->raw_bits & AGX_SHADER_COMPARE_CONDITION_BIT_3), 3);
    at[4] &= (uint8_t)((instruction->raw_bits & AGX_SHADER_COMPARE_TYPE_BITS_CLEAR) ? 0xf9u : 0xffu);
    if (instruction->second_is_register)
    {
      at[3] = (uint8_t)(((instruction->compare_second << 1) & 0x7eu) | (instruction->compare_second_is_half ? 0u : 1u));

      at[2] |= (uint8_t)agx_shader_flag(instruction->compare_discards_second, 4);
      at[5] = agx_shader_compare_byte_5(instruction, false);
    }
    else
    {
      at[3] = (uint8_t)(0x80u | (instruction->compare_immediate & 0x7eu));
      at[4] |= (uint8_t)((instruction->compare_immediate & 1u) << 4);
      at[2] |= (uint8_t)agx_shader_flag((instruction->compare_immediate & 0x80u), 4);
      at[5] = agx_shader_compare_byte_5(instruction, true);
    }

    if (instruction->long_form)
    {
      at[3] |= (uint8_t)agx_shader_flag((instruction->raw_bits & AGX_SHADER_COMPARE_WIDE_BYTE_3_BIT_0), 0);
      at[6] = (uint8_t)(at[4] | agx_shader_flag((instruction->raw_bits & AGX_SHADER_COMPARE_BYTE_6_BIT_0), 0));
      at[7] = at[5];
      at[4] = 0x06;
      at[5] = 0x00;
      at[2] |= 0x01u;

      at[8] = (uint8_t)agx_shader_flag((instruction->raw_bits & AGX_SHADER_COMPARE_BYTE_8_BIT_1), 1);
      at[9] = 0x00;
    }
    break;

  case AGX_SHADER_OP_MOVE_IMMEDIATE:
  {
    uint32_t immediate = instruction->immediate;

    at[0] = (uint8_t)((instruction->destination_half ? 0x04u : 0x0cu) |
                      agx_shader_destination_nibble(instruction->destination));

    if (agx_shader_move_immediate_is_short(instruction))
    {
      at[1] = (uint8_t)immediate;
      break;
    }

    at[1] = (uint8_t)(0x80u | (agx_shader_take_bits(immediate, 7, 0)));

    at[2] = (uint8_t)(0x02u | agx_shader_flag((instruction->destination & 0x10u) != 0, 6) |
                      agx_shader_flag((instruction->destination & 0x20u) != 0, 7) |
                      (instruction->raw_bits & AGX_SHADER_MOVE_IMMEDIATE_DESTINATION_BIT_5) |
                      (instruction->raw_bits & AGX_SHADER_MOVE_IMMEDIATE_BYTE_2_BIT_5) |
                      (instruction->raw_bits & AGX_SHADER_MOVE_IMMEDIATE_BYTE_2_BIT_2) |
                      (instruction->raw_bits & AGX_SHADER_MOVE_IMMEDIATE_BYTE_2_BIT_3));
    at[3] = (uint8_t)(agx_shader_pack_bits((immediate >> 25), 7, 1));
    at[4] = (uint8_t)(agx_shader_pack_bits((immediate >> 7), 4, 1));
    at[5] = (uint8_t)(agx_shader_pack_bits((immediate >> 11), 2, 2));
    at[6] = (uint8_t)(agx_shader_take_bits(immediate, 8, 13));
    at[7] = (uint8_t)(agx_shader_take_bits(immediate, 4, 21));
    break;
  }

  case AGX_SHADER_OP_MOVE:
  case AGX_SHADER_OP_DISCARD:

    at[0] |= (uint8_t)(agx_shader_destination_nibble(instruction->destination));

    at[1] = (uint8_t)((agx_shader_pack_bits(instruction->source, 6, 1)) |
                      agx_shader_flag((instruction->raw_bits & AGX_SHADER_MOVE_BYTE_1_BIT_0), 0) |
                      agx_shader_flag(instruction->move_byte_1_bit_7, 7));

    at[2] =
      (uint8_t)((instruction->move_writes_zero ? 0u : 0x01u) | (instruction->move_keeps_source ? 0u : 0x08u) |
                ((instruction->raw_bits & AGX_SHADER_MOVE_BYTE_2_BIT_1) ? 0x02u : 0u) |
                ((instruction->raw_bits & AGX_SHADER_MOVE_BYTE_2_BIT_2) ? 0x04u : 0u) |
                ((instruction->raw_bits & AGX_SHADER_MOVE_BYTE_2_BIT_4) ? 0x10u : 0u) |
                ((instruction->raw_bits & AGX_SHADER_MOVE_BYTE_2_BIT_5) ? 0x20u : 0u) |
                ((instruction->destination & 0x10u) ? 0x40u : 0u) | ((instruction->destination & 0x20u) ? 0x80u : 0u));

    at[3] = instruction->move_byte_3 != 0u
            ? instruction->move_byte_3
            : (uint8_t)(agx_shader_flag(instruction->op == AGX_SHADER_OP_DISCARD, 2) |
                        (instruction->raw_bits & (AGX_SHADER_MOVE_UNKNOWN_BIT | AGX_SHADER_MOVE_UNKNOWN_BIT_5)));
    break;

  case AGX_SHADER_OP_LOAD:
  {
    at[1] =
      (uint8_t)(instruction->first_load |
                ((instruction->wait && instruction->wait_slot < 4u) ? (uint8_t)(0x10u << instruction->wait_slot) : 0x00u));

    at[2] = (uint8_t)(0x44u | agx_shader_flag(instruction->wait, 1) |
                      agx_shader_flag(!((instruction->raw_bits & AGX_SHADER_LOAD_BYTE_2_BIT_4_CLEAR)), 4));
    at[3] = (uint8_t)(agx_shader_register_field(instruction->destination) | (instruction->destination_half ? 1u : 0u));

    if (instruction->load_form == AGX_SHADER_LOAD_FORM_ORDINARY)
    {
      at[4] = (uint8_t)((agx_shader_take_bits(instruction->index, 7, 0)) | agx_shader_flag(instruction->index_is_pair, 7));
      at[5] = (uint8_t)(agx_shader_take_bits(instruction->base, 7, 0));
    }
    else
    {
      at[4] = (uint8_t)(agx_shader_take_bits(instruction->base, 7, 0));
      at[5] = (uint8_t)((agx_shader_take_bits(instruction->index, 7, 0)) | agx_shader_flag(instruction->index_is_pair, 7));
    }
    at[6] = (instruction->load_form == AGX_SHADER_LOAD_FORM_DYNAMIC_WIDE) ? 0x20 : 0x00;

    at[8] = (uint8_t)(0x01u | agx_shader_access_width_code(instruction->access_width) |
                      (agx_shader_load_word_count_code(instruction->word_count) << 1) |
                      (agx_shader_pack_bits(instruction->slot, 2, 6)));
    at[9] = (uint8_t)((agx_shader_take_bits(instruction->slot, 1, 2)) | agx_shader_flag(instruction->base_discard, 2) |
                      (agx_shader_pack_bits((instruction->offset >> 2), 1, 7)) | (instruction->raw_bits & 0x7au));
    at[10] = (uint8_t)(agx_shader_take_bits(instruction->offset, 8, 3));
    at[11] = (uint8_t)(0x40u | (agx_shader_take_bits(instruction->offset, 5, 11)));

    at[12] = (uint8_t)((instruction->load_form == AGX_SHADER_LOAD_FORM_REGISTER_BASE ? 0x60u
                        : instruction->load_form == AGX_SHADER_LOAD_FORM_ORDINARY    ? 0x20u
                                                                                     : 0x40u) |
                       (uint32_t)(agx_shader_index_scale_code(instruction->index_scale) << 1));
    break;
  }

  case AGX_SHADER_OP_BRANCH:

    at[1] = (uint8_t)(0x80u | agx_shader_flag(instruction->discard_pending, 0));
    at[6] = instruction->landing_site;
    break;

  case AGX_SHADER_OP_HALF_COMPARE_SELECT:

    at[0] = (uint8_t)(agx_shader_destination_nibble(instruction->destination) |
                      (instruction->destination_half ? 0x0au : 0x02u));
    at[1] = (uint8_t)(agx_shader_pack_bits(instruction->source, 6, 1));
    at[2] = (uint8_t)(0x29u | agx_shader_flag((instruction->destination & 0x10u), 6));
    at[6] = (uint8_t)(agx_shader_take_bits(instruction->raw_bits, 8, 0));
    at[7] = (uint8_t)(agx_shader_take_bits(instruction->raw_bits, 8, 8));
    break;

  case AGX_SHADER_OP_MOVE_NIBBLE_3:
    at[0] = (uint8_t)(0x03u | agx_shader_destination_nibble(instruction->destination));

    at[1] = (uint8_t)((agx_shader_pack_bits(instruction->source, 6, 1)) |
                      agx_shader_flag((instruction->move_raw & AGX_SHADER_MOVE_NIBBLE_3_SOURCE_HALF_UNMEASURED), 0));

    at[2] = (uint8_t)((instruction->move_raw & 0xbdu) | agx_shader_flag(instruction->destination_suppressed, 1) |
                      agx_shader_flag((instruction->destination & 0x10u), 6));
    at[3] = (uint8_t)(agx_shader_flag(instruction->destination_half, 0) | ((instruction->move_raw >> 8) & 0xfeu));
    if (instruction->long_form)
    {
      at[4] = (uint8_t)((instruction->move_raw >> AGX_SHADER_MOVE_NIBBLE_3_RAW_NIBBLE_SHIFT) & 0x0fu);

      at[7] = (uint8_t)agx_shader_flag(instruction->wait, 7);
      at[8] = (uint8_t)((instruction->move_raw & AGX_SHADER_MOVE_NIBBLE_3_RAW_TAIL_SHORT) ? 0x20u : 0x60u);
    }
    break;

  case AGX_SHADER_OP_CONVERT_TO_FLOAT:

    at[1] = (uint8_t)(0x07u | (((instruction->wait || instruction->store_waits) && instruction->wait_slot < 4u)
                                 ? (uint8_t)(0x10u << instruction->wait_slot)
                                 : 0x00u));
    at[2] = (uint8_t)(0x54u | agx_shader_flag(instruction->wait, 1));
    at[3] = (uint8_t)(instruction->destination << 1);
    at[4] = (uint8_t)(0x02u | (instruction->raw_bits & 1u));
    at[5] = (uint8_t)(instruction->source << 2);

    at[6] = (uint8_t)(0x80u | agx_shader_flag(instruction->destination_is_word, 2) |
                      agx_shader_flag(instruction->source_is_word, 3) |

                      ((instruction->convert_source_form & 3u) == AGX_SHADER_CONVERT_SOURCE_FORM_KEEP      ? 0x02u
                       : (instruction->convert_source_form & 3u) == AGX_SHADER_CONVERT_SOURCE_FORM_RELEASE ? 0x20u
                                                                                                           : 0x00u));
    at[7] = (uint8_t)(0x20u | agx_shader_flag(instruction->source_signed, 6));
    break;

  case AGX_SHADER_OP_CONVERT_TO_INTEGER:

    at[1] =
      (uint8_t)((agx_shader_take_bits(instruction->convert_bytes, 8, 0)) |
                ((instruction->wait && instruction->wait_slot < 4u) ? (uint8_t)(0x10u << instruction->wait_slot) : 0x00u));
    at[2] = (uint8_t)((((uint32_t)(instruction->convert_bytes >> 8)) & 0xfdu) | agx_shader_flag(instruction->wait, 1));
    at[3] = (uint8_t)(instruction->destination << 1);
    at[4] = (uint8_t)(agx_shader_take_bits(instruction->convert_bytes, 8, 16));
    at[5] = (uint8_t)((uint32_t)(instruction->source << 2) | (((uint32_t)(instruction->convert_bytes >> 24)) & 0x03u));

    at[6] = (uint8_t)((((uint32_t)(instruction->convert_bytes >> 32) & 0xd9u)) |
                      agx_shader_flag(instruction->destination_is_word, 2) |

                      ((instruction->convert_source_form & 3u) == AGX_SHADER_CONVERT_SOURCE_FORM_KEEP      ? 0x02u
                       : (instruction->convert_source_form & 3u) == AGX_SHADER_CONVERT_SOURCE_FORM_RELEASE ? 0x20u
                                                                                                           : 0x00u));

    at[7] = (uint8_t)(agx_shader_take_bits(instruction->convert_destination_type, 8, 0));
    at[8] = (uint8_t)(agx_shader_take_bits(instruction->convert_destination_type, 8, 8));

    at[9] = (uint8_t)(agx_shader_take_bits(instruction->convert_bytes, 8, 40));
    break;

  case AGX_SHADER_OP_GET_SPECIAL:

    if (!instruction->long_form)
    {
      at[0] = (uint8_t)((instruction->destination_half ? 0x04u : 0x0cu) |
                        agx_shader_destination_nibble(instruction->destination));
      at[1] = (uint8_t)instruction->special;

      at[2] = (uint8_t)(((agx_shader_take_bits(instruction->raw_bits, 8, 0))
                           ? (agx_shader_take_bits(instruction->raw_bits, 8, 0))
                           : 0x10u) |
                        agx_shader_flag((instruction->destination & 0x10u), 6));
      at[3] = (uint8_t)((instruction->raw_bits & 0xff00u) ? (agx_shader_take_bits(instruction->raw_bits, 8, 8)) : 0x06u);
      break;
    }

    at[0] |= (uint8_t)(agx_shader_destination_nibble(instruction->destination));
    at[1] = (uint8_t)instruction->special;

    at[2] = (uint8_t)(0x11u | agx_shader_flag((instruction->destination & 0x10u), 6));
    at[4] = (uint8_t)(agx_shader_take_bits(instruction->raw_bits, 8, 0));
    break;

  case AGX_SHADER_OP_CALL_POOL_SHADER:
  {
    uint16_t reference = (uint16_t)(2u * instruction->pool_offset + 0x2au);

    at[0] = (uint8_t)((instruction->raw_bits & AGX_SHADER_CALL_BYTE_0_BIT_7) ? 0xf7u : 0x77u);
    at[1] = (uint8_t)agx_shader_flag(!((instruction->raw_bits & AGX_SHADER_CALL_BYTE_1_CLEAR)), 0);
    at[2] = (uint8_t)(agx_shader_take_bits(reference, 8, 0));
    at[3] = (uint8_t)(reference >> 8);

    at[4] = (uint8_t)(agx_shader_take_bits(instruction->program_address, 8, 15));
    at[5] = (uint8_t)(agx_shader_take_bits(instruction->program_address, 8, 23));
    at[7] = (uint8_t)(agx_shader_flag(!((instruction->raw_bits & AGX_SHADER_CALL_BYTE_7_CLEAR)), 1) |
                      agx_shader_flag((instruction->raw_bits & AGX_SHADER_CALL_BYTE_7_BIT_4), 4));
    break;
  }

  case AGX_SHADER_OP_MOVE_TO_TILE:

    at[0] |= (uint8_t)(agx_shader_destination_nibble(instruction->destination));
    at[1] = (uint8_t)agx_shader_register_field(instruction->source);
    at[3] = (uint8_t)(0x04u | (instruction->raw_bits & AGX_SHADER_MOVE_UNKNOWN_BIT));
    at[4] |= (uint8_t)(agx_shader_destination_nibble(instruction->destination));
    at[5] = (uint8_t)(agx_shader_register_field(instruction->source) | 1u);
    break;

  case AGX_SHADER_OP_STORE_TILE_PIXEL:

    at[3] = instruction->source_immediate
            ? (uint8_t)(agx_shader_take_bits(instruction->source, 7, 0))
            : (uint8_t)(agx_shader_register_field(instruction->source) | (instruction->source_half ? 1u : 0u));
    at[7] = instruction->tile_destination;
    break;

  case AGX_SHADER_OP_END_THREAD:
    at[1] = instruction->end_thread_operand;
    break;

  case AGX_SHADER_OP_END_THREAD_LONG:

    at[4] = instruction->end_thread_operand;
    break;

  case AGX_SHADER_OP_SHIFT_RIGHT:

    if (instruction->shift_arithmetic)
    {
      at[0] = (uint8_t)(instruction->shift_left ? 0x27u : 0xa7u);
      at[1] = (uint8_t)(instruction->shift_amount_is_register ? 0x11u : 0x01u);
      at[3] = (uint8_t)(agx_shader_register_field(instruction->destination) | (instruction->destination_half ? 1u : 0u));

      at[4] = (uint8_t)((instruction->raw_bits & AGX_SHADER_SHIFT_BYTE_4_IS_TWO) ? 0x02u : 0x03u);
      at[5] = (uint8_t)(agx_shader_pack_bits(instruction->source, 6, 2));
      at[6] = (uint8_t)(instruction->shift_amount_is_register ? (agx_shader_pack_bits(instruction->shift_amount, 5, 3))
                                                              : (agx_shader_pack_bits(instruction->shift_amount, 6, 2)));

      at[7] = (uint8_t)((instruction->shift_amount_is_register ? 0x98u : 0x38u) |
                        agx_shader_flag(!(instruction->source_form == AGX_SHADER_SOURCE_FORM_KEEP), 6));
      at[8] = (uint8_t)(instruction->shift_amount_is_register ? 0xe2u : 0x62u);
      break;
    }

    at[0] = (uint8_t)(instruction->shift_left ? 0x27u : 0xa7u);

    at[1] = (uint8_t)((
      ((instruction->first_in_program || instruction->wait) && instruction->wait_slot < 4u)
        ? (uint8_t)(0x10u << instruction->wait_slot)
        : 0x00u
    ));

    at[2] = (uint8_t)(0x54u | agx_shader_flag(instruction->wait, 1));
    at[3] = (uint8_t)(agx_shader_register_field(instruction->destination) | (instruction->destination_half ? 1u : 0u));

    at[4] = (uint8_t)((
      0x03u & ((instruction->raw_bits & AGX_SHADER_SHIFT_BYTE_4_IS_TWO) ? 0xfeu : 0xffu) &
      ((instruction->raw_bits & AGX_SHADER_SHIFT_BYTE_4_BIT_1_CLEAR) ? 0xfdu : 0xffu)
    ));
    at[5] = (uint8_t)((agx_shader_pack_bits(instruction->source, 6, 2)) |
                      agx_shader_flag((instruction->raw_bits & AGX_SHADER_SHIFT_BYTE_5_BIT_1), 1));

    at[6] = (uint8_t)((instruction->shift_amount_is_register ? (agx_shader_pack_bits(instruction->shift_amount, 5, 3))
                                                             : (agx_shader_pack_bits(instruction->shift_amount, 6, 2))) |
                      (instruction->raw_bits & 0x02u));
    at[7] = 0x00;
    at[8] = (uint8_t)(0xf0u & ((instruction->raw_bits & AGX_SHADER_SHIFT_BYTE_8_BIT_4_CLEAR) ? 0xefu : 0xffu) &
                      ((instruction->raw_bits & AGX_SHADER_SHIFT_BYTE_8_BIT_5_CLEAR) ? 0xdfu : 0xffu));
    at[9] = (uint8_t)(0x10u | ((instruction->raw_bits >> AGX_SHADER_SHIFT_BYTE_9_LOW_SHIFT) & 0x07u) |
                      agx_shader_flag((instruction->raw_bits & AGX_SHADER_SHIFT_BYTE_9_BIT_7), 7));

    at[10] = (uint8_t)(agx_shader_flag(!((instruction->raw_bits & AGX_SHADER_SHIFT_BYTE_10_BIT_0_CLEAR)), 0) |
                       agx_shader_flag(instruction->shift_amount_is_register, 2) |
                       agx_shader_destination_nibble(instruction->shift_source_mask_bits));
    at[11] = (uint8_t)(agx_shader_take_bits(instruction->shift_source_mask_bits, 1, 4));
    break;

  case AGX_SHADER_OP_BIT_UNARY:

    at[0] = (uint8_t)(agx_shader_take_bits((uint32_t)instruction->bit_unary, 8, 16));

    at[1] =
      (uint8_t)((agx_shader_take_bits((uint32_t)instruction->bit_unary, 8, 8)) |
                ((instruction->wait && instruction->wait_slot < 4u) ? (uint8_t)(0x10u << instruction->wait_slot) : 0x00u));
    at[2] = (uint8_t)(0x54u | agx_shader_flag(instruction->wait, 1));
    at[3] = (uint8_t)(agx_shader_register_field(instruction->destination) | (instruction->destination_half ? 1u : 0u));
    at[5] = (uint8_t)(agx_shader_pack_bits(instruction->source, 6, 2));

    at[6] = (uint8_t)(((uint32_t)agx_shader_take_bits(instruction->bit_unary, 8, 0)) |
                      agx_shader_flag(!(instruction->source_form == AGX_SHADER_SOURCE_FORM_KEEP), 4));
    break;

  case AGX_SHADER_OP_PACK_TEXEL:
  {
    uint32_t channel = 0;
    uint32_t sources = 0;
    uint32_t byte = 0;

    at[1] = (uint8_t)(at[1] | agx_shader_flag(instruction->packed_sources_are_half, 4));
    at[3] = (uint8_t)(instruction->destination * AGX_SHADER_HALF_PER_REGISTER);
    for (channel = 0; channel < AGX_SHADER_PACK_CHANNELS; channel += 1)
    {
      sources |= agx_shader_take_bits(instruction->packed_sources[channel], AGX_SHADER_PACK_SOURCE_BITS, 0)
              << (AGX_SHADER_PACK_SOURCE_SHIFT + AGX_SHADER_PACK_SOURCE_STRIDE * channel);
    }
    for (byte = 0; byte < 4u; byte += 1)
    {
      at[5 + byte] = (uint8_t)agx_shader_take_bits(sources, 8, 8u * byte);
    }
    at[10] = k_packed_format_template[instruction->packed_format].byte_10;
    at[11] = k_packed_format_template[instruction->packed_format].byte_11;
    at[13] = k_packed_format_template[instruction->packed_format].byte_13;
    if (instruction->packed_alpha_immediate)
    {
      at[9] = (uint8_t)(at[9] | agx_shader_take_bits(instruction->packed_alpha, 2, 0));
      at[11] = (uint8_t)(at[11] & ~AGX_SHADER_PACK_CHANNEL_3_IS_REGISTER_11);
      at[13] = (uint8_t)(at[13] & ~AGX_SHADER_PACK_CHANNEL_3_IS_REGISTER_13);
    }
    break;
  }

  case AGX_SHADER_OP_CONVERT_TO_SURFACE:

    at[1] = instruction->first_converter ? 0x14 : 0x04;

    at[2] = (uint8_t)(0x54u | agx_shader_flag(instruction->wait, 1));
    at[3] = instruction->destination;

    at[5] = (uint8_t)((instruction->source << 2) & 0xffu);
    at[6] = (instruction->convert_form == AGX_SHADER_CONVERT_FORM_SINGLE)
            ? 0x00u
            : (uint8_t)((instruction->source_b << 3) & 0xffu);
    at[7] = (instruction->convert_form == AGX_SHADER_CONVERT_FORM_SINGLE) ? 0x90 : 0xd0;
    at[8] = (instruction->convert_form == AGX_SHADER_CONVERT_FORM_SINGLE) ? 0x44 : 0x45;

    at[9] = (uint8_t)(((uint32_t)instruction->numeric_format << 5) |
                      agx_shader_flag(!((instruction->convert_form == AGX_SHADER_CONVERT_FORM_SINGLE)), 1));
    break;

  case AGX_SHADER_OP_STORE_SURFACE:
    at[1] = instruction->store_wide ? 0x16 : 0x06;
    at[2] = (uint8_t)(0x54u | agx_shader_flag((instruction->store_waits || instruction->wait), 1));

    at[3] = (uint8_t)(agx_shader_register_field(instruction->store_source) | (instruction->source_half ? 1u : 0u));
    at[4] = (uint8_t)(agx_shader_take_bits(instruction->raw_bits, 8, 0));

    at[5] = (uint8_t)(agx_shader_register_field(instruction->color_attachment) |
                      agx_shader_flag((instruction->raw_bits & AGX_SHADER_STORE_SURFACE_BYTE_5_BIT_0), 0));
    at[7] = instruction->store_selector;
    at[8] = (uint8_t)(agx_shader_take_bits(instruction->raw_bits, 8, 8));
    at[11] = (uint8_t)(agx_shader_take_bits(instruction->raw_bits, 8, 16));
    break;

  case AGX_SHADER_OP_LOAD_SURFACE:

    at[1] = (uint8_t)(0x06u | agx_shader_flag((instruction->raw_bits & AGX_SHADER_LOAD_SURFACE_FIRST), 3));
    at[2] = (uint8_t)(0x54u | agx_shader_flag(instruction->wait, 1));
    at[3] = (uint8_t)(agx_shader_register_field(instruction->load_destination) | (instruction->source_half ? 1u : 0u));
    at[5] = (uint8_t)agx_shader_register_field(instruction->color_attachment);
    at[7] = (uint8_t)(instruction->store_selector | agx_shader_flag(instruction->load_byte_7_bit_7, 7));
    at[8] = (uint8_t)agx_shader_flag((instruction->raw_bits & AGX_SHADER_LOAD_SURFACE_FIRST), 1);
    break;

  case AGX_SHADER_OP_CONVERT_FROM_SURFACE:

    at[1] = instruction->first_converter ? 0x14u : 0x04u;
    at[2] = (uint8_t)(0x54u | agx_shader_flag(instruction->wait, 1));
    at[3] = instruction->destination;
    at[4] = (uint8_t)(instruction->raw_bits & AGX_SHADER_CONVERT_FROM_SURFACE_BYTE_4);
    at[5] = (uint8_t)(((instruction->source << 2) & 0xffu) + (instruction->convert_upper_pair ? 2u : 0u));
    at[6] = instruction->convert_from_width_byte;
    at[7] = instruction->convert_from_format_byte;
    break;

  case AGX_SHADER_OP_PACK_CONSTANT:

    at[3] = instruction->destination;
    at[5] = (uint8_t)((agx_shader_take_bits(agx_shader_half_from_float(instruction->pack_first), 4, 8)) << 3);
    at[6] = (uint8_t)((agx_shader_take_bits(agx_shader_half_from_float(instruction->pack_second), 2, 10)) << 6);
    break;

  case AGX_SHADER_OP_OUTPUT_MOVE:

    at[0] |= (uint8_t)(agx_shader_destination_nibble(instruction->output_index));
    if (!instruction->long_form)
    {
      at[1] = (uint8_t)(instruction->source << 1);
      break;
    }
    at[1] = (uint8_t)((instruction->source << 1) |
                      agx_shader_flag(!((instruction->raw_bits & AGX_SHADER_OUTPUT_MOVE_BYTE_1_BIT_0)), 0));

    at[2] = (uint8_t)((
      ((((instruction->raw_bits & AGX_SHADER_OUTPUT_MOVE_BYTE_2_BIT_3_CLEAR) ? 0x27u : 0x2fu) &
        ((instruction->raw_bits & AGX_SHADER_OUTPUT_MOVE_BYTE_2_BIT_0_CLEAR) ? 0xfeu : 0xffu)) &
       ((instruction->raw_bits & AGX_SHADER_OUTPUT_MOVE_BYTE_2_BIT_5_CLEAR) ? 0xdfu : 0xffu)) |
      agx_shader_flag((instruction->raw_bits & AGX_SHADER_OUTPUT_MOVE_BYTE_2_BIT_6), 6)
    ));
    at[3] = (uint8_t)((instruction->raw_bits >> AGX_SHADER_OUTPUT_MOVE_BYTE_3_SHIFT) & 0xffu);

    at[4] = (uint8_t)((instruction->raw_bits & AGX_SHADER_OUTPUT_MOVE_BYTE_4_CLEAR) ? 0x00u
                      : instruction->output_move_byte_4 != 0u                       ? instruction->output_move_byte_4
                                                                                    : 0x42u);
    at[5] = (uint8_t)((instruction->raw_bits >> AGX_SHADER_OUTPUT_MOVE_BYTE_5_SHIFT) & 0xffu);
    at[6] = 0x00;

    at[7] = (uint8_t)(agx_shader_flag(instruction->wait, 7) | (agx_shader_take_bits(instruction->raw_bits, 7, 0)));
    at[8] = (uint8_t)agx_shader_flag((instruction->raw_bits & AGX_SHADER_OUTPUT_MOVE_BYTE_8_IS_0X20), 5);
    at[9] = 0x00;
    break;

  case AGX_SHADER_OP_SAMPLE_MASK:

    at[1] = (uint8_t)(0x14u | agx_shader_destination_nibble(instruction->output_index) |
                      agx_shader_flag(instruction->kill_all, 3));

    at[3] = (uint8_t)(agx_shader_take_bits(instruction->raw_bits, 8, 0));
    at[4] = (uint8_t)(agx_shader_pack_bits(instruction->output_index, 3, 5));
    break;

  case AGX_SHADER_OP_VERTEX_EXPORT:

    at[1] = (uint8_t)((instruction->position_component << 4) |
                      ((instruction->raw_bits & AGX_SHADER_VERTEX_EXPORT_BYTE_1_LOW_CLEAR) ? 0x00u : 0x06u));
    at[2] = (uint8_t)(0x54u + instruction->export_varying);
    at[3] = (uint8_t)(agx_shader_register_field(instruction->source) | (instruction->source_half ? 1u : 0u));
    at[4] = (uint8_t)(agx_shader_pack_bits(instruction->output_index, 3, 5));
    at[5] = (uint8_t)(agx_shader_flag(!((instruction->raw_bits & AGX_SHADER_VERTEX_EXPORT_BYTE_5_BASE_CLEAR)), 6) |
                      agx_shader_flag((instruction->raw_bits & AGX_SHADER_VERTEX_EXPORT_BYTE_5_BIT_4), 4) |
                      (instruction->output_index >> 3));

    at[6] = (uint8_t)((instruction->raw_bits & AGX_SHADER_VERTEX_EXPORT_BYTE_6_BASE_CLEAR)
                        ? 0x00u
                        : (uint8_t)(0x40u | ((0x08u + instruction->export_slot) & 0x0fu)));
    at[7] = (uint8_t)((instruction->raw_bits >> AGX_SHADER_VERTEX_EXPORT_BYTE_7_SHIFT) & 0xffu);
    break;

  case AGX_SHADER_OP_VARYING_READ:

    at[1] = (instruction->varying_form == AGX_SHADER_VARYING_FORM_LEADING) ? 0x0d : 0x05;

    at[2] = (uint8_t)(0x54u | agx_shader_flag(instruction->wait, 1));
    at[3] = (uint8_t)(agx_shader_register_field(instruction->destination) | (instruction->destination_half ? 1u : 0u));
    at[5] = (instruction->varying_slot == 0) ? 0x00 : (uint8_t)(instruction->varying_slot << 1);
    at[6] = (instruction->varying_slot == 0) ? 0x04 : 0x00;

    at[8] = (uint8_t)(instruction->wait ? (agx_shader_pack_bits(instruction->wait_slot, 3, 1)) : 0x10u);
    break;

  case AGX_SHADER_OP_COMPARE_SELECT:

    at[0] = (uint8_t)(0x02u | agx_shader_destination_nibble(instruction->destination));
    at[1] = (uint8_t)((instruction->source << 1) | (instruction->source_half ? 1u : 0u));

    at[2] =
      (uint8_t)((instruction->float_arms ? 0x03u : 0x27u) |
                (instruction->source_form == AGX_SHADER_SOURCE_FORM_KEEP ? 0u : 0x08u) |
                ((instruction->second_is_register && instruction->second_form == AGX_SHADER_OPERAND_FORM_DISCARD) ? 0x10u
                                                                                                                  : 0u) |
                ((instruction->destination & 0x10u) ? 0x40u : 0u));

    at[3] = instruction->second_is_register

            ? (uint8_t)(((instruction->compare_second << 1) & 0x7eu) | 1u)
            : (uint8_t)(0x80u | (instruction->compare_immediate & 0x7eu) |
                        agx_shader_flag(instruction->compare_type == AGX_SHADER_COMPARE_TYPE_FLOAT, 0));

    at[4] =
      (uint8_t)(instruction->select_register_arms ? (0x82u | agx_shader_flag(instruction->negate_true_value, 4))
                                                  : (instruction->compare == AGX_SHADER_COMPARE_EQUAL ? 0x26u : 0x22u));
    at[5] = (uint8_t)(instruction->select_register_arms ? (agx_shader_pack_bits(instruction->true_value, 6, 1))
                                                        : (0x80u | (agx_shader_take_bits(instruction->true_value, 7, 0))));

    at[6] =
      (uint8_t)((instruction->compare == AGX_SHADER_COMPARE_NEVER    ? 0x00u
                 : instruction->compare == AGX_SHADER_COMPARE_ALWAYS ? 0x01u

                 : instruction->compare == AGX_SHADER_COMPARE_EQUAL
                   ? (instruction->compare_type == AGX_SHADER_COMPARE_TYPE_FLOAT ? 0x00u : 0x07u)

                   : (uint32_t)instruction->compare_type | (instruction->compare == AGX_SHADER_COMPARE_LESS ? 1u : 0u))

                | (instruction->second_is_register ? 0u : ((instruction->compare_immediate & 1u) << 4)));

    at[7] = instruction->second_is_register ? 0xc0u : 0xc2u;
    at[8] = (uint8_t)(instruction->select_register_arms ? (0x80u | agx_shader_flag(instruction->negate_false_value, 4))
                                                        : 0x20u);
    at[9] =
      (uint8_t)(instruction->select_register_arms ? (agx_shader_pack_bits(instruction->false_value, 6, 1))
                                                  : (0x80u | (agx_shader_take_bits(instruction->false_value, 7, 0))));
    break;

  case AGX_SHADER_OP_MIN_MAX:

    at[0] = (uint8_t)(0x02u | agx_shader_destination_nibble(instruction->destination));
    at[1] = (uint8_t)((instruction->source << 1) | (instruction->source_half ? 1u : 0u));

    at[2] = (uint8_t)(0x06u | (instruction->source_form == AGX_SHADER_SOURCE_FORM_KEEP ? 0u : 0x08u) |
                      (instruction->second_form == AGX_SHADER_OPERAND_FORM_DISCARD ? 0x10u : 0u) |
                      ((instruction->destination & 0x10u) ? 0x40u : 0u));
    at[3] = (uint8_t)((instruction->source_b << 1) | (instruction->source_b_half ? 1u : 0u));

    at[4] = (uint8_t)(agx_shader_flag(instruction->minimum, 0)

                      | ((uint32_t)instruction->min_max_type & 0x06u) |
                      agx_shader_flag((instruction->raw_bits & AGX_SHADER_MIN_MAX_BYTE_4_BIT_7), 7));

    at[5] = (uint8_t)(((instruction->raw_bits & AGX_SHADER_MIN_MAX_BYTE_5_BASE_CLEAR) ? 0x00u : 0xc0u) |
                      ((instruction->raw_bits >> AGX_SHADER_MIN_MAX_BYTE_5_SHIFT) & 0x3fu));
    break;

  case AGX_SHADER_OP_FLOAT_UNARY:

    at[0] = (uint8_t)(0x2fu | agx_shader_flag((instruction->float_unary & 0x80u), 7));

    at[1] =
      (uint8_t)((agx_shader_take_bits(instruction->float_unary, 2, 0)) |
                ((instruction->wait && instruction->wait_slot < 4u) ? (uint8_t)(0x10u << instruction->wait_slot) : 0x00u));
    at[2] = (uint8_t)(0x54u | agx_shader_flag(instruction->wait, 1));

    at[3] = (uint8_t)((instruction->destination << 1) | (instruction->destination_half ? 1u : 0u));

    at[4] = (uint8_t)(instruction->source_form == AGX_SHADER_SOURCE_FORM_KEEP ? 0x03u : 0x02u);
    at[5] = (uint8_t)((instruction->source << 2) & 0xffu);
    at[6] = (uint8_t)(instruction->source_form == AGX_SHADER_SOURCE_FORM_KEEP ? 0x92u : 0xb0u);
    at[8] = (uint8_t)(((instruction->float_unary >> 3) & 0x06u) | (instruction->negate_source ? 1u : 0u));
    break;

  case AGX_SHADER_OP_RECIPROCAL:

    at[1] =
      (uint8_t)((instruction->wait && instruction->wait_slot < 4u) ? (uint8_t)(0x10u << instruction->wait_slot) : 0x00u);
    at[2] = (uint8_t)(0x54u | agx_shader_flag(instruction->wait, 1));
    at[3] = (uint8_t)(instruction->destination << 1);
    at[5] = (uint8_t)((instruction->source << 2) & 0xffu);

    at[6] = (uint8_t)(instruction->source_form == AGX_SHADER_SOURCE_FORM_KEEP
                        ? 0x02u
                        : (agx_shader_take_bits(instruction->raw_bits, 8, 0)));

    at[9] = (uint8_t)((agx_shader_take_bits(instruction->raw_bits, 8, 8)) ^ 0x01u);
    break;

  case AGX_SHADER_OP_MULTIPLY:

    at[0] |= (uint8_t)(agx_shader_destination_nibble(instruction->destination));
    at[1] = (uint8_t)((instruction->source << 1) | 1u);
    at[2] = instruction->discard_source_b ? 0x3f : 0x2f;
    at[3] = (uint8_t)(((instruction->source_b << 1) | 1u) | agx_shader_flag(!instruction->discard_source_b, 7));
    at[5] = instruction->wait_slot;
    at[6] = (uint8_t)(agx_shader_take_bits(instruction->raw_bits, 8, 0));
    break;

  case AGX_SHADER_OP_WIDEN:
  {
    uint32_t length = ((agx_shader_take_bits(instruction->raw_bits, 8, 0)) != 0 || instruction->long_form) ? 8u : 6u;

    at[0] = (uint8_t)(0x09 | agx_shader_destination_nibble(instruction->destination));
    at[1] = (uint8_t)(instruction->source << 1);
    at[2] = (uint8_t)(0x14u | agx_shader_flag(instruction->reaches_export, 5) |
                      agx_shader_flag(!(instruction->source_form == AGX_SHADER_SOURCE_FORM_KEEP), 3) |
                      (((uint32_t)instruction->destination & 0x30u) << 2));
    at[3] = 0x81;
    at[4] = (uint8_t)(((length - 6u) / 2u) | agx_shader_flag(instruction->result_exported, 6) |
                      agx_shader_flag(instruction->source_half, 3));
    at[5] = (uint8_t)(0x02u | (instruction->wait ? ((instruction->wait_slot + 1u) << 5) : 0u));
    if (length >= 8)
    {
      at[6] = 0x00;
      at[7] = (uint8_t)(agx_shader_take_bits(instruction->raw_bits, 8, 0));
    }
    break;
  }

  case AGX_SHADER_OP_NARROW:
  {
    uint32_t length = ((agx_shader_take_bits(instruction->raw_bits, 8, 0)) != 0 || instruction->long_form) ? 8u : 6u;
    bool     keep = instruction->source_form == AGX_SHADER_SOURCE_FORM_KEEP;

    at[0] = (uint8_t)(0x01 | agx_shader_destination_nibble(instruction->destination));
    at[1] = (uint8_t)(0x01u | (agx_shader_pack_bits(instruction->source, 6, 1)) | agx_shader_flag(keep, 7));
    at[2] = (uint8_t)(0x14u | agx_shader_flag(instruction->reaches_export, 5) | agx_shader_flag(!keep, 3) |
                      (((uint32_t)instruction->destination & 0x30u) << 2));
    at[3] = 0x81;

    at[4] = (uint8_t)(((length - 6u) / 2u) | agx_shader_flag(instruction->result_exported, 6) |
                      agx_shader_flag(instruction->destination_half, 2));
    at[5] = (uint8_t)(0x02u | (instruction->wait ? ((instruction->wait_slot + 1u) << 5) : 0u));
    if (length >= 8)
    {
      at[6] = 0x00;
      at[7] = (uint8_t)(agx_shader_take_bits(instruction->raw_bits, 8, 0));
    }
    break;
  }

  case AGX_SHADER_OP_ADD_INDEX:

    at[0] = (uint8_t)(instruction->subtract ? 0x1fu : 0x9fu);

    at[1] = (uint8_t)(0x01u | (((instruction->first_in_program || instruction->wait) && instruction->wait_slot < 4u)
                                 ? (uint8_t)(0x10u << instruction->wait_slot)
                                 : 0x00u));

    at[2] = (uint8_t)((instruction->raw_bits & AGX_SHADER_ADD_INDEX_BYTE_2_BIT_5 ? 0x64u : 0x54u) |
                      agx_shader_flag(instruction->wait, 1));
    at[3] = (uint8_t)(agx_shader_register_field(instruction->destination) | (instruction->destination_half ? 1u : 0u));
    at[4] = (uint8_t)((instruction->raw_bits & AGX_SHADER_ADD_INDEX_BYTE_4_SWAP) ? 0x02u : 0x03u);

    if (instruction->add_second_source_is_register)
    {
      at[4] = (uint8_t)((instruction->raw_bits & AGX_SHADER_ADD_INDEX_BYTE_4_SWAP) ? 0x03u : 0x02u);

      at[5] = (uint8_t)(((instruction->source_b << 2) & 0xffu) | (agx_shader_take_bits(instruction->raw_bits, 2, 10)));
      at[6] = (uint8_t)(((instruction->source << 3) & 0xffu) | (instruction->raw_bits & 0x06u));

      at[7] = (uint8_t)((instruction->raw_bits & AGX_SHADER_ADD_INDEX_BYTE_7_HIGH_CLEAR ? 0x08u : 0xa8u) |
                        agx_shader_flag(instruction->index_source_form == AGX_SHADER_INDEX_SOURCE_FORM_KEEP, 2) |
                        (agx_shader_pack_bits(instruction->index_shift, 1, 6)) |
                        (agx_shader_take_bits(instruction->source, 2, 5)));

      at[8] = (uint8_t)(((instruction->index_shift != 0 || instruction->index_scaled) ? 0x10u : 0x11u) |
                        agx_shader_flag(!(instruction->second_form == AGX_SHADER_OPERAND_FORM_KEEP), 1) |
                        agx_shader_flag(instruction->index_source_form == AGX_SHADER_INDEX_SOURCE_FORM_DISCARD, 2));

      at[9] = (uint8_t)(agx_shader_flag(!instruction->source_is_uniform, 2) | 0x01u |
                        ((instruction->index_shift & 0x02u) << 3) |
                        agx_shader_flag(instruction->raw_bits & AGX_SHADER_ADD_INDEX_BYTE_9_BIT_1, 1));
      break;
    }
    at[5] = (uint8_t)agx_shader_register_field(instruction->immediate);
    at[6] = (uint8_t)(((instruction->immediate >> 7) & 1u) | (agx_shader_pack_bits(instruction->source, 5, 3)) |
                      (instruction->raw_bits & 0x06u));

    at[7] = (uint8_t)(((instruction->source_is_16_bit ? 0x08u : 0x88u) |
                       agx_shader_flag(instruction->index_source_form == AGX_SHADER_INDEX_SOURCE_FORM_KEEP, 2) |
                       (agx_shader_pack_bits(instruction->index_shift, 1, 6)) |
                       agx_shader_flag((instruction->raw_bits & AGX_SHADER_ADD_INDEX_BYTE_7_BIT_5), 5) |
                       (agx_shader_take_bits(instruction->source, 2, 5))) &
                      ((instruction->raw_bits & AGX_SHADER_ADD_INDEX_BYTE_7_BIT_3_CLEAR) ? 0xf7u : 0xffu));

    at[8] = (uint8_t)(((instruction->index_shift != 0 || instruction->index_scaled) ? 0x10u : 0x11u) |
                      agx_shader_flag(instruction->index_source_form == AGX_SHADER_INDEX_SOURCE_FORM_DISCARD, 2) |
                      agx_shader_flag(instruction->index_second_operand_is_uniform, 1));

    at[9] = (uint8_t)(agx_shader_flag(!instruction->source_is_uniform, 2) |
                      ((instruction->source_is_16_bit && instruction->source_sign_extends) ? 0x08u : 0x00u) |
                      ((instruction->index_shift & 0x02u) << 3));
    break;

  case AGX_SHADER_OP_PRODUCT:
  case AGX_SHADER_OP_SUM:
  {
    uint8_t  code = (uint8_t)agx_shader_flag(!(instruction->op == AGX_SHADER_OP_SUM), 0);
    uint32_t length = agx_shader_product_length(instruction);
    bool     immediate = instruction->second_form == AGX_SHADER_OPERAND_FORM_IMMEDIATE;
    uint8_t  second = 0;

    if (immediate)
    {
      agx_shader_minifloat_from_float(instruction->second_immediate, &second);
      second = (uint8_t)(second | 1u);
    }
    else
    {
      second = (uint8_t)((instruction->source_b << 1) | (instruction->source_b_low_half ? 0u : 1u) |
                         agx_shader_flag(instruction->second_form == AGX_SHADER_OPERAND_FORM_KEEP, 7));
    }

    at[0] = (uint8_t)((instruction->half_precision ? agx_shader_flag(length > 4u, 0) : 0x09u) |
                      agx_shader_destination_nibble(instruction->destination));
    at[1] = second;

    at[2] =
      (uint8_t)(agx_shader_flag(instruction->reaches_export, 5) | code | agx_shader_flag(length > 4u, 2) |
                ((immediate ? instruction->negate_source_b : instruction->second_form == AGX_SHADER_OPERAND_FORM_DISCARD)
                   ? 0x08u
                   : 0x00u) |
                agx_shader_flag(instruction->source_form == AGX_SHADER_SOURCE_FORM_DISCARD, 4) |
                (((uint32_t)instruction->destination & 0x30u) << 2));

    at[3] = (uint8_t)((instruction->source << 1) | agx_shader_flag(!instruction->source_low_half, 0) |
                      agx_shader_flag(instruction->source_form == AGX_SHADER_SOURCE_FORM_KEEP, 7));
    if (length >= 6)
    {
      at[4] = (uint8_t)(((instruction->absolute_source || instruction->absolute_source_b) ? 2u : ((length - 6u) / 2u)) |
                        agx_shader_flag(instruction->destination_high_half, 2) |
                        agx_shader_flag(instruction->result_exported, 6) | agx_shader_flag(immediate, 7));
      at[5] = (uint8_t)((instruction->wait ? ((instruction->wait_slot + 1u) << 5) : 0u) |
                        agx_shader_flag(instruction->negate_source, 3) |
                        agx_shader_flag(instruction->source_form == AGX_SHADER_SOURCE_FORM_UNIFORM, 1));
    }
    if (length >= 8)
    {
      at[6] = (uint8_t)((agx_shader_take_bits(instruction->raw_bits, 8, 8)) |
                        agx_shader_flag((instruction->negate_source_b && !immediate), 1));
      at[7] = (uint8_t)(agx_shader_take_bits(instruction->raw_bits, 8, 0));
    }
    if (instruction->absolute_source || instruction->absolute_source_b)
    {
      uint32_t tail = 0;

      at[7] = 0x80;
      at[8] =
        (uint8_t)(agx_shader_flag(instruction->absolute_source, 1) | agx_shader_flag(instruction->absolute_source_b, 0));
      for (tail = 9; tail < length; tail += 1)
      {
        at[tail] = 0x00;
      }
    }
    break;
  }

  case AGX_SHADER_OP_TOP_BIT:

    at[0] |= (uint8_t)(agx_shader_destination_nibble(instruction->destination));
    at[1] = (uint8_t)((instruction->source << 1) | (instruction->source_half ? 1u : 0u) |
                      agx_shader_flag(!instruction->discard_source, 7));
    at[2] = (uint8_t)(0x27u | agx_shader_flag(instruction->discard_source, 3) |
                      (((uint32_t)instruction->destination & 0x10u) << 2));
    break;

  case AGX_SHADER_OP_MULTIPLY_ADD:
  {
    uint32_t length = agx_shader_multiply_add_length(instruction);

    uint8_t second = (uint8_t)((instruction->source_b << 1) | agx_shader_flag(!instruction->source_b_low_half, 0) |
                               agx_shader_flag(!instruction->discard_source_b, 7));
    uint8_t third = (uint8_t)(instruction->source_c << 1);

    if (instruction->third_form == AGX_SHADER_OPERAND_FORM_IMMEDIATE)
    {
      agx_shader_minifloat_from_float(instruction->third_immediate, &third);
    }

    at[0] = (uint8_t)(0x09 | agx_shader_destination_nibble(instruction->destination));
    at[1] = second;
    if (instruction->accumulate)
    {
      at[2] = (uint8_t)(0x03u | agx_shader_flag(instruction->reaches_export, 5) |
                        (instruction->third_form == AGX_SHADER_OPERAND_FORM_DISCARD ? 0x10u : 0u) |
                        agx_shader_flag(instruction->discard_source_b, 3) |
                        (((uint32_t)instruction->destination & 0x30u) << 2));
      at[3] =
        (uint8_t)((instruction->source_c << 1) | (instruction->third_form == AGX_SHADER_OPERAND_FORM_KEEP ? 0x80u : 0u));
      break;
    }

    at[2] =
      (uint8_t)(0x06u | agx_shader_flag(instruction->reaches_export, 5) |
                agx_shader_flag(instruction->source_form == AGX_SHADER_SOURCE_FORM_DISCARD, 4) |
                agx_shader_flag(instruction->discard_source_b, 3) | (((uint32_t)instruction->destination & 0x30u) << 2));
    at[3] = (uint8_t)((instruction->source << 1) | 1u |
                      agx_shader_flag(instruction->source_form == AGX_SHADER_SOURCE_FORM_KEEP, 7));

    at[4] = (uint8_t)(((length - 6u) / 2u) | agx_shader_flag(instruction->absolute_third, 3) |
                      agx_shader_flag(instruction->negate_third, 4) |
                      (instruction->third_form == AGX_SHADER_OPERAND_FORM_IMMEDIATE ? 0x20u : 0u) |
                      (instruction->third_form == AGX_SHADER_OPERAND_FORM_DISCARD ? 0x80u : 0u));
    at[5] = (uint8_t)(third | (instruction->third_form == AGX_SHADER_OPERAND_FORM_KEEP ? 0x80u : 0u));
    if (length >= 8)
    {
      at[6] = (uint8_t)(0x02u | agx_shader_flag(instruction->result_exported, 6));
      at[7] = (uint8_t)((instruction->wait ? ((uint32_t)instruction->wait_slot + 1u) << 5 : 0u) |
                        agx_shader_flag(instruction->negate_product, 3));
    }
    if (length >= 10)
    {
      at[8] = 0x00;

      at[9] =
        (uint8_t)((agx_shader_take_bits(instruction->raw_bits, 7, 0)) | agx_shader_flag(instruction->result_wait, 7));
    }
    if (length >= 12)
    {
      at[9] = 0x80;
      at[10] =
        (uint8_t)(agx_shader_flag(instruction->absolute_source, 1) | agx_shader_flag(instruction->absolute_source_b, 0));
      at[11] = 0x00;
    }
    break;
  }

  case AGX_SHADER_OP_DERIVATIVE:

    at[1] = (uint8_t)(0x05u | agx_shader_flag(instruction->derivative_vertical, 1) |
                      agx_shader_flag(instruction->derivative_last, 3));
    at[3] = (uint8_t)agx_shader_register_field(instruction->destination);
    at[5] = (uint8_t)(agx_shader_pack_bits(instruction->source, 6, 2));
    at[6] = (uint8_t)(0x90u | agx_shader_flag(instruction->derivative_absolute, 1));
    break;

  case AGX_SHADER_OP_BIT_OP:

    at[0] |= (uint8_t)(agx_shader_destination_nibble(instruction->destination));
    at[1] = instruction->source_b_immediate
            ? (uint8_t)(agx_shader_take_bits(instruction->source_b, 7, 0))
            : (uint8_t)(agx_shader_register_field(instruction->source_b) | (instruction->source_b_half ? 1u : 0u) |
                        agx_shader_flag(instruction->keep_source_b, 7));

    at[2] = (uint8_t)((instruction->keep_source_b ? 0x06u : 0x0eu) | ((instruction->truth_table >> 3) & 1u) |
                      (instruction->source_form == AGX_SHADER_SOURCE_FORM_KEEP ? 0u : 0x10u) |
                      (((instruction->destination >> 4) & 1u) << 6));

    at[3] = (uint8_t)(agx_shader_register_field(instruction->source) | (instruction->source_half ? 1u : 0u));

    at[4] = (uint8_t)((instruction->truth_table & 1u) | (((instruction->truth_table >> 1) & 1u) << 1) |
                      agx_shader_flag(instruction->source_immediate, 4) |
                      agx_shader_flag((instruction->raw_bits & AGX_SHADER_BIT_OP_BYTE_4_BIT_6), 6) |
                      agx_shader_flag(instruction->source_b_immediate, 7));
    at[5] = (uint8_t)((((instruction->truth_table >> 2) & 1u) << 3) |
                      ((instruction->raw_bits >> AGX_SHADER_BIT_OP_BYTE_5_SHIFT) & 0xf7u));
    at[7] = (uint8_t)agx_shader_flag(instruction->wait, 7);
    break;

  case AGX_SHADER_OP_WAIT_FOR_STORE:

    at[0] = instruction->wait ? 0x87 : 0x07;

    at[3] = (uint8_t)(agx_shader_take_bits(instruction->wait_operand, 8, 0));
    at[4] = (uint8_t)(agx_shader_take_bits(instruction->wait_operand, 8, 8));
    break;

  case AGX_SHADER_OP_SCALE_INDEX:
  {
    uint32_t stride = instruction->vertex_stride;
    uint8_t  code = (uint8_t)(agx_shader_take_bits(instruction->index_form, 4, 0));

    at[0] = (uint8_t)(instruction->scale_index_reverse ? 0x1fu : 0x9fu);

    if (instruction->scale_index_short_form)
    {
      uint32_t first = instruction->source;
      uint32_t shift = instruction->index_shift;

      at[1] =
        (uint8_t)(0x01u | ((instruction->wait && instruction->wait_slot < 4u) ? (uint8_t)(0x10u << instruction->wait_slot)
                                                                              : 0x00u));
      at[2] = (uint8_t)((instruction->raw_bits & AGX_SHADER_SCALE_INDEX_SHORT_BYTE_2_IS_0X24)
                          ? 0x24u
                          : (uint8_t)(0x54u | agx_shader_flag(instruction->wait, 1)));
      at[3] = (uint8_t)(agx_shader_register_field(instruction->destination) | (instruction->destination_half ? 1u : 0u));
      at[4] = (uint8_t)(0x02u | ((instruction->raw_bits & AGX_SHADER_SCALE_INDEX_SHORT_BYTE_4_BIT_0) ? 0x01u : 0x00u));

      at[5] = instruction->second_is_register ? (uint8_t)(agx_shader_pack_bits(instruction->source_b, 6, 2))
                                              : (uint8_t)agx_shader_register_field(instruction->scale_index_addend);
      at[6] = (uint8_t)((agx_shader_pack_bits(first, 5, 3)) |
                        (instruction->second_is_register
                           ? 0x00u
                           : (uint8_t)(agx_shader_take_bits(instruction->scale_index_addend, 1, 7))) |
                        (uint8_t)((instruction->raw_bits >> AGX_SHADER_SCALE_INDEX_SHORT_BYTE_6_LOW_SHIFT) & 0x07u));
      at[7] =
        (uint8_t)((agx_shader_take_bits(first, 2, 5)) |
                  agx_shader_flag(instruction->index_source_form == AGX_SHADER_INDEX_SOURCE_FORM_KEEP, 2) |
                  (uint8_t)(((instruction->raw_bits >> AGX_SHADER_SCALE_INDEX_SHORT_BYTE_7_MID_SHIFT) & 0x07u) << 3) |
                  agx_shader_flag((shift & 1u), 6) | agx_shader_flag(instruction->scale_index_width_32, 7));
      at[8] = (uint8_t)(agx_shader_flag(shift == 0u, 0) |
                        ((instruction->raw_bits & AGX_SHADER_SCALE_INDEX_SHORT_BYTE_8_BIT_1) ? 0x02u : 0x00u) |
                        (instruction->index_source_form == AGX_SHADER_INDEX_SOURCE_FORM_DISCARD ? 0x04u : 0x00u) |
                        ((instruction->raw_bits & AGX_SHADER_SCALE_INDEX_SHORT_BYTE_8_BIT_4) ? 0x10u : 0x00u));
      at[9] = (uint8_t)(agx_shader_flag(instruction->second_is_register, 0) |
                        agx_shader_flag(instruction->scale_index_source_is_register, 2) |
                        agx_shader_flag(instruction->signed_index, 3) | agx_shader_flag((shift & 2u), 4));
      break;
    }

    at[1] =
      (uint8_t)(agx_shader_flag(instruction->first_in_program, 4) |
                ((instruction->wait && instruction->wait_slot < 4u) ? (uint8_t)(0x10u << instruction->wait_slot) : 0x00u));

    at[2] = (uint8_t)(0x54u | agx_shader_flag(instruction->wait, 1));
    at[3] = (uint8_t)(agx_shader_register_field(instruction->destination) | (instruction->destination_half ? 1u : 0u));
    at[4] = (uint8_t)(agx_shader_flag(!instruction->destination_suppressed, 1) |
                      agx_shader_flag((instruction->raw_bits & AGX_SHADER_SCALE_INDEX_BYTE_4_BIT_0), 0));
    at[5] = (uint8_t)(instruction->source << 2);
    at[6] = (uint8_t)((instruction->second_is_register ? (uint32_t)agx_shader_pack_bits(instruction->source_b, 5, 3)
                                                       : agx_shader_pack_bits(stride, 6, 2)) |
                      agx_shader_flag(instruction->index_source_form == AGX_SHADER_INDEX_SOURCE_FORM_KEEP, 1));

    at[7] = (uint8_t)((agx_shader_take_bits(stride, 2, 6)) |
                      agx_shader_flag((instruction->raw_bits & AGX_SHADER_SCALE_INDEX_BYTE_7_BIT_2), 2) |
                      (uint8_t)(agx_shader_pack_bits(instruction->scale_index_addend, 5, 3)));

    at[8] = (uint8_t)((code << 4) | (agx_shader_take_bits(instruction->scale_index_addend, 3, 5)));

    at[9] = (uint8_t)((code == AGX_SHADER_INDEX_FORM_FULL ? 0x28u : 0x20u) |
                      agx_shader_flag(instruction->second_form == AGX_SHADER_OPERAND_FORM_DISCARD, 2) |
                      agx_shader_flag(instruction->index_source_form == AGX_SHADER_INDEX_SOURCE_FORM_DISCARD, 1) |
                      agx_shader_flag((instruction->raw_bits & AGX_SHADER_SCALE_INDEX_BYTE_9_BIT_0), 0) |
                      agx_shader_flag((instruction->raw_bits & AGX_SHADER_SCALE_INDEX_BYTE_9_BIT_3), 3));

    at[10] = (uint8_t)(((instruction->signed_index ? 0x06u : 0x02u) &
                        ((instruction->raw_bits & AGX_SHADER_SCALE_INDEX_BYTE_10_BIT_1_CLEAR) ? 0xfdu : 0xffu)) |
                       agx_shader_flag(instruction->second_is_register, 3) |
                       agx_shader_flag(instruction->scale_index_add_register, 5));
    at[11] = 0x00;
    break;
  }

  case AGX_SHADER_OP_MOVE_FOR_SAMPLE:

    at[0] |= (uint8_t)(agx_shader_destination_nibble(instruction->destination));

    at[1] = (uint8_t)((instruction->source << 1) | 1u);

    at[2] = (uint8_t)(0x27u | agx_shader_flag(instruction->source_form != AGX_SHADER_SOURCE_FORM_KEEP, 3) |
                      agx_shader_flag(instruction->release_perspective, 4));
    at[3] = (uint8_t)((agx_shader_pack_bits(instruction->perspective_register, 6, 1)) | 0x01u |
                      agx_shader_flag(!instruction->release_perspective, 7));
    at[5] = instruction->sample_coordinate_slot;
    break;

  case AGX_SHADER_OP_TEXTURE_SAMPLE:

    at[1] = (uint8_t)(0x80u | (agx_shader_pack_bits((instruction->texture_index >> 1), 2, 3)) |
                      ((instruction->raw_bits >> AGX_SHADER_TEXTURE_BYTE_1_LOW_SHIFT) & 0x07u));

    at[3] = (uint8_t)((0xa0u | (agx_shader_pack_bits(instruction->sample_channels_minus_one, 2, 3))) &
                      ((instruction->raw_bits & AGX_SHADER_TEXTURE_BYTE_3_BIT_7_CLEAR) ? 0x7fu : 0xffu));

    at[4] =
      (uint8_t)(((instruction->texture_depth ? 0x90u : 0xb0u) | (agx_shader_take_bits(instruction->sampler_index, 2, 1))) &
                ((instruction->raw_bits & AGX_SHADER_TEXTURE_BYTE_4_BIT_7_CLEAR) ? 0x7fu : 0xffu));
    at[5] = (uint8_t)((instruction->raw_bits >> AGX_SHADER_TEXTURE_BYTE_5_SHIFT) & 0xffu);

    {
      static const uint8_t k_shape[AGX_SHADER_TEXTURE_ACCESS_COUNT][4] = {
        {0x00, 0x00, 0x00, 0x00},
        {0x17, 0x01, 0x00, 0x00},
        {0x09, 0x00, 0x10, 0x00},
        {0x00, 0x00, 0x10, 0x01},
        {0x80, 0x0c, 0x00, 0x00},
        {0x79, 0x00, 0x00, 0x00},
        {0x03, 0x03, 0x00, 0x00},
        {0x13, 0x00, 0x10, 0x00},
        {0x00, 0x00, 0x00, 0x00},
        {0x20, 0x00, 0x00, 0x01},
        {0x09, 0x00, 0x10, 0x01},
        {0x8d, 0x02, 0x10, 0x00},
        {0x0d, 0x00, 0x10, 0x01},
      };
      uint32_t shape = (uint32_t)instruction->texture_access;

      if (shape >= (uint32_t)AGX_SHADER_TEXTURE_ACCESS_COUNT)
      {
        shape = 0;
      }
      at[6] =
        (uint8_t)(k_shape[shape][0] | ((instruction->raw_bits & AGX_SHADER_TEXTURE_BYTE_6_HIGH_BITS) ? 0x30u : 0x00u));

      at[7] = (uint8_t)(k_shape[shape][1] | (agx_shader_pack_bits(instruction->texture_layer, 5, 3)));
      at[8] = (uint8_t)(agx_shader_flag((instruction->texture_index & 1u), 7) | k_shape[shape][3]);
      at[9] = (uint8_t)(instruction->sampler_index & 1u);
      at[10] = k_shape[shape][2];
    }

    at[11] = (uint8_t)(agx_shader_pack_bits(instruction->lod, 6, 2));
    at[12] = (uint8_t)((instruction->raw_bits & AGX_SHADER_TEXTURE_BYTE_12_IS_0X24) ? 0x24u : 0x01u);
    break;

  case AGX_SHADER_OP_RAW:
    memcpy(at, instruction->raw, instruction->raw_length);
    break;

  case AGX_SHADER_OP_COUNT:
    break;
  }
}

static bool
agx_shader_register_fits(uint8_t reg, uint8_t limit)
{
  return reg >= AGX_SHADER_VIRTUAL_REGISTER_BASE || reg <= limit;
}

static bool
agx_shader_instruction_is_spellable(const Agx_Shader_Instruction* instruction)
{
  switch (instruction->op)
  {
  case AGX_SHADER_OP_GET_SPECIAL:
    return agx_shader_register_fits(instruction->destination, 31u);

  case AGX_SHADER_OP_MOVE_NIBBLE_3:
    return agx_shader_register_fits(instruction->destination, 31u);

  case AGX_SHADER_OP_MOVE_IMMEDIATE:
    return agx_shader_register_fits(instruction->destination, agx_shader_move_immediate_is_short(instruction) ? 15u : 63u);

  case AGX_SHADER_OP_COMPARE_SELECT:
  case AGX_SHADER_OP_MIN_MAX:
  case AGX_SHADER_OP_BIT_OP:
  case AGX_SHADER_OP_TOP_BIT:

  case AGX_SHADER_OP_HALF_COMPARE_SELECT:
    return agx_shader_register_fits(instruction->destination, 31u);

  case AGX_SHADER_OP_MULTIPLY:
  case AGX_SHADER_OP_MOVE_FOR_SAMPLE:
    return agx_shader_register_fits(instruction->destination, 15u);

  case AGX_SHADER_OP_SCALE_INDEX:
    return agx_shader_register_fits(instruction->destination, 127u) &&
           agx_shader_register_fits(instruction->source, 63u) &&
           (!instruction->second_is_register ||
            agx_shader_register_fits(instruction->source_b, instruction->scale_index_short_form ? 63u : 31u));

  case AGX_SHADER_OP_PRODUCT:
  case AGX_SHADER_OP_SUM:
    return agx_shader_register_fits(instruction->destination, 63u);

  case AGX_SHADER_OP_MULTIPLY_ADD:
    return agx_shader_register_fits(instruction->destination, 63u);

  case AGX_SHADER_OP_MOVE:
  case AGX_SHADER_OP_DISCARD:
    return agx_shader_register_fits(instruction->destination, 63u) && agx_shader_register_fits(instruction->source, 63u);

  case AGX_SHADER_OP_CONVERT_TO_FLOAT:
  case AGX_SHADER_OP_CONVERT_TO_INTEGER:
  case AGX_SHADER_OP_RECIPROCAL:
  case AGX_SHADER_OP_FLOAT_UNARY:

  case AGX_SHADER_OP_BIT_UNARY:
  case AGX_SHADER_OP_SHIFT_RIGHT:
    return agx_shader_register_fits(instruction->destination, 127u) && agx_shader_register_fits(instruction->source, 63u);

  case AGX_SHADER_OP_WIDEN:
  case AGX_SHADER_OP_NARROW:
    return agx_shader_register_fits(instruction->destination, 63u) && agx_shader_register_fits(instruction->source, 63u);

  case AGX_SHADER_OP_DERIVATIVE:
    return agx_shader_register_fits(instruction->destination, 127u) && agx_shader_register_fits(instruction->source, 63u);

  case AGX_SHADER_OP_COMPARE:
    return agx_shader_register_fits(instruction->source, 127u) &&
           (!instruction->second_is_register || agx_shader_register_fits(instruction->compare_second, 127u));

  case AGX_SHADER_OP_LOAD:
    return agx_shader_register_fits(instruction->destination, 127u) &&
           agx_shader_register_fits(instruction->index, 127u) && agx_shader_register_fits(instruction->base, 127u);

  case AGX_SHADER_OP_STORE_BUFFER:
    return agx_shader_register_fits(instruction->store_source, 127u) &&
           agx_shader_register_fits(instruction->index, 127u) && agx_shader_register_fits(instruction->base, 127u);

  case AGX_SHADER_OP_ATOMIC:
    return agx_shader_register_fits(instruction->destination, 127u);

  case AGX_SHADER_OP_ATOMIC_RESULT:
    return agx_shader_register_fits(instruction->destination, 15u);

  case AGX_SHADER_OP_CONVERT_TO_SURFACE:
    return agx_shader_register_fits(instruction->source_b, 31u) && agx_shader_register_fits(instruction->source, 63u);

  case AGX_SHADER_OP_PACK_TEXEL:
    return agx_shader_register_fits(instruction->packed_sources[0], 63u) &&
           agx_shader_register_fits(instruction->packed_sources[1], 63u) &&
           agx_shader_register_fits(instruction->packed_sources[2], 63u) &&
           (instruction->packed_alpha_immediate || agx_shader_register_fits(instruction->packed_sources[3], 63u)) &&
           agx_shader_register_fits(instruction->destination, 127u);

  case AGX_SHADER_OP_TEXTURE_SAMPLE:
    return instruction->texture_index <= 7u && instruction->sampler_index <= 7u;

  default:
    return true;
  }
}

static bool
agx_shader_operand_survives(const Agx_Shader_Instruction* instruction, uint32_t which)
{
  Agx_Shader_Instruction probe = *instruction;
  Agx_Shader_Operands    operands = {0};
  uint8_t                before[32] = {0};
  uint8_t                after[32] = {0};
  uint32_t               length = agx_shader_instruction_length(instruction);
  uint32_t               value = 0;
  uint32_t               bit = 0;

  if (length == 0 || length > sizeof(before) || !agx_shader_operands(&probe, &operands) || which >= operands.count ||
      operands.scale[which] == 0)
  {
    return true;
  }

  value = agx_shader_operand_register(&operands, which);
  agx_shader_encode_one(&probe, before);

  for (bit = 0; bit < 8u; bit += 1u)
  {
    uint32_t probed = value ^ (1u << bit);

    if (((value >> bit) & 1u) == 0u)
    {
      continue;
    }
    agx_shader_operand_set_register(&operands, which, (uint8_t)probed);
    if (agx_shader_instruction_length(&probe) != length)
    {
      agx_shader_operand_set_register(&operands, which, (uint8_t)value);
      continue;
    }
    agx_shader_encode_one(&probe, after);
    agx_shader_operand_set_register(&operands, which, (uint8_t)value);
    if (memcmp(before, after, length) == 0)
    {
      return false;
    }
  }
  return true;
}

bool
agx_shader_builder_is_spellable(const Agx_Shader_Builder* builder, uint32_t* first_bad)
{
  uint32_t i = 0;

  for (i = 0; i < builder->count; ++i)
  {
    Agx_Shader_Operands operands = {0};
    uint32_t            k = 0;

    if (!agx_shader_instruction_is_spellable(&builder->instructions[i]))
    {
      if (first_bad != NULL)
      {
        *first_bad = i;
      }
      return false;
    }
    if (!agx_shader_operands(&builder->instructions[i], &operands))
    {
      continue;
    }
    for (k = 0; k < operands.count; ++k)
    {
      if (!agx_shader_operand_survives(&builder->instructions[i], k))
      {
        if (first_bad != NULL)
        {
          *first_bad = i;
        }
        return false;
      }
    }
  }
  return true;
}

uint32_t
agx_shader_encode(const Agx_Shader_Builder* builder, uint8_t* bytes, uint32_t capacity)
{
  uint32_t length = 0;
  uint32_t i = 0;

  if (builder->overflowed)
  {
    agx_refuse(
      "agx_shader_encode: the builder overflowed its %u instruction slot(s); "
      "nothing encoded",
      builder->capacity
    );
    return 0;
  }

  for (i = 0; i < builder->count; ++i)
  {
    if (!agx_shader_instruction_is_spellable(&builder->instructions[i]))
    {
      agx_refuse(
        "agx_shader_encode: instruction %u (op %u) names a register its own bytes cannot "
        "spell -- destination r%u, sources r%u and r%u, base r%u, index r%u; "
        "nothing encoded",
        i,
        (unsigned)builder->instructions[i].op,
        (unsigned)builder->instructions[i].destination,
        (unsigned)builder->instructions[i].source,
        (unsigned)builder->instructions[i].source_b,
        (unsigned)builder->instructions[i].base,
        (unsigned)builder->instructions[i].index
      );
      return 0;
    }
    length += agx_shader_instruction_length(&builder->instructions[i]);
  }

  if (length > capacity)
  {
    agx_refuse("agx_shader_encode: %u byte(s) do not fit in %u; nothing encoded", length, capacity);
    return 0;
  }

  length = 0;
  for (i = 0; i < builder->count; ++i)
  {
    Agx_Shader_Instruction resolved = builder->instructions[i];

    if (resolved.op == AGX_SHADER_OP_JUMP_EXEC_NONE && resolved.jump_distance_given)
    {
    }
    else if (resolved.op == AGX_SHADER_OP_JUMP_EXEC_NONE)
    {
      uint32_t distance = agx_shader_instruction_length(&resolved);
      uint32_t j = 0;

      if (resolved.skip_instructions == AGX_SHADER_JUMP_TO_MATCHING_POP)
      {
        uint32_t depth = 0;

        for (j = i + 1u; j < builder->count; ++j)
        {
          Agx_Shader_Op op = builder->instructions[j].op;

          if (op == AGX_SHADER_OP_PUSH_EXEC)
          {
            depth += 1;
          }
          else if (op == AGX_SHADER_OP_POP_EXEC)
          {
            uint8_t closes = builder->instructions[j].nest;

            if (closes == 0)
            {
              closes = 1;
            }
            if (closes > depth)
            {
              break;
            }
            depth -= closes;
          }
          distance += agx_shader_instruction_length(&builder->instructions[j]);
        }
      }
      else
      {
        for (j = 0; j < resolved.skip_instructions && i + 1u + j < builder->count; ++j)
        {
          distance += agx_shader_instruction_length(&builder->instructions[i + 1u + j]);
        }
      }

      resolved.jump_distance = distance;
      resolved.landing_site = (uint8_t)(distance > 0xffu ? 0xffu : distance);
    }

    if (resolved.op == AGX_SHADER_OP_JUMP_EXEC_ANY && resolved.back_distance == 0)
    {
      uint32_t distance = 0;
      uint32_t j = 0;

      if (resolved.jump_forward)
      {
        for (j = 0; j < resolved.skip_instructions && i + 1u + j < builder->count; ++j)
        {
          distance += agx_shader_instruction_length(&builder->instructions[i + 1u + j]);
        }
        resolved.back_distance = (int64_t)distance;
      }
      else
      {
        for (j = 0; j < resolved.skip_instructions && j < i; ++j)
        {
          distance += agx_shader_instruction_length(&builder->instructions[i - 1u - j]);
        }
        resolved.back_distance = -(int64_t)distance;
      }
    }

    agx_shader_encode_one(&resolved, bytes + length);
    length += agx_shader_instruction_length(&resolved);
  }
  return length;
}

static void
agx_shader_operand_add_scaled(Agx_Shader_Operands* out, uint8_t* slot, Agx_Shader_Operand_Role role, uint8_t width, uint8_t scale)
{
  if (out->count < sizeof(out->slot) / sizeof(out->slot[0]))
  {
    out->slot[out->count] = slot;
    out->role[out->count] = role;
    out->width[out->count] = width == 0 ? 1u : width;
    out->scale[out->count] = scale;
    out->count++;
  }
}

static void
agx_shader_operand_add_wide(Agx_Shader_Operands* out, uint8_t* slot, Agx_Shader_Operand_Role role, uint8_t width)
{
  agx_shader_operand_add_scaled(out, slot, role, width, 1);
}

uint8_t
agx_shader_operand_register(const Agx_Shader_Operands* operands, uint32_t which)
{
  uint8_t raw = *operands->slot[which];

  if (raw >= AGX_SHADER_VIRTUAL_REGISTER_BASE || operands->scale[which] <= 1u)
  {
    return raw;
  }
  return (uint8_t)(raw / operands->scale[which]);
}

void
agx_shader_operand_set_register(const Agx_Shader_Operands* operands, uint32_t which, uint8_t reg)
{
  if (operands->scale[which] == 0u)
  {
    return;
  }
  *operands->slot[which] = reg >= AGX_SHADER_VIRTUAL_REGISTER_BASE ? reg : (uint8_t)(reg * operands->scale[which]);
}

static void
agx_shader_operand_add(Agx_Shader_Operands* out, uint8_t* slot, Agx_Shader_Operand_Role role)
{
  agx_shader_operand_add_wide(out, slot, role, 1);
}

bool
agx_shader_operands(Agx_Shader_Instruction* instruction, Agx_Shader_Operands* out)
{
  out->count = 0;
  out->modelled = true;

  switch (instruction->op)
  {
  case AGX_SHADER_OP_NOP:
  case AGX_SHADER_OP_STOP:
  case AGX_SHADER_OP_PUSH_EXEC:
  case AGX_SHADER_OP_POP_EXEC:
  case AGX_SHADER_OP_JUMP_EXEC_NONE:
  case AGX_SHADER_OP_JUMP_EXEC_ANY:
  case AGX_SHADER_OP_JUMP_ABSOLUTE:
  case AGX_SHADER_OP_WAIT_FOR_STORE:
  case AGX_SHADER_OP_BRANCH:

  case AGX_SHADER_OP_BARRIER:
    return true;

  case AGX_SHADER_OP_RAW:
    return true;

  case AGX_SHADER_OP_MOVE_IMMEDIATE:
  case AGX_SHADER_OP_GET_SPECIAL:
    agx_shader_operand_add(out, &instruction->destination, AGX_SHADER_OPERAND_ROLE_DEFINE);
    return true;

  case AGX_SHADER_OP_BIT_OP:
    agx_shader_operand_add(out, &instruction->destination, AGX_SHADER_OPERAND_ROLE_DEFINE);
    agx_shader_operand_add(out, &instruction->source, AGX_SHADER_OPERAND_ROLE_USE);
    agx_shader_operand_add(out, &instruction->source_b, AGX_SHADER_OPERAND_ROLE_USE);
    return true;

  case AGX_SHADER_OP_DERIVATIVE:
    agx_shader_operand_add(out, &instruction->destination, AGX_SHADER_OPERAND_ROLE_DEFINE);
    agx_shader_operand_add(out, &instruction->source, AGX_SHADER_OPERAND_ROLE_USE);
    return true;

  case AGX_SHADER_OP_CONVERT_TO_FLOAT:
  case AGX_SHADER_OP_CONVERT_TO_INTEGER:
    agx_shader_operand_add(out, &instruction->destination, AGX_SHADER_OPERAND_ROLE_DEFINE);
    agx_shader_operand_add(out, &instruction->source, AGX_SHADER_OPERAND_ROLE_USE);
    return true;

  case AGX_SHADER_OP_MOVE:
  case AGX_SHADER_OP_MOVE_NIBBLE_3:
  case AGX_SHADER_OP_WIDEN:
  case AGX_SHADER_OP_NARROW:
  case AGX_SHADER_OP_RECIPROCAL:
  case AGX_SHADER_OP_TOP_BIT:
    agx_shader_operand_add(out, &instruction->destination, AGX_SHADER_OPERAND_ROLE_DEFINE);
    agx_shader_operand_add(out, &instruction->source, AGX_SHADER_OPERAND_ROLE_USE);
    return true;

  case AGX_SHADER_OP_OUTPUT_MOVE:
    agx_shader_operand_add(out, &instruction->source, AGX_SHADER_OPERAND_ROLE_USE);
    return true;

  case AGX_SHADER_OP_SHIFT_RIGHT:
    agx_shader_operand_add(out, &instruction->destination, AGX_SHADER_OPERAND_ROLE_DEFINE);
    agx_shader_operand_add(out, &instruction->source, AGX_SHADER_OPERAND_ROLE_USE);
    if (instruction->shift_amount_is_register)
    {
      agx_shader_operand_add(out, &instruction->shift_amount, AGX_SHADER_OPERAND_ROLE_USE);
    }
    return true;

  case AGX_SHADER_OP_ADD_INDEX:
    agx_shader_operand_add(out, &instruction->destination, AGX_SHADER_OPERAND_ROLE_DEFINE);
    agx_shader_operand_add(out, &instruction->source, AGX_SHADER_OPERAND_ROLE_USE);
    if (instruction->add_second_source_is_register)
    {
      agx_shader_operand_add(out, &instruction->source_b, AGX_SHADER_OPERAND_ROLE_USE);
    }
    return true;

  case AGX_SHADER_OP_HALF_COMPARE_SELECT:
  case AGX_SHADER_OP_BIT_UNARY:
  case AGX_SHADER_OP_FLOAT_UNARY:
    agx_shader_operand_add(out, &instruction->destination, AGX_SHADER_OPERAND_ROLE_DEFINE);
    agx_shader_operand_add(out, &instruction->source, AGX_SHADER_OPERAND_ROLE_USE);
    return true;

  case AGX_SHADER_OP_VARYING_READ:
    agx_shader_operand_add(out, &instruction->destination, AGX_SHADER_OPERAND_ROLE_DEFINE);
    return true;

  case AGX_SHADER_OP_STORE_SURFACE:
    agx_shader_operand_add_wide(
      out, &instruction->store_source, AGX_SHADER_OPERAND_ROLE_USE, instruction->store_register_count
    );
    return true;

  case AGX_SHADER_OP_CALL_POOL_SHADER:
    return true;

  case AGX_SHADER_OP_CONVERT_FROM_SURFACE:
    agx_shader_operand_add_scaled(
      out,
      &instruction->destination,
      AGX_SHADER_OPERAND_ROLE_DEFINE,
      instruction->convert_channel_count > 0u ? instruction->convert_channel_count : 1u,
      2
    );
    agx_shader_operand_add_scaled(out, &instruction->source, AGX_SHADER_OPERAND_ROLE_USE, 1, 4);
    return true;

  case AGX_SHADER_OP_TEXTURE_SAMPLE:
  {
    uint8_t channels = (uint8_t)(instruction->sample_channels_minus_one + 1u);

    instruction->implicit_base = 0;
    agx_shader_operand_add_scaled(out, &instruction->implicit_base, AGX_SHADER_OPERAND_ROLE_USE, channels, 0);
    agx_shader_operand_add_scaled(out, &instruction->implicit_base, AGX_SHADER_OPERAND_ROLE_DEFINE, channels, 0);
    return true;
  }

  case AGX_SHADER_OP_PACK_TEXEL:
    agx_shader_operand_add_scaled(out, &instruction->destination, AGX_SHADER_OPERAND_ROLE_DEFINE, 1, 2);
    agx_shader_operand_add(out, &instruction->packed_sources[0], AGX_SHADER_OPERAND_ROLE_USE);
    agx_shader_operand_add(out, &instruction->packed_sources[1], AGX_SHADER_OPERAND_ROLE_USE);
    agx_shader_operand_add(out, &instruction->packed_sources[2], AGX_SHADER_OPERAND_ROLE_USE);
    agx_shader_operand_add(out, &instruction->packed_sources[3], AGX_SHADER_OPERAND_ROLE_USE);
    return true;

  case AGX_SHADER_OP_CONVERT_TO_SURFACE:
    agx_shader_operand_add_scaled(out, &instruction->destination, AGX_SHADER_OPERAND_ROLE_DEFINE, 1, 2);
    agx_shader_operand_add(out, &instruction->source, AGX_SHADER_OPERAND_ROLE_USE);
    if (instruction->convert_form != AGX_SHADER_CONVERT_FORM_SINGLE)
    {
      agx_shader_operand_add(out, &instruction->source_b, AGX_SHADER_OPERAND_ROLE_USE);
    }
    return true;

  case AGX_SHADER_OP_PACK_CONSTANT:
    agx_shader_operand_add_scaled(out, &instruction->destination, AGX_SHADER_OPERAND_ROLE_DEFINE, 1, 2);
    return true;

  case AGX_SHADER_OP_MOVE_FOR_SAMPLE:
    agx_shader_operand_add(out, &instruction->destination, AGX_SHADER_OPERAND_ROLE_DEFINE);
    agx_shader_operand_add(out, &instruction->source, AGX_SHADER_OPERAND_ROLE_USE);
    agx_shader_operand_add(out, &instruction->perspective_register, AGX_SHADER_OPERAND_ROLE_USE);
    return true;

  case AGX_SHADER_OP_DISCARD:
    agx_shader_operand_add(out, &instruction->source, AGX_SHADER_OPERAND_ROLE_USE);
    return true;

  case AGX_SHADER_OP_SUM:
  case AGX_SHADER_OP_PRODUCT:
    agx_shader_operand_add(out, &instruction->destination, AGX_SHADER_OPERAND_ROLE_DEFINE);
    agx_shader_operand_add(out, &instruction->source, AGX_SHADER_OPERAND_ROLE_USE);
    if (instruction->second_form != AGX_SHADER_OPERAND_FORM_IMMEDIATE)
    {
      agx_shader_operand_add(out, &instruction->source_b, AGX_SHADER_OPERAND_ROLE_USE);
    }
    return true;

  case AGX_SHADER_OP_MULTIPLY:
  case AGX_SHADER_OP_MIN_MAX:
    agx_shader_operand_add(out, &instruction->destination, AGX_SHADER_OPERAND_ROLE_DEFINE);
    agx_shader_operand_add(out, &instruction->source, AGX_SHADER_OPERAND_ROLE_USE);
    agx_shader_operand_add(out, &instruction->source_b, AGX_SHADER_OPERAND_ROLE_USE);
    return true;

  case AGX_SHADER_OP_SCALE_INDEX:
    agx_shader_operand_add_wide(out, &instruction->destination, AGX_SHADER_OPERAND_ROLE_DEFINE, 2);
    agx_shader_operand_add(out, &instruction->source, AGX_SHADER_OPERAND_ROLE_USE);
    if (instruction->second_is_register)
    {
      agx_shader_operand_add(out, &instruction->source_b, AGX_SHADER_OPERAND_ROLE_USE);
    }
    return true;

  case AGX_SHADER_OP_MULTIPLY_ADD:
    if (!instruction->result_exported)
    {
      agx_shader_operand_add(out, &instruction->destination, AGX_SHADER_OPERAND_ROLE_DEFINE);
    }

    if (!instruction->accumulate)
    {
      agx_shader_operand_add(out, &instruction->source, AGX_SHADER_OPERAND_ROLE_USE);
    }
    else
    {
      agx_shader_operand_add(out, &instruction->destination, AGX_SHADER_OPERAND_ROLE_USE);
    }
    agx_shader_operand_add(out, &instruction->source_b, AGX_SHADER_OPERAND_ROLE_USE);
    agx_shader_operand_add(out, &instruction->source_c, AGX_SHADER_OPERAND_ROLE_USE);
    return true;

  case AGX_SHADER_OP_LOAD:
    agx_shader_operand_add_wide(out, &instruction->destination, AGX_SHADER_OPERAND_ROLE_DEFINE, instruction->word_count);
    agx_shader_operand_add(out, &instruction->index, AGX_SHADER_OPERAND_ROLE_USE);

    if (instruction->load_form == AGX_SHADER_LOAD_FORM_REGISTER_BASE)
    {
      agx_shader_operand_add_wide(out, &instruction->base, AGX_SHADER_OPERAND_ROLE_USE, 2);
    }
    return true;

  case AGX_SHADER_OP_STORE_BUFFER:
    agx_shader_operand_add_wide(out, &instruction->store_source, AGX_SHADER_OPERAND_ROLE_USE, instruction->word_count);
    agx_shader_operand_add(out, &instruction->index, AGX_SHADER_OPERAND_ROLE_USE);
    return true;

  case AGX_SHADER_OP_ATOMIC:
    instruction->implicit_base = 0;

    agx_shader_operand_add_scaled(
      out,
      &instruction->implicit_base,
      AGX_SHADER_OPERAND_ROLE_USE,
      instruction->atomic_operation == AGX_SHADER_ATOMIC_OP_COMPARE_EXCHANGE ? 3 : 2,
      0
    );

    if (instruction->atomic_writes_destination)
    {
      agx_shader_operand_add(out, &instruction->destination, AGX_SHADER_OPERAND_ROLE_DEFINE);
    }
    return true;

  case AGX_SHADER_OP_ATOMIC_RESULT:
    agx_shader_operand_add(out, &instruction->destination, AGX_SHADER_OPERAND_ROLE_DEFINE);
    return true;

  case AGX_SHADER_OP_VERTEX_EXPORT:
    return true;

  case AGX_SHADER_OP_SAMPLE_MASK:
    return true;

  case AGX_SHADER_OP_COMPARE_SELECT:
    agx_shader_operand_add(out, &instruction->destination, AGX_SHADER_OPERAND_ROLE_DEFINE);
    agx_shader_operand_add(out, &instruction->source, AGX_SHADER_OPERAND_ROLE_USE);

    if (instruction->second_is_register)
    {
      agx_shader_operand_add(out, &instruction->compare_second, AGX_SHADER_OPERAND_ROLE_USE);
    }
    return true;

  case AGX_SHADER_OP_COMPARE:
    agx_shader_operand_add(out, &instruction->source, AGX_SHADER_OPERAND_ROLE_USE);
    if (instruction->second_is_register)
    {
      agx_shader_operand_add(out, &instruction->compare_second, AGX_SHADER_OPERAND_ROLE_USE);
    }
    return true;

  default:
    break;
  }
  out->modelled = false;
  return false;
}

static uint32_t
agx_shader_wait_point(Agx_Shader_Builder* builder, uint32_t access);

static bool agx_shader_widened[AGX_SHADER_VIRTUAL_REGISTER_BASE];

static bool
agx_shader_live_ranges(Agx_Shader_Builder* builder, uint32_t* start, uint32_t* end, uint32_t widen_below)
{
  uint32_t i, v, w;

  for (v = 0; v < AGX_SHADER_VIRTUAL_REGISTER_BASE; ++v)
  {
    start[v] = AGX_SHADER_RANGE_UNSET;
    end[v] = 0;
    agx_shader_widened[v] = false;
  }
  for (i = 0; i < builder->count; ++i)
  {
    Agx_Shader_Operands operands = {0};
    uint32_t            k = 0;

    if (!agx_shader_operands(&builder->instructions[i], &operands))
    {
      return false;
    }
    for (k = 0; k < operands.count; ++k)
    {
      uint8_t value = agx_shader_operand_register(&operands, k);

      if (value < AGX_SHADER_VIRTUAL_REGISTER_BASE)
      {
        continue;
      }
      v = (uint32_t)(value - AGX_SHADER_VIRTUAL_REGISTER_BASE);

      for (w = 0; w < operands.width[k] && v + w < AGX_SHADER_VIRTUAL_REGISTER_BASE; ++w)
      {
        if (start[v + w] == AGX_SHADER_RANGE_UNSET)
        {
          start[v + w] = i;
        }
        end[v + w] = i;
      }
    }
  }

  {
    bool     widened = true;
    uint32_t rounds = 0;

    while (widened && rounds < AGX_SHADER_VIRTUAL_REGISTER_BASE)
    {
      widened = false;
      rounds += 1;

      for (i = 0; i < builder->count; ++i)
      {
        uint32_t body_first = 0;

        if (builder->instructions[i].op != AGX_SHADER_OP_JUMP_EXEC_ANY ||
            builder->instructions[i].skip_instructions == 0 || builder->instructions[i].skip_instructions > i)
        {
          continue;
        }

        body_first = i - builder->instructions[i].skip_instructions;

        for (v = 0; v < AGX_SHADER_VIRTUAL_REGISTER_BASE; ++v)
        {
          if (v >= widen_below)
          {
            continue;
          }
          if (start[v] == AGX_SHADER_RANGE_UNSET || start[v] > i || end[v] < body_first)
          {
            continue;
          }
          if (start[v] > body_first)
          {
            start[v] = body_first;
            widened = true;
            agx_shader_widened[v] = true;
          }
          if (end[v] < i)
          {
            end[v] = i;
            widened = true;
            agx_shader_widened[v] = true;
          }
        }
      }
    }
  }

  {
    bool     changed = true;
    uint32_t rounds = 0;

    while (changed && rounds < AGX_SHADER_VIRTUAL_REGISTER_BASE)
    {
      changed = false;
      rounds += 1;
      for (i = 0; i < builder->count; ++i)
      {
        Agx_Shader_Operands operands;
        uint32_t            k;

        if (!agx_shader_operands(&builder->instructions[i], &operands))
        {
          return false;
        }
        for (k = 0; k < operands.count; ++k)
        {
          uint8_t  value = agx_shader_operand_register(&operands, k);
          uint32_t lo = AGX_SHADER_RANGE_UNSET;
          uint32_t hi = 0;

          if (value < AGX_SHADER_VIRTUAL_REGISTER_BASE || operands.width[k] < 2)
          {
            continue;
          }
          v = (uint32_t)(value - AGX_SHADER_VIRTUAL_REGISTER_BASE);
          for (w = 0; w < operands.width[k] && v + w < AGX_SHADER_VIRTUAL_REGISTER_BASE; ++w)
          {
            if (start[v + w] != AGX_SHADER_RANGE_UNSET && start[v + w] < lo)
            {
              lo = start[v + w];
            }
            if (end[v + w] > hi)
            {
              hi = end[v + w];
            }
          }
          for (w = 0; w < operands.width[k] && v + w < AGX_SHADER_VIRTUAL_REGISTER_BASE; ++w)
          {
            if (start[v + w] != lo || end[v + w] != hi)
            {
              start[v + w] = lo;
              end[v + w] = hi;
              changed = true;
            }
          }
        }
      }
    }
  }

  for (i = 0; i < builder->count; ++i)
  {
    Agx_Shader_Operands operands = {0};
    uint32_t            barrier = 0;
    uint32_t            k = 0;

    if (builder->instructions[i].op != AGX_SHADER_OP_LOAD && builder->instructions[i].op != AGX_SHADER_OP_TEXTURE_SAMPLE)
    {
      continue;
    }
    if (!agx_shader_operands(&builder->instructions[i], &operands))
    {
      return false;
    }

    barrier = agx_shader_wait_point(builder, i);
    if (barrier >= builder->count)
    {
      barrier = builder->count == 0 ? 0 : builder->count - 1u;
    }

    for (k = 0; k < operands.count; ++k)
    {
      uint8_t value = agx_shader_operand_register(&operands, k);

      if (value < AGX_SHADER_VIRTUAL_REGISTER_BASE)
      {
        continue;
      }
      v = (uint32_t)(value - AGX_SHADER_VIRTUAL_REGISTER_BASE);
      for (w = 0; w < operands.width[k] && v + w < AGX_SHADER_VIRTUAL_REGISTER_BASE; ++w)
      {
        if (end[v + w] < barrier)
        {
          end[v + w] = barrier;
        }
      }
    }
  }

  return true;
}

static bool
agx_shader_op_is_memory(Agx_Shader_Op op);

static uint32_t
agx_shader_wait_point(Agx_Shader_Builder* builder, uint32_t access)
{
  bool     pending[256] = {0};
  uint8_t  pending_slot[256] = {0};
  uint32_t i, k, part;

  memset(pending, 0, sizeof(pending));
  memset(pending_slot, 0, sizeof(pending_slot));

  for (i = 0; i < builder->count; ++i)
  {
    Agx_Shader_Instruction* instruction = &builder->instructions[i];
    Agx_Shader_Operands     operands = {0};
    bool                    consumes = false;
    uint8_t                 slot = 0;

    if (!agx_shader_operands(instruction, &operands))
    {
      return builder->count;
    }

    for (k = 0; k < operands.count && !consumes; ++k)
    {
      uint32_t first = agx_shader_operand_register(&operands, k);
      uint32_t span = operands.width[k] > 0 ? operands.width[k] : 1;

      if (operands.role[k] != AGX_SHADER_OPERAND_ROLE_USE)
      {
        continue;
      }
      for (part = 0; part < span; ++part)
      {
        if (first + part < 256 && pending[first + part])
        {
          consumes = true;
          slot = pending_slot[first + part];
          break;
        }
      }
    }

    if (consumes)
    {
      bool cleared_ours = false;

      for (k = 0; k < 256; ++k)
      {
        if (pending[k] && pending_slot[k] == slot)
        {
          pending[k] = false;
          cleared_ours = true;
        }
      }

      if (cleared_ours && i > access && builder->instructions[access].slot == slot)
      {
        return i;
      }
    }

    if (agx_shader_op_is_memory(instruction->op))
    {
      for (k = 0; k < operands.count; ++k)
      {
        uint32_t first = agx_shader_operand_register(&operands, k);
        uint32_t span = operands.width[k] > 0 ? operands.width[k] : 1;

        if (operands.role[k] != AGX_SHADER_OPERAND_ROLE_DEFINE)
        {
          continue;
        }
        for (part = 0; part < span && first + part < 256; ++part)
        {
          pending[first + part] = true;
          pending_slot[first + part] = instruction->slot;
        }
      }
    }
  }
  return builder->count;
}

static void
agx_shader_physical_ranges(Agx_Shader_Builder* builder, uint32_t* start, uint32_t* end)
{
  uint32_t i, v;

  for (v = 0; v < AGX_SHADER_VIRTUAL_REGISTER_BASE; ++v)
  {
    start[v] = AGX_SHADER_RANGE_UNSET;
    end[v] = 0;
  }
  for (i = 0; i < builder->count; ++i)
  {
    Agx_Shader_Operands operands = {0};
    uint32_t            k = 0;

    if (!agx_shader_operands(&builder->instructions[i], &operands))
    {
      continue;
    }
    for (k = 0; k < operands.count; ++k)
    {
      uint8_t value = agx_shader_operand_register(&operands, k);

      if (value >= AGX_SHADER_VIRTUAL_REGISTER_BASE)
      {
        continue;
      }
      if (start[value] == AGX_SHADER_RANGE_UNSET)
      {
        start[value] = i;
      }
      end[value] = i;

      if (builder->instructions[i].op == AGX_SHADER_OP_LOAD)
      {
        uint32_t barrier = agx_shader_wait_point(builder, i);

        if (barrier >= builder->count)
        {
          barrier = builder->count == 0 ? 0 : builder->count - 1u;
        }
        if (end[value] < barrier)
        {
          end[value] = barrier;
        }
      }
    }
  }
}

static void
agx_shader_value_widths(const Agx_Shader_Builder* builder, uint8_t* need_width)
{
  uint32_t i = 0;
  uint32_t v = 0;

  for (v = 0; v < AGX_SHADER_VIRTUAL_REGISTER_BASE; ++v)
  {
    need_width[v] = 0;
  }
  for (i = 0; i < builder->count; ++i)
  {
    Agx_Shader_Operands operands = {0};
    uint32_t            k = 0;

    if (!agx_shader_operands(&builder->instructions[i], &operands))
    {
      continue;
    }
    for (k = 0; k < operands.count; ++k)
    {
      uint8_t value = agx_shader_operand_register(&operands, k);
      uint8_t width = operands.width[k] ? operands.width[k] : 1u;

      if (value < AGX_SHADER_VIRTUAL_REGISTER_BASE)
      {
        continue;
      }
      v = (uint32_t)(value - AGX_SHADER_VIRTUAL_REGISTER_BASE);
      if (width > need_width[v])
      {
        need_width[v] = width;
      }
    }
  }
}

static uint8_t
agx_shader_take_virtual(
  const Agx_Shader_Instruction* instruction,
  const uint8_t*                free_number,
  uint32_t                      free_count,
  uint32_t*                     next_free,
  uint32_t*                     next_virtual,
  uint32_t                      words
)
{
  Agx_Shader_Operands operands = {0};
  bool                named[AGX_SHADER_VIRTUAL_REGISTER_BASE] = {0};
  uint32_t            k = 0;
  uint8_t             taken = 0;

  if (agx_shader_operands((Agx_Shader_Instruction*)instruction, &operands))
  {
    for (k = 0; k < operands.count; ++k)
    {
      uint32_t value = agx_shader_operand_register(&operands, k);

      if (value >= AGX_SHADER_VIRTUAL_REGISTER_BASE)
      {
        named[value - AGX_SHADER_VIRTUAL_REGISTER_BASE] = true;
      }
    }
  }

  if (*next_virtual + words <= AGX_SHADER_VIRTUAL_REGISTER_BASE)
  {
    uint8_t counted = AGX_SHADER_VIRTUAL_REGISTER(*next_virtual);

    *next_virtual += words;
    return counted;
  }

  while (*next_free + words <= free_count)
  {
    uint32_t start = *next_free;
    uint32_t step = 0;
    bool     fits = true;

    for (step = 0; step < words; ++step)
    {
      if (named[free_number[start + step]] || free_number[start + step] != free_number[start] + step)
      {
        fits = false;
        break;
      }
    }
    if (fits)
    {
      *next_free = start + words;
      return AGX_SHADER_VIRTUAL_REGISTER(free_number[start]);
    }
    *next_free = start + 1u;
  }

  taken = AGX_SHADER_VIRTUAL_REGISTER(*next_virtual);
  *next_virtual += words;
  return taken;
}

static void
agx_shader_value_runs(const Agx_Shader_Builder* builder, uint8_t* run_base, uint8_t* run_index)
{
  uint8_t  need_width[AGX_SHADER_VIRTUAL_REGISTER_BASE] = {0};
  uint32_t v = 0;
  uint32_t j = 0;

  agx_shader_value_widths(builder, need_width);
  for (v = 0; v < AGX_SHADER_VIRTUAL_REGISTER_BASE; ++v)
  {
    run_base[v] = (uint8_t)v;
    run_index[v] = 0;
  }
  for (v = 0; v < AGX_SHADER_VIRTUAL_REGISTER_BASE; ++v)
  {
    uint32_t width = need_width[v] ? need_width[v] : 1u;

    if (width <= 1u || run_base[v] != (uint8_t)v)
    {
      continue;
    }
    for (j = 1; j < width && v + j < AGX_SHADER_VIRTUAL_REGISTER_BASE; ++j)
    {
      if (run_base[v + j] != (uint8_t)(v + j))
      {
        break;
      }
      run_base[v + j] = (uint8_t)v;
      run_index[v + j] = (uint8_t)j;
    }
  }
}

static uint8_t
agx_shader_operand_ceiling(const Agx_Shader_Instruction* instruction, uint32_t which)
{
  Agx_Shader_Instruction probe = *instruction;
  Agx_Shader_Operands    operands = {0};
  uint32_t               k = 0;
  uint32_t               candidate = 0;

  if (!agx_shader_operands(&probe, &operands) || which >= operands.count)
  {
    return (uint8_t)(AGX_SHADER_VIRTUAL_REGISTER_BASE - 1u);
  }

  for (k = 0; k < operands.count; k += 1)
  {
    agx_shader_operand_set_register(&operands, k, 0u);
  }

  for (candidate = AGX_SHADER_VIRTUAL_REGISTER_BASE - 1u;; candidate -= 1u)
  {
    agx_shader_operand_set_register(&operands, which, (uint8_t)candidate);
    if (agx_shader_instruction_is_spellable(&probe))
    {
      return (uint8_t)candidate;
    }
    if (candidate == 0u)
    {
      break;
    }
  }

  return 0u;
}

static void
agx_shader_value_ceilings(const Agx_Shader_Builder* builder, uint8_t* need_ceiling)
{
  uint32_t i = 0;
  uint32_t v = 0;

  for (v = 0; v < AGX_SHADER_VIRTUAL_REGISTER_BASE; ++v)
  {
    need_ceiling[v] = (uint8_t)(AGX_SHADER_VIRTUAL_REGISTER_BASE - 1u);
  }
  for (i = 0; i < builder->count; ++i)
  {
    Agx_Shader_Operands operands = {0};
    uint32_t            k = 0;

    if (!agx_shader_operands(&builder->instructions[i], &operands))
    {
      continue;
    }
    for (k = 0; k < operands.count; ++k)
    {
      uint8_t value = agx_shader_operand_register(&operands, k);
      uint8_t ceiling = 0;

      if (value < AGX_SHADER_VIRTUAL_REGISTER_BASE)
      {
        continue;
      }
      v = (uint32_t)(value - AGX_SHADER_VIRTUAL_REGISTER_BASE);
      ceiling = agx_shader_operand_ceiling(&builder->instructions[i], k);
      if (ceiling < need_ceiling[v])
      {
        need_ceiling[v] = ceiling;
      }
    }
  }
}

static bool
agx_shader_scan(
  Agx_Shader_Builder* builder,
  uint8_t             first,
  uint8_t             count,
  const uint32_t*     start,
  const uint32_t*     end,
  uint8_t*            assigned,
  bool                from_top,
  uint8_t             hold
)
{
  bool     placed[AGX_SHADER_VIRTUAL_REGISTER_BASE] = {0};
  bool     busy[AGX_SHADER_VIRTUAL_REGISTER_BASE] = {0};
  uint32_t slot_owner[AGX_SHADER_VIRTUAL_REGISTER_BASE] = {0};

  uint32_t pinned_start[AGX_SHADER_VIRTUAL_REGISTER_BASE] = {0};
  uint32_t pinned_end[AGX_SHADER_VIRTUAL_REGISTER_BASE] = {0};

  uint32_t end_of_slot[AGX_SHADER_VIRTUAL_REGISTER_BASE] = {0};

  uint8_t need_width[AGX_SHADER_VIRTUAL_REGISTER_BASE] = {0};

  uint8_t  need_ceiling[AGX_SHADER_VIRTUAL_REGISTER_BASE] = {0};
  uint32_t i, v;

  (void)start;
  agx_shader_physical_ranges(builder, pinned_start, pinned_end);

  agx_shader_value_widths(builder, need_width);
  agx_shader_value_ceilings(builder, need_ceiling);
  for (v = 0; v < AGX_SHADER_VIRTUAL_REGISTER_BASE; ++v)
  {
    assigned[v] = 0;
    placed[v] = false;
    busy[v] = false;
    slot_owner[v] = 0;
    end_of_slot[v] = 0;
  }
  if (hold != 0xffu && hold >= first && (uint32_t)hold < (uint32_t)first + count)
  {
    busy[hold - first] = true;
    end_of_slot[hold - first] = AGX_SHADER_RANGE_UNSET;
    slot_owner[hold - first] = AGX_SHADER_VIRTUAL_REGISTER_BASE;
  }

  for (i = 0; i < builder->count; ++i)
  {
    Agx_Shader_Operands operands = {0};
    uint32_t            k, slot;

    for (slot = 0; slot < count; ++slot)
    {
      if (busy[slot] && end_of_slot[slot] < i)
      {
        busy[slot] = false;
      }
    }

    (void)agx_shader_operands(&builder->instructions[i], &operands);
    for (k = 0; k < operands.count; ++k)
    {
      uint8_t value = agx_shader_operand_register(&operands, k);

      if (value < AGX_SHADER_VIRTUAL_REGISTER_BASE)
      {
        continue;
      }
      v = (uint32_t)(value - AGX_SHADER_VIRTUAL_REGISTER_BASE);
      if (placed[v])
      {
        continue;
      }

      {
        uint32_t need = need_width[v] ? need_width[v] : 1u;
        uint32_t base = v;
        uint32_t until = end[v];
        uint32_t j = 0;
        uint32_t run_ceiling = need_ceiling[v];

        for (j = 1; j < need && v + j < AGX_SHADER_VIRTUAL_REGISTER_BASE; ++j)
        {
          if (end[v + j] > until)
          {
            until = end[v + j];
          }
        }

        for (j = 1; j < need && v + j < AGX_SHADER_VIRTUAL_REGISTER_BASE; ++j)
        {
          const uint32_t implied = need_ceiling[v + j] >= j ? (uint32_t)need_ceiling[v + j] - j : 0u;

          if (implied < run_ceiling)
          {
            run_ceiling = implied;
          }
        }

        {
          const uint32_t slots = count + 1u - need;
          uint32_t       highest = slots - 1u;
          uint32_t       step = 0;

          if (run_ceiling < (uint32_t)first)
          {
            slot = count;
            highest = 0;
            step = slots;
          }
          else if (run_ceiling - (uint32_t)first < highest)
          {
            highest = run_ceiling - (uint32_t)first;
          }

          for (; step <= highest; ++step)
          {
            bool fits = true;

            slot = from_top ? highest - step : step;

            for (j = 0; j < need; ++j)
            {
              uint32_t physical = (uint32_t)first + slot + j;

              if (busy[slot + j])
              {
                fits = false;
                break;
              }

              if (physical < AGX_SHADER_VIRTUAL_REGISTER_BASE && pinned_start[physical] != AGX_SHADER_RANGE_UNSET &&
                  pinned_end[physical] >= i)
              {
                fits = false;
                break;
              }
            }
            if (fits)
            {
              break;
            }
            slot = count;
          }
        }
        if (slot + need > count || (uint32_t)first + slot > run_ceiling)
        {
          {
            uint32_t free_total = 0;
            uint32_t free_run = 0;
            uint32_t longest_free = 0;
            uint32_t probe = 0;

            for (probe = 0; probe < count; ++probe)
            {
              if (busy[probe])
              {
                free_run = 0;
                continue;
              }
              free_total += 1u;
              free_run += 1u;
              if (free_run > longest_free)
              {
                longest_free = free_run;
              }
            }
            agx_refuse(
              "agx_shader_allocate_registers: virtual value %u (width %u, spellable up "
              "to r%u) found no free run at instruction %u of %u, range r%u..r%u -- "
              "%u register(s) free there, longest run %u",
              v,
              need,
              run_ceiling,
              i,
              builder->count,
              (unsigned)first,
              (unsigned)(first + count - 1u),
              free_total,
              longest_free
            );
          }
          return false;
        }

        for (j = 0; j < need; ++j)
        {
          busy[slot + j] = true;

          slot_owner[slot + j] = v;
          assigned[base + j] = (uint8_t)(first + slot + j);
          placed[base + j] = true;
          end_of_slot[slot + j] = until;
        }
      }
    }
  }
  return true;
}

static bool
agx_shader_assignment_is_sound(
  const Agx_Shader_Builder* builder,
  const uint32_t*           start,
  const uint32_t*           end,
  const uint8_t*            assigned,
  uint8_t                   first
)
{
  uint8_t  width[AGX_SHADER_VIRTUAL_REGISTER_BASE] = {0};
  uint8_t  run_base[AGX_SHADER_VIRTUAL_REGISTER_BASE] = {0};
  uint8_t  run_index[AGX_SHADER_VIRTUAL_REGISTER_BASE] = {0};
  uint32_t a = 0;
  uint32_t b = 0;

  agx_shader_value_widths(builder, width);
  agx_shader_value_runs(builder, run_base, run_index);

  for (a = 0; a < AGX_SHADER_VIRTUAL_REGISTER_BASE; ++a)
  {
    if (start[a] == AGX_SHADER_RANGE_UNSET || agx_shader_widened[a])
    {
      continue;
    }
    for (b = a + 1u; b < AGX_SHADER_VIRTUAL_REGISTER_BASE; ++b)
    {
      uint32_t wide_a = width[a] ? width[a] : 1u;
      uint32_t wide_b = width[b] ? width[b] : 1u;

      if (start[b] == AGX_SHADER_RANGE_UNSET || agx_shader_widened[b] || run_base[a] == run_base[b])
      {
        continue;
      }
      if (end[a] < start[b] || end[b] < start[a])
      {
        continue;
      }
      if (assigned[a] + wide_a <= assigned[b] || assigned[b] + wide_b <= assigned[a])
      {
        continue;
      }
      agx_refuse(
        "agx_shader_allocate_registers: the assignment overlaps -- value %u is live over "
        "instructions %u..%u in r%u..r%u and value %u over %u..%u in r%u..r%u",
        a,
        start[a],
        end[a],
        (uint32_t)(first + assigned[a]),
        (uint32_t)(first + assigned[a] + wide_a - 1u),
        b,
        start[b],
        end[b],
        (uint32_t)(first + assigned[b]),
        (uint32_t)(first + assigned[b] + wide_b - 1u)
      );
      return false;
    }
  }
  return true;
}

static void
agx_shader_rewrite_registers(Agx_Shader_Builder* builder, const uint8_t* assigned)
{
  uint32_t i = 0;

  for (i = 0; i < builder->count; ++i)
  {
    Agx_Shader_Operands operands = {0};
    uint32_t            k = 0;

    (void)agx_shader_operands(&builder->instructions[i], &operands);
    for (k = 0; k < operands.count; ++k)
    {
      uint8_t value = agx_shader_operand_register(&operands, k);

      if (value >= AGX_SHADER_VIRTUAL_REGISTER_BASE)
      {
        agx_shader_operand_set_register(&operands, k, assigned[value - AGX_SHADER_VIRTUAL_REGISTER_BASE]);
      }
    }
  }
}

static int32_t
agx_shader_choose_spills(
  const Agx_Shader_Builder* builder,
  uint32_t                  instruction_count,
  uint8_t                   count,
  const uint32_t*           start,
  const uint32_t*           end,
  bool*                     spilled,
  uint32_t                  floor
)
{
  uint32_t room = count > AGX_SHADER_SPILL_RESERVE ? count - AGX_SHADER_SPILL_RESERVE : 0;
  int32_t  chosen = 0;

  uint8_t width[AGX_SHADER_VIRTUAL_REGISTER_BASE] = {0};

  uint8_t ceiling[AGX_SHADER_VIRTUAL_REGISTER_BASE] = {0};

  if (room == 0)
  {
    return -1;
  }
  agx_shader_value_widths(builder, width);
  agx_shader_value_ceilings(builder, ceiling);
  for (;;)
  {
    uint32_t worst_at = 0, worst_pressure = 0, worst_ceiling = 0;
    uint32_t victim = AGX_SHADER_VIRTUAL_REGISTER_BASE;
    uint32_t longest = 0;
    uint32_t i, v;

    for (i = 0; i < instruction_count; ++i)
    {
      uint32_t c = 0;

      for (c = 0; c < AGX_SHADER_VIRTUAL_REGISTER_BASE; ++c)
      {
        uint32_t live = 0;
        uint32_t here = (uint32_t)c + 1u < room ? (uint32_t)c + 1u : room;
        uint32_t over = 0;

        for (v = 0; v < AGX_SHADER_VIRTUAL_REGISTER_BASE; ++v)
        {
          if (!spilled[v] && start[v] != AGX_SHADER_RANGE_UNSET && start[v] <= i && i <= end[v] && ceiling[v] <= c)
          {
            live += width[v] ? width[v] : 1u;
          }
        }
        if (live <= here)
        {
          continue;
        }
        over = live - here;
        if (over > worst_pressure)
        {
          worst_pressure = over;
          worst_at = i;
          worst_ceiling = c;
        }
      }
    }
    if (worst_pressure == 0 && (uint32_t)chosen >= floor)
    {
      return chosen;
    }

    if (worst_pressure == 0)
    {
      worst_at = 0;
      worst_ceiling = AGX_SHADER_VIRTUAL_REGISTER_BASE - 1u;
    }

    for (v = 0; v < AGX_SHADER_VIRTUAL_REGISTER_BASE; ++v)
    {
      if (spilled[v] || start[v] == AGX_SHADER_RANGE_UNSET || ceiling[v] > worst_ceiling)
      {
        continue;
      }
      if (worst_pressure != 0 && (start[v] > worst_at || end[v] < worst_at))
      {
        continue;
      }

      if ((uint64_t)(end[v] - start[v] + 1u) * (width[v] ? width[v] : 1u) > longest ||
          victim == AGX_SHADER_VIRTUAL_REGISTER_BASE)
      {
        longest = (uint32_t)((end[v] - start[v] + 1u) * (width[v] ? width[v] : 1u));
        victim = v;
      }
    }
    if (victim == AGX_SHADER_VIRTUAL_REGISTER_BASE)
    {
      return -1;
    }
    spilled[victim] = true;
    chosen++;
  }
}

static uint32_t
agx_shader_spill_inserted(const Agx_Shader_Builder* builder, const bool* spilled, uint32_t from, uint32_t to)
{
  uint32_t inserted = 0;
  uint32_t i = 0;

  for (i = from; i < to && i < builder->count; ++i)
  {
    Agx_Shader_Operands operands = {0};
    uint32_t            k = 0;

    (void)agx_shader_operands(&builder->instructions[i], &operands);
    for (k = 0; k < operands.count; ++k)
    {
      uint8_t value = agx_shader_operand_register(&operands, k);

      if (value >= AGX_SHADER_VIRTUAL_REGISTER_BASE && spilled[value - AGX_SHADER_VIRTUAL_REGISTER_BASE])
      {
        inserted++;
      }
    }
  }
  return inserted;
}

static bool
agx_shader_shift_jumps(Agx_Shader_Builder* builder, uint32_t at)
{
  uint32_t i = 0;

  for (i = 0; i < builder->count; ++i)
  {
    const Agx_Shader_Instruction* jump = &builder->instructions[i];
    uint32_t                      target = 0;

    if (jump->op != AGX_SHADER_OP_JUMP_EXEC_ANY || jump->back_distance != 0 || jump->skip_instructions == 0 ||
        jump->skip_instructions == AGX_SHADER_JUMP_TO_MATCHING_POP)
    {
      continue;
    }
    if (!jump->jump_forward && jump->skip_instructions <= i && i - jump->skip_instructions == at)
    {
      return false;
    }
    (void)target;
  }

  for (i = 0; i < builder->count; ++i)
  {
    Agx_Shader_Instruction* jump = &builder->instructions[i];
    uint32_t                target = 0;
    uint32_t                moved_jump = 0;
    uint32_t                moved_target = 0;

    if (jump->op != AGX_SHADER_OP_JUMP_EXEC_ANY || jump->back_distance != 0 || jump->skip_instructions == 0 ||
        jump->skip_instructions == AGX_SHADER_JUMP_TO_MATCHING_POP)
    {
      continue;
    }
    if (jump->jump_forward)
    {
      target = i + 1u + jump->skip_instructions;
    }
    else
    {
      if (jump->skip_instructions > i)
      {
        continue;
      }
      target = i - jump->skip_instructions;
    }
    moved_jump = i + (i >= at ? 1u : 0u);
    moved_target = target + (target >= at ? 1u : 0u);
    jump->skip_instructions = jump->jump_forward ? moved_target - moved_jump - 1u : moved_jump - moved_target;
  }
  return true;
}

static void
agx_shader_recount_jumps(Agx_Shader_Builder* builder, const bool* spilled)
{
  uint32_t i = 0;

  for (i = 0; i < builder->count; ++i)
  {
    Agx_Shader_Instruction* jump = &builder->instructions[i];
    Agx_Shader_Operands     operands = {0};
    uint32_t                own = 0;
    uint32_t                k = 0;
    uint32_t                target = 0;

    if (jump->op != AGX_SHADER_OP_JUMP_EXEC_ANY || jump->back_distance != 0 || jump->skip_instructions == 0 ||
        jump->skip_instructions == AGX_SHADER_JUMP_TO_MATCHING_POP)
    {
      continue;
    }

    (void)agx_shader_operands(jump, &operands);
    for (k = 0; k < operands.count; ++k)
    {
      uint8_t value = agx_shader_operand_register(&operands, k);
      bool is_spilled = value >= AGX_SHADER_VIRTUAL_REGISTER_BASE && spilled[value - AGX_SHADER_VIRTUAL_REGISTER_BASE];

      if (!is_spilled)
      {
        continue;
      }
      if (jump->jump_forward)
      {
        own += operands.role[k] == AGX_SHADER_OPERAND_ROLE_DEFINE ? 1u : 0u;
      }
      else
      {
        own += operands.role[k] == AGX_SHADER_OPERAND_ROLE_USE ? 1u : 0u;
      }
    }

    if (jump->jump_forward)
    {
      target = i + 1u + jump->skip_instructions;
      jump->skip_instructions += own + agx_shader_spill_inserted(builder, spilled, i + 1u, target);
    }
    else
    {
      if (jump->skip_instructions > i)
      {
        continue;
      }
      target = i - jump->skip_instructions;
      jump->skip_instructions += own + agx_shader_spill_inserted(builder, spilled, target, i);
    }
  }
}

static void
agx_shader_find_wait_scratch(Agx_Shader_Builder* builder, uint8_t first, uint8_t count)
{
  bool     used[AGX_SHADER_VIRTUAL_REGISTER_BASE] = {0};
  uint32_t i = 0;
  uint32_t k = 0;

  builder->wait_scratch = 0;
  builder->has_wait_scratch = false;

  for (i = 0; i < builder->count; ++i)
  {
    Agx_Shader_Operands operands = {0};

    if (!agx_shader_operands(&builder->instructions[i], &operands))
    {
      return;
    }
    for (k = 0; k < operands.count; ++k)
    {
      uint32_t named = agx_shader_operand_register(&operands, k);
      uint32_t span = operands.width[k] > 0 ? operands.width[k] : 1u;
      uint32_t part = 0;

      for (part = 0; part < span; ++part)
      {
        if (named + part < AGX_SHADER_VIRTUAL_REGISTER_BASE)
        {
          used[named + part] = true;
        }
      }
    }
  }

  {
    Agx_Shader_Instruction probe = {0};
    uint8_t                ceiling = 0;

    probe.op = AGX_SHADER_OP_BIT_OP;
    ceiling = agx_shader_operand_ceiling(&probe, 0);
    for (i = (uint32_t)first + count; i > (uint32_t)first; --i)
    {
      if (!used[i - 1u] && i - 1u <= (uint32_t)ceiling)
      {
        builder->wait_scratch = (uint8_t)(i - 1u);
        builder->has_wait_scratch = true;
        return;
      }
    }
  }
}

static uint8_t
agx_shader_carrier_hold(uint8_t first, uint8_t count)
{
  Agx_Shader_Instruction probe = {0};
  uint32_t               ceiling = 0;
  uint32_t               top = (uint32_t)first + count;

  probe.op = AGX_SHADER_OP_BIT_OP;
  ceiling = agx_shader_operand_ceiling(&probe, 0);
  if (top > ceiling + 1u)
  {
    top = ceiling + 1u;
  }
  if (top <= (uint32_t)first)
  {
    return 0xffu;
  }
  return (uint8_t)(top - 1u);
}

bool
agx_shader_allocate_registers(Agx_Shader_Builder* builder, uint8_t first, uint8_t count, const Agx_Shader_Spill_Area* area)
{
  uint32_t start[AGX_SHADER_VIRTUAL_REGISTER_BASE] = {0};
  uint32_t end[AGX_SHADER_VIRTUAL_REGISTER_BASE] = {0};
  uint8_t  assigned[AGX_SHADER_VIRTUAL_REGISTER_BASE] = {0};
  bool     spilled[AGX_SHADER_VIRTUAL_REGISTER_BASE] = {0};
  uint8_t  spill_slot[AGX_SHADER_VIRTUAL_REGISTER_BASE] = {0};

  uint8_t  run_base[AGX_SHADER_VIRTUAL_REGISTER_BASE] = {0};
  uint8_t  run_index[AGX_SHADER_VIRTUAL_REGISTER_BASE] = {0};
  uint8_t  run_width[AGX_SHADER_VIRTUAL_REGISTER_BASE] = {0};
  uint32_t i, v;
  uint32_t highest_virtual = 0;
  uint32_t added = 0;
  uint32_t temps = 0;
  int32_t  chosen = 0;

  if (count == 0 || (uint32_t)first + count > AGX_SHADER_VIRTUAL_REGISTER_BASE)
  {
    return false;
  }

  if (!agx_shader_live_ranges(builder, start, end, AGX_SHADER_VIRTUAL_REGISTER_BASE))
  {
    return false;
  }

  if ((agx_shader_scan(builder, first, count, start, end, assigned, false, 0xffu) &&
       agx_shader_assignment_is_sound(builder, start, end, assigned, first)) ||
      (agx_shader_scan(builder, first, count, start, end, assigned, true, 0xffu) &&
       agx_shader_assignment_is_sound(builder, start, end, assigned, first)))
  {
    agx_shader_rewrite_registers(builder, assigned);
    agx_shader_find_wait_scratch(builder, first, count);

    agx_refusal_clear();
    return true;
  }
  if (!area)
  {
    return false;
  }

  {
    uint32_t original = builder->count;
    uint32_t stash = builder->capacity - original;
    uint32_t floor = 0;
    uint32_t attempt = 0;

    if (original > builder->capacity)
    {
      return false;
    }
    memmove(builder->instructions + stash, builder->instructions, original * sizeof(builder->instructions[0]));

    for (;;)
    {
      memmove(builder->instructions, builder->instructions + stash, original * sizeof(builder->instructions[0]));
      builder->count = original;
      attempt += 1u;
      added = 0;
      temps = 0;
      highest_virtual = 0;
      if (!agx_shader_live_ranges(builder, start, end, AGX_SHADER_VIRTUAL_REGISTER_BASE))
      {
        return false;
      }
      for (v = 0; v < AGX_SHADER_VIRTUAL_REGISTER_BASE; ++v)
      {
        spilled[v] = false;
        spill_slot[v] = 0;
        if (start[v] != AGX_SHADER_RANGE_UNSET)
        {
          highest_virtual = v + 1u;
        }
      }

      agx_shader_value_runs(builder, run_base, run_index);
      agx_shader_value_widths(builder, run_width);
      chosen = agx_shader_choose_spills(builder, builder->count, count, start, end, spilled, floor);

      if (chosen < 0)
      {
        agx_refuse(
          "agx_shader_allocate_registers: the pressure cannot be brought under %u live "
          "value(s) -- every value live at the worst instruction is already spilled",
          (uint32_t)(count > AGX_SHADER_SPILL_RESERVE ? count - AGX_SHADER_SPILL_RESERVE : 0)
        );
        return false;
      }
      if (chosen == 0)
      {
        agx_refuse(
          "agx_shader_allocate_registers: the scan did not fit and the spill chooser found "
          "nothing to spill"
        );
        return false;
      }

      if ((uint32_t)chosen > area->slot_count)
      {
        agx_refuse(
          "agx_shader_allocate_registers: %d value(s) have to go to memory and the spill "
          "area holds %u slot(s)",
          chosen,
          (uint32_t)area->slot_count
        );
        return false;
      }

      {
        uint8_t next_slot = 0;

        for (v = 0; v < AGX_SHADER_VIRTUAL_REGISTER_BASE; ++v)
        {
          if (spilled[v])
          {
            uint32_t j = 0;
            uint32_t base = run_base[v];

            spilled[base] = true;
            for (j = 1; j < run_width[base] && base + j < AGX_SHADER_VIRTUAL_REGISTER_BASE; ++j)
            {
              spilled[base + j] = true;
            }
          }
        }
        for (v = 0; v < AGX_SHADER_VIRTUAL_REGISTER_BASE; ++v)
        {
          if (spilled[v] && run_base[v] == (uint8_t)v)
          {
            uint32_t width = run_width[v] ? run_width[v] : 1u;

            if ((uint32_t)next_slot + width > area->slot_count)
            {
              agx_refuse(
                "agx_shader_allocate_registers: the spilled values need %u slot(s) -- a "
                "four-word load takes four -- and the spill area holds %u",
                (uint32_t)next_slot + width,
                (uint32_t)area->slot_count
              );
              return false;
            }
            spill_slot[v] = next_slot;
            next_slot = (uint8_t)(next_slot + width);
          }
        }
        for (v = 0; v < AGX_SHADER_VIRTUAL_REGISTER_BASE; ++v)
        {
          if (spilled[v] && run_base[v] != (uint8_t)v)
          {
            spill_slot[v] = (uint8_t)(spill_slot[run_base[v]] + run_index[v]);
          }
        }
      }

      for (i = 0; i < builder->count; ++i)
      {
        Agx_Shader_Operands operands = {0};
        uint32_t            k = 0;
        uint32_t            here = 0;

        (void)agx_shader_operands(&builder->instructions[i], &operands);
        for (k = 0; k < operands.count; ++k)
        {
          uint8_t value = agx_shader_operand_register(&operands, k);

          if (value >= AGX_SHADER_VIRTUAL_REGISTER_BASE && spilled[value - AGX_SHADER_VIRTUAL_REGISTER_BASE])
          {
            added++;

            here += operands.width[k] ? operands.width[k] : 1u;
          }
        }
        if (here > temps)
        {
          temps = here;
        }
      }
      if (builder->count + added > builder->capacity)
      {
        agx_refuse(
          "agx_shader_allocate_registers: spilling adds %u instruction(s) to %u and the "
          "builder holds %u",
          added,
          builder->count,
          builder->capacity
        );
        return false;
      }

      uint8_t  free_number[AGX_SHADER_VIRTUAL_REGISTER_BASE] = {0};
      uint32_t free_count = 0;

      for (v = 0; v < AGX_SHADER_VIRTUAL_REGISTER_BASE; ++v)
      {
        if (spilled[v])
        {
          free_number[free_count++] = (uint8_t)v;
        }
      }
      if (highest_virtual + temps > AGX_SHADER_VIRTUAL_REGISTER_BASE + free_count)
      {
        agx_refuse(
          "agx_shader_allocate_registers: spilling needs %u more virtual register(s) on top "
          "of %u, the numbering stops at %u, and the %u number(s) spilling freed are not "
          "enough",
          temps,
          highest_virtual,
          (uint32_t)AGX_SHADER_VIRTUAL_REGISTER_BASE,
          free_count
        );
        return false;
      }

      agx_shader_recount_jumps(builder, spilled);

      {
        uint32_t original = builder->count;
        uint32_t read = builder->capacity - original;
        uint32_t write = 0;

        memmove(builder->instructions + read, builder->instructions, original * sizeof(builder->instructions[0]));

        for (i = 0; i < original; ++i)
        {
          Agx_Shader_Instruction  source = builder->instructions[read + i];
          Agx_Shader_Instruction* placed = NULL;

          uint32_t next_virtual = highest_virtual;

          uint32_t            next_free = 0;
          Agx_Shader_Operands operands = {0};
          uint32_t            k = 0;
          uint8_t             store_back[4] = {0};
          uint8_t             store_from[4] = {0};
          uint8_t             store_words[4] = {0};
          uint32_t            store_count = 0;

          (void)agx_shader_operands(&source, &operands);

          for (k = 0; k < operands.count; ++k)
          {
            uint8_t value = agx_shader_operand_register(&operands, k);
            uint8_t temp = 0;

            if (value < AGX_SHADER_VIRTUAL_REGISTER_BASE || !spilled[value - AGX_SHADER_VIRTUAL_REGISTER_BASE] ||
                operands.role[k] != AGX_SHADER_OPERAND_ROLE_USE)
            {
              continue;
            }

            uint32_t words = operands.width[k] ? operands.width[k] : 1u;

            temp = agx_shader_take_virtual(&source, free_number, free_count, &next_free, &next_virtual, words);
            placed = &builder->instructions[write++];
            memset(placed, 0, sizeof(*placed));
            placed->op = AGX_SHADER_OP_LOAD;
            placed->destination = temp;
            placed->base = area->base;
            placed->index = area->index;
            placed->word_count = (uint8_t)words;
            placed->offset = area->offset + (uint32_t)spill_slot[value - AGX_SHADER_VIRTUAL_REGISTER_BASE] * 4u;

            placed->load_form = AGX_SHADER_LOAD_FORM_DYNAMIC_WIDE;
            placed->index_scale = 4;

            placed->slot = 5;
            agx_shader_operand_set_register(&operands, k, temp);
          }

          for (k = 0; k < operands.count; ++k)
          {
            uint8_t value = agx_shader_operand_register(&operands, k);
            uint8_t temp;

            if (value < AGX_SHADER_VIRTUAL_REGISTER_BASE || !spilled[value - AGX_SHADER_VIRTUAL_REGISTER_BASE] ||
                operands.role[k] != AGX_SHADER_OPERAND_ROLE_DEFINE)
            {
              continue;
            }
            uint32_t words = operands.width[k] ? operands.width[k] : 1u;

            temp = agx_shader_take_virtual(&source, free_number, free_count, &next_free, &next_virtual, words);
            store_back[store_count] = spill_slot[value - AGX_SHADER_VIRTUAL_REGISTER_BASE];
            store_from[store_count] = temp;
            store_words[store_count] = (uint8_t)words;
            store_count++;
            agx_shader_operand_set_register(&operands, k, temp);
          }

          builder->instructions[write++] = source;

          for (k = 0; k < store_count; ++k)
          {
            placed = &builder->instructions[write++];
            memset(placed, 0, sizeof(*placed));
            placed->op = AGX_SHADER_OP_STORE_BUFFER;
            placed->store_source = store_from[k];
            placed->base = area->base;
            placed->index = area->index;
            placed->word_count = store_words[k];
            placed->offset = area->offset + (uint32_t)store_back[k] * 4u;
            placed->index_scale = 4;
          }
        }
        builder->count = write;
      }

      if (agx_shader_live_ranges(builder, start, end, highest_virtual) &&
          ((agx_shader_scan(builder, first, count, start, end, assigned, false, 0xffu) &&
            agx_shader_assignment_is_sound(builder, start, end, assigned, first)) ||
           (agx_shader_scan(builder, first, count, start, end, assigned, true, 0xffu) &&
            agx_shader_assignment_is_sound(builder, start, end, assigned, first))))
      {
        agx_shader_find_wait_scratch(builder, first, count);

        if (!builder->has_wait_scratch)
        {
          uint8_t hold = agx_shader_carrier_hold(first, count);
          uint8_t held[AGX_SHADER_VIRTUAL_REGISTER_BASE] = {0};

          if (hold != 0xffu && (agx_shader_scan(builder, first, count, start, end, held, false, hold) ||
                                agx_shader_scan(builder, first, count, start, end, held, true, hold)))
          {
            uint32_t v = 0;

            for (v = 0; v < AGX_SHADER_VIRTUAL_REGISTER_BASE; ++v)
            {
              assigned[v] = held[v];
            }
            builder->wait_scratch = hold;
            builder->has_wait_scratch = true;
          }
        }
        agx_shader_rewrite_registers(builder, assigned);
        agx_refusal_clear();
        return true;
      }

      {
        char        said[AGX_REFUSAL_BYTES] = {0};
        const char* was = agx_refusal();
        uint32_t    c = 0;

        for (c = 0; c + 1u < AGX_REFUSAL_BYTES && was[c] != '\0'; ++c)
        {
          said[c] = was[c];
        }
        agx_refuse(
          "%s; attempt %u with %d value(s) in memory of %u slot(s)", said, attempt, chosen, (uint32_t)area->slot_count
        );
      }
      if (original + added > stash)
      {
        agx_refuse(
          "agx_shader_allocate_registers: the scan refused the spilled program and the "
          "originals cannot be recovered to spill more -- %u instruction(s) rewritten to "
          "%u and the stash starts at %u",
          original,
          original + added,
          stash
        );
        return false;
      }
      if ((uint32_t)chosen >= area->slot_count)
      {
        return false;
      }
      floor = (uint32_t)chosen + 1u;
    }
  }
}

static bool
agx_shader_op_wait_names_slot(Agx_Shader_Op op)
{
  return op != AGX_SHADER_OP_BIT_OP && op != AGX_SHADER_OP_MOVE_NIBBLE_3;
}

bool
agx_shader_op_can_wait(Agx_Shader_Op op)
{
  switch (op)
  {
  case AGX_SHADER_OP_LOAD:
  case AGX_SHADER_OP_STORE_BUFFER:
  case AGX_SHADER_OP_ADD_INDEX:

  case AGX_SHADER_OP_SCALE_INDEX:

  case AGX_SHADER_OP_COMPARE:
  case AGX_SHADER_OP_FLOAT_UNARY:
  case AGX_SHADER_OP_SUM:
  case AGX_SHADER_OP_PRODUCT:
  case AGX_SHADER_OP_MULTIPLY_ADD:
  case AGX_SHADER_OP_OUTPUT_MOVE:
  case AGX_SHADER_OP_BIT_UNARY:
  case AGX_SHADER_OP_WIDEN:
  case AGX_SHADER_OP_NARROW:

  case AGX_SHADER_OP_CONVERT_TO_FLOAT:
  case AGX_SHADER_OP_CONVERT_TO_INTEGER:

  case AGX_SHADER_OP_VARYING_READ:

  case AGX_SHADER_OP_BIT_OP:

  case AGX_SHADER_OP_MOVE_NIBBLE_3:
    return true;
  default:
    return false;
  }
}

static bool
agx_shader_op_is_memory(Agx_Shader_Op op)
{
  return op == AGX_SHADER_OP_LOAD || op == AGX_SHADER_OP_TEXTURE_SAMPLE || op == AGX_SHADER_OP_ATOMIC_RESULT;
}

static bool
agx_shader_release_operand(Agx_Shader_Instruction* instruction, const Agx_Shader_Operands* operands, uint32_t which)
{
  const void* at = operands->slot[which];

  switch (instruction->op)
  {
  case AGX_SHADER_OP_MIN_MAX:
  case AGX_SHADER_OP_PRODUCT:
  case AGX_SHADER_OP_SUM:
    if (at == &instruction->source)
    {
      instruction->source_form = AGX_SHADER_SOURCE_FORM_DISCARD;
      return true;
    }
    if (at == &instruction->source_b)
    {
      instruction->second_form = AGX_SHADER_OPERAND_FORM_DISCARD;
      return true;
    }
    return false;

  case AGX_SHADER_OP_COMPARE_SELECT:
    if (at == &instruction->source)
    {
      instruction->source_form = AGX_SHADER_SOURCE_FORM_DISCARD;
      return true;
    }
    if (at == &instruction->compare_second)
    {
      instruction->second_form = AGX_SHADER_OPERAND_FORM_DISCARD;
      return true;
    }
    return false;

  case AGX_SHADER_OP_BIT_OP:
    if (at == &instruction->source)
    {
      instruction->source_form = AGX_SHADER_SOURCE_FORM_DISCARD;
      return true;
    }
    if (at == &instruction->source_b)
    {
      instruction->keep_source_b = false;
      return true;
    }
    return false;

  case AGX_SHADER_OP_MULTIPLY_ADD:
    if (at == &instruction->source)
    {
      instruction->source_form = AGX_SHADER_SOURCE_FORM_DISCARD;
      return true;
    }
    if (at == &instruction->source_b)
    {
      instruction->discard_source_b = true;
      return true;
    }
    if (at == &instruction->source_c)
    {
      instruction->third_form = AGX_SHADER_OPERAND_FORM_DISCARD;
      return true;
    }
    return false;

  case AGX_SHADER_OP_WIDEN:
  case AGX_SHADER_OP_NARROW:
    if (at == &instruction->source)
    {
      instruction->source_form = AGX_SHADER_SOURCE_FORM_DISCARD;
      return true;
    }
    return false;

  case AGX_SHADER_OP_CONVERT_TO_INTEGER:
    if (at == &instruction->source)
    {
      instruction->convert_source_form = AGX_SHADER_CONVERT_SOURCE_FORM_RELEASE;
      return true;
    }
    return false;

  case AGX_SHADER_OP_ADD_INDEX:

  case AGX_SHADER_OP_SCALE_INDEX:
    if (at == &instruction->source_b)
    {
      instruction->second_form = AGX_SHADER_OPERAND_FORM_DISCARD;
      return true;
    }
    return false;

  default:
    return false;
  }
}

bool
agx_shader_release_dead_operands(Agx_Shader_Builder* builder)
{
  uint32_t i, j, k, m;

  for (i = 0; i < builder->count; ++i)
  {
    Agx_Shader_Op op = builder->instructions[i].op;

    if (op == AGX_SHADER_OP_JUMP_EXEC_ANY || op == AGX_SHADER_OP_JUMP_ABSOLUTE || op == AGX_SHADER_OP_BRANCH)
    {
      return true;
    }
  }

  for (i = 0; i < builder->count; ++i)
  {
    Agx_Shader_Operands operands = {0};

    if (!agx_shader_operands(&builder->instructions[i], &operands) || !operands.modelled)
    {
      continue;
    }

    for (k = 0; k < operands.count; ++k)
    {
      uint32_t first = agx_shader_operand_register(&operands, k);
      uint32_t span = operands.width[k] > 0 ? operands.width[k] : 1;
      bool     read_later = false;

      if (operands.role[k] != AGX_SHADER_OPERAND_ROLE_USE || operands.scale[k] == 0)
      {
        continue;
      }

      for (j = i + 1; j < builder->count && !read_later; ++j)
      {
        Agx_Shader_Operands later = {0};

        if (!agx_shader_operands(&builder->instructions[j], &later) || !later.modelled)
        {
          read_later = true;
          break;
        }
        for (m = 0; m < later.count; ++m)
        {
          uint32_t other = agx_shader_operand_register(&later, m);
          uint32_t width = later.width[m] > 0 ? later.width[m] : 1;

          if (later.role[m] != AGX_SHADER_OPERAND_ROLE_USE)
          {
            continue;
          }
          if (other < first + span && first < other + width)
          {
            read_later = true;
            break;
          }
        }
      }

      if (!read_later)
      {
        (void)agx_shader_release_operand(&builder->instructions[i], &operands, k);
      }
    }
  }

  return true;
}

bool
agx_shader_schedule_waits_once(Agx_Shader_Builder* builder, uint32_t* carrier_at, uint8_t* carrier_register, uint8_t* carrier_destination)
{
  bool pending[256] = {0};

  uint8_t  pending_slot[256] = {0};
  uint32_t i, k;

  memset(pending, 0, sizeof(pending));
  memset(pending_slot, 0, sizeof(pending_slot));

  for (i = 0; i < builder->count; ++i)
  {
    Agx_Shader_Operands operands = {0};

    if (!agx_shader_operands(&builder->instructions[i], &operands))
    {
      agx_refuse(
        "agx_shader_schedule_waits: instruction %u of %u (op %u) is not in the operand "
        "model, so the pass cannot see what it reads",
        i,
        builder->count,
        (uint32_t)builder->instructions[i].op
      );
      return false;
    }
    builder->instructions[i].wait = false;
    builder->instructions[i].store_waits = false;
  }

  for (i = 0; i < builder->count; ++i)
  {
    Agx_Shader_Instruction* instruction = &builder->instructions[i];
    Agx_Shader_Operands     operands = {0};
    bool                    consumes = false;

    uint32_t consumed_register = 0;

    (void)agx_shader_operands(instruction, &operands);
    for (k = 0; k < operands.count; ++k)
    {
      uint32_t first = agx_shader_operand_register(&operands, k);
      uint32_t span = operands.width[k] > 0 ? operands.width[k] : 1;
      uint32_t part = 0;
      uint32_t hit = first;
      bool     any = false;

      for (part = 0; part < span; ++part)
      {
        if (first + part < 256 && pending[first + part])
        {
          hit = first + part;
          any = true;
          break;
        }
      }

      if (operands.role[k] == AGX_SHADER_OPERAND_ROLE_USE && any)
      {
        if (!agx_shader_op_can_wait(instruction->op))
        {
          agx_refuse(
            "agx_shader_schedule_waits: instruction %u of %u (op %u) reads r%u, which is "
            "outstanding on slot %u, and its bytes carry no wait bit",
            i,
            builder->count,
            (uint32_t)instruction->op,
            hit,
            (uint32_t)pending_slot[hit]
          );

          if (carrier_at != NULL && pending_slot[hit] == AGX_SHADER_WAIT_SLOT_IMPLICIT)
          {
            if (builder->has_wait_scratch)
            {
              *carrier_at = i;
              *carrier_register = (uint8_t)hit;
              *carrier_destination = builder->wait_scratch;
            }
          }
          return false;
        }

        if (!agx_shader_op_wait_names_slot(instruction->op) && pending_slot[hit] != AGX_SHADER_WAIT_SLOT_IMPLICIT)
        {
          agx_refuse(
            "agx_shader_schedule_waits: instruction %u of %u (op %u) reads r%u, which is "
            "outstanding on slot %u, and its wait bit can only name slot %u",
            i,
            builder->count,
            (uint32_t)instruction->op,
            hit,
            (uint32_t)pending_slot[hit],
            (uint32_t)AGX_SHADER_WAIT_SLOT_IMPLICIT
          );
          return false;
        }

        if (consumes && instruction->wait_slot != pending_slot[hit])
        {
          if (carrier_at != NULL && builder->has_wait_scratch &&
              (pending_slot[hit] == AGX_SHADER_WAIT_SLOT_IMPLICIT ||
               instruction->wait_slot == AGX_SHADER_WAIT_SLOT_IMPLICIT))
          {
            *carrier_at = i;
            *carrier_register =
              pending_slot[hit] == AGX_SHADER_WAIT_SLOT_IMPLICIT ? (uint8_t)hit : (uint8_t)consumed_register;
            *carrier_destination = builder->wait_scratch;
          }
          agx_refuse(
            "agx_shader_schedule_waits: instruction %u of %u (op %u) needs two slots "
            "outstanding at once -- slot %u already, and r%u on slot %u -- and an "
            "instruction names one",
            i,
            builder->count,
            (uint32_t)instruction->op,
            (uint32_t)instruction->wait_slot,
            hit,
            (uint32_t)pending_slot[hit]
          );
          return false;
        }
        consumes = true;
        consumed_register = hit;
        instruction->wait = true;
        instruction->wait_slot = pending_slot[hit];
      }
    }
    if (consumes)
    {
      uint32_t reg = 0;

      for (reg = 0; reg < 256; ++reg)
      {
        if (pending[reg] && pending_slot[reg] == instruction->wait_slot)
        {
          pending[reg] = false;
        }
      }
    }
    if (agx_shader_op_is_memory(instruction->op))
    {
      for (k = 0; k < operands.count; ++k)
      {
        if (operands.role[k] == AGX_SHADER_OPERAND_ROLE_DEFINE)
        {
          uint32_t first = agx_shader_operand_register(&operands, k);
          uint32_t span = operands.width[k] > 0 ? operands.width[k] : 1;
          uint32_t part = 0;

          for (part = 0; part < span && first + part < 256; ++part)
          {
            pending[first + part] = true;
            pending_slot[first + part] = instruction->slot;
          }
        }
      }
    }
  }

  {
    uint32_t store_at[64] = {0};
    uint32_t stores = 0;

    for (i = 0; i < builder->count; ++i)
    {
      Agx_Shader_Instruction* instruction = &builder->instructions[i];

      if (instruction->op == AGX_SHADER_OP_STORE_BUFFER)
      {
        if (stores < sizeof(store_at) / sizeof(store_at[0]))
        {
          store_at[stores++] = i;
        }
      }
      else if (instruction->op == AGX_SHADER_OP_LOAD)
      {
        for (k = 0; k < stores; ++k)
        {
          const Agx_Shader_Instruction* wrote = &builder->instructions[store_at[k]];

          if (wrote->base == instruction->base && wrote->index == instruction->index && wrote->offset == instruction->offset)
          {
            instruction->wait = true;
            stores = 0;
            break;
          }
        }
      }
    }
  }
  return true;
}

bool
agx_shader_schedule_waits(Agx_Shader_Builder* builder)
{
  uint32_t round = 0;

  for (round = 0; round <= builder->capacity; ++round)
  {
    uint32_t                carrier_at = AGX_SHADER_RANGE_UNSET;
    uint8_t                 carrier_register = 0;
    uint8_t                 carrier_destination = 0;
    Agx_Shader_Instruction* placed = NULL;

    if (agx_shader_schedule_waits_once(builder, &carrier_at, &carrier_register, &carrier_destination))
    {
      return true;
    }
    if (carrier_at == AGX_SHADER_RANGE_UNSET)
    {
      return false;
    }

    if (builder->count + 1u > builder->capacity)
    {
      agx_refuse(
        "agx_shader_schedule_waits: a wait carrier is needed before instruction %u and "
        "the builder holds %u instruction(s)",
        carrier_at,
        builder->capacity
      );
      return false;
    }

    memmove(
      builder->instructions + carrier_at + 1u,
      builder->instructions + carrier_at,
      (builder->count - carrier_at) * sizeof(builder->instructions[0])
    );
    builder->count += 1u;
    if (!agx_shader_shift_jumps(builder, carrier_at))
    {
      agx_refuse(
        "agx_shader_schedule_waits: a wait carrier is needed at instruction %u, which is "
        "a loop's first instruction -- one placed there would run on the way in and be "
        "skipped every iteration after",
        carrier_at
      );
      return false;
    }

    placed = &builder->instructions[carrier_at];
    memset(placed, 0, sizeof(*placed));
    placed->op = AGX_SHADER_OP_BIT_OP;
    placed->truth_table = AGX_SHADER_BIT_AND;
    placed->source = carrier_register;
    placed->source_b = carrier_register;
    placed->destination = carrier_destination;
    placed->keep_source_b = true;
    placed->source_form = AGX_SHADER_SOURCE_FORM_KEEP;
  }
  agx_refuse("agx_shader_schedule_waits: %u wait carrier(s) inserted and the pass still refuses", builder->capacity);
  return false;
}
