#pragma once

#include <stdbool.h>
#include <stdint.h>

#define AGX_SHADER_RAW_MAX          16
#define AGX_SHADER_MOVE_UNKNOWN_BIT 0x40u

#define AGX_SHADER_LOAD_BYTE_2_BIT_4_CLEAR      0x1000u
#define AGX_SHADER_ADD_INDEX_BYTE_7_HIGH_CLEAR  0x0100u
#define AGX_SHADER_ADD_INDEX_BYTE_9_BIT_1       0x0200u
#define AGX_SHADER_ADD_INDEX_BYTE_2_BIT_5       0x2000u
#define AGX_SHADER_ADD_INDEX_BYTE_4_SWAP        0x4000u
#define AGX_SHADER_ADD_INDEX_BYTE_7_BIT_5       0x8000u
#define AGX_SHADER_ADD_INDEX_BYTE_7_BIT_3_CLEAR 0x10000u

#define AGX_SHADER_MOVE_NIBBLE_3_SOURCE_HALF_UNMEASURED 0x10000u
#define AGX_SHADER_MOVE_NIBBLE_3_RAW_NIBBLE_SHIFT       20u
#define AGX_SHADER_MOVE_NIBBLE_3_RAW_TAIL_SHORT         0x40000u
#define AGX_SHADER_BIT_OP_BYTE_5_SHIFT                  24u
#define AGX_SHADER_BIT_OP_BYTE_4_BIT_6                  0x00800000u
#define AGX_SHADER_TEXTURE_BYTE_1_LOW_SHIFT             2u
#define AGX_SHADER_TEXTURE_BYTE_3_BIT_7_CLEAR           0x0020u
#define AGX_SHADER_TEXTURE_BYTE_4_BIT_7_CLEAR           0x0040u
#define AGX_SHADER_TEXTURE_BYTE_5_SHIFT                 8u
#define AGX_SHADER_TEXTURE_BYTE_12_IS_0X24              0x10000u
#define AGX_SHADER_TEXTURE_BYTE_6_HIGH_BITS             0x20000u
#define AGX_SHADER_SCALE_INDEX_BYTE_9_BIT_0             0x0001u
#define AGX_SHADER_SCALE_INDEX_BYTE_10_BIT_1_CLEAR      0x0004u
#define AGX_SHADER_SCALE_INDEX_BYTE_9_BIT_3             0x0008u
#define AGX_SHADER_SCALE_INDEX_BYTE_4_BIT_0             0x0010u
#define AGX_SHADER_SCALE_INDEX_BYTE_7_BIT_2             0x0020u
#define AGX_SHADER_SCALE_INDEX_SHORT_BYTE_2_IS_0X24     0x0040u
#define AGX_SHADER_SCALE_INDEX_SHORT_BYTE_4_BIT_0       0x0080u
#define AGX_SHADER_SCALE_INDEX_SHORT_BYTE_8_BIT_1       0x0100u
#define AGX_SHADER_SCALE_INDEX_SHORT_BYTE_8_BIT_4       0x0200u
#define AGX_SHADER_SCALE_INDEX_SHORT_BYTE_6_LOW_SHIFT   16u
#define AGX_SHADER_SCALE_INDEX_SHORT_BYTE_7_MID_SHIFT   20u
#define AGX_SHADER_SHIFT_BYTE_4_IS_TWO                  0x10000u
#define AGX_SHADER_SHIFT_BYTE_4_BIT_1_CLEAR             0x20000u
#define AGX_SHADER_SHIFT_BYTE_9_LOW_SHIFT               20u
#define AGX_SHADER_SHIFT_BYTE_9_BIT_7                   0x800000u
#define AGX_SHADER_SHIFT_BYTE_8_BIT_4_CLEAR             0x1000000u
#define AGX_SHADER_SHIFT_BYTE_8_BIT_5_CLEAR             0x2000000u
#define AGX_SHADER_SHIFT_BYTE_5_BIT_1                   0x4000000u
#define AGX_SHADER_SHIFT_BYTE_10_BIT_0_CLEAR            0x8000000u
#define AGX_SHADER_COMPARE_BYTE_4_BIT_7                 0x0080u
#define AGX_SHADER_COMPARE_TYPE_BITS_CLEAR              0x0100u
#define AGX_SHADER_COMPARE_CONDITION_BIT_3              0x0200u
#define AGX_SHADER_COMPARE_WIDE_BYTE_3_BIT_0            0x0400u
#define AGX_SHADER_COMPARE_BYTE_0_HIGH_SHIFT            12u
#define AGX_SHADER_COMPARE_BYTE_6_BIT_0                 0x0800u
#define AGX_SHADER_COMPARE_BYTE_8_BIT_1                 0x0010u
#define AGX_SHADER_STORE_SURFACE_BYTE_5_BIT_0           0x01000000u
#define AGX_SHADER_LOAD_SURFACE_FIRST                   0x02000000u
#define AGX_SHADER_CONVERT_FROM_SURFACE_BYTE_4          0x00000001u
#define AGX_SHADER_NOP_BYTE_0_HIGH_SHIFT                4u

#define AGX_SHADER_STORE_INDEX_PRESCALED           0x00u
#define AGX_SHADER_STORE_INDEX_HARDWARE_SCALED     0x20u
#define AGX_SHADER_LOAD_ELEMENT_BYTES_MAX          16u
#define AGX_SHADER_VERTEX_EXPORT_BYTE_1_LOW_CLEAR  0x0001u
#define AGX_SHADER_VERTEX_EXPORT_BYTE_5_BASE_CLEAR 0x0002u
#define AGX_SHADER_VERTEX_EXPORT_BYTE_6_BASE_CLEAR 0x0004u
#define AGX_SHADER_VERTEX_EXPORT_BYTE_5_BIT_4      0x0008u
#define AGX_SHADER_VERTEX_EXPORT_BYTE_7_SHIFT      8u
#define AGX_SHADER_CALL_BYTE_0_BIT_7               0x0001u
#define AGX_SHADER_CALL_BYTE_1_CLEAR               0x0002u
#define AGX_SHADER_CALL_BYTE_7_CLEAR               0x0004u
#define AGX_SHADER_CALL_BYTE_7_BIT_4               0x0008u
#define AGX_SHADER_MIN_MAX_BYTE_4_BIT_7            0x0001u
#define AGX_SHADER_MIN_MAX_BYTE_5_BASE_CLEAR       0x0002u
#define AGX_SHADER_MIN_MAX_BYTE_5_SHIFT            8u
#define AGX_SHADER_BARRIER_BYTE_3_BIT_1            0x0040u
#define AGX_SHADER_PUSH_EXEC_BYTE_3_LOW_IS_TWO     0x0004u
#define AGX_SHADER_PUSH_EXEC_BYTE_2_BIT_5          0x0020u
#define AGX_SHADER_PUSH_EXEC_BYTE_2_BIT_4_CLEAR    0x0040u
#define AGX_SHADER_MOVE_UNKNOWN_BIT_5              0x20u
#define AGX_SHADER_MOVE_BYTE_2_BIT_5               0x0100u
#define AGX_SHADER_MOVE_BYTE_1_BIT_0               0x0800u
#define AGX_SHADER_MOVE_BYTE_2_BIT_1               0x0200u
#define AGX_SHADER_MOVE_BYTE_2_BIT_4               0x0400u
#define AGX_SHADER_MOVE_BYTE_2_BIT_2               0x1000u
#define AGX_SHADER_OUTPUT_MOVE_BYTE_2_BIT_3_CLEAR  0x0200u
#define AGX_SHADER_OUTPUT_MOVE_BYTE_2_BIT_0_CLEAR  0x0100u
#define AGX_SHADER_OUTPUT_MOVE_BYTE_1_BIT_0        0x0400u
#define AGX_SHADER_OUTPUT_MOVE_BYTE_2_BIT_5_CLEAR  0x0800u
#define AGX_SHADER_OUTPUT_MOVE_BYTE_4_CLEAR        0x1000u
#define AGX_SHADER_OUTPUT_MOVE_BYTE_3_SHIFT        16u
#define AGX_SHADER_OUTPUT_MOVE_BYTE_5_SHIFT        24u
#define AGX_SHADER_OUTPUT_MOVE_BYTE_8_IS_0X20      0x2000u
#define AGX_SHADER_OUTPUT_MOVE_BYTE_2_BIT_6        0x4000u

#define AGX_SHADER_MOVE_IMMEDIATE_DESTINATION_BIT_5 0x80u
#define AGX_SHADER_MOVE_IMMEDIATE_BYTE_2_BIT_3      0x08u
#define AGX_SHADER_MOVE_IMMEDIATE_BYTE_2_BIT_5      0x20u
#define AGX_SHADER_MOVE_IMMEDIATE_BYTE_2_BIT_2      0x04u
#define AGX_SHADER_JUMP_ANY_BYTE_1_HIGH             0x10000u
#define AGX_SHADER_JUMP_ANY_BYTE_2_SHIFT            17u
#define AGX_SHADER_JUMP_TO_MATCHING_POP             0xffffffffu
#define AGX_SHADER_HALF_SELECT_ONE                  0x0207u
#define AGX_SHADER_HALF_SELECT_ZERO_A               0xa20fu
#define AGX_SHADER_HALF_SELECT_ZERO_B               0xc20fu

#define AGX_SHADER_PACK_SOURCES_ARE_HALF 0x10u

#define AGX_SHADER_PACK_CHANNEL_3_IS_REGISTER_11 0x02u
#define AGX_SHADER_PACK_CHANNEL_3_IS_REGISTER_13 0x01u

#define AGX_SHADER_PACK_CHANNELS      4u
#define AGX_SHADER_PACK_SOURCE_BITS   6u
#define AGX_SHADER_PACK_SOURCE_SHIFT  2u
#define AGX_SHADER_PACK_SOURCE_STRIDE 9u

#define AGX_SHADER_HALF_PER_REGISTER 2u

#define AGX_SHADER_WAIT_OPERAND_DEFAULT 0x000cu
#define AGX_SHADER_WAIT_OPERAND_FIRST   0x0600u
#define AGX_SHADER_WAIT_OPERAND_SECOND  0x080cu
#define AGX_SHADER_WAIT_OPERAND_THIRD   0x020cu

#define AGX_SHADER_WAIT_OPERAND_BLEND_LOAD 0x0808u

#define AGX_SHADER_WAIT_OPERAND_BLEND_STORE   0x0804u
#define AGX_SHADER_COVERAGE_KEEP              0x00000101u
#define AGX_SHADER_COVERAGE_KILL              0xffffff00u
#define AGX_SHADER_WAIT_OPERAND_COVERAGE      0x0001u
#define AGX_SHADER_WAIT_OPERAND_COVERAGE_KILL 0x0201u

#define AGX_SHADER_BIT_ZERO              0x0u
#define AGX_SHADER_BIT_NOR               0x1u
#define AGX_SHADER_BIT_ANDN2             0x2u
#define AGX_SHADER_BIT_NOT_FIRST         0x3u
#define AGX_SHADER_BIT_ANDN1             0x4u
#define AGX_SHADER_BIT_NOT_SECOND        0x5u
#define AGX_SHADER_BIT_XOR               0x6u
#define AGX_SHADER_BIT_NAND              0x7u
#define AGX_SHADER_BIT_AND               0x8u
#define AGX_SHADER_BIT_XNOR              0x9u
#define AGX_SHADER_BIT_FIRST             0xau
#define AGX_SHADER_BIT_ORN2              0xbu
#define AGX_SHADER_BIT_ORN1              0xdu
#define AGX_SHADER_BIT_OR                0xeu
#define AGX_SHADER_BIT_ONE               0xfu
#define AGX_SHADER_VIRTUAL_REGISTER_BASE 128u
#define AGX_SHADER_VIRTUAL_REGISTER(n)   ((uint8_t)(AGX_SHADER_VIRTUAL_REGISTER_BASE + (n)))
#define AGX_SHADER_SPILL_RESERVE         4u

#define AGX_SHADER_RANGE_UNSET 0xffffffffu

typedef enum Agx_Shader_Op
{
  AGX_SHADER_OP_NOP,
  AGX_SHADER_OP_STOP,
  AGX_SHADER_OP_MOVE_IMMEDIATE,
  AGX_SHADER_OP_MOVE,
  AGX_SHADER_OP_DISCARD,
  AGX_SHADER_OP_LOAD,
  AGX_SHADER_OP_BRANCH,
  AGX_SHADER_OP_MOVE_NIBBLE_3,
  AGX_SHADER_OP_HALF_COMPARE_SELECT,
  AGX_SHADER_OP_CONVERT_TO_FLOAT,
  AGX_SHADER_OP_GET_SPECIAL,
  AGX_SHADER_OP_CALL_POOL_SHADER,
  AGX_SHADER_OP_MOVE_TO_TILE,
  AGX_SHADER_OP_STORE_TILE_PIXEL,
  AGX_SHADER_OP_END_THREAD,
  AGX_SHADER_OP_SHIFT_RIGHT,
  AGX_SHADER_OP_CONVERT_TO_SURFACE,
  AGX_SHADER_OP_PACK_TEXEL,
  AGX_SHADER_OP_STORE_SURFACE,
  AGX_SHADER_OP_OUTPUT_MOVE,
  AGX_SHADER_OP_VERTEX_EXPORT,
  AGX_SHADER_OP_VARYING_READ,
  AGX_SHADER_OP_RECIPROCAL,
  AGX_SHADER_OP_MULTIPLY,
  AGX_SHADER_OP_MULTIPLY_ADD,
  AGX_SHADER_OP_TOP_BIT,
  AGX_SHADER_OP_PRODUCT,
  AGX_SHADER_OP_ADD_INDEX,
  AGX_SHADER_OP_SUM,
  AGX_SHADER_OP_WIDEN,
  AGX_SHADER_OP_NARROW,
  AGX_SHADER_OP_PACK_CONSTANT,
  AGX_SHADER_OP_SCALE_INDEX,
  AGX_SHADER_OP_MOVE_FOR_SAMPLE,
  AGX_SHADER_OP_TEXTURE_SAMPLE,
  AGX_SHADER_OP_WAIT_FOR_STORE,
  AGX_SHADER_OP_POP_EXEC,
  AGX_SHADER_OP_JUMP_EXEC_NONE,
  AGX_SHADER_OP_PUSH_EXEC,
  AGX_SHADER_OP_COMPARE,
  AGX_SHADER_OP_STORE_BUFFER,
  AGX_SHADER_OP_JUMP_EXEC_ANY,
  AGX_SHADER_OP_JUMP_ABSOLUTE,
  AGX_SHADER_OP_MIN_MAX,
  AGX_SHADER_OP_FLOAT_UNARY,
  AGX_SHADER_OP_COMPARE_SELECT,
  AGX_SHADER_OP_BIT_OP,
  AGX_SHADER_OP_BIT_UNARY,
  AGX_SHADER_OP_CONVERT_TO_INTEGER,
  AGX_SHADER_OP_BARRIER,
  AGX_SHADER_OP_SAMPLE_MASK,
  AGX_SHADER_OP_DERIVATIVE,
  AGX_SHADER_OP_ATOMIC,
  AGX_SHADER_OP_ATOMIC_RESULT,
  AGX_SHADER_OP_RAW,
  AGX_SHADER_OP_END_THREAD_LONG,
  AGX_SHADER_OP_LOAD_SURFACE,
  AGX_SHADER_OP_CONVERT_FROM_SURFACE,
  AGX_SHADER_OP_COUNT
} Agx_Shader_Op;

typedef enum Agx_Shader_Memory_Scope
{
  AGX_SHADER_MEMORY_SCOPE_NONE,
  AGX_SHADER_MEMORY_SCOPE_THREADGROUP,
  AGX_SHADER_MEMORY_SCOPE_DEVICE,
  AGX_SHADER_MEMORY_SCOPE_TEXTURE_FIRST,
  AGX_SHADER_MEMORY_SCOPE_TEXTURE_SECOND,
  AGX_SHADER_MEMORY_SCOPE_COUNT
} Agx_Shader_Memory_Scope;

typedef enum Agx_Shader_Packed_Format
{
  AGX_SHADER_PACKED_FORMAT_RGB10A2,
  AGX_SHADER_PACKED_FORMAT_RG11B10,
  AGX_SHADER_PACKED_FORMAT_RGB9E5,
  AGX_SHADER_PACKED_FORMAT_COUNT,
} Agx_Shader_Packed_Format;

typedef enum Agx_Shader_Numeric_Format
{
  AGX_SHADER_NUMERIC_FORMAT_UNORM8 = 0x6u,
  AGX_SHADER_NUMERIC_FORMAT_UNORM16 = 0x4u,
  AGX_SHADER_NUMERIC_FORMAT_SNORM8 = 0x3u,
  AGX_SHADER_NUMERIC_FORMAT_SRGB_BOTH = 0x1u,
  AGX_SHADER_NUMERIC_FORMAT_SRGB_LOW_ONLY = 0x5u,
  AGX_SHADER_NUMERIC_FORMAT_COUNT = 8
} Agx_Shader_Numeric_Format;

typedef enum Agx_Shader_Convert_Form
{
  AGX_SHADER_CONVERT_FORM_PAIR,
  AGX_SHADER_CONVERT_FORM_SINGLE,
  AGX_SHADER_CONVERT_FORM_COUNT
} Agx_Shader_Convert_Form;

typedef enum Agx_Shader_Index_Form
{
  AGX_SHADER_INDEX_FORM_FULL = 7,
  AGX_SHADER_INDEX_FORM_NARROW_WIDE = 6,
  AGX_SHADER_INDEX_FORM_NARROW = 5,
  AGX_SHADER_INDEX_FORM_MASKED = 1,
  AGX_SHADER_INDEX_FORM_MULTIPLY = 13,
} Agx_Shader_Index_Form;

typedef enum Agx_Shader_Index_Source_Form
{
  AGX_SHADER_INDEX_SOURCE_FORM_DISCARD,
  AGX_SHADER_INDEX_SOURCE_FORM_KEEP,
  AGX_SHADER_INDEX_SOURCE_FORM_UNMARKED,
  AGX_SHADER_INDEX_SOURCE_FORM_COUNT
} Agx_Shader_Index_Source_Form;

typedef enum Agx_Shader_Load_Form
{
  AGX_SHADER_LOAD_FORM_ORDINARY,
  AGX_SHADER_LOAD_FORM_DYNAMIC_WIDE,
  AGX_SHADER_LOAD_FORM_BACKGROUND_OBJECT,
  AGX_SHADER_LOAD_FORM_REGISTER_BASE,
} Agx_Shader_Load_Form;

typedef enum Agx_Shader_Varying_Form
{
  AGX_SHADER_VARYING_FORM_INTERPOLATED = 0,
  AGX_SHADER_VARYING_FORM_LEADING,
  AGX_SHADER_VARYING_FORM_COUNT
} Agx_Shader_Varying_Form;

typedef enum Agx_Shader_Convert_Source_Form
{
  AGX_SHADER_CONVERT_SOURCE_FORM_UNMARKED = 0,
  AGX_SHADER_CONVERT_SOURCE_FORM_RELEASE,
  AGX_SHADER_CONVERT_SOURCE_FORM_KEEP,
  AGX_SHADER_CONVERT_SOURCE_FORM_COUNT
} Agx_Shader_Convert_Source_Form;

typedef enum Agx_Shader_Operand_Form
{
  AGX_SHADER_OPERAND_FORM_DISCARD,
  AGX_SHADER_OPERAND_FORM_KEEP,
  AGX_SHADER_OPERAND_FORM_IMMEDIATE,
  AGX_SHADER_OPERAND_FORM_COUNT
} Agx_Shader_Operand_Form;

typedef enum Agx_Shader_Source_Form
{
  AGX_SHADER_SOURCE_FORM_DISCARD,
  AGX_SHADER_SOURCE_FORM_KEEP,
  AGX_SHADER_SOURCE_FORM_UNIFORM,
  AGX_SHADER_SOURCE_FORM_COUNT
} Agx_Shader_Source_Form;

typedef enum Agx_Shader_Special
{
  AGX_SHADER_SPECIAL_VERTEX_INDEX = 0xdd,
  AGX_SHADER_SPECIAL_INSTANCE_INDEX = 0xd8,
  AGX_SHADER_SPECIAL_THREAD_INDEX = 0xa0,
  AGX_SHADER_SPECIAL_PIXEL_X = 0xa0,
  AGX_SHADER_SPECIAL_PIXEL_Y = 0xa1,
  AGX_SHADER_SPECIAL_UNKNOWN_81 = 0x81,
  AGX_SHADER_SPECIAL_LOCAL_THREAD_INDEX = 0xa4,
  AGX_SHADER_SPECIAL_THREADGROUP_INDEX = 0x9c,
  AGX_SHADER_SPECIAL_FLAT_LOCAL_INDEX = 0xa7,
  AGX_SHADER_SPECIAL_THREADGROUP_SIZE = 0x98,
  AGX_SHADER_SPECIAL_COVERAGE_MASK = 0x84,
  AGX_SHADER_SPECIAL_TILE_PROGRAM = 0xb8,
} Agx_Shader_Special;

typedef enum Agx_Shader_Compare
{
  AGX_SHADER_COMPARE_GREATER,
  AGX_SHADER_COMPARE_LESS,
  AGX_SHADER_COMPARE_EQUAL,
  AGX_SHADER_COMPARE_ALWAYS,
  AGX_SHADER_COMPARE_NEVER,
  AGX_SHADER_COMPARE_COUNT
} Agx_Shader_Compare;

typedef enum Agx_Shader_Min_Max_Type
{
  AGX_SHADER_MIN_MAX_TYPE_FLOAT = 0x00,
  AGX_SHADER_MIN_MAX_TYPE_UNSIGNED = 0x04,
  AGX_SHADER_MIN_MAX_TYPE_SIGNED = 0x06
} Agx_Shader_Min_Max_Type;

typedef enum Agx_Shader_Compare_Type
{
  AGX_SHADER_COMPARE_TYPE_FLOAT = 0x02,
  AGX_SHADER_COMPARE_TYPE_UNSIGNED = 0x04,
  AGX_SHADER_COMPARE_TYPE_SIGNED = 0x06
} Agx_Shader_Compare_Type;

typedef enum Agx_Shader_Mask_Source
{
  AGX_SHADER_MASK_SOURCE_COMPARE = 0,
  AGX_SHADER_MASK_SOURCE_NONE = 1,
  AGX_SHADER_MASK_SOURCE_KEEP = 6
} Agx_Shader_Mask_Source;

typedef enum Agx_Shader_Bit_Unary
{
  AGX_SHADER_BIT_UNARY_POPCOUNT = 0x27054c,
  AGX_SHADER_BIT_UNARY_REVERSE = 0xa7044c,
  AGX_SHADER_BIT_UNARY_FIND_LAST_SET = 0xa7054e
} Agx_Shader_Bit_Unary;

typedef enum Agx_Shader_Float_Unary
{
  AGX_SHADER_FLOAT_UNARY_RSQRT = 0x81,
  AGX_SHADER_FLOAT_UNARY_LOG2 = 0x02,
  AGX_SHADER_FLOAT_UNARY_EXP2 = 0x82,
  AGX_SHADER_FLOAT_UNARY_RINT = 0x00,
  AGX_SHADER_FLOAT_UNARY_FLOOR = 0x10,
  AGX_SHADER_FLOAT_UNARY_CEIL = 0x20,
  AGX_SHADER_FLOAT_UNARY_TRUNC = 0x30
} Agx_Shader_Float_Unary;

typedef enum Agx_Shader_Texture_Access
{
  AGX_SHADER_TEXTURE_ACCESS_DEFAULT_ZEROS,
  AGX_SHADER_TEXTURE_ACCESS_READ_2D,
  AGX_SHADER_TEXTURE_ACCESS_SAMPLE_2D,
  AGX_SHADER_TEXTURE_ACCESS_SAMPLE_STAGE_IN,
  AGX_SHADER_TEXTURE_ACCESS_READ_1D,
  AGX_SHADER_TEXTURE_ACCESS_READ_3D,
  AGX_SHADER_TEXTURE_ACCESS_READ_ARRAY,
  AGX_SHADER_TEXTURE_ACCESS_SAMPLE_CUBE,
  AGX_SHADER_TEXTURE_ACCESS_GATHER,
  AGX_SHADER_TEXTURE_ACCESS_SAMPLE_COMPARE_STAGE_IN,
  AGX_SHADER_TEXTURE_ACCESS_SAMPLE_STAGE_IN_LOD,
  AGX_SHADER_TEXTURE_ACCESS_SAMPLE_2D_ARRAY,
  AGX_SHADER_TEXTURE_ACCESS_SAMPLE_STAGE_IN_ARRAY,
  AGX_SHADER_TEXTURE_ACCESS_COUNT
} Agx_Shader_Texture_Access;

typedef enum Agx_Shader_Access_Width
{
  AGX_SHADER_ACCESS_WIDTH_32 = 0,
  AGX_SHADER_ACCESS_WIDTH_16,
  AGX_SHADER_ACCESS_WIDTH_8,
  AGX_SHADER_ACCESS_WIDTH_COUNT
} Agx_Shader_Access_Width;

typedef enum Agx_Shader_Channel_Width
{
  AGX_SHADER_CHANNEL_WIDTH_8 = 8,
  AGX_SHADER_CHANNEL_WIDTH_16 = 16,
  AGX_SHADER_CHANNEL_WIDTH_32 = 32,
} Agx_Shader_Channel_Width;

typedef enum Agx_Shader_Atomic_Op
{
  AGX_SHADER_ATOMIC_OP_ADD = 0x60,
  AGX_SHADER_ATOMIC_OP_AND = 0x62,
  AGX_SHADER_ATOMIC_OP_SIGNED_MAX = 0x68,
  AGX_SHADER_ATOMIC_OP_SIGNED_MIN = 0x6a,
  AGX_SHADER_ATOMIC_OP_OR = 0x6c,
  AGX_SHADER_ATOMIC_OP_SUBTRACT = 0x76,
  AGX_SHADER_ATOMIC_OP_UNSIGNED_MAX = 0x78,
  AGX_SHADER_ATOMIC_OP_UNSIGNED_MIN = 0x7a,
  AGX_SHADER_ATOMIC_OP_EXCHANGE = 0x7c,
  AGX_SHADER_ATOMIC_OP_XOR = 0x7e,
  AGX_SHADER_ATOMIC_OP_COMPARE_EXCHANGE = 0x64,
  AGX_SHADER_ATOMIC_OP_COUNT = 11
} Agx_Shader_Atomic_Op;

typedef enum Agx_Shader_Operand_Role
{
  AGX_SHADER_OPERAND_ROLE_DEFINE,
  AGX_SHADER_OPERAND_ROLE_USE
} Agx_Shader_Operand_Role;

typedef struct Agx_Shader_Instruction
{
  Agx_Shader_Op                  op;
  uint8_t                        destination;
  uint8_t                        source;
  uint8_t                        source_b;
  uint8_t                        base;
  uint8_t                        index;
  Agx_Shader_Load_Form           load_form;
  bool                           index_is_pair;
  uint8_t                        index_scale;
  bool                           destination_half;
  uint8_t                        first_load;
  bool                           base_discard;
  uint8_t                        tail;
  uint8_t                        word_count;
  uint8_t                        slot;
  uint32_t                       offset;
  uint32_t                       immediate;
  Agx_Shader_Special             special;
  uint8_t                        shift_amount;
  bool                           shift_amount_is_register;
  uint8_t                        end_thread_operand;
  uint8_t                        tile_destination;
  bool                           long_form;
  uint32_t                       pool_offset;
  uint64_t                       program_address;
  uint8_t                        raw[AGX_SHADER_RAW_MAX];
  uint32_t                       raw_length;
  Agx_Shader_Convert_Form        convert_form;
  Agx_Shader_Numeric_Format      numeric_format;
  uint8_t                        packed_sources[4];
  Agx_Shader_Packed_Format       packed_format;
  bool                           packed_alpha_immediate;
  uint8_t                        packed_alpha;
  bool                           packed_sources_are_half;
  uint8_t                        convert_channel_count;
  uint16_t                       wait_operand;
  bool                           first_converter;
  float                          pack_first;
  float                          pack_second;
  uint8_t                        output_index;
  uint8_t                        position_component;
  uint32_t                       vertex_stride;
  Agx_Shader_Index_Form          index_form;
  bool                           second_is_register;
  uint8_t                        landing_site;
  bool                           discard_pending;
  uint8_t                        store_source;
  bool                           store_waits;
  bool                           store_discards_index;
  bool                           store_index_prescaled;
  bool                           invert;
  bool                           derivative_vertical;
  bool                           derivative_absolute;
  bool                           derivative_last;
  bool                           kill_all;
  bool                           float_arms;
  bool                           compare_discards_second;
  bool                           compare_source_is_half;
  bool                           compare_second_is_half;
  uint8_t                        compare_immediate;
  uint8_t                        compare_second;
  Agx_Shader_Compare             compare;
  uint8_t                        nest;
  uint32_t                       skip_instructions;
  uint32_t                       jump_distance;
  bool                           jump_distance_given;
  uint64_t                       jump_target;
  bool                           source_signed;
  bool                           source_is_word;
  bool                           destination_is_word;
  bool                           destination_suppressed;
  Agx_Shader_Index_Source_Form   index_source_form;
  bool                           first_in_program;
  bool                           signed_index;
  uint8_t                        varying_slot;
  Agx_Shader_Varying_Form        varying_form;
  uint8_t                        lod;
  uint8_t                        wait_slot;
  uint8_t                        source_c;
  float                          third_immediate;
  Agx_Shader_Operand_Form        third_form;
  bool                           negate_third;
  bool                           negate_product;
  Agx_Shader_Source_Form         source_form;
  bool                           result_exported;
  bool                           discard_source;
  bool                           reaches_export;
  bool                           half_precision;
  Agx_Shader_Bit_Unary           bit_unary;
  Agx_Shader_Memory_Scope        memory_scope;
  bool                           source_low_half;
  bool                           source_b_low_half;
  bool                           source_is_uniform;
  uint8_t                        texture_index;
  uint8_t                        sampler_index;
  bool                           texture_depth;
  uint8_t                        true_value;
  uint8_t                        false_value;
  bool                           source_b_half;
  bool                           minimum;
  Agx_Shader_Min_Max_Type        min_max_type;
  Agx_Shader_Float_Unary         float_unary;
  int64_t                        back_distance;
  bool                           jump_forward;
  Agx_Shader_Mask_Source         mask_source;
  bool                           source_half;
  bool                           destination_high_half;
  bool                           negate_source;
  bool                           negate_source_b;
  Agx_Shader_Operand_Form        second_form;
  float                          second_immediate;
  bool                           accumulate;
  bool                           result_wait;
  bool                           discard_source_b;
  bool                           wait;
  uint8_t                        store_selector;
  bool                           store_wide;
  uint32_t                       raw_bits;
  Agx_Shader_Texture_Access      texture_access;
  uint8_t                        texture_layer;
  uint8_t                        sample_coordinate_slot;
  uint8_t                        perspective_register;
  bool                           release_perspective;
  uint8_t                        export_slot;
  uint8_t                        export_varying;
  uint8_t                        truth_table;
  bool                           keep_source_b;
  bool                           source_immediate;
  bool                           source_b_immediate;
  bool                           add_second_source_is_register;
  bool                           shift_left;
  bool                           shift_arithmetic;
  uint8_t                        shift_source_mask_bits;
  Agx_Shader_Compare_Type        compare_type;
  bool                           absolute_source;
  bool                           absolute_source_b;
  bool                           absolute_third;
  uint8_t                        sample_channels_minus_one;
  uint32_t                       move_raw;
  uint8_t                        index_shift;
  bool                           subtract;
  bool                           select_register_arms;
  bool                           negate_true_value;
  bool                           negate_false_value;
  uint64_t                       convert_bytes;
  uint16_t                       convert_destination_type;
  Agx_Shader_Convert_Source_Form convert_source_form;
  uint8_t                        color_attachment;
  uint8_t                        store_register_count;
  uint8_t                        implicit_base;
  uint8_t                        atomic_operation;
  bool                           atomic_writes_destination;
  uint8_t                        atomic_result_slot;
  bool                           index_second_operand_is_uniform;
  bool                           source_is_16_bit;
  bool                           source_sign_extends;
  uint8_t                        access_width;
  bool                           pop_short_form;
  bool                           move_writes_zero;
  bool                           index_scaled;
  bool                           move_keeps_source;
  bool                           move_byte_1_bit_7;
  uint8_t                        output_move_byte_4;
  uint8_t                        move_byte_3;
  bool                           scale_index_reverse;
  uint8_t                        scale_index_addend;
  bool                           scale_index_add_register;
  bool                           scale_index_short_form;
  bool                           scale_index_source_is_register;
  bool                           scale_index_width_32;
  uint8_t                        load_destination;
  bool                           load_byte_7_bit_7;
  uint8_t                        store_write_mask;
  uint8_t                        store_channel_width;
  uint8_t                        convert_from_width_byte;
  uint8_t                        convert_from_format_byte;
  bool                           convert_upper_pair;
} Agx_Shader_Instruction;

typedef struct Agx_Shader_Builder
{
  Agx_Shader_Instruction* instructions;
  uint32_t                count;
  uint32_t                capacity;
  uint8_t                 wait_scratch;
  bool                    has_wait_scratch;
  bool                    overflowed;
} Agx_Shader_Builder;

void
agx_shader_builder_init(Agx_Shader_Builder* builder, Agx_Shader_Instruction* storage, uint32_t capacity);

typedef struct Agx_Shader_Operands
{
  uint8_t*                slot[4];
  Agx_Shader_Operand_Role role[4];
  uint8_t                 width[4];
  uint8_t                 scale[4];
  uint32_t                count;
  bool                    modelled;
} Agx_Shader_Operands;

typedef struct Agx_Shader_Spill_Area
{
  uint8_t  base;
  uint8_t  index;
  uint32_t offset;
  uint8_t  slot_count;
} Agx_Shader_Spill_Area;

Agx_Shader_Instruction*
agx_shader_nop(Agx_Shader_Builder* builder);
Agx_Shader_Instruction*
agx_shader_stop(Agx_Shader_Builder* builder);
Agx_Shader_Instruction*
agx_shader_move_immediate(Agx_Shader_Builder* builder, uint8_t destination, uint32_t immediate);
Agx_Shader_Instruction*
agx_shader_move(Agx_Shader_Builder* builder, uint8_t destination, uint8_t source);

Agx_Shader_Instruction*
agx_shader_move_zero(Agx_Shader_Builder* builder, uint8_t destination);
Agx_Shader_Instruction*
agx_shader_discard(Agx_Shader_Builder* builder, uint8_t reg);

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
);

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
);

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
);

Agx_Shader_Instruction*
agx_shader_get_special(Agx_Shader_Builder* builder, uint8_t destination, Agx_Shader_Special special);
Agx_Shader_Instruction*
agx_shader_call_pool_shader(Agx_Shader_Builder* builder, uint32_t pool_offset, uint64_t program_address);
Agx_Shader_Instruction*
agx_shader_move_to_tile(Agx_Shader_Builder* builder, uint8_t destination, uint8_t source);
Agx_Shader_Instruction*
agx_shader_store_tile_pixel(Agx_Shader_Builder* builder, uint8_t data_register, uint8_t tile_destination);
Agx_Shader_Instruction*
agx_shader_end_thread(Agx_Shader_Builder* builder, uint8_t operand);

Agx_Shader_Instruction*
agx_shader_end_thread_long(Agx_Shader_Builder* builder, uint8_t operand);

Agx_Shader_Instruction*
agx_shader_shift_left(Agx_Shader_Builder* builder, uint8_t destination, uint8_t source, uint8_t amount);

Agx_Shader_Instruction*
agx_shader_shift_right(Agx_Shader_Builder* builder, uint8_t destination, uint8_t source, uint8_t amount);

Agx_Shader_Instruction*
agx_shader_shift_right_arithmetic(Agx_Shader_Builder* builder, uint8_t destination, uint8_t source, uint8_t amount);

Agx_Shader_Instruction*
agx_shader_convert_to_surface(
  Agx_Shader_Builder*       builder,
  uint8_t                   destination,
  bool                      first,
  Agx_Shader_Convert_Form   form,
  uint8_t                   source_a,
  uint8_t                   source_b,
  Agx_Shader_Numeric_Format format
);

Agx_Shader_Instruction*
agx_shader_pack_texel(
  Agx_Shader_Builder*      builder,
  uint8_t                  destination,
  const uint8_t            sources[4],
  bool                     sources_are_half,
  bool                     alpha_immediate,
  uint8_t                  alpha,
  Agx_Shader_Packed_Format format
);

Agx_Shader_Instruction*
agx_shader_store_surface(
  Agx_Shader_Builder*      builder,
  uint8_t                  channel_count,
  Agx_Shader_Channel_Width channel_width,
  uint8_t                  color_attachment
);

bool
agx_shader_writes_coverage_mask(const Agx_Shader_Builder* builder);

uint8_t
agx_shader_store_write_mask_for_channels(uint8_t channel_count);

bool
agx_shader_store_surface_set_write_mask(Agx_Shader_Instruction* instruction, uint8_t write_mask);

Agx_Shader_Instruction*
agx_shader_load_surface(
  Agx_Shader_Builder*      builder,
  uint8_t                  channel_count,
  Agx_Shader_Channel_Width channel_width,
  uint8_t                  color_attachment,
  uint8_t                  destination
);

Agx_Shader_Instruction*
agx_shader_convert_from_surface(
  Agx_Shader_Builder*       builder,
  uint8_t                   destination,
  bool                      first,
  uint8_t                   source,
  bool                      upper_pair,
  uint8_t                   channel_count,
  Agx_Shader_Numeric_Format format
);

Agx_Shader_Instruction*
agx_shader_pack_constant(Agx_Shader_Builder* builder, uint8_t destination, float first, float second);

bool
agx_shader_pack_constant_representable(float first, float second);

Agx_Shader_Instruction*
agx_shader_output_move(Agx_Shader_Builder* builder, uint8_t output_index, uint8_t source, bool wait);

Agx_Shader_Instruction*
agx_shader_vertex_export(Agx_Shader_Builder* builder, uint8_t output_index, uint8_t source, uint8_t position_component);

Agx_Shader_Instruction*
agx_shader_varying_read(Agx_Shader_Builder* builder, uint8_t destination, uint8_t varying_slot, Agx_Shader_Varying_Form form);

Agx_Shader_Instruction*
agx_shader_reciprocal(Agx_Shader_Builder* builder, uint8_t destination, uint8_t source, uint8_t unknown_byte6);

Agx_Shader_Instruction*
agx_shader_multiply(
  Agx_Shader_Builder* builder,
  uint8_t             destination,
  uint8_t             source_a,
  uint8_t             source_b,
  uint8_t             wait_slot,
  bool                discard_source_b
);

bool
agx_shader_minifloat_is_exact(float value);

Agx_Shader_Instruction*
agx_shader_top_bit(Agx_Shader_Builder* builder, uint8_t destination, uint8_t source, bool discard_source);

Agx_Shader_Instruction*
agx_shader_product(Agx_Shader_Builder* builder, uint8_t destination, uint8_t source, uint8_t source_b);

Agx_Shader_Instruction*
agx_shader_sum(Agx_Shader_Builder* builder, uint8_t destination, uint8_t source, uint8_t source_b);

Agx_Shader_Instruction*
agx_shader_narrow(Agx_Shader_Builder* builder, uint8_t destination, uint8_t source);

Agx_Shader_Instruction*
agx_shader_bit_unary(Agx_Shader_Builder* builder, uint8_t destination, uint8_t source, Agx_Shader_Bit_Unary which);

Agx_Shader_Instruction*
agx_shader_widen(Agx_Shader_Builder* builder, uint8_t destination, uint8_t source);

Agx_Shader_Instruction*
agx_shader_branch(Agx_Shader_Builder* builder, uint8_t landing_site);

Agx_Shader_Instruction*
agx_shader_atomic(Agx_Shader_Builder* builder, Agx_Shader_Atomic_Op operation);

Agx_Shader_Instruction*
agx_shader_atomic_returning(Agx_Shader_Builder* builder, Agx_Shader_Atomic_Op operation);

Agx_Shader_Instruction*
agx_shader_atomic_result(Agx_Shader_Builder* builder, uint8_t destination);

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
);

Agx_Shader_Instruction*
agx_shader_compare_immediate(Agx_Shader_Builder* builder, uint8_t source, Agx_Shader_Compare compare, uint8_t immediate);

Agx_Shader_Instruction*
agx_shader_compare_register(Agx_Shader_Builder* builder, uint8_t source, Agx_Shader_Compare compare, uint8_t second);

Agx_Shader_Instruction*
agx_shader_push_exec(Agx_Shader_Builder* builder, bool invert);

Agx_Shader_Instruction*
agx_shader_pop_exec(Agx_Shader_Builder* builder, uint8_t nest);

Agx_Shader_Instruction*
agx_shader_pop_exec_short(Agx_Shader_Builder* builder, uint8_t byte_3);

Agx_Shader_Instruction*
agx_shader_barrier(Agx_Shader_Builder* builder, Agx_Shader_Memory_Scope scope);

Agx_Shader_Instruction*
agx_shader_jump_exec_any(Agx_Shader_Builder* builder, uint32_t instructions);

Agx_Shader_Instruction*
agx_shader_jump_exec_none_bytes(Agx_Shader_Builder* builder, uint32_t distance);

Agx_Shader_Instruction*
agx_shader_jump_exec_none_to_pop(Agx_Shader_Builder* builder);

Agx_Shader_Instruction*
agx_shader_jump_absolute(Agx_Shader_Builder* builder, uint64_t target);

Agx_Shader_Instruction*
agx_shader_move_nibble_3(Agx_Shader_Builder* builder, uint8_t destination, uint8_t source);

Agx_Shader_Instruction*
agx_shader_half_compare_select(Agx_Shader_Builder* builder, uint8_t destination, uint8_t source);

Agx_Shader_Instruction*
agx_shader_convert_to_integer(Agx_Shader_Builder* builder, uint8_t destination, uint8_t source, bool is_signed);

Agx_Shader_Instruction*
agx_shader_convert_to_float(Agx_Shader_Builder* builder, uint8_t destination, uint8_t source);

Agx_Shader_Instruction*
agx_shader_subtract_registers(
  Agx_Shader_Builder*          builder,
  uint8_t                      destination,
  uint8_t                      minuend,
  uint8_t                      subtrahend,
  Agx_Shader_Index_Source_Form source_form
);

Agx_Shader_Instruction*
agx_shader_add_registers(
  Agx_Shader_Builder*          builder,
  uint8_t                      destination,
  uint8_t                      source,
  uint8_t                      source_b,
  Agx_Shader_Index_Source_Form source_form
);

Agx_Shader_Instruction*
agx_shader_add_index(
  Agx_Shader_Builder*          builder,
  uint8_t                      destination,
  uint8_t                      source,
  uint8_t                      immediate,
  Agx_Shader_Index_Source_Form source_form,
  bool                         first_in_program
);

Agx_Shader_Instruction*
agx_shader_extend_16(Agx_Shader_Builder* builder, uint8_t destination, uint8_t source, bool sign_extends);

Agx_Shader_Instruction*
agx_shader_multiply_add(Agx_Shader_Builder* builder, uint8_t destination, uint8_t source_a, uint8_t source_b, uint8_t source_c);

Agx_Shader_Instruction*
agx_shader_move_for_sample(Agx_Shader_Builder* builder, uint8_t destination, uint8_t source);

Agx_Shader_Instruction*
agx_shader_derivative(Agx_Shader_Builder* builder, uint8_t destination, uint8_t source, bool vertical, bool absolute_result, bool last);

Agx_Shader_Instruction*
agx_shader_bit_op(Agx_Shader_Builder* builder, uint8_t destination, uint8_t source, uint8_t source_b, uint8_t table);

Agx_Shader_Instruction*
agx_shader_wait_for_store(Agx_Shader_Builder* builder, bool active);

Agx_Shader_Instruction*
agx_shader_multiply_integer(
  Agx_Shader_Builder*          builder,
  uint8_t                      destination,
  uint8_t                      source,
  uint8_t                      source_b,
  Agx_Shader_Index_Source_Form source_form
);

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
);

Agx_Shader_Instruction*
agx_shader_texture_sample(Agx_Shader_Builder* builder, uint8_t channel_count, uint8_t lod);

uint8_t
agx_shader_operand_register(const Agx_Shader_Operands* operands, uint32_t which);

bool
agx_shader_register_is_sampled(const Agx_Shader_Builder* builder, uint8_t reg);

void
agx_shader_operand_set_register(const Agx_Shader_Operands* operands, uint32_t which, uint8_t reg);

bool
agx_shader_operands(Agx_Shader_Instruction* instruction, Agx_Shader_Operands* out);

bool
agx_shader_allocate_registers(Agx_Shader_Builder* builder, uint8_t first, uint8_t count, const Agx_Shader_Spill_Area* area);

bool
agx_shader_schedule_waits(Agx_Shader_Builder* builder);

bool
agx_shader_release_dead_operands(Agx_Shader_Builder* builder);

bool
agx_shader_op_can_wait(Agx_Shader_Op op);

Agx_Shader_Instruction*
agx_shader_min_max(
  Agx_Shader_Builder*     builder,
  uint8_t                 destination,
  uint8_t                 source,
  uint8_t                 source_b,
  bool                    minimum,
  Agx_Shader_Min_Max_Type type
);

Agx_Shader_Instruction*
agx_shader_float_unary(Agx_Shader_Builder* builder, uint8_t destination, uint8_t source, Agx_Shader_Float_Unary which);

bool
agx_shader_float_immediate(float value, uint8_t* out);

Agx_Shader_Instruction*
agx_shader_compare_select(
  Agx_Shader_Builder* builder,
  uint8_t             destination,
  uint8_t             source,
  Agx_Shader_Compare  compare,
  uint8_t             immediate,
  uint8_t             true_value,
  uint8_t             false_value
);

Agx_Shader_Instruction*
agx_shader_sample_mask(Agx_Shader_Builder* builder, uint8_t source, uint8_t slot, bool kill_all);

Agx_Shader_Instruction*
agx_shader_pad(Agx_Shader_Builder* builder, uint32_t length);

Agx_Shader_Instruction*
agx_shader_raw(Agx_Shader_Builder* builder, const uint8_t* bytes, uint32_t length);

uint32_t
agx_shader_instruction_length(const Agx_Shader_Instruction* instruction);

uint32_t
agx_shader_encode(const Agx_Shader_Builder* builder, uint8_t* bytes, uint32_t capacity);

bool
agx_shader_builder_is_spellable(const Agx_Shader_Builder* builder, uint32_t* first_bad);
