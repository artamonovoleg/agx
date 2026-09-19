#pragma once

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

static inline uint32_t
agx_float_bits(float f)
{
  uint32_t u = 0;
  memcpy(&u, &f, 4);
  return u;
}

static inline float
agx_bits_float(uint32_t u)
{
  float f = 0;
  memcpy(&f, &u, 4);
  return f;
}

#define agx_align_pot(v, pot) (((v) + ((pot) - 1)) & ~((pot) - 1))

static inline uint64_t
agx_cmd_uint(uint64_t v, uint32_t start, uint32_t end)
{
#ifndef NDEBUG
  const uint32_t width = end - start + 1;
  if (width < 64)
  {
    const uint64_t max = (1ull << width) - 1;
    assert(v <= max);
  }
#endif

  return v << start;
}

static inline uint32_t
agx_cmd_sint(int32_t v, uint32_t start, uint32_t end)
{
#ifndef NDEBUG
  const uint32_t width = end - start + 1;
  if (width < 64)
  {
    const int64_t max = (1ll << (width - 1)) - 1;
    const int64_t min = -(1ll << (width - 1));
    assert(min <= v && v <= max);
  }
#endif

  return (((uint32_t)v) << start) & ((2ll << end) - 1);
}

static inline uint64_t
agx_unpack_uint(const uint8_t* restrict cl, uint32_t start, uint32_t end)
{
  const uint32_t width = end - start + 1;
  const uint64_t mask = (width == 64 ? ~0ull : (1ull << width) - 1);
  uint64_t       val = 0;

  for (uint32_t byte = start / 8; byte <= end / 8; byte++)
  {
    val |= ((uint64_t)cl[byte]) << ((byte - start / 8) * 8);
  }

  return (val >> (start % 8)) & mask;
}

static inline int64_t
agx_unpack_sint(const uint8_t* restrict cl, uint32_t start, uint32_t end)
{
  const uint32_t width = end - start + 1;
  const int64_t  val = (int64_t)agx_unpack_uint(cl, start, end);

  return (val << (64 - width)) >> (64 - width);
}

static inline float
agx_unpack_float(const uint8_t* restrict cl, uint32_t start, uint32_t end)
{
  return agx_bits_float((uint32_t)agx_unpack_uint(cl, start, end));
}

#define agx_cmd(dst, T, name)                                                                                    \
  for (__typeof__(agx_##T##_default()) name = agx_##T##_default(), *_agx_cmd_once = (void*)&name; _agx_cmd_once; \
       (agx_##T##_pack((uint32_t*)(dst), &name), _agx_cmd_once = NULL))

#define agx_push(ptr, T, name)                                                                                     \
  for (__typeof__(agx_##T##_default()) name = agx_##T##_default(), *_agx_push_once = (void*)&name; _agx_push_once; \
       ((ptr) = (void*)((uintptr_t)(ptr) + agx_##T##_pack((uint32_t*)(ptr), &name)), _agx_push_once = NULL))

#define agx_unpack(src, T, name)        \
  __typeof__(agx_##T##_default()) name; \
  agx_##T##_unpack((const uint8_t*)(src), &name)

typedef enum Agx_Depth_Format
{
  AGX_DEPTH_FORMAT_FLOAT32 = 0,
  AGX_DEPTH_FORMAT_UNORM16 = 4,
} Agx_Depth_Format;

typedef enum Agx_Image_Channels
{
  AGX_IMAGE_CHANNELS_R8 = 0,
  AGX_IMAGE_CHANNELS_R16 = 9,
  AGX_IMAGE_CHANNELS_R8G8 = 10,
  AGX_IMAGE_CHANNELS_R5G6B5 = 11,
  AGX_IMAGE_CHANNELS_R4G4B4A4 = 12,
  AGX_IMAGE_CHANNELS_A1R5G5B5 = 13,
  AGX_IMAGE_CHANNELS_R5G5B5A1 = 14,
  AGX_IMAGE_CHANNELS_R32 = 33,
  AGX_IMAGE_CHANNELS_R16G16 = 35,
  AGX_IMAGE_CHANNELS_R11G11B10 = 37,
  AGX_IMAGE_CHANNELS_R10G10B10A2 = 38,
  AGX_IMAGE_CHANNELS_R9G9B9E5 = 39,
  AGX_IMAGE_CHANNELS_R8G8B8A8 = 40,
  AGX_IMAGE_CHANNELS_R32G32 = 49,
  AGX_IMAGE_CHANNELS_R16G16B16A16 = 50,
  AGX_IMAGE_CHANNELS_R32G32B32A32 = 56,
} Agx_Image_Channels;

typedef enum Agx_Image_Layout
{
  AGX_IMAGE_LAYOUT_LINEAR = 0,
  AGX_IMAGE_LAYOUT_TWIDDLED = 1,
  AGX_IMAGE_LAYOUT_GPU = 2,
  AGX_IMAGE_LAYOUT_INTERCHANGE = 3,
} Agx_Image_Layout;

typedef enum Agx_Load_Action
{
  AGX_LOAD_ACTION_DONT_CARE = 1,
  AGX_LOAD_ACTION_CLEAR_OR_LOAD = 3,
} Agx_Load_Action;

typedef enum Agx_Image_Dimension
{
  AGX_IMAGE_DIMENSION_1D = 0,
  AGX_IMAGE_DIMENSION_1D_ARRAY = 1,
  AGX_IMAGE_DIMENSION_2D = 2,
  AGX_IMAGE_DIMENSION_2D_ARRAY = 3,
  AGX_IMAGE_DIMENSION_2D_MULTISAMPLED = 4,
  AGX_IMAGE_DIMENSION_3D = 5,
  AGX_IMAGE_DIMENSION_CUBE = 6,
  AGX_IMAGE_DIMENSION_CUBE_ARRAY = 7,
  AGX_IMAGE_DIMENSION_2D_ARRAY_MULTISAMPLED = 8,
} Agx_Image_Dimension;

typedef enum Agx_Channel
{
  AGX_CHANNEL_R = 0,
  AGX_CHANNEL_G = 1,
  AGX_CHANNEL_B = 2,
  AGX_CHANNEL_A = 3,
} Agx_Channel;

typedef enum Agx_Primitive
{
  AGX_PRIMITIVE_POINTS = 0,
  AGX_PRIMITIVE_LINES = 1,
  AGX_PRIMITIVE_LINE_STRIP = 3,
  AGX_PRIMITIVE_TRIANGLES = 6,
  AGX_PRIMITIVE_TRIANGLE_STRIP = 9,
} Agx_Primitive;

typedef struct Agx_Encoder_Operation
{
  uint32_t marker;
  uint32_t kind;
  uint32_t cursor;
  uint32_t program;
  uint32_t unk_07;
  uint32_t tag;
  uint32_t primary_x;
  uint32_t primary_y;
  uint32_t primary_z;
  uint32_t secondary_x;
  uint32_t secondary_y;
  uint32_t secondary_z;
} Agx_Encoder_Operation;

#define AGX_ENCODER_OPERATION_LENGTH 40

static inline Agx_Encoder_Operation
agx_encoder_operation_default(void)
{
  return (Agx_Encoder_Operation) {0};
}

static inline uint32_t
agx_encoder_operation_pack(uint32_t* restrict cl, const Agx_Encoder_Operation* restrict values)
{
  cl[0] = agx_cmd_uint(values->marker, 0, 31);
  cl[1] = agx_cmd_uint(values->kind, 0, 31);
  cl[2] = agx_cmd_uint(values->cursor, 0, 7) | agx_cmd_uint(values->program, 8, 23) | agx_cmd_uint(values->unk_07, 24, 31);
  cl[3] = agx_cmd_uint(values->tag, 0, 31);
  cl[4] = agx_cmd_uint(values->primary_x, 0, 31);
  cl[5] = agx_cmd_uint(values->primary_y, 0, 31);
  cl[6] = agx_cmd_uint(values->primary_z, 0, 31);
  cl[7] = agx_cmd_uint(values->secondary_x, 0, 31);
  cl[8] = agx_cmd_uint(values->secondary_y, 0, 31);
  cl[9] = agx_cmd_uint(values->secondary_z, 0, 31);

  return AGX_ENCODER_OPERATION_LENGTH;
}

static inline void
agx_encoder_operation_unpack(const uint8_t* restrict cl, Agx_Encoder_Operation* restrict values)
{
  values->marker = agx_unpack_uint(cl, 0, 31);
  values->kind = agx_unpack_uint(cl, 32, 63);
  values->cursor = agx_unpack_uint(cl, 64, 71);
  values->program = agx_unpack_uint(cl, 72, 87);
  values->unk_07 = agx_unpack_uint(cl, 88, 95);
  values->tag = agx_unpack_uint(cl, 96, 127);
  values->primary_x = agx_unpack_uint(cl, 128, 159);
  values->primary_y = agx_unpack_uint(cl, 160, 191);
  values->primary_z = agx_unpack_uint(cl, 192, 223);
  values->secondary_x = agx_unpack_uint(cl, 224, 255);
  values->secondary_y = agx_unpack_uint(cl, 256, 287);
  values->secondary_z = agx_unpack_uint(cl, 288, 319);
}

typedef struct Agx_Blit_Copy
{
  uint32_t source_width;
  uint32_t source_height;
  uint32_t tile_width;
  uint32_t tile_height;
  uint32_t destination_width;
  uint32_t destination_height;
  uint32_t unk_40;
  uint32_t unk_44;
  uint32_t unk_48;
  uint32_t unk_4c;
} Agx_Blit_Copy;

#define AGX_BLIT_COPY_LENGTH 112

static inline Agx_Blit_Copy
agx_blit_copy_default(void)
{
  return (Agx_Blit_Copy) {0};
}

static inline uint32_t
agx_blit_copy_pack(uint32_t* restrict cl, const Agx_Blit_Copy* restrict values)
{
  cl[0] = agx_cmd_uint(values->source_width, 0, 31);
  cl[1] = agx_cmd_uint(values->source_height, 0, 31);
  cl[2] = agx_cmd_uint(values->tile_width, 0, 31);
  cl[3] = agx_cmd_uint(values->tile_height, 0, 31);
  cl[4] = 0;
  cl[5] = 0;
  cl[6] = 0;
  cl[7] = 0;
  cl[8] = 0;
  cl[9] = 0;
  cl[10] = 0;
  cl[11] = 0;
  cl[12] = 0;
  cl[13] = 0;
  cl[14] = agx_cmd_uint(values->destination_width, 0, 31);
  cl[15] = agx_cmd_uint(values->destination_height, 0, 31);
  cl[16] = agx_cmd_uint(values->unk_40, 0, 31);
  cl[17] = agx_cmd_uint(values->unk_44, 0, 31);
  cl[18] = agx_cmd_uint(values->unk_48, 0, 31);
  cl[19] = agx_cmd_uint(values->unk_4c, 0, 31);
  cl[20] = 0;
  cl[21] = 0;
  cl[22] = 0;
  cl[23] = 0;
  cl[24] = 0;
  cl[25] = 0;
  cl[26] = 0;
  cl[27] = 0;

  return AGX_BLIT_COPY_LENGTH;
}

static inline void
agx_blit_copy_unpack(const uint8_t* restrict cl, Agx_Blit_Copy* restrict values)
{
  values->source_width = agx_unpack_uint(cl, 0, 31);
  values->source_height = agx_unpack_uint(cl, 32, 63);
  values->tile_width = agx_unpack_uint(cl, 64, 95);
  values->tile_height = agx_unpack_uint(cl, 96, 127);
  values->destination_width = agx_unpack_uint(cl, 448, 479);
  values->destination_height = agx_unpack_uint(cl, 480, 511);
  values->unk_40 = agx_unpack_uint(cl, 512, 543);
  values->unk_44 = agx_unpack_uint(cl, 544, 575);
  values->unk_48 = agx_unpack_uint(cl, 576, 607);
  values->unk_4c = agx_unpack_uint(cl, 608, 639);
}

typedef enum Agx_Record_Stage
{
  AGX_RECORD_STAGE_DISPATCH = 1,
  AGX_RECORD_STAGE_BLIT = 2,
  AGX_RECORD_STAGE_ACCELERATION_STRUCTURE = 4,
  AGX_RECORD_STAGE_VERTEX = 8,
  AGX_RECORD_STAGE_FRAGMENT = 16,
  AGX_RECORD_STAGE_TILE = 32,
  AGX_RECORD_STAGE_OBJECT = 64,
  AGX_RECORD_STAGE_MESH = 128,
  AGX_RECORD_STAGE_RESOURCE_STATE = 256,
  AGX_RECORD_STAGE_MACHINE_LEARNING = 512,
} Agx_Record_Stage;

typedef struct Agx_Render_Pass_Attachment
{
  uint32_t unk_0;
  uint32_t slot_tag;
  uint32_t block_bytes;
  uint32_t unk_c;
  uint32_t share;
  uint32_t unk_14;
  uint32_t next_slot;
} Agx_Render_Pass_Attachment;

#define AGX_RENDER_PASS_ATTACHMENT_LENGTH 24

static inline Agx_Render_Pass_Attachment
agx_render_pass_attachment_default(void)
{
  return (Agx_Render_Pass_Attachment) {0};
}

static inline uint32_t
agx_render_pass_attachment_pack(uint32_t* restrict cl, const Agx_Render_Pass_Attachment* restrict values)
{
  cl[0] = agx_cmd_uint(values->unk_0, 0, 31);
  cl[1] = agx_cmd_uint(values->slot_tag, 0, 31);
  cl[2] = agx_cmd_uint(values->block_bytes, 0, 31);
  cl[3] = agx_cmd_uint(values->unk_c, 0, 31);
  cl[4] = agx_cmd_uint(values->share, 0, 31);
  cl[5] = agx_cmd_uint(values->unk_14, 0, 15) | agx_cmd_uint(values->next_slot, 16, 31);

  return AGX_RENDER_PASS_ATTACHMENT_LENGTH;
}

static inline void
agx_render_pass_attachment_unpack(const uint8_t* restrict cl, Agx_Render_Pass_Attachment* restrict values)
{
  values->unk_0 = agx_unpack_uint(cl, 0, 31);
  values->slot_tag = agx_unpack_uint(cl, 32, 63);
  values->block_bytes = agx_unpack_uint(cl, 64, 95);
  values->unk_c = agx_unpack_uint(cl, 96, 127);
  values->share = agx_unpack_uint(cl, 128, 159);
  values->unk_14 = agx_unpack_uint(cl, 160, 175);
  values->next_slot = agx_unpack_uint(cl, 176, 191);
}

typedef struct Agx_Cmd_Header
{
  uint32_t unk_0;
  uint32_t unk_4;
  uint32_t unk_ac;
} Agx_Cmd_Header;

#define AGX_CMD_HEADER_LENGTH 176

static inline Agx_Cmd_Header
agx_cmd_header_default(void)
{
  return (Agx_Cmd_Header) {0};
}

static inline uint32_t
agx_cmd_header_pack(uint32_t* restrict cl, const Agx_Cmd_Header* restrict values)
{
  cl[0] = agx_cmd_uint(values->unk_0, 0, 31);
  cl[1] = agx_cmd_uint(values->unk_4, 0, 31);
  cl[2] = 0;
  cl[3] = 0;
  cl[4] = 0;
  cl[5] = 0;
  cl[6] = 0;
  cl[7] = 0;
  cl[8] = 0;
  cl[9] = 0;
  cl[10] = 0;
  cl[11] = 0;
  cl[12] = 0;
  cl[13] = 0;
  cl[14] = 0;
  cl[15] = 0;
  cl[16] = 0;
  cl[17] = 0;
  cl[18] = 0;
  cl[19] = 0;
  cl[20] = 0;
  cl[21] = 0;
  cl[22] = 0;
  cl[23] = 0;
  cl[24] = 0;
  cl[25] = 0;
  cl[26] = 0;
  cl[27] = 0;
  cl[28] = 0;
  cl[29] = 0;
  cl[30] = 0;
  cl[31] = 0;
  cl[32] = 0;
  cl[33] = 0;
  cl[34] = 0;
  cl[35] = 0;
  cl[36] = 0;
  cl[37] = 0;
  cl[38] = 0;
  cl[39] = 0;
  cl[40] = 0;
  cl[41] = 0;
  cl[42] = 0;
  cl[43] = agx_cmd_uint(values->unk_ac, 0, 31);

  return AGX_CMD_HEADER_LENGTH;
}

static inline void
agx_cmd_header_unpack(const uint8_t* restrict cl, Agx_Cmd_Header* restrict values)
{
  values->unk_0 = agx_unpack_uint(cl, 0, 31);
  values->unk_4 = agx_unpack_uint(cl, 32, 63);
  values->unk_ac = agx_unpack_uint(cl, 1376, 1407);
}

typedef struct Agx_Copy_Record
{
  uint32_t         size;
  Agx_Record_Stage stages;
  Agx_Record_Stage unk_bc;
  uint32_t         inline_command_bytes;
  uint32_t         source_address_bits_15_to_23;
  uint32_t         run_token;
  uint32_t         unk_84;
  uint32_t         unk_a0;
  uint32_t         unk_a4;
  Agx_Record_Stage barrier_consumer_after_stages;
  Agx_Record_Stage barrier_consumer_before_stages;
  Agx_Record_Stage barrier_producer_after_stages;
  Agx_Record_Stage barrier_producer_before_stages;
  uint64_t         pointer_c4;
  uint64_t         pointer_11c;
  uint64_t         pointer_134;
  uint64_t         pointer_13c;
  uint64_t         pointer_144;
  uint64_t         pointer_14c;
  uint64_t         pointer_15c;
  uint32_t         source_address_bits_32_to_63;
  uint32_t         unk_154;
  uint32_t         unk_164;
  uint32_t         unk_168;
  uint32_t         unk_190;
  uint32_t         unk_214;
  uint32_t         unk_21c;
  uint32_t         unk_224;
  uint32_t         unk_2f8;
  uint32_t         unk_30c;
  uint32_t         unk_318;
  uint32_t         unk_31c;
  uint32_t         unk_320;
  uint32_t         unk_32c;
  bool             encoder_follows;
  uint32_t         unk_32e_hi;
} Agx_Copy_Record;

#define AGX_COPY_RECORD_LENGTH 816

static inline Agx_Copy_Record
agx_copy_record_default(void)
{
  return (Agx_Copy_Record) {0};
}

static inline uint32_t
agx_copy_record_pack(uint32_t* restrict cl, const Agx_Copy_Record* restrict values)
{
  cl[0] = agx_cmd_uint(values->size, 0, 15);
  cl[1] = 0;
  cl[2] = 0;
  cl[3] = 0;
  cl[4] = 0;
  cl[5] = 0;
  cl[6] = 0;
  cl[7] = 0;
  cl[8] = 0;
  cl[9] = 0;
  cl[10] = 0;
  cl[11] = 0;
  cl[12] = 0;
  cl[13] = 0;
  cl[14] = 0;
  cl[15] = 0;
  cl[16] = 0;
  cl[17] = 0;
  cl[18] = 0;
  cl[19] = 0;
  cl[20] = 0;
  cl[21] = 0;
  cl[22] = 0;
  cl[23] = 0;
  cl[24] = 0;
  cl[25] = 0;
  cl[26] = 0;
  cl[27] = 0;
  cl[28] = 0;
  cl[29] = 0;
  cl[30] = 0;
  cl[31] = 0;
  cl[32] = 0;
  cl[33] = agx_cmd_uint(values->unk_84, 0, 31);
  cl[34] = 0;
  cl[35] = 0;
  cl[36] = 0;
  cl[37] = 0;
  cl[38] = 0;
  cl[39] = 0;
  cl[40] = agx_cmd_uint(values->unk_a0, 0, 31);
  cl[41] = agx_cmd_uint(values->unk_a4, 0, 31);
  cl[42] = agx_cmd_uint(values->barrier_consumer_after_stages, 0, 31);
  cl[43] = agx_cmd_uint(values->barrier_consumer_before_stages, 0, 31);
  cl[44] = agx_cmd_uint(values->barrier_producer_after_stages, 0, 31);
  cl[45] = agx_cmd_uint(values->barrier_producer_before_stages, 0, 31);
  cl[46] = agx_cmd_uint(values->stages, 0, 31);
  cl[47] = agx_cmd_uint(values->unk_bc, 0, 31);
  cl[48] = 0;
  cl[49] = agx_cmd_uint(values->pointer_c4, 0, 63);
  cl[50] = agx_cmd_uint(values->pointer_c4, 0, 63) >> 32;
  cl[51] = 0;
  cl[52] = 0;
  cl[53] = 0;
  cl[54] = 0;
  cl[55] = 0;
  cl[56] = 0;
  cl[57] = 0;
  cl[58] = 0;
  cl[59] = 0;
  cl[60] = 0;
  cl[61] = 0;
  cl[62] = 0;
  cl[63] = 0;
  cl[64] = 0;
  cl[65] = 0;
  cl[66] = 0;
  cl[67] = 0;
  cl[68] = 0;
  cl[69] = 0;
  cl[70] = 0;
  cl[71] = agx_cmd_uint(values->pointer_11c, 0, 63);
  cl[72] = agx_cmd_uint(values->pointer_11c, 0, 63) >> 32;
  cl[73] = agx_cmd_uint(values->inline_command_bytes, 0, 14) | agx_cmd_uint(values->source_address_bits_15_to_23, 15, 23);
  cl[74] = agx_cmd_uint(values->source_address_bits_32_to_63, 0, 31);
  cl[75] = 0;
  cl[76] = 0;
  cl[77] = agx_cmd_uint(values->pointer_134, 0, 63);
  cl[78] = agx_cmd_uint(values->pointer_134, 0, 63) >> 32;
  cl[79] = agx_cmd_uint(values->pointer_13c, 0, 63);
  cl[80] = agx_cmd_uint(values->pointer_13c, 0, 63) >> 32;
  cl[81] = agx_cmd_uint(values->pointer_144, 0, 63);
  cl[82] = agx_cmd_uint(values->pointer_144, 0, 63) >> 32;
  cl[83] = agx_cmd_uint(values->pointer_14c, 0, 63);
  cl[84] = agx_cmd_uint(values->pointer_14c, 0, 63) >> 32;
  cl[85] = agx_cmd_uint(values->unk_154, 0, 31);
  cl[86] = 0;
  cl[87] = agx_cmd_uint(values->pointer_15c, 0, 63);
  cl[88] = agx_cmd_uint(values->pointer_15c, 0, 63) >> 32;
  cl[89] = agx_cmd_uint(values->unk_164, 0, 31);
  cl[90] = agx_cmd_uint(values->unk_168, 0, 31);
  cl[91] = 0;
  cl[92] = 0;
  cl[93] = 0;
  cl[94] = 0;
  cl[95] = 0;
  cl[96] = 0;
  cl[97] = agx_cmd_uint(values->run_token, 0, 31);
  cl[98] = 0;
  cl[99] = 0;
  cl[100] = agx_cmd_uint(values->unk_190, 0, 31);
  cl[101] = 0;
  cl[102] = 0;
  cl[103] = 0;
  cl[104] = 0;
  cl[105] = 0;
  cl[106] = 0;
  cl[107] = 0;
  cl[108] = 0;
  cl[109] = 0;
  cl[110] = 0;
  cl[111] = 0;
  cl[112] = 0;
  cl[113] = 0;
  cl[114] = 0;
  cl[115] = 0;
  cl[116] = 0;
  cl[117] = 0;
  cl[118] = 0;
  cl[119] = 0;
  cl[120] = 0;
  cl[121] = 0;
  cl[122] = 0;
  cl[123] = 0;
  cl[124] = 0;
  cl[125] = 0;
  cl[126] = 0;
  cl[127] = 0;
  cl[128] = 0;
  cl[129] = 0;
  cl[130] = 0;
  cl[131] = 0;
  cl[132] = 0;
  cl[133] = agx_cmd_uint(values->unk_214, 0, 31);
  cl[134] = 0;
  cl[135] = agx_cmd_uint(values->unk_21c, 0, 31);
  cl[136] = 0;
  cl[137] = agx_cmd_uint(values->unk_224, 0, 31);
  cl[138] = 0;
  cl[139] = 0;
  cl[140] = 0;
  cl[141] = 0;
  cl[142] = 0;
  cl[143] = 0;
  cl[144] = 0;
  cl[145] = 0;
  cl[146] = 0;
  cl[147] = 0;
  cl[148] = 0;
  cl[149] = 0;
  cl[150] = 0;
  cl[151] = 0;
  cl[152] = 0;
  cl[153] = 0;
  cl[154] = 0;
  cl[155] = 0;
  cl[156] = 0;
  cl[157] = 0;
  cl[158] = 0;
  cl[159] = 0;
  cl[160] = 0;
  cl[161] = 0;
  cl[162] = 0;
  cl[163] = 0;
  cl[164] = 0;
  cl[165] = 0;
  cl[166] = 0;
  cl[167] = 0;
  cl[168] = 0;
  cl[169] = 0;
  cl[170] = 0;
  cl[171] = 0;
  cl[172] = 0;
  cl[173] = 0;
  cl[174] = 0;
  cl[175] = 0;
  cl[176] = 0;
  cl[177] = 0;
  cl[178] = 0;
  cl[179] = 0;
  cl[180] = 0;
  cl[181] = 0;
  cl[182] = 0;
  cl[183] = 0;
  cl[184] = 0;
  cl[185] = 0;
  cl[186] = 0;
  cl[187] = 0;
  cl[188] = 0;
  cl[189] = 0;
  cl[190] = agx_cmd_uint(values->unk_2f8, 0, 31);
  cl[191] = 0;
  cl[192] = 0;
  cl[193] = 0;
  cl[194] = 0;
  cl[195] = agx_cmd_uint(values->unk_30c, 0, 31);
  cl[196] = 0;
  cl[197] = 0;
  cl[198] = agx_cmd_uint(values->unk_318, 0, 31);
  cl[199] = agx_cmd_uint(values->unk_31c, 0, 31);
  cl[200] = agx_cmd_uint(values->unk_320, 0, 31);
  cl[201] = 0;
  cl[202] = 0;
  cl[203] = agx_cmd_uint(values->unk_32c, 0, 15) | agx_cmd_uint(values->encoder_follows, 16, 16) |
            agx_cmd_uint(values->unk_32e_hi, 17, 31);

  return AGX_COPY_RECORD_LENGTH;
}

static inline void
agx_copy_record_unpack(const uint8_t* restrict cl, Agx_Copy_Record* restrict values)
{
  values->size = agx_unpack_uint(cl, 0, 15);
  values->stages = (Agx_Record_Stage)agx_unpack_uint(cl, 1472, 1503);
  values->unk_bc = (Agx_Record_Stage)agx_unpack_uint(cl, 1504, 1535);
  values->inline_command_bytes = agx_unpack_uint(cl, 2336, 2350);
  values->source_address_bits_15_to_23 = agx_unpack_uint(cl, 2351, 2359);
  values->run_token = agx_unpack_uint(cl, 3104, 3135);
  values->unk_84 = agx_unpack_uint(cl, 1056, 1087);
  values->unk_a0 = agx_unpack_uint(cl, 1280, 1311);
  values->unk_a4 = agx_unpack_uint(cl, 1312, 1343);
  values->barrier_consumer_after_stages = (Agx_Record_Stage)agx_unpack_uint(cl, 1344, 1375);
  values->barrier_consumer_before_stages = (Agx_Record_Stage)agx_unpack_uint(cl, 1376, 1407);
  values->barrier_producer_after_stages = (Agx_Record_Stage)agx_unpack_uint(cl, 1408, 1439);
  values->barrier_producer_before_stages = (Agx_Record_Stage)agx_unpack_uint(cl, 1440, 1471);
  values->pointer_c4 = agx_unpack_uint(cl, 1568, 1631);
  values->pointer_11c = agx_unpack_uint(cl, 2272, 2335);
  values->pointer_134 = agx_unpack_uint(cl, 2464, 2527);
  values->pointer_13c = agx_unpack_uint(cl, 2528, 2591);
  values->pointer_144 = agx_unpack_uint(cl, 2592, 2655);
  values->pointer_14c = agx_unpack_uint(cl, 2656, 2719);
  values->pointer_15c = agx_unpack_uint(cl, 2784, 2847);
  values->source_address_bits_32_to_63 = agx_unpack_uint(cl, 2368, 2399);
  values->unk_154 = agx_unpack_uint(cl, 2720, 2751);
  values->unk_164 = agx_unpack_uint(cl, 2848, 2879);
  values->unk_168 = agx_unpack_uint(cl, 2880, 2911);
  values->unk_190 = agx_unpack_uint(cl, 3200, 3231);
  values->unk_214 = agx_unpack_uint(cl, 4256, 4287);
  values->unk_21c = agx_unpack_uint(cl, 4320, 4351);
  values->unk_224 = agx_unpack_uint(cl, 4384, 4415);
  values->unk_2f8 = agx_unpack_uint(cl, 6080, 6111);
  values->unk_30c = agx_unpack_uint(cl, 6240, 6271);
  values->unk_318 = agx_unpack_uint(cl, 6336, 6367);
  values->unk_31c = agx_unpack_uint(cl, 6368, 6399);
  values->unk_320 = agx_unpack_uint(cl, 6400, 6431);
  values->unk_32c = agx_unpack_uint(cl, 6496, 6511);
  values->encoder_follows = agx_unpack_uint(cl, 6512, 6512);
  values->unk_32e_hi = agx_unpack_uint(cl, 6513, 6527);
}

typedef struct Agx_Render_Pass
{
  uint32_t         size;
  uint32_t         attachment_bytes;
  Agx_Record_Stage stages;
  Agx_Record_Stage unk_bc;
  uint32_t         inline_command_bytes;
  bool             inline_commands_present;
  uint32_t         kind_tag;
  uint64_t         render_target_descriptors;
  uint32_t         color_load_action_mask;
  uint64_t         background_program;
  uint64_t         store_program;
  uint64_t         store_program_copy;
  uint32_t         unk_1136;
  uint64_t         scissor_buffer;
  uint64_t         depth_bias_table;
  uint32_t         depth_unk_4a4;
  uint64_t         depth_buffer;
  uint64_t         depth_meta;
  uint64_t         depth_buffer_copy;
  uint64_t         depth_meta_copy;
  uint64_t         depth_buffer_partial_render;
  uint64_t         depth_meta_partial_render;
  uint64_t         stencil_buffer;
  uint64_t         stencil_meta;
  uint64_t         stencil_buffer_copy;
  uint64_t         stencil_meta_copy;
  uint64_t         stencil_buffer_partial_render;
  uint64_t         stencil_meta_partial_render;
  uint32_t         zls_control;
  Agx_Depth_Format depth_format;
  bool             few_primitives;
  uint32_t         width;
  uint32_t         height;
  uint64_t         unknown_program;
  float            depth_clear;
  uint32_t         stencil_clear;
  Agx_Load_Action  load_action;
  bool             any_load;
  uint32_t         tile_stores;
  uint32_t         partial_load_action_mask;
  uint64_t         partial_background_program;
  uint64_t         partial_store_program;
  uint64_t         partial_store_program_copy;
  bool             tile_memory_request_copy;
  bool             clear;
  uint32_t         run_token;
  bool             clear_and_store;
  uint64_t         uniform_slice;
  uint32_t         uniform_bind_count;
  uint32_t         width_copy;
  uint32_t         height_copy;
  uint32_t         sample_count;
  uint32_t         imageblock_sample_length;
  uint32_t         tile_width;
  uint32_t         tile_height;
  uint32_t         attachment_count;
  uint32_t         attachment_address_low;
  uint32_t         unk_84;
  uint32_t         unk_a0;
  uint32_t         unk_a4;
  Agx_Record_Stage barrier_consumer_after_stages;
  Agx_Record_Stage barrier_consumer_before_stages;
  Agx_Record_Stage barrier_producer_after_stages;
  Agx_Record_Stage barrier_producer_before_stages;
  uint32_t         encoder_stream_low;
  uint64_t         encoder_stream;
  uint32_t         unk_12c;
  uint32_t         unk_17c;
  uint32_t         unk_180;
  uint32_t         unk_184;
  uint32_t         unk_206;
  uint32_t         unk_214;
  uint32_t         unk_2ed;
  uint32_t         unk_2f1;
  uint32_t         unk_2f5;
  uint32_t         unk_2f9;
  bool             tile_memory_request;
  uint32_t         unk_310;
  uint32_t         unk_314;
  uint32_t         unk_318;
  uint32_t         unk_380;
  uint32_t         unk_434;
  uint32_t         unk_494;
  uint32_t         tile_memory_blocks;
  uint32_t         tile_memory_samples;
  uint32_t         unk_555;
  uint32_t         unk_566;
  uint32_t         unk_615;
  uint32_t         unk_619;
  uint32_t         unk_61d;
  uint32_t         unk_621;
  uint32_t         unk_624;
  uint32_t         unk_71c;
  uint32_t         unk_720;
  uint32_t         unk_724;
  uint32_t         unk_770;
  uint32_t         unk_7ac;
  uint32_t         unk_8cc;
  uint32_t         unk_8e0;
  uint32_t         unk_95c;
  uint32_t         unk_974;
  uint32_t         unk_a9c;
  uint32_t         sample_position_0;
  uint32_t         sample_position_1;
  uint32_t         sample_position_2;
  uint32_t         sample_position_3;
  uint32_t         sample_position_4;
  uint32_t         sample_position_5;
  uint32_t         sample_position_6;
  uint32_t         sample_position_7;
} Agx_Render_Pass;

#define AGX_RENDER_PASS_LENGTH 2752

static inline Agx_Render_Pass
agx_render_pass_default(void)
{
  return (Agx_Render_Pass) {
    .unk_1136 = 0x100,
    .depth_unk_4a4 = 0x0,
    .depth_format = AGX_DEPTH_FORMAT_FLOAT32,
  };
}

static inline uint32_t
agx_render_pass_pack(uint32_t* restrict cl, const Agx_Render_Pass* restrict values)
{
  assert((values->render_target_descriptors & 0xf) == 0);
  assert((values->encoder_stream & 0xff) == 0);
  cl[0] = agx_cmd_uint(values->size, 0, 15);
  cl[1] = 0;
  cl[2] = 0;
  cl[3] = 0;
  cl[4] = 0;
  cl[5] = 0;
  cl[6] = 0;
  cl[7] = 0;
  cl[8] = 0;
  cl[9] = 0;
  cl[10] = 0;
  cl[11] = 0;
  cl[12] = 0;
  cl[13] = 0;
  cl[14] = 0;
  cl[15] = 0;
  cl[16] = 0;
  cl[17] = 0;
  cl[18] = 0;
  cl[19] = 0;
  cl[20] = 0;
  cl[21] = 0;
  cl[22] = 0;
  cl[23] = 0;
  cl[24] = 0;
  cl[25] = 0;
  cl[26] = 0;
  cl[27] = 0;
  cl[28] = 0;
  cl[29] = 0;
  cl[30] = 0;
  cl[31] = 0;
  cl[32] = 0;
  cl[33] = agx_cmd_uint(values->unk_84, 0, 7);
  cl[34] = 0;
  cl[35] = 0;
  cl[36] = 0;
  cl[37] = agx_cmd_uint(values->attachment_bytes, 0, 7);
  cl[38] = 0;
  cl[39] = 0;
  cl[40] = agx_cmd_uint(values->unk_a0, 0, 31);
  cl[41] = agx_cmd_uint(values->unk_a4, 0, 7);
  cl[42] = agx_cmd_uint(values->barrier_consumer_after_stages, 0, 31);
  cl[43] = agx_cmd_uint(values->barrier_consumer_before_stages, 0, 31);
  cl[44] = agx_cmd_uint(values->barrier_producer_after_stages, 0, 31);
  cl[45] = agx_cmd_uint(values->barrier_producer_before_stages, 0, 31);
  cl[46] = agx_cmd_uint(values->stages, 0, 31);
  cl[47] = agx_cmd_uint(values->unk_bc, 0, 31);
  cl[48] = 0;
  cl[49] = agx_cmd_uint(values->encoder_stream_low, 0, 7) | agx_cmd_uint(values->encoder_stream >> 8, 8, 31);
  cl[50] = 0;
  cl[51] = 0;
  cl[52] = 0;
  cl[53] = 0;
  cl[54] = 0;
  cl[55] = 0;
  cl[56] = 0;
  cl[57] = 0;
  cl[58] = 0;
  cl[59] = 0;
  cl[60] = 0;
  cl[61] = 0;
  cl[62] = 0;
  cl[63] = 0;
  cl[64] = 0;
  cl[65] = 0;
  cl[66] = 0;
  cl[67] = 0;
  cl[68] = 0;
  cl[69] = 0;
  cl[70] = 0;
  cl[71] = 0;
  cl[72] = 0;
  cl[73] = agx_cmd_uint(values->inline_command_bytes, 0, 14) | agx_cmd_uint(values->inline_commands_present, 15, 15) |
           agx_cmd_uint(values->kind_tag, 16, 23);
  cl[74] = 0;
  cl[75] = agx_cmd_uint(values->unk_12c, 0, 23);
  cl[76] = 0;
  cl[77] = 0;
  cl[78] = 0;
  cl[79] = 0;
  cl[80] = 0;
  cl[81] = 0;
  cl[82] = 0;
  cl[83] = 0;
  cl[84] = 0;
  cl[85] = 0;
  cl[86] = 0;
  cl[87] = 0;
  cl[88] = 0;
  cl[89] = 0;
  cl[90] = 0;
  cl[91] = 0;
  cl[92] = 0;
  cl[93] = 0;
  cl[94] = 0;
  cl[95] = agx_cmd_uint(values->unk_17c, 0, 31);
  cl[96] = agx_cmd_uint(values->unk_180, 0, 31);
  cl[97] = agx_cmd_uint(values->unk_184, 0, 7);
  cl[98] = 0;
  cl[99] = 0;
  cl[100] = 0;
  cl[101] = 0;
  cl[102] = 0;
  cl[103] = 0;
  cl[104] = 0;
  cl[105] = 0;
  cl[106] = 0;
  cl[107] = 0;
  cl[108] = 0;
  cl[109] = 0;
  cl[110] = 0;
  cl[111] = 0;
  cl[112] = 0;
  cl[113] = 0;
  cl[114] = 0;
  cl[115] = 0;
  cl[116] = 0;
  cl[117] = 0;
  cl[118] = 0;
  cl[119] = 0;
  cl[120] = 0;
  cl[121] = 0;
  cl[122] = 0;
  cl[123] = 0;
  cl[124] = 0;
  cl[125] = 0;
  cl[126] = 0;
  cl[127] = 0;
  cl[128] = 0;
  cl[129] = agx_cmd_uint(values->unk_206, 16, 23);
  cl[130] = 0;
  cl[131] = agx_cmd_uint(values->render_target_descriptors >> 4, 8, 39);
  cl[132] = agx_cmd_uint(values->render_target_descriptors >> 4, 8, 39) >> 32;
  cl[133] = agx_cmd_uint(values->unk_214, 0, 15);
  cl[134] = 0;
  cl[135] = 0;
  cl[136] = 0;
  cl[137] = 0;
  cl[138] = 0;
  cl[139] = 0;
  cl[140] = 0;
  cl[141] = 0;
  cl[142] = 0;
  cl[143] = 0;
  cl[144] = 0;
  cl[145] = 0;
  cl[146] = 0;
  cl[147] = 0;
  cl[148] = 0;
  cl[149] = 0;
  cl[150] = 0;
  cl[151] = 0;
  cl[152] = 0;
  cl[153] = 0;
  cl[154] = 0;
  cl[155] = 0;
  cl[156] = 0;
  cl[157] = 0;
  cl[158] = 0;
  cl[159] = 0;
  cl[160] = 0;
  cl[161] = 0;
  cl[162] = 0;
  cl[163] = 0;
  cl[164] = 0;
  cl[165] = 0;
  cl[166] = 0;
  cl[167] = 0;
  cl[168] = 0;
  cl[169] = 0;
  cl[170] = 0;
  cl[171] = 0;
  cl[172] = 0;
  cl[173] = 0;
  cl[174] = 0;
  cl[175] = 0;
  cl[176] = 0;
  cl[177] = 0;
  cl[178] = 0;
  cl[179] = 0;
  cl[180] = 0;
  cl[181] = 0;
  cl[182] = 0;
  cl[183] = 0;
  cl[184] = 0;
  cl[185] = 0;
  cl[186] = 0;
  cl[187] = agx_cmd_uint(values->unk_2ed, 8, 39);
  cl[188] = agx_cmd_uint(values->unk_2ed, 8, 39) >> 32 | agx_cmd_uint(values->unk_2f1, 8, 39);
  cl[189] = agx_cmd_uint(values->unk_2f1, 8, 39) >> 32 | agx_cmd_uint(values->unk_2f5, 8, 39);
  cl[190] = agx_cmd_uint(values->unk_2f5, 8, 39) >> 32 | agx_cmd_uint(values->unk_2f9, 8, 39);
  cl[191] = agx_cmd_uint(values->unk_2f9, 8, 39) >> 32;
  cl[192] = 0;
  cl[193] = agx_cmd_uint(values->tile_memory_request, 0, 0);
  cl[194] = 0;
  cl[195] = 0;
  cl[196] = agx_cmd_uint(values->unk_310, 0, 31);
  cl[197] = agx_cmd_uint(values->unk_314, 0, 31);
  cl[198] = agx_cmd_uint(values->unk_318, 0, 31);
  cl[199] = 0;
  cl[200] = 0;
  cl[201] = 0;
  cl[202] = 0;
  cl[203] = 0;
  cl[204] = 0;
  cl[205] = 0;
  cl[206] = 0;
  cl[207] = 0;
  cl[208] = 0;
  cl[209] = 0;
  cl[210] = 0;
  cl[211] = 0;
  cl[212] = 0;
  cl[213] = 0;
  cl[214] = 0;
  cl[215] = 0;
  cl[216] = 0;
  cl[217] = 0;
  cl[218] = 0;
  cl[219] = 0;
  cl[220] = 0;
  cl[221] = 0;
  cl[222] = 0;
  cl[223] = 0;
  cl[224] = agx_cmd_uint(values->unk_380, 0, 7);
  cl[225] = 0;
  cl[226] = 0;
  cl[227] = 0;
  cl[228] = 0;
  cl[229] = 0;
  cl[230] = 0;
  cl[231] = 0;
  cl[232] = 0;
  cl[233] = 0;
  cl[234] = 0;
  cl[235] = 0;
  cl[236] = 0;
  cl[237] = 0;
  cl[238] = 0;
  cl[239] = 0;
  cl[240] = 0;
  cl[241] = 0;
  cl[242] = 0;
  cl[243] = 0;
  cl[244] = 0;
  cl[245] = 0;
  cl[246] = 0;
  cl[247] = 0;
  cl[248] = 0;
  cl[249] = 0;
  cl[250] = 0;
  cl[251] = 0;
  cl[252] = 0;
  cl[253] = 0;
  cl[254] = 0;
  cl[255] = 0;
  cl[256] = 0;
  cl[257] = 0;
  cl[258] = 0;
  cl[259] = 0;
  cl[260] = 0;
  cl[261] = 0;
  cl[262] = 0;
  cl[263] = 0;
  cl[264] = 0;
  cl[265] = 0;
  cl[266] = 0;
  cl[267] = 0;
  cl[268] = 0;
  cl[269] = agx_cmd_uint(values->unk_434, 0, 7);
  cl[270] = agx_cmd_uint(values->color_load_action_mask, 8, 31);
  cl[271] = agx_cmd_uint(values->background_program, 0, 63);
  cl[272] = agx_cmd_uint(values->background_program, 0, 63) >> 32;
  cl[273] = 0;
  cl[274] = 0;
  cl[275] = 0;
  cl[276] = 0;
  cl[277] = 0;
  cl[278] = 0;
  cl[279] = agx_cmd_uint(values->store_program, 0, 63);
  cl[280] = agx_cmd_uint(values->store_program, 0, 63) >> 32;
  cl[281] = 0;
  cl[282] = 0;
  cl[283] = agx_cmd_uint(values->store_program_copy, 0, 63);
  cl[284] = agx_cmd_uint(values->store_program_copy, 0, 63) >> 32 | agx_cmd_uint(values->unk_1136, 0, 31);
  cl[285] = agx_cmd_uint(values->scissor_buffer, 0, 63);
  cl[286] = agx_cmd_uint(values->scissor_buffer, 0, 63) >> 32;
  cl[287] = agx_cmd_uint(values->depth_bias_table, 0, 63);
  cl[288] = agx_cmd_uint(values->depth_bias_table, 0, 63) >> 32;
  cl[289] = 0;
  cl[290] = 0;
  cl[291] = agx_cmd_uint(values->zls_control, 0, 23) | agx_cmd_uint(values->depth_format, 24, 31);
  cl[292] = 0;
  cl[293] = agx_cmd_uint(values->unk_494, 0, 31);
  cl[294] = 0;
  cl[295] = 0;
  cl[296] = 0;
  cl[297] = agx_cmd_uint(values->depth_unk_4a4, 0, 31);
  cl[298] = 0;
  cl[299] = agx_cmd_uint(values->depth_buffer, 0, 63);
  cl[300] = agx_cmd_uint(values->depth_buffer, 0, 63) >> 32;
  cl[301] = 0;
  cl[302] = 0;
  cl[303] = 0;
  cl[304] = 0;
  cl[305] = agx_cmd_uint(values->depth_meta, 0, 63);
  cl[306] = agx_cmd_uint(values->depth_meta, 0, 63) >> 32;
  cl[307] = 0;
  cl[308] = 0;
  cl[309] = agx_cmd_uint(values->depth_buffer_copy, 0, 63);
  cl[310] = agx_cmd_uint(values->depth_buffer_copy, 0, 63) >> 32;
  cl[311] = 0;
  cl[312] = 0;
  cl[313] = 0;
  cl[314] = 0;
  cl[315] = agx_cmd_uint(values->depth_meta_copy, 0, 63);
  cl[316] = agx_cmd_uint(values->depth_meta_copy, 0, 63) >> 32;
  cl[317] = 0;
  cl[318] = 0;
  cl[319] = agx_cmd_uint(values->stencil_buffer, 0, 63);
  cl[320] = agx_cmd_uint(values->stencil_buffer, 0, 63) >> 32;
  cl[321] = 0;
  cl[322] = 0;
  cl[323] = 0;
  cl[324] = 0;
  cl[325] = agx_cmd_uint(values->stencil_meta, 0, 63);
  cl[326] = agx_cmd_uint(values->stencil_meta, 0, 63) >> 32;
  cl[327] = 0;
  cl[328] = 0;
  cl[329] = agx_cmd_uint(values->stencil_buffer_copy, 0, 63);
  cl[330] = agx_cmd_uint(values->stencil_buffer_copy, 0, 63) >> 32;
  cl[331] = 0;
  cl[332] = 0;
  cl[333] = 0;
  cl[334] = 0;
  cl[335] = agx_cmd_uint(values->stencil_meta_copy, 0, 63);
  cl[336] = agx_cmd_uint(values->stencil_meta_copy, 0, 63) >> 32;
  cl[337] = 0;
  cl[338] = 0;
  cl[339] = agx_cmd_uint(values->tile_memory_blocks, 0, 7) | agx_cmd_uint(values->tile_memory_samples, 16, 23);
  cl[340] = 0;
  cl[341] = agx_cmd_uint(values->few_primitives, 0, 0) | agx_cmd_uint(values->unk_555, 8, 15);
  cl[342] = 0;
  cl[343] = agx_cmd_uint(values->width, 0, 31);
  cl[344] = agx_cmd_uint(values->height, 0, 31);
  cl[345] = agx_cmd_uint(values->unk_566, 16, 23);
  cl[346] = 0;
  cl[347] = agx_cmd_uint(values->unknown_program, 0, 63);
  cl[348] = agx_cmd_uint(values->unknown_program, 0, 63) >> 32;
  cl[349] = 0;
  cl[350] = 0;
  cl[351] = 0;
  cl[352] = 0;
  cl[353] = 0;
  cl[354] = 0;
  cl[355] = 0;
  cl[356] = 0;
  cl[357] = 0;
  cl[358] = 0;
  cl[359] = 0;
  cl[360] = 0;
  cl[361] = 0;
  cl[362] = 0;
  cl[363] = 0;
  cl[364] = 0;
  cl[365] = 0;
  cl[366] = 0;
  cl[367] = 0;
  cl[368] = 0;
  cl[369] = 0;
  cl[370] = 0;
  cl[371] = 0;
  cl[372] = 0;
  cl[373] = 0;
  cl[374] = 0;
  cl[375] = 0;
  cl[376] = 0;
  cl[377] = 0;
  cl[378] = 0;
  cl[379] = 0;
  cl[380] = 0;
  cl[381] = 0;
  cl[382] = 0;
  cl[383] = 0;
  cl[384] = 0;
  cl[385] = 0;
  cl[386] = 0;
  cl[387] = 0;
  cl[388] = 0;
  cl[389] = agx_cmd_uint(values->unk_615, 8, 39);
  cl[390] = agx_cmd_uint(values->unk_615, 8, 39) >> 32 | agx_cmd_uint(values->unk_619, 8, 39);
  cl[391] = agx_cmd_uint(values->unk_619, 8, 39) >> 32 | agx_cmd_uint(values->unk_61d, 8, 39);
  cl[392] = agx_cmd_uint(values->unk_61d, 8, 39) >> 32 | agx_cmd_uint(values->unk_621, 8, 31);
  cl[393] = agx_cmd_uint(values->unk_624, 0, 15);
  cl[394] = 0;
  cl[395] = 0;
  cl[396] = 0;
  cl[397] = 0;
  cl[398] = 0;
  cl[399] = 0;
  cl[400] = 0;
  cl[401] = 0;
  cl[402] = 0;
  cl[403] = 0;
  cl[404] = 0;
  cl[405] = 0;
  cl[406] = 0;
  cl[407] = 0;
  cl[408] = 0;
  cl[409] = 0;
  cl[410] = 0;
  cl[411] = 0;
  cl[412] = 0;
  cl[413] = 0;
  cl[414] = 0;
  cl[415] = 0;
  cl[416] = 0;
  cl[417] = 0;
  cl[418] = 0;
  cl[419] = 0;
  cl[420] = 0;
  cl[421] = 0;
  cl[422] = 0;
  cl[423] = 0;
  cl[424] = 0;
  cl[425] = 0;
  cl[426] = 0;
  cl[427] = 0;
  cl[428] = 0;
  cl[429] = 0;
  cl[430] = 0;
  cl[431] = 0;
  cl[432] = 0;
  cl[433] = 0;
  cl[434] = 0;
  cl[435] = 0;
  cl[436] = 0;
  cl[437] = 0;
  cl[438] = 0;
  cl[439] = 0;
  cl[440] = 0;
  cl[441] = 0;
  cl[442] = 0;
  cl[443] = 0;
  cl[444] = 0;
  cl[445] = 0;
  cl[446] = 0;
  cl[447] = agx_cmd_uint(agx_float_bits(values->depth_clear), 0, 32);
  cl[448] = agx_cmd_uint(values->stencil_clear, 0, 7) | agx_cmd_uint(values->load_action, 8, 15);
  cl[449] = 0;
  cl[450] = agx_cmd_uint(values->any_load, 8, 8);
  cl[451] = 0;
  cl[452] = 0;
  cl[453] = 0;
  cl[454] = agx_cmd_uint(values->tile_stores, 0, 7);
  cl[455] = agx_cmd_uint(values->unk_71c, 0, 31);
  cl[456] = agx_cmd_uint(values->unk_720, 0, 31);
  cl[457] = agx_cmd_uint(values->unk_724, 0, 31);
  cl[458] = 0;
  cl[459] = 0;
  cl[460] = 0;
  cl[461] = 0;
  cl[462] = 0;
  cl[463] = 0;
  cl[464] = 0;
  cl[465] = 0;
  cl[466] = 0;
  cl[467] = 0;
  cl[468] = 0;
  cl[469] = 0;
  cl[470] = 0;
  cl[471] = 0;
  cl[472] = 0;
  cl[473] = 0;
  cl[474] = 0;
  cl[475] = 0;
  cl[476] = agx_cmd_uint(values->unk_770, 0, 7);
  cl[477] = 0;
  cl[478] = 0;
  cl[479] = 0;
  cl[480] = 0;
  cl[481] = 0;
  cl[482] = 0;
  cl[483] = 0;
  cl[484] = 0;
  cl[485] = 0;
  cl[486] = 0;
  cl[487] = 0;
  cl[488] = 0;
  cl[489] = 0;
  cl[490] = 0;
  cl[491] = agx_cmd_uint(values->unk_7ac, 0, 7);
  cl[492] = agx_cmd_uint(values->partial_load_action_mask, 8, 31);
  cl[493] = agx_cmd_uint(values->partial_background_program, 0, 63);
  cl[494] = agx_cmd_uint(values->partial_background_program, 0, 63) >> 32;
  cl[495] = 0;
  cl[496] = 0;
  cl[497] = 0;
  cl[498] = 0;
  cl[499] = 0;
  cl[500] = 0;
  cl[501] = agx_cmd_uint(values->partial_store_program, 0, 63);
  cl[502] = agx_cmd_uint(values->partial_store_program, 0, 63) >> 32;
  cl[503] = 0;
  cl[504] = 0;
  cl[505] = agx_cmd_uint(values->partial_store_program_copy, 0, 63);
  cl[506] = agx_cmd_uint(values->partial_store_program_copy, 0, 63) >> 32;
  cl[507] = 0;
  cl[508] = 0;
  cl[509] = 0;
  cl[510] = 0;
  cl[511] = 0;
  cl[512] = 0;
  cl[513] = 0;
  cl[514] = 0;
  cl[515] = 0;
  cl[516] = 0;
  cl[517] = 0;
  cl[518] = 0;
  cl[519] = 0;
  cl[520] = 0;
  cl[521] = 0;
  cl[522] = 0;
  cl[523] = 0;
  cl[524] = 0;
  cl[525] = 0;
  cl[526] = 0;
  cl[527] = 0;
  cl[528] = 0;
  cl[529] = 0;
  cl[530] = 0;
  cl[531] = 0;
  cl[532] = 0;
  cl[533] = 0;
  cl[534] = 0;
  cl[535] = 0;
  cl[536] = 0;
  cl[537] = 0;
  cl[538] = 0;
  cl[539] = agx_cmd_uint(values->depth_buffer_partial_render, 0, 63);
  cl[540] = agx_cmd_uint(values->depth_buffer_partial_render, 0, 63) >> 32;
  cl[541] = agx_cmd_uint(values->depth_meta_partial_render, 0, 63);
  cl[542] = agx_cmd_uint(values->depth_meta_partial_render, 0, 63) >> 32;
  cl[543] = agx_cmd_uint(values->stencil_buffer_partial_render, 0, 63);
  cl[544] = agx_cmd_uint(values->stencil_buffer_partial_render, 0, 63) >> 32;
  cl[545] = agx_cmd_uint(values->stencil_meta_partial_render, 0, 63);
  cl[546] = agx_cmd_uint(values->stencil_meta_partial_render, 0, 63) >> 32;
  cl[547] = 0;
  cl[548] = 0;
  cl[549] = agx_cmd_uint(values->tile_memory_request_copy, 0, 0);
  cl[550] = 0;
  cl[551] = agx_cmd_uint(values->clear, 0, 0);
  cl[552] = 0;
  cl[553] = 0;
  cl[554] = 0;
  cl[555] = 0;
  cl[556] = 0;
  cl[557] = 0;
  cl[558] = 0;
  cl[559] = 0;
  cl[560] = 0;
  cl[561] = 0;
  cl[562] = 0;
  cl[563] = agx_cmd_uint(values->unk_8cc, 0, 7);
  cl[564] = 0;
  cl[565] = agx_cmd_uint(values->run_token, 0, 31);
  cl[566] = 0;
  cl[567] = 0;
  cl[568] = agx_cmd_uint(values->unk_8e0, 0, 31);
  cl[569] = agx_cmd_uint(values->clear_and_store, 0, 0);
  cl[570] = 0;
  cl[571] = 0;
  cl[572] = 0;
  cl[573] = agx_cmd_uint(values->uniform_slice, 0, 63);
  cl[574] = agx_cmd_uint(values->uniform_slice, 0, 63) >> 32;
  cl[575] = agx_cmd_uint(values->uniform_bind_count, 0, 31);
  cl[576] = 0;
  cl[577] = agx_cmd_uint(values->width_copy, 0, 31);
  cl[578] = agx_cmd_uint(values->height_copy, 0, 31);
  cl[579] = agx_cmd_uint(values->sample_count, 0, 31);
  cl[580] = agx_cmd_uint(values->sample_position_0, 0, 31);
  cl[581] = agx_cmd_uint(values->sample_position_1, 0, 31);
  cl[582] = agx_cmd_uint(values->sample_position_2, 0, 31);
  cl[583] = agx_cmd_uint(values->sample_position_3, 0, 31);
  cl[584] = agx_cmd_uint(values->sample_position_4, 0, 31);
  cl[585] = agx_cmd_uint(values->sample_position_5, 0, 31);
  cl[586] = agx_cmd_uint(values->sample_position_6, 0, 31);
  cl[587] = agx_cmd_uint(values->sample_position_7, 0, 31);
  cl[588] = 0;
  cl[589] = 0;
  cl[590] = 0;
  cl[591] = 0;
  cl[592] = 0;
  cl[593] = 0;
  cl[594] = 0;
  cl[595] = 0;
  cl[596] = agx_cmd_uint(values->imageblock_sample_length, 0, 31);
  cl[597] = agx_cmd_uint(values->tile_width, 0, 31);
  cl[598] = agx_cmd_uint(values->tile_height, 0, 31);
  cl[599] = agx_cmd_uint(values->unk_95c, 0, 7);
  cl[600] = 0;
  cl[601] = 0;
  cl[602] = 0;
  cl[603] = 0;
  cl[604] = 0;
  cl[605] = agx_cmd_uint(values->unk_974, 0, 7);
  cl[606] = 0;
  cl[607] = 0;
  cl[608] = 0;
  cl[609] = 0;
  cl[610] = 0;
  cl[611] = 0;
  cl[612] = 0;
  cl[613] = 0;
  cl[614] = 0;
  cl[615] = 0;
  cl[616] = 0;
  cl[617] = 0;
  cl[618] = 0;
  cl[619] = 0;
  cl[620] = 0;
  cl[621] = 0;
  cl[622] = 0;
  cl[623] = 0;
  cl[624] = 0;
  cl[625] = 0;
  cl[626] = 0;
  cl[627] = 0;
  cl[628] = 0;
  cl[629] = 0;
  cl[630] = 0;
  cl[631] = 0;
  cl[632] = 0;
  cl[633] = 0;
  cl[634] = 0;
  cl[635] = 0;
  cl[636] = 0;
  cl[637] = 0;
  cl[638] = 0;
  cl[639] = 0;
  cl[640] = 0;
  cl[641] = 0;
  cl[642] = 0;
  cl[643] = 0;
  cl[644] = 0;
  cl[645] = 0;
  cl[646] = 0;
  cl[647] = 0;
  cl[648] = 0;
  cl[649] = 0;
  cl[650] = 0;
  cl[651] = 0;
  cl[652] = 0;
  cl[653] = 0;
  cl[654] = 0;
  cl[655] = 0;
  cl[656] = 0;
  cl[657] = 0;
  cl[658] = 0;
  cl[659] = 0;
  cl[660] = 0;
  cl[661] = 0;
  cl[662] = 0;
  cl[663] = 0;
  cl[664] = 0;
  cl[665] = 0;
  cl[666] = 0;
  cl[667] = 0;
  cl[668] = 0;
  cl[669] = 0;
  cl[670] = 0;
  cl[671] = 0;
  cl[672] = 0;
  cl[673] = 0;
  cl[674] = 0;
  cl[675] = 0;
  cl[676] = 0;
  cl[677] = 0;
  cl[678] = agx_cmd_uint(values->attachment_count, 0, 31);
  cl[679] = agx_cmd_uint(values->unk_a9c, 0, 7);
  cl[680] = 0;
  cl[681] = agx_cmd_uint(values->attachment_address_low, 0, 31);
  cl[682] = 0;
  cl[683] = 0;
  cl[684] = 0;
  cl[685] = 0;
  cl[686] = 0;
  cl[687] = 0;

  return AGX_RENDER_PASS_LENGTH;
}

static inline void
agx_render_pass_unpack(const uint8_t* restrict cl, Agx_Render_Pass* restrict values)
{
  values->size = agx_unpack_uint(cl, 0, 15);
  values->attachment_bytes = agx_unpack_uint(cl, 1184, 1191);
  values->stages = (Agx_Record_Stage)agx_unpack_uint(cl, 1472, 1503);
  values->unk_bc = (Agx_Record_Stage)agx_unpack_uint(cl, 1504, 1535);
  values->inline_command_bytes = agx_unpack_uint(cl, 2336, 2350);
  values->inline_commands_present = agx_unpack_uint(cl, 2351, 2351);
  values->kind_tag = agx_unpack_uint(cl, 2352, 2359);
  values->render_target_descriptors = agx_unpack_uint(cl, 4200, 4231) << 4;
  values->color_load_action_mask = agx_unpack_uint(cl, 8648, 8671);
  values->background_program = agx_unpack_uint(cl, 8672, 8735);
  values->store_program = agx_unpack_uint(cl, 8928, 8991);
  values->store_program_copy = agx_unpack_uint(cl, 9056, 9119);
  values->unk_1136 = agx_unpack_uint(cl, 9088, 9119);
  values->scissor_buffer = agx_unpack_uint(cl, 9120, 9183);
  values->depth_bias_table = agx_unpack_uint(cl, 9184, 9247);
  values->depth_unk_4a4 = agx_unpack_uint(cl, 9504, 9535);
  values->depth_buffer = agx_unpack_uint(cl, 9568, 9631);
  values->depth_meta = agx_unpack_uint(cl, 9760, 9823);
  values->depth_buffer_copy = agx_unpack_uint(cl, 9888, 9951);
  values->depth_meta_copy = agx_unpack_uint(cl, 10080, 10143);
  values->depth_buffer_partial_render = agx_unpack_uint(cl, 17248, 17311);
  values->depth_meta_partial_render = agx_unpack_uint(cl, 17312, 17375);
  values->stencil_buffer = agx_unpack_uint(cl, 10208, 10271);
  values->stencil_meta = agx_unpack_uint(cl, 10400, 10463);
  values->stencil_buffer_copy = agx_unpack_uint(cl, 10528, 10591);
  values->stencil_meta_copy = agx_unpack_uint(cl, 10720, 10783);
  values->stencil_buffer_partial_render = agx_unpack_uint(cl, 17376, 17439);
  values->stencil_meta_partial_render = agx_unpack_uint(cl, 17440, 17503);
  values->zls_control = agx_unpack_uint(cl, 9312, 9335);
  values->depth_format = (Agx_Depth_Format)agx_unpack_uint(cl, 9336, 9343);
  values->few_primitives = agx_unpack_uint(cl, 10912, 10912);
  values->width = agx_unpack_uint(cl, 10976, 11007);
  values->height = agx_unpack_uint(cl, 11008, 11039);
  values->unknown_program = agx_unpack_uint(cl, 11104, 11167);
  values->depth_clear = agx_unpack_float(cl, 14304, 14335);
  values->stencil_clear = agx_unpack_uint(cl, 14336, 14343);
  values->load_action = (Agx_Load_Action)agx_unpack_uint(cl, 14344, 14351);
  values->any_load = agx_unpack_uint(cl, 14408, 14408);
  values->tile_stores = agx_unpack_uint(cl, 14528, 14535);
  values->partial_load_action_mask = agx_unpack_uint(cl, 15752, 15775);
  values->partial_background_program = agx_unpack_uint(cl, 15776, 15839);
  values->partial_store_program = agx_unpack_uint(cl, 16032, 16095);
  values->partial_store_program_copy = agx_unpack_uint(cl, 16160, 16223);
  values->tile_memory_request_copy = agx_unpack_uint(cl, 17568, 17568);
  values->clear = agx_unpack_uint(cl, 17632, 17632);
  values->run_token = agx_unpack_uint(cl, 18080, 18111);
  values->clear_and_store = agx_unpack_uint(cl, 18208, 18208);
  values->uniform_slice = agx_unpack_uint(cl, 18336, 18399);
  values->uniform_bind_count = agx_unpack_uint(cl, 18400, 18431);
  values->width_copy = agx_unpack_uint(cl, 18464, 18495);
  values->height_copy = agx_unpack_uint(cl, 18496, 18527);
  values->sample_count = agx_unpack_uint(cl, 18528, 18559);
  values->imageblock_sample_length = agx_unpack_uint(cl, 19072, 19103);
  values->tile_width = agx_unpack_uint(cl, 19104, 19135);
  values->tile_height = agx_unpack_uint(cl, 19136, 19167);
  values->attachment_count = agx_unpack_uint(cl, 21696, 21727);
  values->attachment_address_low = agx_unpack_uint(cl, 21792, 21823);
  values->unk_84 = agx_unpack_uint(cl, 1056, 1063);
  values->unk_a0 = agx_unpack_uint(cl, 1280, 1311);
  values->unk_a4 = agx_unpack_uint(cl, 1312, 1319);
  values->barrier_consumer_after_stages = (Agx_Record_Stage)agx_unpack_uint(cl, 1344, 1375);
  values->barrier_consumer_before_stages = (Agx_Record_Stage)agx_unpack_uint(cl, 1376, 1407);
  values->barrier_producer_after_stages = (Agx_Record_Stage)agx_unpack_uint(cl, 1408, 1439);
  values->barrier_producer_before_stages = (Agx_Record_Stage)agx_unpack_uint(cl, 1440, 1471);
  values->encoder_stream_low = agx_unpack_uint(cl, 1568, 1575);
  values->encoder_stream = agx_unpack_uint(cl, 1576, 1599) << 8;
  values->unk_12c = agx_unpack_uint(cl, 2400, 2423);
  values->unk_17c = agx_unpack_uint(cl, 3040, 3071);
  values->unk_180 = agx_unpack_uint(cl, 3072, 3103);
  values->unk_184 = agx_unpack_uint(cl, 3104, 3111);
  values->unk_206 = agx_unpack_uint(cl, 4144, 4151);
  values->unk_214 = agx_unpack_uint(cl, 4256, 4271);
  values->unk_2ed = agx_unpack_uint(cl, 5992, 6023);
  values->unk_2f1 = agx_unpack_uint(cl, 6024, 6055);
  values->unk_2f5 = agx_unpack_uint(cl, 6056, 6087);
  values->unk_2f9 = agx_unpack_uint(cl, 6088, 6119);
  values->tile_memory_request = agx_unpack_uint(cl, 6176, 6176);
  values->unk_310 = agx_unpack_uint(cl, 6272, 6303);
  values->unk_314 = agx_unpack_uint(cl, 6304, 6335);
  values->unk_318 = agx_unpack_uint(cl, 6336, 6367);
  values->unk_380 = agx_unpack_uint(cl, 7168, 7175);
  values->unk_434 = agx_unpack_uint(cl, 8608, 8615);
  values->unk_494 = agx_unpack_uint(cl, 9376, 9407);
  values->tile_memory_blocks = agx_unpack_uint(cl, 10848, 10855);
  values->tile_memory_samples = agx_unpack_uint(cl, 10864, 10871);
  values->unk_555 = agx_unpack_uint(cl, 10920, 10927);
  values->unk_566 = agx_unpack_uint(cl, 11056, 11063);
  values->unk_615 = agx_unpack_uint(cl, 12456, 12487);
  values->unk_619 = agx_unpack_uint(cl, 12488, 12519);
  values->unk_61d = agx_unpack_uint(cl, 12520, 12551);
  values->unk_621 = agx_unpack_uint(cl, 12552, 12575);
  values->unk_624 = agx_unpack_uint(cl, 12576, 12591);
  values->unk_71c = agx_unpack_uint(cl, 14560, 14591);
  values->unk_720 = agx_unpack_uint(cl, 14592, 14623);
  values->unk_724 = agx_unpack_uint(cl, 14624, 14655);
  values->unk_770 = agx_unpack_uint(cl, 15232, 15239);
  values->unk_7ac = agx_unpack_uint(cl, 15712, 15719);
  values->unk_8cc = agx_unpack_uint(cl, 18016, 18023);
  values->unk_8e0 = agx_unpack_uint(cl, 18176, 18207);
  values->unk_95c = agx_unpack_uint(cl, 19168, 19175);
  values->unk_974 = agx_unpack_uint(cl, 19360, 19367);
  values->unk_a9c = agx_unpack_uint(cl, 21728, 21735);
  values->sample_position_0 = agx_unpack_uint(cl, 18560, 18591);
  values->sample_position_1 = agx_unpack_uint(cl, 18592, 18623);
  values->sample_position_2 = agx_unpack_uint(cl, 18624, 18655);
  values->sample_position_3 = agx_unpack_uint(cl, 18656, 18687);
  values->sample_position_4 = agx_unpack_uint(cl, 18688, 18719);
  values->sample_position_5 = agx_unpack_uint(cl, 18720, 18751);
  values->sample_position_6 = agx_unpack_uint(cl, 18752, 18783);
  values->sample_position_7 = agx_unpack_uint(cl, 18784, 18815);
}

typedef enum Agx_Vdm_Block_Type
{
  AGX_VDM_BLOCK_TYPE_PPP_STATE_UPDATE = 0,
  AGX_VDM_BLOCK_TYPE_BARRIER = 1,
  AGX_VDM_BLOCK_TYPE_VDM_STATE_UPDATE = 2,
  AGX_VDM_BLOCK_TYPE_INDEX_LIST = 3,
  AGX_VDM_BLOCK_TYPE_STREAM_LINK = 4,
  AGX_VDM_BLOCK_TYPE_TESSELLATE = 5,
  AGX_VDM_BLOCK_TYPE_STREAM_TERMINATE = 6,
} Agx_Vdm_Block_Type;

typedef enum Agx_Index_Size
{
  AGX_INDEX_SIZE_U8 = 0,
  AGX_INDEX_SIZE_U16 = 1,
  AGX_INDEX_SIZE_U32 = 2,
} Agx_Index_Size;

typedef struct Agx_Vdm_State_Header
{
  uint32_t           present;
  uint32_t           unk_08;
  Agx_Vdm_Block_Type block_type;
} Agx_Vdm_State_Header;

#define AGX_VDM_STATE_HEADER_LENGTH 4

static inline Agx_Vdm_State_Header
agx_vdm_state_header_default(void)
{
  return (Agx_Vdm_State_Header) {
    .present = 0x0,
    .unk_08 = 0x0,
    .block_type = AGX_VDM_BLOCK_TYPE_VDM_STATE_UPDATE,
  };
}

static inline uint32_t
agx_vdm_state_header_pack(uint32_t* restrict cl, const Agx_Vdm_State_Header* restrict values)
{
  cl[0] = agx_cmd_uint(values->present, 0, 7) | agx_cmd_uint(values->unk_08, 8, 28) |
          agx_cmd_uint(values->block_type, 29, 31);

  return AGX_VDM_STATE_HEADER_LENGTH;
}

static inline void
agx_vdm_state_header_unpack(const uint8_t* restrict cl, Agx_Vdm_State_Header* restrict values)
{
  values->present = agx_unpack_uint(cl, 0, 7);
  values->unk_08 = agx_unpack_uint(cl, 8, 28);
  values->block_type = (Agx_Vdm_Block_Type)agx_unpack_uint(cl, 29, 31);
}

typedef struct Agx_Vdm_State_Pipeline
{
  uint32_t shader_word_0;
  bool     vertex_amplify;
  uint64_t pipeline;
  uint32_t vertex_caller_count;
  uint64_t program_buffer;
  uint32_t padding;
} Agx_Vdm_State_Pipeline;

#define AGX_VDM_STATE_PIPELINE_LENGTH 12

static inline Agx_Vdm_State_Pipeline
agx_vdm_state_pipeline_default(void)
{
  return (Agx_Vdm_State_Pipeline) {
    .shader_word_0 = 0x0,
    .vertex_amplify = false,
    .padding = 0x0,
  };
}

static inline uint32_t
agx_vdm_state_pipeline_pack(uint32_t* restrict cl, const Agx_Vdm_State_Pipeline* restrict values)
{
  assert((values->program_buffer & 0x3fff) == 0);
  cl[0] = agx_cmd_uint(values->shader_word_0, 0, 30) | agx_cmd_uint(values->vertex_amplify, 31, 31);
  cl[1] = agx_cmd_uint(values->pipeline, 0, 31);
  cl[2] = agx_cmd_uint(values->vertex_caller_count, 0, 7) | agx_cmd_uint(values->program_buffer >> 14, 8, 23) |
          agx_cmd_uint(values->padding, 24, 31);

  return AGX_VDM_STATE_PIPELINE_LENGTH;
}

static inline void
agx_vdm_state_pipeline_unpack(const uint8_t* restrict cl, Agx_Vdm_State_Pipeline* restrict values)
{
  values->shader_word_0 = agx_unpack_uint(cl, 0, 30);
  values->vertex_amplify = agx_unpack_uint(cl, 31, 31);
  values->pipeline = agx_unpack_uint(cl, 32, 63);
  values->vertex_caller_count = agx_unpack_uint(cl, 64, 71);
  values->program_buffer = agx_unpack_uint(cl, 72, 87) << 14;
  values->padding = agx_unpack_uint(cl, 88, 95);
}

typedef struct Agx_Vdm_State_Varyings
{
  uint32_t vertex_output_count;
  uint32_t vertex_output_count_copy;
  uint32_t vertex_output_count_high;
  uint32_t unk_424;
} Agx_Vdm_State_Varyings;

#define AGX_VDM_STATE_VARYINGS_LENGTH 4

static inline Agx_Vdm_State_Varyings
agx_vdm_state_varyings_default(void)
{
  return (Agx_Vdm_State_Varyings) {
    .unk_424 = 0x0,
  };
}

static inline uint32_t
agx_vdm_state_varyings_pack(uint32_t* restrict cl, const Agx_Vdm_State_Varyings* restrict values)
{
  cl[0] = agx_cmd_uint(values->vertex_output_count, 0, 7) | agx_cmd_uint(values->vertex_output_count_copy, 8, 15) |
          agx_cmd_uint(values->vertex_output_count_high, 16, 23) | agx_cmd_uint(values->unk_424, 24, 31);

  return AGX_VDM_STATE_VARYINGS_LENGTH;
}

static inline void
agx_vdm_state_varyings_unpack(const uint8_t* restrict cl, Agx_Vdm_State_Varyings* restrict values)
{
  values->vertex_output_count = agx_unpack_uint(cl, 0, 7);
  values->vertex_output_count_copy = agx_unpack_uint(cl, 8, 15);
  values->vertex_output_count_high = agx_unpack_uint(cl, 16, 23);
  values->unk_424 = agx_unpack_uint(cl, 24, 31);
}

typedef struct Agx_Vdm_State_Unk_Word
{
  uint32_t value;
} Agx_Vdm_State_Unk_Word;

#define AGX_VDM_STATE_UNK_WORD_LENGTH 4

static inline Agx_Vdm_State_Unk_Word
agx_vdm_state_unk_word_default(void)
{
  return (Agx_Vdm_State_Unk_Word) {
    .value = 0x0,
  };
}

static inline uint32_t
agx_vdm_state_unk_word_pack(uint32_t* restrict cl, const Agx_Vdm_State_Unk_Word* restrict values)
{
  cl[0] = agx_cmd_uint(values->value, 0, 31);

  return AGX_VDM_STATE_UNK_WORD_LENGTH;
}

static inline void
agx_vdm_state_unk_word_unpack(const uint8_t* restrict cl, Agx_Vdm_State_Unk_Word* restrict values)
{
  values->value = agx_unpack_uint(cl, 0, 31);
}

typedef struct Agx_Vdm_State_Vertex_Unknown
{
  uint32_t flat_shading_control;
  uint32_t unk_2;
  bool     unk_4;
  bool     unk_5;
  bool     generate_primitive_id;
  uint32_t unk_7;
} Agx_Vdm_State_Vertex_Unknown;

#define AGX_VDM_STATE_VERTEX_UNKNOWN_LENGTH 4

static inline Agx_Vdm_State_Vertex_Unknown
agx_vdm_state_vertex_unknown_default(void)
{
  return (Agx_Vdm_State_Vertex_Unknown) {
    .flat_shading_control = 0,
    .unk_2 = 0x0,
    .unk_4 = false,
    .unk_5 = false,
    .generate_primitive_id = false,
    .unk_7 = 0x0,
  };
}

static inline uint32_t
agx_vdm_state_vertex_unknown_pack(uint32_t* restrict cl, const Agx_Vdm_State_Vertex_Unknown* restrict values)
{
  cl[0] = agx_cmd_uint(values->flat_shading_control, 0, 1) | agx_cmd_uint(values->unk_2, 2, 3) |
          agx_cmd_uint(values->unk_4, 4, 4) | agx_cmd_uint(values->unk_5, 5, 5) |
          agx_cmd_uint(values->generate_primitive_id, 6, 6) | agx_cmd_uint(values->unk_7, 7, 31);

  return AGX_VDM_STATE_VERTEX_UNKNOWN_LENGTH;
}

static inline void
agx_vdm_state_vertex_unknown_unpack(const uint8_t* restrict cl, Agx_Vdm_State_Vertex_Unknown* restrict values)
{
  values->flat_shading_control = agx_unpack_uint(cl, 0, 1);
  values->unk_2 = agx_unpack_uint(cl, 2, 3);
  values->unk_4 = agx_unpack_uint(cl, 4, 4);
  values->unk_5 = agx_unpack_uint(cl, 5, 5);
  values->generate_primitive_id = agx_unpack_uint(cl, 6, 6);
  values->unk_7 = agx_unpack_uint(cl, 7, 31);
}

typedef struct Agx_Vdm_State
{
  uint32_t           present;
  uint32_t           unk_08;
  Agx_Vdm_Block_Type block_type;
  uint32_t           shader_word_0;
  bool               vertex_amplify;
  uint64_t           pipeline;
  uint32_t           vertex_caller_count;
  uint64_t           program_buffer;
  uint32_t           padding;
  uint32_t           vertex_output_count;
  uint32_t           vertex_output_count_copy;
  uint32_t           unk_416;
  uint32_t           unk_5;
  uint32_t           unk_6;
} Agx_Vdm_State;

#define AGX_VDM_STATE_LENGTH 28

static inline Agx_Vdm_State
agx_vdm_state_default(void)
{
  return (Agx_Vdm_State) {
    .present = 0x2e,
    .unk_08 = 0x0,
    .block_type = AGX_VDM_BLOCK_TYPE_VDM_STATE_UPDATE,
    .shader_word_0 = 0x0,
    .vertex_amplify = false,
    .padding = 0x0,
    .unk_416 = 0x0,
    .unk_5 = 0x0,
    .unk_6 = 0x0,
  };
}

static inline uint32_t
agx_vdm_state_pack(uint32_t* restrict cl, const Agx_Vdm_State* restrict values)
{
  assert((values->program_buffer & 0x3fff) == 0);
  cl[0] = agx_cmd_uint(values->present, 0, 7) | agx_cmd_uint(values->unk_08, 8, 28) |
          agx_cmd_uint(values->block_type, 29, 31);
  cl[1] = agx_cmd_uint(values->shader_word_0, 0, 30) | agx_cmd_uint(values->vertex_amplify, 31, 31);
  cl[2] = agx_cmd_uint(values->pipeline, 0, 31);
  cl[3] = agx_cmd_uint(values->vertex_caller_count, 0, 7) | agx_cmd_uint(values->program_buffer >> 14, 8, 23) |
          agx_cmd_uint(values->padding, 24, 31);
  cl[4] = agx_cmd_uint(values->vertex_output_count, 0, 7) | agx_cmd_uint(values->vertex_output_count_copy, 8, 15) |
          agx_cmd_uint(values->unk_416, 16, 31);
  cl[5] = agx_cmd_uint(values->unk_5, 0, 31);
  cl[6] = agx_cmd_uint(values->unk_6, 0, 31);

  return AGX_VDM_STATE_LENGTH;
}

static inline void
agx_vdm_state_unpack(const uint8_t* restrict cl, Agx_Vdm_State* restrict values)
{
  values->present = agx_unpack_uint(cl, 0, 7);
  values->unk_08 = agx_unpack_uint(cl, 8, 28);
  values->block_type = (Agx_Vdm_Block_Type)agx_unpack_uint(cl, 29, 31);
  values->shader_word_0 = agx_unpack_uint(cl, 32, 62);
  values->vertex_amplify = agx_unpack_uint(cl, 63, 63);
  values->pipeline = agx_unpack_uint(cl, 64, 95);
  values->vertex_caller_count = agx_unpack_uint(cl, 96, 103);
  values->program_buffer = agx_unpack_uint(cl, 104, 119) << 14;
  values->padding = agx_unpack_uint(cl, 120, 127);
  values->vertex_output_count = agx_unpack_uint(cl, 128, 135);
  values->vertex_output_count_copy = agx_unpack_uint(cl, 136, 143);
  values->unk_416 = agx_unpack_uint(cl, 144, 159);
  values->unk_5 = agx_unpack_uint(cl, 160, 191);
  values->unk_6 = agx_unpack_uint(cl, 192, 223);
}

typedef struct Agx_Ppp_Frg_Face_Ctl_Pso
{
  uint32_t sref;
  uint32_t pointlinewidth;
  uint32_t fill_mode_override;
  bool     linefilllastpixel;
  bool     dwritedisable;
  uint32_t fs_depth_dir_qual;
  uint32_t dcmpmode;
  uint32_t rsvd_0;
  uint32_t objtype;
} Agx_Ppp_Frg_Face_Ctl_Pso;

#define AGX_PPP_FRG_FACE_CTL_PSO_LENGTH 4

static inline Agx_Ppp_Frg_Face_Ctl_Pso
agx_ppp_frg_face_ctl_pso_default(void)
{
  return (Agx_Ppp_Frg_Face_Ctl_Pso) {
    .sref = 0x0,
    .pointlinewidth = 15,
    .fill_mode_override = 0x0,
    .linefilllastpixel = false,
    .dwritedisable = false,
    .fs_depth_dir_qual = 0x0,
    .dcmpmode = 0x0,
    .rsvd_0 = 0x0,
    .objtype = 0,
  };
}

static inline uint32_t
agx_ppp_frg_face_ctl_pso_pack(uint32_t* restrict cl, const Agx_Ppp_Frg_Face_Ctl_Pso* restrict values)
{
  cl[0] = agx_cmd_uint(values->sref, 0, 7) | agx_cmd_uint(values->pointlinewidth, 8, 17) |
          agx_cmd_uint(values->fill_mode_override, 18, 19) | agx_cmd_uint(values->linefilllastpixel, 20, 20) |
          agx_cmd_uint(values->dwritedisable, 21, 21) | agx_cmd_uint(values->fs_depth_dir_qual, 22, 23) |
          agx_cmd_uint(values->dcmpmode, 24, 26) | agx_cmd_uint(values->rsvd_0, 27, 27) |
          agx_cmd_uint(values->objtype, 28, 31);

  return AGX_PPP_FRG_FACE_CTL_PSO_LENGTH;
}

static inline void
agx_ppp_frg_face_ctl_pso_unpack(const uint8_t* restrict cl, Agx_Ppp_Frg_Face_Ctl_Pso* restrict values)
{
  values->sref = agx_unpack_uint(cl, 0, 7);
  values->pointlinewidth = agx_unpack_uint(cl, 8, 17);
  values->fill_mode_override = agx_unpack_uint(cl, 18, 19);
  values->linefilllastpixel = agx_unpack_uint(cl, 20, 20);
  values->dwritedisable = agx_unpack_uint(cl, 21, 21);
  values->fs_depth_dir_qual = agx_unpack_uint(cl, 22, 23);
  values->dcmpmode = agx_unpack_uint(cl, 24, 26);
  values->rsvd_0 = agx_unpack_uint(cl, 27, 27);
  values->objtype = agx_unpack_uint(cl, 28, 31);
}

typedef enum Agx_Pass_Type
{
  AGX_PASS_TYPE_OPAQUE = 0,
  AGX_PASS_TYPE_TRANSLUCENT = 1,
  AGX_PASS_TYPE_PUNCH_THROUGH = 2,
  AGX_PASS_TYPE_TRANSLUCENT_PUNCH_THROUGH = 3,
} Agx_Pass_Type;

typedef struct Agx_Ppp_Frg_Common_Ctl
{
  bool          front_face_dir;
  uint32_t      cullmode;
  uint32_t      rsvd_0;
  bool          in_batch_window;
  uint32_t      tagsort_flush_ctl;
  uint32_t      rsvd_1;
  bool          tagsort_accum_disable;
  bool          visbool;
  bool          vistest;
  bool          scenable;
  bool          dbenable;
  bool          face_stencil_pres;
  bool          two_sided;
  bool          rect_warp_disable;
  bool          tagwritedisable;
  bool          two_pass_fb_opa;
  bool          mid_render_compute;
  uint32_t      rsvd_2;
  bool          sample_mask_select;
  bool          tri_merge_disable;
  bool          overlap_check_mode;
  bool          usc_esl_2;
  Agx_Pass_Type passtype;
} Agx_Ppp_Frg_Common_Ctl;

#define AGX_PPP_FRG_COMMON_CTL_LENGTH 4

static inline Agx_Ppp_Frg_Common_Ctl
agx_ppp_frg_common_ctl_default(void)
{
  return (Agx_Ppp_Frg_Common_Ctl) {
    .front_face_dir = false,
    .cullmode = 0,
    .rsvd_0 = 0x0,
    .in_batch_window = false,
    .tagsort_flush_ctl = 0,
    .rsvd_1 = 0x0,
    .tagsort_accum_disable = false,
    .visbool = false,
    .vistest = false,
    .scenable = false,
    .dbenable = false,
    .face_stencil_pres = false,
    .two_sided = false,
    .rect_warp_disable = false,
    .tagwritedisable = false,
    .two_pass_fb_opa = false,
    .mid_render_compute = false,
    .rsvd_2 = 0x0,
    .sample_mask_select = false,
    .tri_merge_disable = false,
    .overlap_check_mode = false,
    .usc_esl_2 = false,
    .passtype = AGX_PASS_TYPE_OPAQUE,
  };
}

static inline uint32_t
agx_ppp_frg_common_ctl_pack(uint32_t* restrict cl, const Agx_Ppp_Frg_Common_Ctl* restrict values)
{
  cl[0] = agx_cmd_uint(values->front_face_dir, 0, 0) | agx_cmd_uint(values->cullmode, 1, 2) |
          agx_cmd_uint(values->rsvd_0, 3, 6) | agx_cmd_uint(values->in_batch_window, 7, 7) |
          agx_cmd_uint(values->tagsort_flush_ctl, 8, 9) | agx_cmd_uint(values->rsvd_1, 10, 12) |
          agx_cmd_uint(values->tagsort_accum_disable, 13, 13) | agx_cmd_uint(values->visbool, 14, 14) |
          agx_cmd_uint(values->vistest, 15, 15) | agx_cmd_uint(values->scenable, 16, 16) |
          agx_cmd_uint(values->dbenable, 17, 17) | agx_cmd_uint(values->face_stencil_pres, 18, 18) |
          agx_cmd_uint(values->two_sided, 19, 19) | agx_cmd_uint(values->rect_warp_disable, 20, 20) |
          agx_cmd_uint(values->tagwritedisable, 21, 21) | agx_cmd_uint(values->two_pass_fb_opa, 22, 22) |
          agx_cmd_uint(values->mid_render_compute, 23, 23) | agx_cmd_uint(values->rsvd_2, 24, 24) |
          agx_cmd_uint(values->sample_mask_select, 25, 25) | agx_cmd_uint(values->tri_merge_disable, 26, 26) |
          agx_cmd_uint(values->overlap_check_mode, 27, 27) | agx_cmd_uint(values->usc_esl_2, 28, 28) |
          agx_cmd_uint(values->passtype, 29, 31);

  return AGX_PPP_FRG_COMMON_CTL_LENGTH;
}

static inline void
agx_ppp_frg_common_ctl_unpack(const uint8_t* restrict cl, Agx_Ppp_Frg_Common_Ctl* restrict values)
{
  values->front_face_dir = agx_unpack_uint(cl, 0, 0);
  values->cullmode = agx_unpack_uint(cl, 1, 2);
  values->rsvd_0 = agx_unpack_uint(cl, 3, 6);
  values->in_batch_window = agx_unpack_uint(cl, 7, 7);
  values->tagsort_flush_ctl = agx_unpack_uint(cl, 8, 9);
  values->rsvd_1 = agx_unpack_uint(cl, 10, 12);
  values->tagsort_accum_disable = agx_unpack_uint(cl, 13, 13);
  values->visbool = agx_unpack_uint(cl, 14, 14);
  values->vistest = agx_unpack_uint(cl, 15, 15);
  values->scenable = agx_unpack_uint(cl, 16, 16);
  values->dbenable = agx_unpack_uint(cl, 17, 17);
  values->face_stencil_pres = agx_unpack_uint(cl, 18, 18);
  values->two_sided = agx_unpack_uint(cl, 19, 19);
  values->rect_warp_disable = agx_unpack_uint(cl, 20, 20);
  values->tagwritedisable = agx_unpack_uint(cl, 21, 21);
  values->two_pass_fb_opa = agx_unpack_uint(cl, 22, 22);
  values->mid_render_compute = agx_unpack_uint(cl, 23, 23);
  values->rsvd_2 = agx_unpack_uint(cl, 24, 24);
  values->sample_mask_select = agx_unpack_uint(cl, 25, 25);
  values->tri_merge_disable = agx_unpack_uint(cl, 26, 26);
  values->overlap_check_mode = agx_unpack_uint(cl, 27, 27);
  values->usc_esl_2 = agx_unpack_uint(cl, 28, 28);
  values->passtype = (Agx_Pass_Type)agx_unpack_uint(cl, 29, 31);
}

typedef struct Agx_Ppp_Frg_Face_Stencil
{
  uint32_t swmask;
  uint32_t scmpmask;
  uint32_t sop_3;
  uint32_t sop_2;
  uint32_t sop_1;
  uint32_t scmpmode;
  uint32_t rsvd_0;
} Agx_Ppp_Frg_Face_Stencil;

#define AGX_PPP_FRG_FACE_STENCIL_LENGTH 4

static inline Agx_Ppp_Frg_Face_Stencil
agx_ppp_frg_face_stencil_default(void)
{
  return (Agx_Ppp_Frg_Face_Stencil) {
    .swmask = 0xff,
    .scmpmask = 0xff,
    .sop_3 = 0,
    .sop_2 = 0,
    .sop_1 = 0,
    .scmpmode = 7,
    .rsvd_0 = 0x0,
  };
}

static inline uint32_t
agx_ppp_frg_face_stencil_pack(uint32_t* restrict cl, const Agx_Ppp_Frg_Face_Stencil* restrict values)
{
  cl[0] = agx_cmd_uint(values->swmask, 0, 7) | agx_cmd_uint(values->scmpmask, 8, 15) |
          agx_cmd_uint(values->sop_3, 16, 18) | agx_cmd_uint(values->sop_2, 19, 21) | agx_cmd_uint(values->sop_1, 22, 24) |
          agx_cmd_uint(values->scmpmode, 25, 27) | agx_cmd_uint(values->rsvd_0, 28, 31);

  return AGX_PPP_FRG_FACE_STENCIL_LENGTH;
}

static inline void
agx_ppp_frg_face_stencil_unpack(const uint8_t* restrict cl, Agx_Ppp_Frg_Face_Stencil* restrict values)
{
  values->swmask = agx_unpack_uint(cl, 0, 7);
  values->scmpmask = agx_unpack_uint(cl, 8, 15);
  values->sop_3 = agx_unpack_uint(cl, 16, 18);
  values->sop_2 = agx_unpack_uint(cl, 19, 21);
  values->sop_1 = agx_unpack_uint(cl, 22, 24);
  values->scmpmode = agx_unpack_uint(cl, 25, 27);
  values->rsvd_0 = agx_unpack_uint(cl, 28, 31);
}

typedef struct Agx_Ppp_Frg_Vis_Query_Tag_Flush_Pso
{
  uint32_t tagsort_flush_data;
  uint32_t vis_query_index;
} Agx_Ppp_Frg_Vis_Query_Tag_Flush_Pso;

#define AGX_PPP_FRG_VIS_QUERY_TAG_FLUSH_PSO_LENGTH 4

static inline Agx_Ppp_Frg_Vis_Query_Tag_Flush_Pso
agx_ppp_frg_vis_query_tag_flush_pso_default(void)
{
  return (Agx_Ppp_Frg_Vis_Query_Tag_Flush_Pso) {
    .vis_query_index = 0,
  };
}

static inline uint32_t
agx_ppp_frg_vis_query_tag_flush_pso_pack(uint32_t* restrict cl, const Agx_Ppp_Frg_Vis_Query_Tag_Flush_Pso* restrict values)
{
  cl[0] = agx_cmd_uint(values->tagsort_flush_data, 0, 16) | agx_cmd_uint(values->vis_query_index, 17, 31);

  return AGX_PPP_FRG_VIS_QUERY_TAG_FLUSH_PSO_LENGTH;
}

static inline void
agx_ppp_frg_vis_query_tag_flush_pso_unpack(const uint8_t* restrict cl, Agx_Ppp_Frg_Vis_Query_Tag_Flush_Pso* restrict values)
{
  values->tagsort_flush_data = agx_unpack_uint(cl, 0, 16);
  values->vis_query_index = agx_unpack_uint(cl, 17, 31);
}

typedef struct Agx_Ppp_State
{
  uint32_t           pointer_hi;
  uint32_t           size_words;
  uint32_t           unk_16;
  Agx_Vdm_Block_Type block_type;
  uint64_t           pointer;
} Agx_Ppp_State;

#define AGX_PPP_STATE_LENGTH 8

static inline Agx_Ppp_State
agx_ppp_state_default(void)
{
  return (Agx_Ppp_State) {
    .pointer_hi = 0x0,
    .unk_16 = 0x0,
    .block_type = AGX_VDM_BLOCK_TYPE_PPP_STATE_UPDATE,
  };
}

static inline uint32_t
agx_ppp_state_pack(uint32_t* restrict cl, const Agx_Ppp_State* restrict values)
{
  cl[0] = agx_cmd_uint(values->pointer_hi, 0, 7) | agx_cmd_uint(values->size_words, 8, 15) |
          agx_cmd_uint(values->unk_16, 16, 28) | agx_cmd_uint(values->block_type, 29, 31);
  cl[1] = agx_cmd_uint(values->pointer, 0, 31);

  return AGX_PPP_STATE_LENGTH;
}

static inline void
agx_ppp_state_unpack(const uint8_t* restrict cl, Agx_Ppp_State* restrict values)
{
  values->pointer_hi = agx_unpack_uint(cl, 0, 7);
  values->size_words = agx_unpack_uint(cl, 8, 15);
  values->unk_16 = agx_unpack_uint(cl, 16, 28);
  values->block_type = (Agx_Vdm_Block_Type)agx_unpack_uint(cl, 29, 31);
  values->pointer = agx_unpack_uint(cl, 32, 63);
}

typedef struct Agx_Ppp_Header
{
  uint32_t rsvd_0;
  bool     pres_frg_face_ctl_f;
  bool     pres_frg_face_ctl_f_pso;
  bool     pres_frg_face_stencil_f;
  bool     pres_frg_face_ctl_b;
  bool     pres_frg_face_ctl_b_pso;
  bool     pres_frg_face_stencil_b;
  bool     pres_frg_dbsc;
  bool     viewport_state_is_16;
  bool     pres_region_clip;
  bool     pres_viewport;
  uint32_t viewport_count;
  bool     pres_wclamp;
  bool     pres_outselects;
  bool     pres_varying_words;
  bool     pres_ms_prim_output;
  bool     pres_frg_usc_opt_esl;
  bool     pres_pppctrl;
  bool     pres_pppctrl_pso;
  bool     pres_frg_shader_words;
  bool     pres_frg_vis_query_tag_flush;
  bool     pres_frg_vis_query_tag_flush_pso;
  bool     pres_vs_amplify_ctrl;
  bool     pres_vs_output_size;
  bool     pres_amplify_varying_words;
  bool     last_pipe;
  bool     context_switch;
  bool     terminate;
} Agx_Ppp_Header;

#define AGX_PPP_HEADER_LENGTH 4

static inline Agx_Ppp_Header
agx_ppp_header_default(void)
{
  return (Agx_Ppp_Header) {
    .rsvd_0 = 0x0,
    .pres_frg_face_ctl_f = false,
    .pres_frg_face_ctl_f_pso = false,
    .pres_frg_face_stencil_f = false,
    .pres_frg_face_ctl_b = false,
    .pres_frg_face_ctl_b_pso = false,
    .pres_frg_face_stencil_b = false,
    .pres_frg_dbsc = false,
    .viewport_state_is_16 = false,
    .pres_region_clip = false,
    .pres_viewport = false,
    .pres_wclamp = false,
    .pres_outselects = false,
    .pres_varying_words = false,
    .pres_ms_prim_output = false,
    .pres_frg_usc_opt_esl = false,
    .pres_pppctrl = false,
    .pres_pppctrl_pso = false,
    .pres_frg_shader_words = false,
    .pres_frg_vis_query_tag_flush = false,
    .pres_frg_vis_query_tag_flush_pso = false,
    .pres_vs_amplify_ctrl = false,
    .pres_vs_output_size = false,
    .pres_amplify_varying_words = false,
    .last_pipe = false,
    .context_switch = false,
    .terminate = false,
  };
}

static inline uint32_t
agx_ppp_header_pack(uint32_t* restrict cl, const Agx_Ppp_Header* restrict values)
{
  assert(values->viewport_count >= 1);
  cl[0] = agx_cmd_uint(values->rsvd_0, 0, 1) | agx_cmd_uint(values->pres_frg_face_ctl_f, 2, 2) |
          agx_cmd_uint(values->pres_frg_face_ctl_f_pso, 3, 3) | agx_cmd_uint(values->pres_frg_face_stencil_f, 4, 4) |
          agx_cmd_uint(values->pres_frg_face_ctl_b, 5, 5) | agx_cmd_uint(values->pres_frg_face_ctl_b_pso, 6, 6) |
          agx_cmd_uint(values->pres_frg_face_stencil_b, 7, 7) | agx_cmd_uint(values->pres_frg_dbsc, 8, 8) |
          agx_cmd_uint(values->viewport_state_is_16, 9, 9) | agx_cmd_uint(values->pres_region_clip, 10, 10) |
          agx_cmd_uint(values->pres_viewport, 11, 11) | agx_cmd_uint(values->viewport_count - 1, 12, 15) |
          agx_cmd_uint(values->pres_wclamp, 16, 16) | agx_cmd_uint(values->pres_outselects, 17, 17) |
          agx_cmd_uint(values->pres_varying_words, 18, 18) | agx_cmd_uint(values->pres_ms_prim_output, 19, 19) |
          agx_cmd_uint(values->pres_frg_usc_opt_esl, 20, 20) | agx_cmd_uint(values->pres_pppctrl, 21, 21) |
          agx_cmd_uint(values->pres_pppctrl_pso, 22, 22) | agx_cmd_uint(values->pres_frg_shader_words, 23, 23) |
          agx_cmd_uint(values->pres_frg_vis_query_tag_flush, 24, 24) |
          agx_cmd_uint(values->pres_frg_vis_query_tag_flush_pso, 25, 25) |
          agx_cmd_uint(values->pres_vs_amplify_ctrl, 26, 26) | agx_cmd_uint(values->pres_vs_output_size, 27, 27) |
          agx_cmd_uint(values->pres_amplify_varying_words, 28, 28) | agx_cmd_uint(values->last_pipe, 29, 29) |
          agx_cmd_uint(values->context_switch, 30, 30) | agx_cmd_uint(values->terminate, 31, 31);

  return AGX_PPP_HEADER_LENGTH;
}

static inline void
agx_ppp_header_unpack(const uint8_t* restrict cl, Agx_Ppp_Header* restrict values)
{
  values->rsvd_0 = agx_unpack_uint(cl, 0, 1);
  values->pres_frg_face_ctl_f = agx_unpack_uint(cl, 2, 2);
  values->pres_frg_face_ctl_f_pso = agx_unpack_uint(cl, 3, 3);
  values->pres_frg_face_stencil_f = agx_unpack_uint(cl, 4, 4);
  values->pres_frg_face_ctl_b = agx_unpack_uint(cl, 5, 5);
  values->pres_frg_face_ctl_b_pso = agx_unpack_uint(cl, 6, 6);
  values->pres_frg_face_stencil_b = agx_unpack_uint(cl, 7, 7);
  values->pres_frg_dbsc = agx_unpack_uint(cl, 8, 8);
  values->viewport_state_is_16 = agx_unpack_uint(cl, 9, 9);
  values->pres_region_clip = agx_unpack_uint(cl, 10, 10);
  values->pres_viewport = agx_unpack_uint(cl, 11, 11);
  values->viewport_count = agx_unpack_uint(cl, 12, 15) + 1;
  values->pres_wclamp = agx_unpack_uint(cl, 16, 16);
  values->pres_outselects = agx_unpack_uint(cl, 17, 17);
  values->pres_varying_words = agx_unpack_uint(cl, 18, 18);
  values->pres_ms_prim_output = agx_unpack_uint(cl, 19, 19);
  values->pres_frg_usc_opt_esl = agx_unpack_uint(cl, 20, 20);
  values->pres_pppctrl = agx_unpack_uint(cl, 21, 21);
  values->pres_pppctrl_pso = agx_unpack_uint(cl, 22, 22);
  values->pres_frg_shader_words = agx_unpack_uint(cl, 23, 23);
  values->pres_frg_vis_query_tag_flush = agx_unpack_uint(cl, 24, 24);
  values->pres_frg_vis_query_tag_flush_pso = agx_unpack_uint(cl, 25, 25);
  values->pres_vs_amplify_ctrl = agx_unpack_uint(cl, 26, 26);
  values->pres_vs_output_size = agx_unpack_uint(cl, 27, 27);
  values->pres_amplify_varying_words = agx_unpack_uint(cl, 28, 28);
  values->last_pipe = agx_unpack_uint(cl, 29, 29);
  values->context_switch = agx_unpack_uint(cl, 30, 30);
  values->terminate = agx_unpack_uint(cl, 31, 31);
}

typedef struct Agx_Fragment_Shader_Word_0
{
  uint32_t unk_0;
  bool     coefficients_present;
  uint32_t unk_9;
  uint32_t cf_binding_count;
  uint32_t unk_23;
} Agx_Fragment_Shader_Word_0;

#define AGX_FRAGMENT_SHADER_WORD_0_LENGTH 4

static inline Agx_Fragment_Shader_Word_0
agx_fragment_shader_word_0_default(void)
{
  return (Agx_Fragment_Shader_Word_0) {
    .unk_0 = 0x0,
    .coefficients_present = false,
    .unk_9 = 0x0,
    .cf_binding_count = 0,
    .unk_23 = 0x0,
  };
}

static inline uint32_t
agx_fragment_shader_word_0_pack(uint32_t* restrict cl, const Agx_Fragment_Shader_Word_0* restrict values)
{
  cl[0] = agx_cmd_uint(values->unk_0, 0, 7) | agx_cmd_uint(values->coefficients_present, 8, 8) |
          agx_cmd_uint(values->unk_9, 9, 15) | agx_cmd_uint(values->cf_binding_count, 16, 22) |
          agx_cmd_uint(values->unk_23, 23, 31);

  return AGX_FRAGMENT_SHADER_WORD_0_LENGTH;
}

static inline void
agx_fragment_shader_word_0_unpack(const uint8_t* restrict cl, Agx_Fragment_Shader_Word_0* restrict values)
{
  values->unk_0 = agx_unpack_uint(cl, 0, 7);
  values->coefficients_present = agx_unpack_uint(cl, 8, 8);
  values->unk_9 = agx_unpack_uint(cl, 9, 15);
  values->cf_binding_count = agx_unpack_uint(cl, 16, 22);
  values->unk_23 = agx_unpack_uint(cl, 23, 31);
}

typedef struct Agx_Fragment_Shader_Word_3
{
  uint32_t unk_0;
  uint32_t unk_18;
  uint32_t unk_20;
  bool     enable;
  uint32_t unk_25;
} Agx_Fragment_Shader_Word_3;

#define AGX_FRAGMENT_SHADER_WORD_3_LENGTH 4

static inline Agx_Fragment_Shader_Word_3
agx_fragment_shader_word_3_default(void)
{
  return (Agx_Fragment_Shader_Word_3) {
    .unk_0 = 0x0,
    .unk_18 = 0x0,
    .unk_20 = 0x0,
    .enable = true,
    .unk_25 = 0x0,
  };
}

static inline uint32_t
agx_fragment_shader_word_3_pack(uint32_t* restrict cl, const Agx_Fragment_Shader_Word_3* restrict values)
{
  cl[0] = agx_cmd_uint(values->unk_0, 0, 17) | agx_cmd_uint(values->unk_18, 18, 19) |
          agx_cmd_uint(values->unk_20, 20, 23) | agx_cmd_uint(values->enable, 24, 24) |
          agx_cmd_uint(values->unk_25, 25, 31);

  return AGX_FRAGMENT_SHADER_WORD_3_LENGTH;
}

static inline void
agx_fragment_shader_word_3_unpack(const uint8_t* restrict cl, Agx_Fragment_Shader_Word_3* restrict values)
{
  values->unk_0 = agx_unpack_uint(cl, 0, 17);
  values->unk_18 = agx_unpack_uint(cl, 18, 19);
  values->unk_20 = agx_unpack_uint(cl, 20, 23);
  values->enable = agx_unpack_uint(cl, 24, 24);
  values->unk_25 = agx_unpack_uint(cl, 25, 31);
}

typedef struct Agx_Output_Select
{
  bool     clip_distance_plane_0;
  bool     clip_distance_plane_1;
  bool     clip_distance_plane_2;
  bool     clip_distance_plane_3;
  bool     clip_distance_plane_4;
  bool     clip_distance_plane_5;
  bool     clip_distance_plane_6;
  bool     clip_distance_plane_7;
  bool     clip_distance_plane_8;
  bool     clip_distance_plane_9;
  bool     clip_distance_plane_10;
  bool     clip_distance_plane_11;
  bool     clip_distance_plane_12;
  bool     clip_distance_plane_13;
  bool     clip_distance_plane_14;
  bool     clip_distance_plane_15;
  bool     varyings;
  uint32_t unk_17;
  bool     point_size;
  bool     viewport_target;
  bool     render_target;
  bool     frag_coord_z;
  bool     barycentric_coordinates;
  uint32_t unk_23;
  uint32_t unk_24;
} Agx_Output_Select;

#define AGX_OUTPUT_SELECT_LENGTH 4

static inline Agx_Output_Select
agx_output_select_default(void)
{
  return (Agx_Output_Select) {
    .clip_distance_plane_0 = false,
    .clip_distance_plane_1 = false,
    .clip_distance_plane_2 = false,
    .clip_distance_plane_3 = false,
    .clip_distance_plane_4 = false,
    .clip_distance_plane_5 = false,
    .clip_distance_plane_6 = false,
    .clip_distance_plane_7 = false,
    .clip_distance_plane_8 = false,
    .clip_distance_plane_9 = false,
    .clip_distance_plane_10 = false,
    .clip_distance_plane_11 = false,
    .clip_distance_plane_12 = false,
    .clip_distance_plane_13 = false,
    .clip_distance_plane_14 = false,
    .clip_distance_plane_15 = false,
    .varyings = false,
    .unk_17 = 0x0,
    .point_size = false,
    .viewport_target = false,
    .render_target = false,
    .frag_coord_z = false,
    .barycentric_coordinates = false,
    .unk_23 = 0x0,
    .unk_24 = 0x0,
  };
}

static inline uint32_t
agx_output_select_pack(uint32_t* restrict cl, const Agx_Output_Select* restrict values)
{
  cl[0] = agx_cmd_uint(values->clip_distance_plane_0, 0, 0) | agx_cmd_uint(values->clip_distance_plane_1, 1, 1) |
          agx_cmd_uint(values->clip_distance_plane_2, 2, 2) | agx_cmd_uint(values->clip_distance_plane_3, 3, 3) |
          agx_cmd_uint(values->clip_distance_plane_4, 4, 4) | agx_cmd_uint(values->clip_distance_plane_5, 5, 5) |
          agx_cmd_uint(values->clip_distance_plane_6, 6, 6) | agx_cmd_uint(values->clip_distance_plane_7, 7, 7) |
          agx_cmd_uint(values->clip_distance_plane_8, 8, 8) | agx_cmd_uint(values->clip_distance_plane_9, 9, 9) |
          agx_cmd_uint(values->clip_distance_plane_10, 10, 10) | agx_cmd_uint(values->clip_distance_plane_11, 11, 11) |
          agx_cmd_uint(values->clip_distance_plane_12, 12, 12) | agx_cmd_uint(values->clip_distance_plane_13, 13, 13) |
          agx_cmd_uint(values->clip_distance_plane_14, 14, 14) | agx_cmd_uint(values->clip_distance_plane_15, 15, 15) |
          agx_cmd_uint(values->varyings, 16, 16) | agx_cmd_uint(values->unk_17, 17, 17) |
          agx_cmd_uint(values->point_size, 18, 18) | agx_cmd_uint(values->viewport_target, 19, 19) |
          agx_cmd_uint(values->render_target, 20, 20) | agx_cmd_uint(values->frag_coord_z, 21, 21) |
          agx_cmd_uint(values->barycentric_coordinates, 22, 22) | agx_cmd_uint(values->unk_23, 23, 23) |
          agx_cmd_uint(values->unk_24, 24, 31);

  return AGX_OUTPUT_SELECT_LENGTH;
}

static inline void
agx_output_select_unpack(const uint8_t* restrict cl, Agx_Output_Select* restrict values)
{
  values->clip_distance_plane_0 = agx_unpack_uint(cl, 0, 0);
  values->clip_distance_plane_1 = agx_unpack_uint(cl, 1, 1);
  values->clip_distance_plane_2 = agx_unpack_uint(cl, 2, 2);
  values->clip_distance_plane_3 = agx_unpack_uint(cl, 3, 3);
  values->clip_distance_plane_4 = agx_unpack_uint(cl, 4, 4);
  values->clip_distance_plane_5 = agx_unpack_uint(cl, 5, 5);
  values->clip_distance_plane_6 = agx_unpack_uint(cl, 6, 6);
  values->clip_distance_plane_7 = agx_unpack_uint(cl, 7, 7);
  values->clip_distance_plane_8 = agx_unpack_uint(cl, 8, 8);
  values->clip_distance_plane_9 = agx_unpack_uint(cl, 9, 9);
  values->clip_distance_plane_10 = agx_unpack_uint(cl, 10, 10);
  values->clip_distance_plane_11 = agx_unpack_uint(cl, 11, 11);
  values->clip_distance_plane_12 = agx_unpack_uint(cl, 12, 12);
  values->clip_distance_plane_13 = agx_unpack_uint(cl, 13, 13);
  values->clip_distance_plane_14 = agx_unpack_uint(cl, 14, 14);
  values->clip_distance_plane_15 = agx_unpack_uint(cl, 15, 15);
  values->varyings = agx_unpack_uint(cl, 16, 16);
  values->unk_17 = agx_unpack_uint(cl, 17, 17);
  values->point_size = agx_unpack_uint(cl, 18, 18);
  values->viewport_target = agx_unpack_uint(cl, 19, 19);
  values->render_target = agx_unpack_uint(cl, 20, 20);
  values->frag_coord_z = agx_unpack_uint(cl, 21, 21);
  values->barycentric_coordinates = agx_unpack_uint(cl, 22, 22);
  values->unk_23 = agx_unpack_uint(cl, 23, 23);
  values->unk_24 = agx_unpack_uint(cl, 24, 31);
}

typedef struct Agx_Ppp_Control
{
  uint32_t cullmode;
  uint32_t rsvd_0;
  bool     wbuffen;
  bool     wclampen;
  bool     pretransform;
  uint32_t flatshade_vtx;
  bool     drawclippededges;
  uint32_t clip_mode;
  bool     primitive_id_pres;
  bool     ms_fs_buffer_needed;
  uint32_t rsvd_1;
  bool     prim_msaa;
  bool     front_face_dir;
  bool     rasterizer_discard;
  uint32_t rsvd_2;
} Agx_Ppp_Control;

#define AGX_PPP_CONTROL_LENGTH 4

static inline Agx_Ppp_Control
agx_ppp_control_default(void)
{
  return (Agx_Ppp_Control) {
    .cullmode = 0,
    .rsvd_0 = 0x0,
    .wbuffen = false,
    .wclampen = false,
    .pretransform = false,
    .flatshade_vtx = 1,
    .drawclippededges = false,
    .clip_mode = 1,
    .primitive_id_pres = false,
    .ms_fs_buffer_needed = false,
    .rsvd_1 = 0x0,
    .prim_msaa = false,
    .front_face_dir = false,
    .rasterizer_discard = false,
    .rsvd_2 = 0x0,
  };
}

static inline uint32_t
agx_ppp_control_pack(uint32_t* restrict cl, const Agx_Ppp_Control* restrict values)
{
  cl[0] = agx_cmd_uint(values->cullmode, 0, 1) | agx_cmd_uint(values->rsvd_0, 2, 3) | agx_cmd_uint(values->wbuffen, 4, 4) |
          agx_cmd_uint(values->wclampen, 5, 5) | agx_cmd_uint(values->pretransform, 6, 6) |
          agx_cmd_uint(values->flatshade_vtx, 7, 8) | agx_cmd_uint(values->drawclippededges, 9, 9) |
          agx_cmd_uint(values->clip_mode, 10, 11) | agx_cmd_uint(values->primitive_id_pres, 12, 12) |
          agx_cmd_uint(values->ms_fs_buffer_needed, 13, 13) | agx_cmd_uint(values->rsvd_1, 14, 14) |
          agx_cmd_uint(values->prim_msaa, 15, 15) | agx_cmd_uint(values->front_face_dir, 16, 16) |
          agx_cmd_uint(values->rasterizer_discard, 17, 17) | agx_cmd_uint(values->rsvd_2, 18, 31);

  return AGX_PPP_CONTROL_LENGTH;
}

static inline void
agx_ppp_control_unpack(const uint8_t* restrict cl, Agx_Ppp_Control* restrict values)
{
  values->cullmode = agx_unpack_uint(cl, 0, 1);
  values->rsvd_0 = agx_unpack_uint(cl, 2, 3);
  values->wbuffen = agx_unpack_uint(cl, 4, 4);
  values->wclampen = agx_unpack_uint(cl, 5, 5);
  values->pretransform = agx_unpack_uint(cl, 6, 6);
  values->flatshade_vtx = agx_unpack_uint(cl, 7, 8);
  values->drawclippededges = agx_unpack_uint(cl, 9, 9);
  values->clip_mode = agx_unpack_uint(cl, 10, 11);
  values->primitive_id_pres = agx_unpack_uint(cl, 12, 12);
  values->ms_fs_buffer_needed = agx_unpack_uint(cl, 13, 13);
  values->rsvd_1 = agx_unpack_uint(cl, 14, 14);
  values->prim_msaa = agx_unpack_uint(cl, 15, 15);
  values->front_face_dir = agx_unpack_uint(cl, 16, 16);
  values->rasterizer_discard = agx_unpack_uint(cl, 17, 17);
  values->rsvd_2 = agx_unpack_uint(cl, 18, 31);
}

typedef struct Agx_Region_Clip
{
  uint32_t x_max;
  uint32_t x_min;
  bool     enable;
  uint32_t y_max;
  uint32_t y_min;
} Agx_Region_Clip;

#define AGX_REGION_CLIP_LENGTH 8

static inline Agx_Region_Clip
agx_region_clip_default(void)
{
  return (Agx_Region_Clip) {
    .enable = true,
  };
}

static inline uint32_t
agx_region_clip_pack(uint32_t* restrict cl, const Agx_Region_Clip* restrict values)
{
  cl[0] = agx_cmd_uint(values->x_max, 0, 15) | agx_cmd_uint(values->x_min, 16, 30) | agx_cmd_uint(values->enable, 31, 31);
  cl[1] = agx_cmd_uint(values->y_max, 0, 15) | agx_cmd_uint(values->y_min, 16, 31);

  return AGX_REGION_CLIP_LENGTH;
}

static inline void
agx_region_clip_unpack(const uint8_t* restrict cl, Agx_Region_Clip* restrict values)
{
  values->x_max = agx_unpack_uint(cl, 0, 15);
  values->x_min = agx_unpack_uint(cl, 16, 30);
  values->enable = agx_unpack_uint(cl, 31, 31);
  values->y_max = agx_unpack_uint(cl, 32, 47);
  values->y_min = agx_unpack_uint(cl, 48, 63);
}

typedef struct Agx_Viewport_Control
{
  uint32_t last_viewport;
} Agx_Viewport_Control;

#define AGX_VIEWPORT_CONTROL_LENGTH 4

static inline Agx_Viewport_Control
agx_viewport_control_default(void)
{
  return (Agx_Viewport_Control) {0};
}

static inline uint32_t
agx_viewport_control_pack(uint32_t* restrict cl, const Agx_Viewport_Control* restrict values)
{
  assert(values->last_viewport >= 1);
  cl[0] = agx_cmd_uint(values->last_viewport - 1, 0, 31);

  return AGX_VIEWPORT_CONTROL_LENGTH;
}

static inline void
agx_viewport_control_unpack(const uint8_t* restrict cl, Agx_Viewport_Control* restrict values)
{
  values->last_viewport = agx_unpack_uint(cl, 0, 31) + 1;
}

typedef enum Agx_Address_Mode
{
  AGX_ADDRESS_MODE_CLAMP_TO_EDGE = 0,
  AGX_ADDRESS_MODE_REPEAT = 1,
  AGX_ADDRESS_MODE_MIRROR_REPEAT = 2,
  AGX_ADDRESS_MODE_CLAMP_TO_BORDER_COLOR = 3,
  AGX_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE = 5,
} Agx_Address_Mode;

typedef struct Agx_Render_Target
{
  Agx_Image_Dimension dimension;
  Agx_Image_Layout    layout;
  Agx_Image_Channels  channels;
  uint32_t            unk_13;
  bool                is_float;
  Agx_Channel         swizzle_r;
  Agx_Channel         swizzle_g;
  Agx_Channel         swizzle_b;
  Agx_Channel         swizzle_a;
  uint32_t            width;
  uint32_t            height;
  bool                unk_52;
  bool                rotate_90;
  bool                transpose;
  uint32_t            unk_55;
  bool                multisample_4x;
  uint32_t            unk_57;
  bool                mipmapped;
  bool                compress;
  uint32_t            unk_60;
  uint64_t            buffer;
  uint32_t            unk_101;
  uint32_t            tiles_per_row;
  uint32_t            unk_118;
  bool                layered;
  bool                srgb;
  uint32_t            unk_126;
  bool                extended;
} Agx_Render_Target;

#define AGX_RENDER_TARGET_LENGTH 16

static inline Agx_Render_Target
agx_render_target_default(void)
{
  return (Agx_Render_Target) {
    .is_float = false,
    .rotate_90 = false,
    .transpose = false,
    .multisample_4x = false,
    .mipmapped = false,
    .compress = false,
    .layered = false,
    .srgb = false,
    .extended = false,
  };
}

static inline uint32_t
agx_render_target_pack(uint32_t* restrict cl, const Agx_Render_Target* restrict values)
{
  assert(values->width >= 1);
  assert(values->height >= 1);
  assert((values->buffer & 0xf) == 0);
  assert(values->tiles_per_row >= 1);
  cl[0] = agx_cmd_uint(values->dimension, 0, 3) | agx_cmd_uint(values->layout, 4, 5) |
          agx_cmd_uint(values->channels, 6, 12) | agx_cmd_uint(values->unk_13, 13, 14) |
          agx_cmd_uint(values->is_float, 15, 15) | agx_cmd_uint(values->swizzle_r, 16, 17) |
          agx_cmd_uint(values->swizzle_g, 18, 19) | agx_cmd_uint(values->swizzle_b, 20, 21) |
          agx_cmd_uint(values->swizzle_a, 22, 23) | agx_cmd_uint(values->width - 1, 24, 37);
  cl[1] = agx_cmd_uint(values->width - 1, 24, 37) >> 32 | agx_cmd_uint(values->height - 1, 6, 19) |
          agx_cmd_uint(values->unk_52, 20, 20) | agx_cmd_uint(values->rotate_90, 21, 21) |
          agx_cmd_uint(values->transpose, 22, 22) | agx_cmd_uint(values->unk_55, 23, 23) |
          agx_cmd_uint(values->multisample_4x, 24, 24) | agx_cmd_uint(values->unk_57, 25, 25) |
          agx_cmd_uint(values->mipmapped, 26, 26) | agx_cmd_uint(values->compress, 27, 27) |
          agx_cmd_uint(values->unk_60, 28, 31);
  cl[2] = agx_cmd_uint(values->buffer >> 4, 0, 36);
  cl[3] = agx_cmd_uint(values->buffer >> 4, 0, 36) >> 32 | agx_cmd_uint(values->unk_101, 5, 11) |
          agx_cmd_uint(values->tiles_per_row - 1, 12, 21) | agx_cmd_uint(values->unk_118, 22, 27) |
          agx_cmd_uint(values->layered, 28, 28) | agx_cmd_uint(values->srgb, 29, 29) |
          agx_cmd_uint(values->unk_126, 30, 30) | agx_cmd_uint(values->extended, 31, 31);

  return AGX_RENDER_TARGET_LENGTH;
}

static inline void
agx_render_target_unpack(const uint8_t* restrict cl, Agx_Render_Target* restrict values)
{
  values->dimension = (Agx_Image_Dimension)agx_unpack_uint(cl, 0, 3);
  values->layout = (Agx_Image_Layout)agx_unpack_uint(cl, 4, 5);
  values->channels = (Agx_Image_Channels)agx_unpack_uint(cl, 6, 12);
  values->unk_13 = agx_unpack_uint(cl, 13, 14);
  values->is_float = agx_unpack_uint(cl, 15, 15);
  values->swizzle_r = (Agx_Channel)agx_unpack_uint(cl, 16, 17);
  values->swizzle_g = (Agx_Channel)agx_unpack_uint(cl, 18, 19);
  values->swizzle_b = (Agx_Channel)agx_unpack_uint(cl, 20, 21);
  values->swizzle_a = (Agx_Channel)agx_unpack_uint(cl, 22, 23);
  values->width = agx_unpack_uint(cl, 24, 37) + 1;
  values->height = agx_unpack_uint(cl, 38, 51) + 1;
  values->unk_52 = agx_unpack_uint(cl, 52, 52);
  values->rotate_90 = agx_unpack_uint(cl, 53, 53);
  values->transpose = agx_unpack_uint(cl, 54, 54);
  values->unk_55 = agx_unpack_uint(cl, 55, 55);
  values->multisample_4x = agx_unpack_uint(cl, 56, 56);
  values->unk_57 = agx_unpack_uint(cl, 57, 57);
  values->mipmapped = agx_unpack_uint(cl, 58, 58);
  values->compress = agx_unpack_uint(cl, 59, 59);
  values->unk_60 = agx_unpack_uint(cl, 60, 63);
  values->buffer = agx_unpack_uint(cl, 64, 100) << 4;
  values->unk_101 = agx_unpack_uint(cl, 101, 107);
  values->tiles_per_row = agx_unpack_uint(cl, 108, 117) + 1;
  values->unk_118 = agx_unpack_uint(cl, 118, 123);
  values->layered = agx_unpack_uint(cl, 124, 124);
  values->srgb = agx_unpack_uint(cl, 125, 125);
  values->unk_126 = agx_unpack_uint(cl, 126, 126);
  values->extended = agx_unpack_uint(cl, 127, 127);
}

typedef struct Agx_Image_Descriptor_Trailer
{
  uint64_t metadata_shifted;
  uint32_t base_level;
  uint32_t last_level;
  uint32_t unk_07;
} Agx_Image_Descriptor_Trailer;

#define AGX_IMAGE_DESCRIPTOR_TRAILER_LENGTH 8

static inline Agx_Image_Descriptor_Trailer
agx_image_descriptor_trailer_default(void)
{
  return (Agx_Image_Descriptor_Trailer) {0};
}

static inline uint32_t
agx_image_descriptor_trailer_pack(uint32_t* restrict cl, const Agx_Image_Descriptor_Trailer* restrict values)
{
  assert((values->metadata_shifted & 0xf) == 0);
  cl[0] = agx_cmd_uint(values->metadata_shifted >> 4, 0, 43);
  cl[1] = agx_cmd_uint(values->metadata_shifted >> 4, 0, 43) >> 32 | agx_cmd_uint(values->base_level, 12, 15) |
          agx_cmd_uint(values->last_level, 16, 23) | agx_cmd_uint(values->unk_07, 24, 31);

  return AGX_IMAGE_DESCRIPTOR_TRAILER_LENGTH;
}

static inline void
agx_image_descriptor_trailer_unpack(const uint8_t* restrict cl, Agx_Image_Descriptor_Trailer* restrict values)
{
  values->metadata_shifted = agx_unpack_uint(cl, 0, 43) << 4;
  values->base_level = agx_unpack_uint(cl, 44, 47);
  values->last_level = agx_unpack_uint(cl, 48, 55);
  values->unk_07 = agx_unpack_uint(cl, 56, 63);
}

typedef enum Agx_Sampler_Filter
{
  AGX_SAMPLER_FILTER_NEAREST = 0,
  AGX_SAMPLER_FILTER_LINEAR = 1,
} Agx_Sampler_Filter;

typedef enum Agx_Sampler_Mip_Filter
{
  AGX_SAMPLER_MIP_FILTER_NOT_MIPMAPPED = 0,
  AGX_SAMPLER_MIP_FILTER_NEAREST = 1,
  AGX_SAMPLER_MIP_FILTER_LINEAR = 2,
} Agx_Sampler_Mip_Filter;

typedef enum Agx_Sampler_Border_Mode
{
  AGX_SAMPLER_BORDER_MODE_TRANSPARENT_BLACK = 0,
  AGX_SAMPLER_BORDER_MODE_OPAQUE_BLACK = 1,
  AGX_SAMPLER_BORDER_MODE_OPAQUE_WHITE = 2,
} Agx_Sampler_Border_Mode;

typedef enum Agx_Sampler_Compare_Func
{
  AGX_SAMPLER_COMPARE_FUNC_LESS_EQUAL = 0,
  AGX_SAMPLER_COMPARE_FUNC_GREATER_EQUAL = 1,
  AGX_SAMPLER_COMPARE_FUNC_LESS = 2,
  AGX_SAMPLER_COMPARE_FUNC_GREATER = 3,
  AGX_SAMPLER_COMPARE_FUNC_EQUAL = 4,
  AGX_SAMPLER_COMPARE_FUNC_NOT_EQUAL = 5,
  AGX_SAMPLER_COMPARE_FUNC_ALWAYS = 6,
  AGX_SAMPLER_COMPARE_FUNC_NEVER = 7,
} Agx_Sampler_Compare_Func;

typedef struct Agx_Sampler
{
  uint32_t                 min_lod;
  uint32_t                 max_lod;
  uint32_t                 max_aniso;
  Agx_Sampler_Filter       mag_filter;
  Agx_Sampler_Filter       min_filter;
  Agx_Sampler_Mip_Filter   mip_filter;
  Agx_Address_Mode         wrap_s;
  Agx_Address_Mode         wrap_t;
  Agx_Address_Mode         wrap_r;
  bool                     non_normalized_coords;
  Agx_Sampler_Compare_Func compare_func;
  bool                     compare_mode;
  uint32_t                 lod_bias;
  uint32_t                 tri_adjust;
  uint32_t                 aniso_adjust;
  bool                     alpha_border_mode;
  Agx_Sampler_Border_Mode  border_mode;
  uint32_t                 border_color_0;
  uint32_t                 border_color_1;
  uint32_t                 border_color_2;
  uint32_t                 border_color_3;
  uint32_t                 unk_192;
  uint32_t                 unk_224;
} Agx_Sampler;

#define AGX_SAMPLER_LENGTH 32

static inline Agx_Sampler
agx_sampler_default(void)
{
  return (Agx_Sampler) {0};
}

static inline uint32_t
agx_sampler_pack(uint32_t* restrict cl, const Agx_Sampler* restrict values)
{
  cl[0] = agx_cmd_uint(values->min_lod, 0, 9) | agx_cmd_uint(values->max_lod, 10, 19) |
          agx_cmd_uint(values->max_aniso, 20, 22) | agx_cmd_uint(values->mag_filter, 23, 24) |
          agx_cmd_uint(values->min_filter, 25, 26) | agx_cmd_uint(values->mip_filter, 27, 28) |
          agx_cmd_uint(values->wrap_s, 29, 31);
  cl[1] = agx_cmd_uint(values->wrap_t, 0, 2) | agx_cmd_uint(values->wrap_r, 3, 5) |
          agx_cmd_uint(values->non_normalized_coords, 6, 6) | agx_cmd_uint(values->compare_func, 7, 9) |
          agx_cmd_uint(values->compare_mode, 10, 10) | agx_cmd_uint(values->lod_bias, 11, 21) |
          agx_cmd_uint(values->tri_adjust, 22, 24) | agx_cmd_uint(values->aniso_adjust, 25, 27) |
          agx_cmd_uint(values->alpha_border_mode, 28, 28) | agx_cmd_uint(values->border_mode, 29, 31);
  cl[2] = agx_cmd_uint(values->border_color_0, 0, 31);
  cl[3] = agx_cmd_uint(values->border_color_1, 0, 31);
  cl[4] = agx_cmd_uint(values->border_color_2, 0, 31);
  cl[5] = agx_cmd_uint(values->border_color_3, 0, 31);
  cl[6] = agx_cmd_uint(values->unk_192, 0, 31);
  cl[7] = agx_cmd_uint(values->unk_224, 0, 31);

  return AGX_SAMPLER_LENGTH;
}

static inline void
agx_sampler_unpack(const uint8_t* restrict cl, Agx_Sampler* restrict values)
{
  values->min_lod = agx_unpack_uint(cl, 0, 9);
  values->max_lod = agx_unpack_uint(cl, 10, 19);
  values->max_aniso = agx_unpack_uint(cl, 20, 22);
  values->mag_filter = (Agx_Sampler_Filter)agx_unpack_uint(cl, 23, 24);
  values->min_filter = (Agx_Sampler_Filter)agx_unpack_uint(cl, 25, 26);
  values->mip_filter = (Agx_Sampler_Mip_Filter)agx_unpack_uint(cl, 27, 28);
  values->wrap_s = (Agx_Address_Mode)agx_unpack_uint(cl, 29, 31);
  values->wrap_t = (Agx_Address_Mode)agx_unpack_uint(cl, 32, 34);
  values->wrap_r = (Agx_Address_Mode)agx_unpack_uint(cl, 35, 37);
  values->non_normalized_coords = agx_unpack_uint(cl, 38, 38);
  values->compare_func = (Agx_Sampler_Compare_Func)agx_unpack_uint(cl, 39, 41);
  values->compare_mode = agx_unpack_uint(cl, 42, 42);
  values->lod_bias = agx_unpack_uint(cl, 43, 53);
  values->tri_adjust = agx_unpack_uint(cl, 54, 56);
  values->aniso_adjust = agx_unpack_uint(cl, 57, 59);
  values->alpha_border_mode = agx_unpack_uint(cl, 60, 60);
  values->border_mode = (Agx_Sampler_Border_Mode)agx_unpack_uint(cl, 61, 63);
  values->border_color_0 = agx_unpack_uint(cl, 64, 95);
  values->border_color_1 = agx_unpack_uint(cl, 96, 127);
  values->border_color_2 = agx_unpack_uint(cl, 128, 159);
  values->border_color_3 = agx_unpack_uint(cl, 160, 191);
  values->unk_192 = agx_unpack_uint(cl, 192, 223);
  values->unk_224 = agx_unpack_uint(cl, 224, 255);
}

typedef struct Agx_Render_Target_Metadata
{
  uint64_t buffer;
  uint32_t unk_37;
  uint32_t unk_64;
  uint32_t unk_96;
} Agx_Render_Target_Metadata;

#define AGX_RENDER_TARGET_METADATA_LENGTH 16

static inline Agx_Render_Target_Metadata
agx_render_target_metadata_default(void)
{
  return (Agx_Render_Target_Metadata) {0};
}

static inline uint32_t
agx_render_target_metadata_pack(uint32_t* restrict cl, const Agx_Render_Target_Metadata* restrict values)
{
  assert((values->buffer & 0xf) == 0);
  cl[0] = agx_cmd_uint(values->buffer >> 4, 0, 36);
  cl[1] = agx_cmd_uint(values->buffer >> 4, 0, 36) >> 32 | agx_cmd_uint(values->unk_37, 5, 31);
  cl[2] = agx_cmd_uint(values->unk_64, 0, 31);
  cl[3] = agx_cmd_uint(values->unk_96, 0, 31);

  return AGX_RENDER_TARGET_METADATA_LENGTH;
}

static inline void
agx_render_target_metadata_unpack(const uint8_t* restrict cl, Agx_Render_Target_Metadata* restrict values)
{
  values->buffer = agx_unpack_uint(cl, 0, 36) << 4;
  values->unk_37 = agx_unpack_uint(cl, 37, 63);
  values->unk_64 = agx_unpack_uint(cl, 64, 95);
  values->unk_96 = agx_unpack_uint(cl, 96, 127);
}

typedef struct Agx_Scissor
{
  uint32_t x_max;
  uint32_t x_min;
  uint32_t y_max;
  uint32_t y_min;
  float    z_min;
  float    z_max;
} Agx_Scissor;

#define AGX_SCISSOR_LENGTH 16

static inline Agx_Scissor
agx_scissor_default(void)
{
  return (Agx_Scissor) {
    .z_min = 0.0,
    .z_max = 1.0,
  };
}

static inline uint32_t
agx_scissor_pack(uint32_t* restrict cl, const Agx_Scissor* restrict values)
{
  cl[0] = agx_cmd_uint(values->x_max, 0, 15) | agx_cmd_uint(values->x_min, 16, 31);
  cl[1] = agx_cmd_uint(values->y_max, 0, 15) | agx_cmd_uint(values->y_min, 16, 31);
  cl[2] = agx_cmd_uint(agx_float_bits(values->z_min), 0, 32);
  cl[3] = agx_cmd_uint(agx_float_bits(values->z_max), 0, 32);

  return AGX_SCISSOR_LENGTH;
}

static inline void
agx_scissor_unpack(const uint8_t* restrict cl, Agx_Scissor* restrict values)
{
  values->x_max = agx_unpack_uint(cl, 0, 15);
  values->x_min = agx_unpack_uint(cl, 16, 31);
  values->y_max = agx_unpack_uint(cl, 32, 47);
  values->y_min = agx_unpack_uint(cl, 48, 63);
  values->z_min = agx_unpack_float(cl, 64, 95);
  values->z_max = agx_unpack_float(cl, 96, 127);
}

typedef struct Agx_Ppp_Frg_Dbsc
{
  uint32_t scissor_index;
  uint32_t depth_bias_index;
} Agx_Ppp_Frg_Dbsc;

#define AGX_PPP_FRG_DBSC_LENGTH 4

static inline Agx_Ppp_Frg_Dbsc
agx_ppp_frg_dbsc_default(void)
{
  return (Agx_Ppp_Frg_Dbsc) {0};
}

static inline uint32_t
agx_ppp_frg_dbsc_pack(uint32_t* restrict cl, const Agx_Ppp_Frg_Dbsc* restrict values)
{
  cl[0] = agx_cmd_uint(values->scissor_index, 0, 15) | agx_cmd_uint(values->depth_bias_index, 16, 31);

  return AGX_PPP_FRG_DBSC_LENGTH;
}

static inline void
agx_ppp_frg_dbsc_unpack(const uint8_t* restrict cl, Agx_Ppp_Frg_Dbsc* restrict values)
{
  values->scissor_index = agx_unpack_uint(cl, 0, 15);
  values->depth_bias_index = agx_unpack_uint(cl, 16, 31);
}

typedef struct Agx_Depth_Bias
{
  float constant;
  float slope_scale;
  float clamp;
} Agx_Depth_Bias;

#define AGX_DEPTH_BIAS_LENGTH 12

static inline Agx_Depth_Bias
agx_depth_bias_default(void)
{
  return (Agx_Depth_Bias) {
    .constant = 0.0,
    .slope_scale = 0.0,
    .clamp = 0.0,
  };
}

static inline uint32_t
agx_depth_bias_pack(uint32_t* restrict cl, const Agx_Depth_Bias* restrict values)
{
  cl[0] = agx_cmd_uint(agx_float_bits(values->constant), 0, 32);
  cl[1] = agx_cmd_uint(agx_float_bits(values->slope_scale), 0, 32);
  cl[2] = agx_cmd_uint(agx_float_bits(values->clamp), 0, 32);

  return AGX_DEPTH_BIAS_LENGTH;
}

static inline void
agx_depth_bias_unpack(const uint8_t* restrict cl, Agx_Depth_Bias* restrict values)
{
  values->constant = agx_unpack_float(cl, 0, 31);
  values->slope_scale = agx_unpack_float(cl, 32, 63);
  values->clamp = agx_unpack_float(cl, 64, 95);
}

typedef struct Agx_Viewport
{
  float translate_x;
  float scale_x;
  float translate_y;
  float scale_y;
  float near_z;
  float far_z;
} Agx_Viewport;

#define AGX_VIEWPORT_LENGTH 24

static inline Agx_Viewport
agx_viewport_default(void)
{
  return (Agx_Viewport) {0};
}

static inline uint32_t
agx_viewport_pack(uint32_t* restrict cl, const Agx_Viewport* restrict values)
{
  cl[0] = agx_cmd_uint(agx_float_bits(values->translate_x), 0, 32);
  cl[1] = agx_cmd_uint(agx_float_bits(values->scale_x), 0, 32);
  cl[2] = agx_cmd_uint(agx_float_bits(values->translate_y), 0, 32);
  cl[3] = agx_cmd_uint(agx_float_bits(values->scale_y), 0, 32);
  cl[4] = agx_cmd_uint(agx_float_bits(values->near_z), 0, 32);
  cl[5] = agx_cmd_uint(agx_float_bits(values->far_z), 0, 32);

  return AGX_VIEWPORT_LENGTH;
}

static inline void
agx_viewport_unpack(const uint8_t* restrict cl, Agx_Viewport* restrict values)
{
  values->translate_x = agx_unpack_float(cl, 0, 31);
  values->scale_x = agx_unpack_float(cl, 32, 63);
  values->translate_y = agx_unpack_float(cl, 64, 95);
  values->scale_y = agx_unpack_float(cl, 96, 127);
  values->near_z = agx_unpack_float(cl, 128, 159);
  values->far_z = agx_unpack_float(cl, 160, 191);
}

typedef struct Agx_Linkage
{
  uint32_t tag;
  uint32_t unk_1;
  uint32_t unk_2;
  uint32_t varying_count;
} Agx_Linkage;

#define AGX_LINKAGE_LENGTH 16

static inline Agx_Linkage
agx_linkage_default(void)
{
  return (Agx_Linkage) {
    .tag = 0xC020000,
    .unk_1 = 0x100,
    .unk_2 = 0x0,
  };
}

static inline uint32_t
agx_linkage_pack(uint32_t* restrict cl, const Agx_Linkage* restrict values)
{
  cl[0] = agx_cmd_uint(values->tag, 0, 31);
  cl[1] = agx_cmd_uint(values->unk_1, 0, 31);
  cl[2] = agx_cmd_uint(values->unk_2, 0, 31);
  cl[3] = agx_cmd_uint(values->varying_count, 0, 31);

  return AGX_LINKAGE_LENGTH;
}

static inline void
agx_linkage_unpack(const uint8_t* restrict cl, Agx_Linkage* restrict values)
{
  values->tag = agx_unpack_uint(cl, 0, 31);
  values->unk_1 = agx_unpack_uint(cl, 32, 63);
  values->unk_2 = agx_unpack_uint(cl, 64, 95);
  values->varying_count = agx_unpack_uint(cl, 96, 127);
}

typedef enum Agx_Usc_Control
{
  AGX_USC_CONTROL_SHADER = 13,
  AGX_USC_CONTROL_UNIFORM = 29,
  AGX_USC_CONTROL_PRESHADER = 56,
  AGX_USC_CONTROL_UNIFORM_HIGH = 61,
  AGX_USC_CONTROL_SHARED = 77,
  AGX_USC_CONTROL_FRAGMENT_PROPERTIES = 88,
  AGX_USC_CONTROL_NO_PRESHADER = 136,
  AGX_USC_CONTROL_REGISTERS = 141,
  AGX_USC_CONTROL_SAMPLER = 157,
  AGX_USC_CONTROL_TEXTURE = 221,
} Agx_Usc_Control;

typedef struct Agx_Usc_Uniform
{
  Agx_Usc_Control tag;
  uint32_t        start_halfs;
  uint32_t        unk_16;
  uint32_t        size_halfs;
  uint64_t        buffer;
} Agx_Usc_Uniform;

#define AGX_USC_UNIFORM_LENGTH 8

static inline Agx_Usc_Uniform
agx_usc_uniform_default(void)
{
  return (Agx_Usc_Uniform) {
    .tag = AGX_USC_CONTROL_UNIFORM,
    .unk_16 = 0x0,
  };
}

static inline uint32_t
agx_usc_uniform_pack(uint32_t* restrict cl, const Agx_Usc_Uniform* restrict values)
{
  assert((values->buffer & 0x3) == 0);
  cl[0] = agx_cmd_uint(values->tag, 0, 7) | agx_cmd_uint(values->start_halfs, 8, 15) |
          agx_cmd_uint(values->unk_16, 16, 19) | agx_cmd_uint(values->size_halfs, 20, 25) |
          agx_cmd_uint(values->buffer >> 2, 26, 63);
  cl[1] = agx_cmd_uint(values->buffer >> 2, 26, 63) >> 32;

  return AGX_USC_UNIFORM_LENGTH;
}

static inline void
agx_usc_uniform_unpack(const uint8_t* restrict cl, Agx_Usc_Uniform* restrict values)
{
  values->tag = (Agx_Usc_Control)agx_unpack_uint(cl, 0, 7);
  values->start_halfs = agx_unpack_uint(cl, 8, 15);
  values->unk_16 = agx_unpack_uint(cl, 16, 19);
  values->size_halfs = agx_unpack_uint(cl, 20, 25);
  values->buffer = agx_unpack_uint(cl, 26, 63) << 2;
}

typedef struct Agx_Usc_Uniform_High
{
  Agx_Usc_Control tag;
  uint32_t        start_halfs;
  uint32_t        unk_16;
  uint32_t        size_halfs;
  uint64_t        buffer;
} Agx_Usc_Uniform_High;

#define AGX_USC_UNIFORM_HIGH_LENGTH 8

static inline Agx_Usc_Uniform_High
agx_usc_uniform_high_default(void)
{
  return (Agx_Usc_Uniform_High) {
    .tag = AGX_USC_CONTROL_UNIFORM_HIGH,
    .unk_16 = 0x0,
  };
}

static inline uint32_t
agx_usc_uniform_high_pack(uint32_t* restrict cl, const Agx_Usc_Uniform_High* restrict values)
{
  assert((values->buffer & 0x3) == 0);
  cl[0] = agx_cmd_uint(values->tag, 0, 7) | agx_cmd_uint(values->start_halfs, 8, 15) |
          agx_cmd_uint(values->unk_16, 16, 19) | agx_cmd_uint(values->size_halfs, 20, 25) |
          agx_cmd_uint(values->buffer >> 2, 26, 63);
  cl[1] = agx_cmd_uint(values->buffer >> 2, 26, 63) >> 32;

  return AGX_USC_UNIFORM_HIGH_LENGTH;
}

static inline void
agx_usc_uniform_high_unpack(const uint8_t* restrict cl, Agx_Usc_Uniform_High* restrict values)
{
  values->tag = (Agx_Usc_Control)agx_unpack_uint(cl, 0, 7);
  values->start_halfs = agx_unpack_uint(cl, 8, 15);
  values->unk_16 = agx_unpack_uint(cl, 16, 19);
  values->size_halfs = agx_unpack_uint(cl, 20, 25);
  values->buffer = agx_unpack_uint(cl, 26, 63) << 2;
}

typedef struct Agx_Usc_Texture
{
  Agx_Usc_Control tag;
  uint32_t        start;
  uint32_t        unk_16;
  uint32_t        count;
  uint64_t        buffer;
} Agx_Usc_Texture;

#define AGX_USC_TEXTURE_LENGTH 8

static inline Agx_Usc_Texture
agx_usc_texture_default(void)
{
  return (Agx_Usc_Texture) {
    .tag = AGX_USC_CONTROL_TEXTURE,
    .unk_16 = 0x0,
  };
}

static inline uint32_t
agx_usc_texture_pack(uint32_t* restrict cl, const Agx_Usc_Texture* restrict values)
{
  assert((values->buffer & 0x7) == 0);
  cl[0] = agx_cmd_uint(values->tag, 0, 7) | agx_cmd_uint(values->start, 8, 15) | agx_cmd_uint(values->unk_16, 16, 19) |
          agx_cmd_uint(values->count, 20, 26) | agx_cmd_uint(values->buffer >> 3, 27, 62);
  cl[1] = agx_cmd_uint(values->buffer >> 3, 27, 62) >> 32;

  return AGX_USC_TEXTURE_LENGTH;
}

static inline void
agx_usc_texture_unpack(const uint8_t* restrict cl, Agx_Usc_Texture* restrict values)
{
  values->tag = (Agx_Usc_Control)agx_unpack_uint(cl, 0, 7);
  values->start = agx_unpack_uint(cl, 8, 15);
  values->unk_16 = agx_unpack_uint(cl, 16, 19);
  values->count = agx_unpack_uint(cl, 20, 26);
  values->buffer = agx_unpack_uint(cl, 27, 62) << 3;
}

typedef struct Agx_Usc_Sampler
{
  Agx_Usc_Control tag;
  uint32_t        start;
  uint32_t        unk_16;
  uint32_t        count;
  uint64_t        buffer;
} Agx_Usc_Sampler;

#define AGX_USC_SAMPLER_LENGTH 8

static inline Agx_Usc_Sampler
agx_usc_sampler_default(void)
{
  return (Agx_Usc_Sampler) {
    .tag = AGX_USC_CONTROL_SAMPLER,
    .unk_16 = 0x0,
  };
}

static inline uint32_t
agx_usc_sampler_pack(uint32_t* restrict cl, const Agx_Usc_Sampler* restrict values)
{
  assert((values->buffer & 0x7) == 0);
  cl[0] = agx_cmd_uint(values->tag, 0, 7) | agx_cmd_uint(values->start, 8, 15) | agx_cmd_uint(values->unk_16, 16, 19) |
          agx_cmd_uint(values->count, 20, 26) | agx_cmd_uint(values->buffer >> 3, 27, 62);
  cl[1] = agx_cmd_uint(values->buffer >> 3, 27, 62) >> 32;

  return AGX_USC_SAMPLER_LENGTH;
}

static inline void
agx_usc_sampler_unpack(const uint8_t* restrict cl, Agx_Usc_Sampler* restrict values)
{
  values->tag = (Agx_Usc_Control)agx_unpack_uint(cl, 0, 7);
  values->start = agx_unpack_uint(cl, 8, 15);
  values->unk_16 = agx_unpack_uint(cl, 16, 19);
  values->count = agx_unpack_uint(cl, 20, 26);
  values->buffer = agx_unpack_uint(cl, 27, 62) << 3;
}

typedef enum Agx_Usc_Shared_Layout
{
  AGX_USC_SHARED_LAYOUT_VERTEX_OR_COMPUTE = 36,
  AGX_USC_SHARED_LAYOUT_TILE_16X16 = 54,
  AGX_USC_SHARED_LAYOUT_TILE_32X32 = 47,
  AGX_USC_SHARED_LAYOUT_TILE_32X16 = 63,
} Agx_Usc_Shared_Layout;

typedef struct Agx_Usc_Shared
{
  Agx_Usc_Control       tag;
  bool                  uses_shared_memory;
  uint32_t              unk_9;
  Agx_Usc_Shared_Layout layout;
  uint32_t              sample_count_log2;
  uint32_t              sample_stride_in_8_bytes;
  uint32_t              bytes_per_threadgroup_over_256;
} Agx_Usc_Shared;

#define AGX_USC_SHARED_LENGTH 4

static inline Agx_Usc_Shared
agx_usc_shared_default(void)
{
  return (Agx_Usc_Shared) {
    .tag = AGX_USC_CONTROL_SHARED,
    .uses_shared_memory = false,
    .unk_9 = 0x0,
    .layout = AGX_USC_SHARED_LAYOUT_VERTEX_OR_COMPUTE,
  };
}

static inline uint32_t
agx_usc_shared_pack(uint32_t* restrict cl, const Agx_Usc_Shared* restrict values)
{
  cl[0] = agx_cmd_uint(values->tag, 0, 7) | agx_cmd_uint(values->uses_shared_memory, 8, 8) |
          agx_cmd_uint(values->unk_9, 9, 9) | agx_cmd_uint(values->layout, 10, 15) |
          agx_cmd_uint(values->sample_count_log2, 16, 17) | agx_cmd_uint(values->sample_stride_in_8_bytes, 20, 23) |
          agx_cmd_uint(values->bytes_per_threadgroup_over_256, 24, 31);

  return AGX_USC_SHARED_LENGTH;
}

static inline void
agx_usc_shared_unpack(const uint8_t* restrict cl, Agx_Usc_Shared* restrict values)
{
  values->tag = (Agx_Usc_Control)agx_unpack_uint(cl, 0, 7);
  values->uses_shared_memory = agx_unpack_uint(cl, 8, 8);
  values->unk_9 = agx_unpack_uint(cl, 9, 9);
  values->layout = (Agx_Usc_Shared_Layout)agx_unpack_uint(cl, 10, 15);
  values->sample_count_log2 = agx_unpack_uint(cl, 16, 17);
  values->sample_stride_in_8_bytes = agx_unpack_uint(cl, 20, 23);
  values->bytes_per_threadgroup_over_256 = agx_unpack_uint(cl, 24, 31);
}

typedef struct Agx_Usc_Shader
{
  Agx_Usc_Control tag;
  bool            loads_varyings;
  bool            unk_9;
  uint32_t        unk_10;
  uint64_t        code;
} Agx_Usc_Shader;

#define AGX_USC_SHADER_LENGTH 6

static inline Agx_Usc_Shader
agx_usc_shader_default(void)
{
  return (Agx_Usc_Shader) {
    .tag = AGX_USC_CONTROL_SHADER,
    .loads_varyings = false,
    .unk_9 = false,
  };
}

static inline uint32_t
agx_usc_shader_pack(uint32_t* restrict cl, const Agx_Usc_Shader* restrict values)
{
  cl[0] = agx_cmd_uint(values->tag, 0, 7) | agx_cmd_uint(values->loads_varyings, 8, 8) |
          agx_cmd_uint(values->unk_9, 9, 9) | agx_cmd_uint(values->unk_10, 10, 15) | agx_cmd_uint(values->code, 16, 47);
  cl[1] = agx_cmd_uint(values->code, 16, 47) >> 32;

  return AGX_USC_SHADER_LENGTH;
}

static inline void
agx_usc_shader_unpack(const uint8_t* restrict cl, Agx_Usc_Shader* restrict values)
{
  values->tag = (Agx_Usc_Control)agx_unpack_uint(cl, 0, 7);
  values->loads_varyings = agx_unpack_uint(cl, 8, 8);
  values->unk_9 = agx_unpack_uint(cl, 9, 9);
  values->unk_10 = agx_unpack_uint(cl, 10, 15);
  values->code = agx_unpack_uint(cl, 16, 47);
}

typedef struct Agx_Usc_Registers
{
  Agx_Usc_Control tag;
  uint32_t        register_count_over_8;
  bool            unk_13;
  uint32_t        unk_14;
  uint32_t        spill_size;
  uint32_t        unk_22;
  uint32_t        unk_24;
} Agx_Usc_Registers;

#define AGX_USC_REGISTERS_LENGTH 4

static inline Agx_Usc_Registers
agx_usc_registers_default(void)
{
  return (Agx_Usc_Registers) {
    .tag = AGX_USC_CONTROL_REGISTERS,
    .unk_13 = false,
    .unk_14 = 0x0,
    .spill_size = 0x0,
    .unk_22 = 0x0,
    .unk_24 = 0x0,
  };
}

static inline uint32_t
agx_usc_registers_pack(uint32_t* restrict cl, const Agx_Usc_Registers* restrict values)
{
  cl[0] = agx_cmd_uint(values->tag, 0, 7) | agx_cmd_uint(values->register_count_over_8, 8, 12) |
          agx_cmd_uint(values->unk_13, 13, 13) | agx_cmd_uint(values->unk_14, 14, 17) |
          agx_cmd_uint(values->spill_size, 18, 21) | agx_cmd_uint(values->unk_22, 22, 23) |
          agx_cmd_uint(values->unk_24, 24, 31);

  return AGX_USC_REGISTERS_LENGTH;
}

static inline void
agx_usc_registers_unpack(const uint8_t* restrict cl, Agx_Usc_Registers* restrict values)
{
  values->tag = (Agx_Usc_Control)agx_unpack_uint(cl, 0, 7);
  values->register_count_over_8 = agx_unpack_uint(cl, 8, 12);
  values->unk_13 = agx_unpack_uint(cl, 13, 13);
  values->unk_14 = agx_unpack_uint(cl, 14, 17);
  values->spill_size = agx_unpack_uint(cl, 18, 21);
  values->unk_22 = agx_unpack_uint(cl, 22, 23);
  values->unk_24 = agx_unpack_uint(cl, 24, 31);
}

typedef struct Agx_Usc_No_Preshader
{
  Agx_Usc_Control tag;
  uint32_t        unk_8;
} Agx_Usc_No_Preshader;

#define AGX_USC_NO_PRESHADER_LENGTH 2

static inline Agx_Usc_No_Preshader
agx_usc_no_preshader_default(void)
{
  return (Agx_Usc_No_Preshader) {
    .tag = AGX_USC_CONTROL_NO_PRESHADER,
    .unk_8 = 0x0,
  };
}

static inline uint32_t
agx_usc_no_preshader_pack(uint32_t* restrict cl, const Agx_Usc_No_Preshader* restrict values)
{
  cl[0] = agx_cmd_uint(values->tag, 0, 7) | agx_cmd_uint(values->unk_8, 8, 15);

  return AGX_USC_NO_PRESHADER_LENGTH;
}

static inline void
agx_usc_no_preshader_unpack(const uint8_t* restrict cl, Agx_Usc_No_Preshader* restrict values)
{
  values->tag = (Agx_Usc_Control)agx_unpack_uint(cl, 0, 7);
  values->unk_8 = agx_unpack_uint(cl, 8, 15);
}

typedef struct Agx_Usc_Preshader
{
  Agx_Usc_Control tag;
  uint32_t        unk_8;
  uint64_t        code;
} Agx_Usc_Preshader;

#define AGX_USC_PRESHADER_LENGTH 8

static inline Agx_Usc_Preshader
agx_usc_preshader_default(void)
{
  return (Agx_Usc_Preshader) {
    .tag = AGX_USC_CONTROL_PRESHADER,
    .unk_8 = 0xc08000,
  };
}

static inline uint32_t
agx_usc_preshader_pack(uint32_t* restrict cl, const Agx_Usc_Preshader* restrict values)
{
  cl[0] = agx_cmd_uint(values->tag, 0, 7) | agx_cmd_uint(values->unk_8, 8, 31);
  cl[1] = agx_cmd_uint(values->code, 0, 31);

  return AGX_USC_PRESHADER_LENGTH;
}

static inline void
agx_usc_preshader_unpack(const uint8_t* restrict cl, Agx_Usc_Preshader* restrict values)
{
  values->tag = (Agx_Usc_Control)agx_unpack_uint(cl, 0, 7);
  values->unk_8 = agx_unpack_uint(cl, 8, 31);
  values->code = agx_unpack_uint(cl, 32, 63);
}

typedef struct Agx_Usc_Fragment_Properties
{
  Agx_Usc_Control tag;
  bool            early_z_testing;
  bool            unk_9;
  bool            unconditional_discard_1;
  bool            unconditional_discard_2;
  uint32_t        unk_12;
  uint32_t        unk_16;
  uint32_t        unk_24;
} Agx_Usc_Fragment_Properties;

#define AGX_USC_FRAGMENT_PROPERTIES_LENGTH 4

static inline Agx_Usc_Fragment_Properties
agx_usc_fragment_properties_default(void)
{
  return (Agx_Usc_Fragment_Properties) {
    .tag = AGX_USC_CONTROL_FRAGMENT_PROPERTIES,
    .early_z_testing = false,
    .unk_9 = false,
    .unconditional_discard_1 = false,
    .unconditional_discard_2 = false,
    .unk_12 = 0x0,
    .unk_16 = 0x0,
    .unk_24 = 0x0,
  };
}

static inline uint32_t
agx_usc_fragment_properties_pack(uint32_t* restrict cl, const Agx_Usc_Fragment_Properties* restrict values)
{
  cl[0] = agx_cmd_uint(values->tag, 0, 7) | agx_cmd_uint(values->early_z_testing, 8, 8) |
          agx_cmd_uint(values->unk_9, 9, 9) | agx_cmd_uint(values->unconditional_discard_1, 10, 10) |
          agx_cmd_uint(values->unconditional_discard_2, 11, 11) | agx_cmd_uint(values->unk_12, 12, 15) |
          agx_cmd_uint(values->unk_16, 16, 23) | agx_cmd_uint(values->unk_24, 24, 31);

  return AGX_USC_FRAGMENT_PROPERTIES_LENGTH;
}

static inline void
agx_usc_fragment_properties_unpack(const uint8_t* restrict cl, Agx_Usc_Fragment_Properties* restrict values)
{
  values->tag = (Agx_Usc_Control)agx_unpack_uint(cl, 0, 7);
  values->early_z_testing = agx_unpack_uint(cl, 8, 8);
  values->unk_9 = agx_unpack_uint(cl, 9, 9);
  values->unconditional_discard_1 = agx_unpack_uint(cl, 10, 10);
  values->unconditional_discard_2 = agx_unpack_uint(cl, 11, 11);
  values->unk_12 = agx_unpack_uint(cl, 12, 15);
  values->unk_16 = agx_unpack_uint(cl, 16, 23);
  values->unk_24 = agx_unpack_uint(cl, 24, 31);
}

typedef struct Agx_Bind_Pipeline
{
  uint32_t tag;
  uint32_t unk_1;
  uint32_t input_count;
  uint32_t padding_1;
  uint64_t pipeline;
  uint32_t vertex_caller_count;
  uint64_t program_buffer;
  uint32_t padding_2;
} Agx_Bind_Pipeline;

#define AGX_BIND_PIPELINE_LENGTH 16

static inline Agx_Bind_Pipeline
agx_bind_pipeline_default(void)
{
  return (Agx_Bind_Pipeline) {
    .tag = 0x4000002e,
    .unk_1 = 0x1002,
    .input_count = 0,
    .padding_1 = 0x0,
    .vertex_caller_count = 0,
    .padding_2 = 0x0,
  };
}

static inline uint32_t
agx_bind_pipeline_pack(uint32_t* restrict cl, const Agx_Bind_Pipeline* restrict values)
{
  assert((values->program_buffer & 0x3fff) == 0);
  cl[0] = agx_cmd_uint(values->tag, 0, 31);
  cl[1] = agx_cmd_uint(values->unk_1, 0, 15) | agx_cmd_uint(values->input_count, 16, 23) |
          agx_cmd_uint(values->padding_1, 24, 31);
  cl[2] = agx_cmd_uint(values->pipeline, 0, 31);
  cl[3] = agx_cmd_uint(values->vertex_caller_count, 0, 7) | agx_cmd_uint(values->program_buffer >> 14, 8, 23) |
          agx_cmd_uint(values->padding_2, 24, 31);

  return AGX_BIND_PIPELINE_LENGTH;
}

static inline void
agx_bind_pipeline_unpack(const uint8_t* restrict cl, Agx_Bind_Pipeline* restrict values)
{
  values->tag = agx_unpack_uint(cl, 0, 31);
  values->unk_1 = agx_unpack_uint(cl, 32, 47);
  values->input_count = agx_unpack_uint(cl, 48, 55);
  values->padding_1 = agx_unpack_uint(cl, 56, 63);
  values->pipeline = agx_unpack_uint(cl, 64, 95);
  values->vertex_caller_count = agx_unpack_uint(cl, 96, 103);
  values->program_buffer = agx_unpack_uint(cl, 104, 119) << 14;
  values->padding_2 = agx_unpack_uint(cl, 120, 127);
}

typedef struct Agx_Record
{
  uint32_t size_words;
  uint32_t tag;
  uint64_t data;
} Agx_Record;

#define AGX_RECORD_LENGTH 8

static inline Agx_Record
agx_record_default(void)
{
  return (Agx_Record) {
    .tag = 0x0000,
  };
}

static inline uint32_t
agx_record_pack(uint32_t* restrict cl, const Agx_Record* restrict values)
{
  cl[0] = agx_cmd_uint(values->size_words, 0, 7) | agx_cmd_uint(values->tag, 8, 23) | agx_cmd_uint(values->data, 24, 63);
  cl[1] = agx_cmd_uint(values->data, 24, 63) >> 32;

  return AGX_RECORD_LENGTH;
}

static inline void
agx_record_unpack(const uint8_t* restrict cl, Agx_Record* restrict values)
{
  values->size_words = agx_unpack_uint(cl, 0, 7);
  values->tag = agx_unpack_uint(cl, 8, 23);
  values->data = agx_unpack_uint(cl, 24, 63);
}

typedef struct Agx_Index_List
{
  uint32_t           index_buffer_hi;
  Agx_Primitive      primitive;
  bool               restart_enable;
  Agx_Index_Size     index_size;
  bool               index_buffer_size_present;
  bool               index_buffer_present;
  bool               index_count_present;
  bool               instance_count_present;
  bool               start_present;
  uint32_t           unk_25;
  bool               indirect_present;
  uint32_t           unk_27;
  Agx_Vdm_Block_Type block_type;
} Agx_Index_List;

#define AGX_INDEX_LIST_LENGTH 4

static inline Agx_Index_List
agx_index_list_default(void)
{
  return (Agx_Index_List) {
    .index_buffer_hi = 0x0,
    .restart_enable = false,
    .index_size = AGX_INDEX_SIZE_U32,
    .index_buffer_size_present = false,
    .index_buffer_present = false,
    .index_count_present = true,
    .instance_count_present = true,
    .start_present = true,
    .unk_25 = 0x0,
    .indirect_present = false,
    .unk_27 = 0x0,
    .block_type = AGX_VDM_BLOCK_TYPE_INDEX_LIST,
  };
}

static inline uint32_t
agx_index_list_pack(uint32_t* restrict cl, const Agx_Index_List* restrict values)
{
  cl[0] = agx_cmd_uint(values->index_buffer_hi, 0, 7) | agx_cmd_uint(values->primitive, 8, 15) |
          agx_cmd_uint(values->restart_enable, 16, 16) | agx_cmd_uint(values->index_size, 17, 19) |
          agx_cmd_uint(values->index_buffer_size_present, 20, 20) | agx_cmd_uint(values->index_buffer_present, 21, 21) |
          agx_cmd_uint(values->index_count_present, 22, 22) | agx_cmd_uint(values->instance_count_present, 23, 23) |
          agx_cmd_uint(values->start_present, 24, 24) | agx_cmd_uint(values->unk_25, 25, 25) |
          agx_cmd_uint(values->indirect_present, 26, 26) | agx_cmd_uint(values->unk_27, 27, 28) |
          agx_cmd_uint(values->block_type, 29, 31);

  return AGX_INDEX_LIST_LENGTH;
}

static inline void
agx_index_list_unpack(const uint8_t* restrict cl, Agx_Index_List* restrict values)
{
  values->index_buffer_hi = agx_unpack_uint(cl, 0, 7);
  values->primitive = (Agx_Primitive)agx_unpack_uint(cl, 8, 15);
  values->restart_enable = agx_unpack_uint(cl, 16, 16);
  values->index_size = (Agx_Index_Size)agx_unpack_uint(cl, 17, 19);
  values->index_buffer_size_present = agx_unpack_uint(cl, 20, 20);
  values->index_buffer_present = agx_unpack_uint(cl, 21, 21);
  values->index_count_present = agx_unpack_uint(cl, 22, 22);
  values->instance_count_present = agx_unpack_uint(cl, 23, 23);
  values->start_present = agx_unpack_uint(cl, 24, 24);
  values->unk_25 = agx_unpack_uint(cl, 25, 25);
  values->indirect_present = agx_unpack_uint(cl, 26, 26);
  values->unk_27 = agx_unpack_uint(cl, 27, 28);
  values->block_type = (Agx_Vdm_Block_Type)agx_unpack_uint(cl, 29, 31);
}

typedef struct Agx_Stream_Link
{
  uint32_t           unk_0;
  bool               unk_28;
  Agx_Vdm_Block_Type block_type;
  uint32_t           target_lo;
} Agx_Stream_Link;

#define AGX_STREAM_LINK_LENGTH 8

static inline Agx_Stream_Link
agx_stream_link_default(void)
{
  return (Agx_Stream_Link) {
    .unk_0 = 0x0,
    .unk_28 = false,
    .block_type = AGX_VDM_BLOCK_TYPE_STREAM_LINK,
  };
}

static inline uint32_t
agx_stream_link_pack(uint32_t* restrict cl, const Agx_Stream_Link* restrict values)
{
  cl[0] = agx_cmd_uint(values->unk_0, 0, 27) | agx_cmd_uint(values->unk_28, 28, 28) |
          agx_cmd_uint(values->block_type, 29, 31);
  cl[1] = agx_cmd_uint(values->target_lo, 0, 31);

  return AGX_STREAM_LINK_LENGTH;
}

static inline void
agx_stream_link_unpack(const uint8_t* restrict cl, Agx_Stream_Link* restrict values)
{
  values->unk_0 = agx_unpack_uint(cl, 0, 27);
  values->unk_28 = agx_unpack_uint(cl, 28, 28);
  values->block_type = (Agx_Vdm_Block_Type)agx_unpack_uint(cl, 29, 31);
  values->target_lo = agx_unpack_uint(cl, 32, 63);
}

typedef struct Agx_Index_List_Indirect
{
  uint32_t arguments_hi;
  uint32_t arguments_lo;
} Agx_Index_List_Indirect;

#define AGX_INDEX_LIST_INDIRECT_LENGTH 8

static inline Agx_Index_List_Indirect
agx_index_list_indirect_default(void)
{
  return (Agx_Index_List_Indirect) {0};
}

static inline uint32_t
agx_index_list_indirect_pack(uint32_t* restrict cl, const Agx_Index_List_Indirect* restrict values)
{
  cl[0] = agx_cmd_uint(values->arguments_hi, 0, 31);
  cl[1] = agx_cmd_uint(values->arguments_lo, 0, 31);

  return AGX_INDEX_LIST_INDIRECT_LENGTH;
}

static inline void
agx_index_list_indirect_unpack(const uint8_t* restrict cl, Agx_Index_List_Indirect* restrict values)
{
  values->arguments_hi = agx_unpack_uint(cl, 0, 31);
  values->arguments_lo = agx_unpack_uint(cl, 32, 63);
}

typedef struct Agx_Index_List_Buffer
{
  uint32_t buffer_lo;
} Agx_Index_List_Buffer;

#define AGX_INDEX_LIST_BUFFER_LENGTH 4

static inline Agx_Index_List_Buffer
agx_index_list_buffer_default(void)
{
  return (Agx_Index_List_Buffer) {0};
}

static inline uint32_t
agx_index_list_buffer_pack(uint32_t* restrict cl, const Agx_Index_List_Buffer* restrict values)
{
  cl[0] = agx_cmd_uint(values->buffer_lo, 0, 31);

  return AGX_INDEX_LIST_BUFFER_LENGTH;
}

static inline void
agx_index_list_buffer_unpack(const uint8_t* restrict cl, Agx_Index_List_Buffer* restrict values)
{
  values->buffer_lo = agx_unpack_uint(cl, 0, 31);
}

typedef struct Agx_Index_List_Count
{
  uint32_t count;
} Agx_Index_List_Count;

#define AGX_INDEX_LIST_COUNT_LENGTH 4

static inline Agx_Index_List_Count
agx_index_list_count_default(void)
{
  return (Agx_Index_List_Count) {0};
}

static inline uint32_t
agx_index_list_count_pack(uint32_t* restrict cl, const Agx_Index_List_Count* restrict values)
{
  cl[0] = agx_cmd_uint(values->count, 0, 31);

  return AGX_INDEX_LIST_COUNT_LENGTH;
}

static inline void
agx_index_list_count_unpack(const uint8_t* restrict cl, Agx_Index_List_Count* restrict values)
{
  values->count = agx_unpack_uint(cl, 0, 31);
}

typedef struct Agx_Index_List_Instances
{
  uint32_t count;
} Agx_Index_List_Instances;

#define AGX_INDEX_LIST_INSTANCES_LENGTH 4

static inline Agx_Index_List_Instances
agx_index_list_instances_default(void)
{
  return (Agx_Index_List_Instances) {0};
}

static inline uint32_t
agx_index_list_instances_pack(uint32_t* restrict cl, const Agx_Index_List_Instances* restrict values)
{
  cl[0] = agx_cmd_uint(values->count, 0, 31);

  return AGX_INDEX_LIST_INSTANCES_LENGTH;
}

static inline void
agx_index_list_instances_unpack(const uint8_t* restrict cl, Agx_Index_List_Instances* restrict values)
{
  values->count = agx_unpack_uint(cl, 0, 31);
}

typedef struct Agx_Index_List_Start
{
  uint32_t start;
} Agx_Index_List_Start;

#define AGX_INDEX_LIST_START_LENGTH 4

static inline Agx_Index_List_Start
agx_index_list_start_default(void)
{
  return (Agx_Index_List_Start) {0};
}

static inline uint32_t
agx_index_list_start_pack(uint32_t* restrict cl, const Agx_Index_List_Start* restrict values)
{
  cl[0] = agx_cmd_uint(values->start, 0, 31);

  return AGX_INDEX_LIST_START_LENGTH;
}

static inline void
agx_index_list_start_unpack(const uint8_t* restrict cl, Agx_Index_List_Start* restrict values)
{
  values->start = agx_unpack_uint(cl, 0, 31);
}

typedef struct Agx_Index_List_Buffer_Size
{
  uint32_t size;
} Agx_Index_List_Buffer_Size;

#define AGX_INDEX_LIST_BUFFER_SIZE_LENGTH 4

static inline Agx_Index_List_Buffer_Size
agx_index_list_buffer_size_default(void)
{
  return (Agx_Index_List_Buffer_Size) {0};
}

static inline uint32_t
agx_index_list_buffer_size_pack(uint32_t* restrict cl, const Agx_Index_List_Buffer_Size* restrict values)
{
  cl[0] = agx_cmd_uint(values->size, 0, 31);

  return AGX_INDEX_LIST_BUFFER_SIZE_LENGTH;
}

static inline void
agx_index_list_buffer_size_unpack(const uint8_t* restrict cl, Agx_Index_List_Buffer_Size* restrict values)
{
  values->size = agx_unpack_uint(cl, 0, 31);
}

typedef struct Agx_Index_List_Unk
{
  uint32_t value;
} Agx_Index_List_Unk;

#define AGX_INDEX_LIST_UNK_LENGTH 4

static inline Agx_Index_List_Unk
agx_index_list_unk_default(void)
{
  return (Agx_Index_List_Unk) {
    .value = 0x1,
  };
}

static inline uint32_t
agx_index_list_unk_pack(uint32_t* restrict cl, const Agx_Index_List_Unk* restrict values)
{
  cl[0] = agx_cmd_uint(values->value, 0, 31);

  return AGX_INDEX_LIST_UNK_LENGTH;
}

static inline void
agx_index_list_unk_unpack(const uint8_t* restrict cl, Agx_Index_List_Unk* restrict values)
{
  values->value = agx_unpack_uint(cl, 0, 31);
}

typedef struct Agx_Launch
{
  uint32_t command;
  uint64_t pipeline;
  uint32_t group_count_x;
  uint32_t group_count_y;
  uint32_t group_count_z;
  uint32_t local_size_x;
  uint32_t local_size_y;
  uint32_t local_size_z;
  uint32_t unk;
} Agx_Launch;

#define AGX_LAUNCH_LENGTH 36

static inline Agx_Launch
agx_launch_default(void)
{
  return (Agx_Launch) {
    .command = 0x1002,
    .unk = 0x60000160,
  };
}

static inline uint32_t
agx_launch_pack(uint32_t* restrict cl, const Agx_Launch* restrict values)
{
  cl[0] = agx_cmd_uint(values->command, 0, 31);
  cl[1] = agx_cmd_uint(values->pipeline, 0, 31);
  cl[2] = agx_cmd_uint(values->group_count_x, 0, 31);
  cl[3] = agx_cmd_uint(values->group_count_y, 0, 31);
  cl[4] = agx_cmd_uint(values->group_count_z, 0, 31);
  cl[5] = agx_cmd_uint(values->local_size_x, 0, 31);
  cl[6] = agx_cmd_uint(values->local_size_y, 0, 31);
  cl[7] = agx_cmd_uint(values->local_size_z, 0, 31);
  cl[8] = agx_cmd_uint(values->unk, 0, 31);

  return AGX_LAUNCH_LENGTH;
}

static inline void
agx_launch_unpack(const uint8_t* restrict cl, Agx_Launch* restrict values)
{
  values->command = agx_unpack_uint(cl, 0, 31);
  values->pipeline = agx_unpack_uint(cl, 32, 63);
  values->group_count_x = agx_unpack_uint(cl, 64, 95);
  values->group_count_y = agx_unpack_uint(cl, 96, 127);
  values->group_count_z = agx_unpack_uint(cl, 128, 159);
  values->local_size_x = agx_unpack_uint(cl, 160, 191);
  values->local_size_y = agx_unpack_uint(cl, 192, 223);
  values->local_size_z = agx_unpack_uint(cl, 224, 255);
  values->unk = agx_unpack_uint(cl, 256, 287);
}

typedef struct Agx_Stage_Record_List_Entry
{
  uint32_t tag;
  uint64_t unk_0;
  uint64_t unk_1;
  uint64_t unk_2;
  uint64_t unk_3;
  uint32_t payload_word_0;
  uint32_t payload_word_1;
  uint32_t payload_word_2;
  uint32_t payload_word_3;
  uint32_t payload_word_4;
  uint32_t payload_word_5;
  uint32_t payload_word_6;
  uint32_t payload_word_7;
} Agx_Stage_Record_List_Entry;

#define AGX_STAGE_RECORD_LIST_ENTRY_LENGTH 64

static inline Agx_Stage_Record_List_Entry
agx_stage_record_list_entry_default(void)
{
  return (Agx_Stage_Record_List_Entry) {0};
}

static inline uint32_t
agx_stage_record_list_entry_pack(uint32_t* restrict cl, const Agx_Stage_Record_List_Entry* restrict values)
{
  cl[0] = agx_cmd_uint(values->tag, 0, 7) | agx_cmd_uint(values->unk_0, 8, 63);
  cl[1] = agx_cmd_uint(values->unk_0, 8, 63) >> 32;
  cl[2] = agx_cmd_uint(values->unk_1, 0, 63);
  cl[3] = agx_cmd_uint(values->unk_1, 0, 63) >> 32;
  cl[4] = agx_cmd_uint(values->unk_2, 0, 63);
  cl[5] = agx_cmd_uint(values->unk_2, 0, 63) >> 32;
  cl[6] = agx_cmd_uint(values->unk_3, 0, 63);
  cl[7] = agx_cmd_uint(values->unk_3, 0, 63) >> 32;
  cl[8] = agx_cmd_uint(values->payload_word_0, 0, 31);
  cl[9] = agx_cmd_uint(values->payload_word_1, 0, 31);
  cl[10] = agx_cmd_uint(values->payload_word_2, 0, 31);
  cl[11] = agx_cmd_uint(values->payload_word_3, 0, 31);
  cl[12] = agx_cmd_uint(values->payload_word_4, 0, 31);
  cl[13] = agx_cmd_uint(values->payload_word_5, 0, 31);
  cl[14] = agx_cmd_uint(values->payload_word_6, 0, 31);
  cl[15] = agx_cmd_uint(values->payload_word_7, 0, 31);

  return AGX_STAGE_RECORD_LIST_ENTRY_LENGTH;
}

static inline void
agx_stage_record_list_entry_unpack(const uint8_t* restrict cl, Agx_Stage_Record_List_Entry* restrict values)
{
  values->tag = agx_unpack_uint(cl, 0, 7);
  values->unk_0 = agx_unpack_uint(cl, 8, 63);
  values->unk_1 = agx_unpack_uint(cl, 64, 127);
  values->unk_2 = agx_unpack_uint(cl, 128, 191);
  values->unk_3 = agx_unpack_uint(cl, 192, 255);
  values->payload_word_0 = agx_unpack_uint(cl, 256, 287);
  values->payload_word_1 = agx_unpack_uint(cl, 288, 319);
  values->payload_word_2 = agx_unpack_uint(cl, 320, 351);
  values->payload_word_3 = agx_unpack_uint(cl, 352, 383);
  values->payload_word_4 = agx_unpack_uint(cl, 384, 415);
  values->payload_word_5 = agx_unpack_uint(cl, 416, 447);
  values->payload_word_6 = agx_unpack_uint(cl, 448, 479);
  values->payload_word_7 = agx_unpack_uint(cl, 480, 511);
}
