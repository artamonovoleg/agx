#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <mach-o/dyld.h>
#include <IOKit/IOKitLib.h>
#include "agx.h"
#include "agx_driver_programs.h"
#include "agx_cmd.h"

#define AGX_SAMPLER_MAX_ANISOTROPY_LOG2 4u

#define AGX_UNORM16_MAX         65535u
#define AGX_RGB10A2_COLOR_BITS  10u
#define AGX_RGB10A2_ALPHA_SHIFT 30u
#define AGX_UNORM10_MAX         1023u
#define AGX_UNORM8_MAX          255u
#define AGX_UNORM2_MAX          3u

#define AGX_BO_BITS_BASE                0x430u
#define AGX_VERTEX_PIPELINE_CALLER_UNIT 64u
#define AGX_VERTEX_PIPELINE_CALLER_COUNT \
  (AGX_PIPELINE_PROGRAMS_FRAGMENT_CALLER_OFFSET / AGX_VERTEX_PIPELINE_CALLER_UNIT)

static void
agx_image_write_descriptor(void* at, const Agx_Image* desc);

static inline bool
agx_open_service(const char* name, uint32_t type, mach_port_t* connection)
{
  CFDictionaryRef matching = IOServiceNameMatching(name);

  io_service_t service = IOServiceGetMatchingService(kIOMainPortDefault, matching);

  if (!service)
  {
    fprintf(stderr, "IOServiceGetMatchingService(%s) failed\n", name);
    return false;
  }

  kern_return_t ret = IOServiceOpen(service, mach_task_self(), type, connection);

  if (ret)
  {
    fprintf(stderr, "IOServiceOpen(%u) failed: %u\n", type, ret);
    return false;
  }

  return true;
}

bool
agx_create_device(Agx_Device* device)
{
  memset(device, 0, sizeof(*device));
  kern_return_t ret;

  if (!agx_open_service("IOSurfaceRoot", IOSURFACE_SERVICE_TYPE, &device->iosrf))
  {
    return false;
  }

  {
    char   selectorD_resp[40] = {0};
    size_t selectorD_resp_size = sizeof(selectorD_resp);

    ret = IOConnectCallStructMethod(device->iosrf, IOSURFACE_SELECTOR_UNKD, NULL, 0, selectorD_resp, &selectorD_resp_size);

    if (ret)
    {
      fprintf(stderr, "IOSURFACE_SELECTOR_UNKD call failed\n");
      return false;
    }
  }

  {
    uint64_t scalar = 0;
    uint32_t scalar_count = 1;
    ret = IOConnectCallScalarMethod(
      device->iosrf, IOSURFACE_SELECTOR_GET_GRAPHICS_COMM_PAGE_ADDRESS, NULL, 0, &scalar, &scalar_count
    );

    if (ret)
    {
      fprintf(stderr, "IOSURFACE_SELECTOR_GET_GRAPHICS_COMM_PAGE_ADDRESS call failed\n");
      return false;
    }

    device->comm_page_address = scalar;
  }

  if (!agx_open_service("AGXAcceleratorG16G", AGX_SERVICE_TYPE, &device->agx))
  {
    return false;
  }

  char   selector2_resp[536] = {0};
  size_t selector2_resp_size = sizeof(selector2_resp);

  ret = IOConnectCallStructMethod(device->agx, AGX_SELECTOR_UNK2, NULL, 0, selector2_resp, &selector2_resp_size);

  if (ret)
  {
    fprintf(stderr, "AGX_SELECTOR_UNK2 call failed\n");
    return false;
  }

  char   selector0_resp[64] = {0};
  size_t selector0_resp_size = sizeof(selector0_resp);

  ret = IOConnectCallStructMethod(device->agx, AGX_SELECTOR_UNK0, NULL, 0, selector0_resp, &selector0_resp_size);

  if (ret)
  {
    fprintf(stderr, "AGX_SELECTOR_UNK0 call failed\n");
    return false;
  }

  char   selector5_resp[32] = {0};
  size_t selector5_resp_size = sizeof(selector5_resp);

  ret = IOConnectCallStructMethod(device->agx, AGX_SELECTOR_UNK5, NULL, 0, selector5_resp, &selector5_resp_size);

  if (ret)
  {
    fprintf(stderr, "AGX_SELECTOR_UNK5 call failed\n");
    return false;
  }

  char   version[456] = {0};
  size_t version_len = sizeof(version);

  ret = IOConnectCallStructMethod(device->agx, AGX_SELECTOR_GET_VERSION, NULL, 0, version, &version_len);

  if (ret)
  {
    fprintf(stderr, "Error getting version: %u\n", ret);
  }

  if (version_len != sizeof(version))
  {
    return agx_refuse(
      "agx_create_device: GET_VERSION returned %zu bytes, not the %zu this library reads "
      "the kext build date out of; the kernel's response has changed shape",
      version_len,
      sizeof(version)
    );
  }

  printf("Kext build date: %s\n", version + (25 * 8));

  char   selector100_resp[152] = {0};
  size_t selector100_resp_size = sizeof(selector100_resp);

  ret = IOConnectCallStructMethod(device->agx, AGX_SELECTOR_UNK100, NULL, 0, selector100_resp, &selector100_resp_size);

  if (ret)
  {
    fprintf(stderr, "AGX_SELECTOR_UNK100 call failed\n");
    return false;
  }

  return true;
}

bool
agx_usc_profile_init(Agx_Device device)
{
  char   usc_init_req[72] = {0};
  char   usc_init_resp[72] = {0};
  size_t usc_init_resp_size = sizeof(usc_init_resp);

  usc_init_req[0] = 0x0B;

  kern_return_t ret = IOConnectCallStructMethod(
    device.agx, AGX_SELECTOR_USC_PROFILE_INIT, usc_init_req, sizeof(usc_init_req), usc_init_resp, &usc_init_resp_size
  );

  if (ret)
  {
    fprintf(stderr, "AGX_SELECTOR_USC_PROFILE_INIT call failed\n");
    return false;
  }

  return true;
}

void
agx_destroy_device(Agx_Device device)
{
  kern_return_t ret;

  ret = IOServiceClose(device.agx);

  if (ret)
  {
    fprintf(stderr, "IOServiceClose(agx) failed: %u\n", ret);
  }

  ret = IOServiceClose(device.iosrf);

  if (ret)
  {
    fprintf(stderr, "IOServiceClose(iosurface) failed: %u\n", ret);
  }
}

typedef struct Agx_Segment_List_First_Header
{
  uint64_t token0;
  uint32_t unk0;
  uint32_t unk1;
  uint32_t unk2;
  uint32_t begin_again;
  uint64_t token1;
  uint32_t record_count;
  uint32_t record_span;
  uint64_t token2;
  uint32_t begin;
  uint32_t end;
  uint32_t unk7, unk8;
  uint32_t alloc_count;
  uint32_t entry_count;
} __attribute__((packed)) Agx_Segment_List_First_Header;

typedef struct Agx_Segment_List_Next_Header
{
  uint64_t token;
  uint32_t begin;
  uint32_t end;
  uint32_t unk0, unk1;
  uint32_t alloc_count;
  uint32_t entry_count;
} __attribute__((packed)) Agx_Segment_List_Next_Header;

typedef struct Agx_Segment_List_Entry
{
  uint32_t allocs[AGX_SEGMENT_LIST_SLOTS_PER_ENTRY];
  uint32_t param32[AGX_SEGMENT_LIST_SLOTS_PER_ENTRY];
  uint16_t param16[AGX_SEGMENT_LIST_SLOTS_PER_ENTRY];
  uint16_t unk0;
  uint16_t alloc_count;
} __attribute__((packed)) Agx_Segment_List_Entry;

void
agx_segment_list_begin(Agx_Segment_List_Builder* b, const Agx_Bo* memory)
{
  memset(b, 0, sizeof(*b));
  b->base = memory->cpu;
  b->capacity = memory->size;
  b->list_at = (size_t)-1;
}

static void
finish_current(Agx_Segment_List_Builder* b)
{
  if (b->list_at == (size_t)-1)
  {
    return;
  }

  uint32_t entries = (b->count + AGX_SEGMENT_LIST_SLOTS_PER_ENTRY - 1) / AGX_SEGMENT_LIST_SLOTS_PER_ENTRY;

  if (b->records == 1)
  {
    Agx_Segment_List_First_Header* h = (Agx_Segment_List_First_Header*)(b->base + b->list_at);
    h->alloc_count = b->count;
    h->entry_count = entries;
  }
  else
  {
    Agx_Segment_List_Next_Header* h = (Agx_Segment_List_Next_Header*)(b->base + b->list_at);
    h->alloc_count = b->count;
    h->entry_count = entries;
  }
}

void
agx_segment_list_record(Agx_Segment_List_Builder* b, uint32_t record_offset, uint32_t record_size)
{
  finish_current(b);

  size_t header = b->records == 0 ? sizeof(Agx_Segment_List_First_Header) : sizeof(Agx_Segment_List_Next_Header);
  if (b->cursor + header > b->capacity)
  {
    b->overflowed = true;
    return;
  }

  memset(b->base + b->cursor, 0, header);
  b->list_at = b->cursor;
  b->cursor += header;
  b->count = 0;
  b->records++;

  if (b->records == 1)
  {
    Agx_Segment_List_First_Header* h = (Agx_Segment_List_First_Header*)(b->base + b->list_at);
    h->unk0 = 1;
    h->unk1 = 0x40000001;
    h->begin_again = record_offset - 4;
    h->begin = record_offset - 4;
    h->end = record_offset + record_size - 4;
  }
  else
  {
    Agx_Segment_List_Next_Header* h = (Agx_Segment_List_Next_Header*)(b->base + b->list_at);
    h->begin = record_offset - 4;
    h->end = record_offset + record_size - 4;
  }
}

bool
agx_segment_list_add_sized(Agx_Segment_List_Builder* b, uint32_t kernel_index, uint64_t size, Agx_Residency_Class cls)
{
  if (b->list_at == (size_t)-1)
  {
    fprintf(stderr, "agx_segment_list: add before any record was started\n");
    return false;
  }

  uint32_t entry = b->count / AGX_SEGMENT_LIST_SLOTS_PER_ENTRY;
  uint32_t slot = b->count % AGX_SEGMENT_LIST_SLOTS_PER_ENTRY;

  size_t entries_at =
    b->list_at + (b->records == 1 ? sizeof(Agx_Segment_List_First_Header) : sizeof(Agx_Segment_List_Next_Header));
  size_t needed = entries_at + (size_t)(entry + 1) * sizeof(Agx_Segment_List_Entry);
  if (needed > b->capacity)
  {
    b->overflowed = true;
    return false;
  }

  Agx_Segment_List_Entry* e = (Agx_Segment_List_Entry*)(b->base + entries_at + (size_t)entry * sizeof(*e));
  if (slot == 0)
  {
    memset(e, 0, sizeof(*e));
  }

  e->allocs[slot] = kernel_index;

  e->param32[slot] =
    (uint32_t)((size + AGX_RESIDENCY_GRANULE - 1) / AGX_RESIDENCY_GRANULE * (AGX_RESIDENCY_GRANULE >> 10));
  e->param16[slot] = (uint16_t)cls;
  e->alloc_count = (uint16_t)(slot + 1);

  b->count++;
  b->cursor = entries_at + (size_t)(entry + 1) * sizeof(*e);
  return true;
}

bool
agx_segment_list_add(Agx_Segment_List_Builder* b, const Agx_Bo* alloc, Agx_Residency_Class cls)
{
  return agx_segment_list_add_sized(b, (uint32_t)alloc->index, alloc->size, cls);
}

size_t
agx_segment_list_finish(Agx_Segment_List_Builder* b)
{
  if (b->overflowed)
  {
    fprintf(stderr, "agx_segment_list: the lists did not fit in %zu bytes\n", b->capacity);
    return 0;
  }

  finish_current(b);

  if (b->records)
  {
    Agx_Segment_List_First_Header* h = (Agx_Segment_List_First_Header*)b->base;

    h->record_count = b->records;
    h->record_span = 0x80000000u | (uint32_t)(0x50 + 0x120 * b->records);
  }

  return b->cursor;
}

void
agx_command_queue_desc_init(Agx_Command_Queue_Desc* desc)
{
  memset(desc, 0, sizeof(*desc));

  desc->queue_type = 2;
  desc->unknown_policed_0x0c = 0x0E;
}

void
agx_command_queue_desc_set_type(Agx_Command_Queue_Desc* desc, uint32_t queue_type)
{
  if (queue_type > 4u)
  {
    agx_refuse("agx_command_queue_desc_set_type: queue type %u is out of range; the kernel accepts 0 through 4", queue_type);
    return;
  }

  desc->queue_type = queue_type;
}

void
agx_bo_desc_init(Agx_Bo_Desc* desc)
{
  memset(desc, 0, sizeof(*desc));

  desc->bits = AGX_BO_BITS_BASE;
  desc->type = AGX_MEMORY_TYPE_UNK;
  desc->width_height = (1u << 16) | 1u;
  desc->unknown_0x0c = 0x1;
  desc->bytes_per_pixel = 1;
  desc->unknown_0x10[0] = 0x01;
  desc->unknown_0x10[1] = 0x01;
}

void
agx_bo_desc_set_size(Agx_Bo_Desc* desc, uint64_t bytes)
{
  desc->root_size = bytes;
}

void
agx_bo_desc_set_space(Agx_Bo_Desc* desc, Agx_Bo_Space space)
{
  switch (space)
  {
  case AGX_BO_SPACE_MAIN:
    desc->type = AGX_MEMORY_TYPE_UNK;
    desc->bits &= ~(uint32_t)AGX_BO_BIT_LOW_VA;
    break;
  case AGX_BO_SPACE_USC:
    desc->type = AGX_MEMORY_TYPE_SHADER;
    desc->bits |= AGX_BO_BIT_LOW_VA;
    break;
  case AGX_BO_SPACE_CMDBUF:
    desc->type = AGX_MEMORY_TYPE_CMD_64;
    desc->bits &= ~(uint32_t)AGX_BO_BIT_LOW_VA;
    break;
  case AGX_BO_SPACE_AUX:
    desc->type = AGX_MEMORY_TYPE_CMD_32;
    desc->bits &= ~(uint32_t)AGX_BO_BIT_LOW_VA;
    break;
  }
}

void
agx_bo_desc_set_width(Agx_Bo_Desc* desc, uint32_t width)
{
  desc->width_height = (desc->width_height & 0xffff0000u) | (width & 0xffffu);
}

void
agx_bo_desc_set_height(Agx_Bo_Desc* desc, uint32_t height)
{
  desc->width_height = (desc->width_height & 0x0000ffffu) | (height << 16);
}

void
agx_bo_desc_set_bytes_per_pixel(Agx_Bo_Desc* desc, uint32_t bpp)
{
  desc->bytes_per_pixel = (uint8_t)bpp;
}

void
agx_bo_desc_set_image(Agx_Bo_Desc* desc, uint32_t width, uint32_t height, uint32_t bytes_per_pixel)
{
  agx_bo_desc_set_width(desc, width);
  agx_bo_desc_set_height(desc, height);
  agx_bo_desc_set_bytes_per_pixel(desc, bytes_per_pixel);

  desc->type = AGX_MEMORY_TYPE_IMAGE;
  agx_bo_desc_set_size(desc, AGX_IMAGE_BYTES(width, height));
}

void
agx_bo_desc_set_render_target(Agx_Bo_Desc* desc, uint32_t width, uint32_t height, uint32_t bytes)
{
  agx_bo_desc_set_width(desc, width);
  agx_bo_desc_set_height(desc, height);
  agx_bo_desc_set_bytes_per_pixel(desc, bytes);
  desc->type = AGX_MEMORY_TYPE_IMAGE;
  agx_bo_desc_set_size(desc, AGX_RENDER_TARGET_BYTES(width, height, bytes));
}

void
agx_bo_desc_set_depth_image(Agx_Bo_Desc* desc, uint32_t width, uint32_t height, Agx_Depth_Format format)
{
  uint32_t bytes = format == AGX_DEPTH_FORMAT_UNORM16 ? 2u : 4u;

  agx_bo_desc_set_width(desc, width);
  agx_bo_desc_set_height(desc, height);
  agx_bo_desc_set_bytes_per_pixel(desc, bytes);
  desc->type = AGX_MEMORY_TYPE_IMAGE;
  agx_bo_desc_set_size(desc, AGX_DEPTH_BYTES_AT(width, height, bytes));
}

void
agx_bo_desc_set_iosurface(Agx_Bo_Desc* desc, uint32_t surface_id, uint32_t width, uint32_t height)
{
  agx_bo_desc_set_image(desc, width, height, 4);
  desc->request_kind = AGX_BO_REQUEST_KIND_IOSURFACE;
  desc->bits = AGX_BO_BITS_COMMON;
  desc->sub_cpu = surface_id;
  desc->unknown_0x5c = 0x5;
  desc->shared_guid = 0x8000;
  desc->root_size = 0;
}

void
agx_bo_desc_set_storage_mode(Agx_Bo_Desc* desc, Agx_Storage_Mode mode)
{
  agx_bo_desc_set_cpu_visible(desc, mode == AGX_STORAGE_MODE_SHARED);
}

void
agx_bo_desc_set_cpu_visible(Agx_Bo_Desc* desc, bool visible)
{
  if (visible)
  {
    desc->bits &= ~(uint32_t)AGX_BO_BIT_NO_CPU_MAP;
  }
  else
  {
    desc->bits |= AGX_BO_BIT_NO_CPU_MAP;
  }
}

void
agx_bo_desc_set_suballocation(Agx_Bo_Desc* desc, const Agx_Bo* parent, uint64_t offset)
{
  if (!parent)
  {
    return;
  }

  desc->bits |= AGX_BO_BIT_SUBALLOC;
  desc->type = 0;
  desc->heap_index = parent->index;
  desc->root_cpu = (uint64_t)parent->cpu;
  desc->sub_cpu = (uint64_t)parent->cpu + offset;
  desc->root_size = parent->size;
  desc->shared_guid = parent->shared_guid;
}

void
agx_bo_desc_set_memory_type(Agx_Bo_Desc* desc, Agx_Memory_Type type)
{
  desc->type = type;
}

void
agx_bo_desc_set_cache_mode(Agx_Bo_Desc* desc, Agx_Cache_Mode mode)
{
  desc->cache_mode = (uint32_t)mode;
}

void
agx_bo_desc_set_bits(Agx_Bo_Desc* desc, Agx_Bo_Bits bits)
{
  desc->bits = bits;
}

void
agx_bo_desc_set_shared_guid(Agx_Bo_Desc* desc, uint64_t guid)
{
  desc->shared_guid = guid;
}

void
agx_pipeline_desc_init(Agx_Pipeline_Desc* pipeline)
{
  memset(pipeline, 0, sizeof(*pipeline));
  pipeline->scissor_enable = false;
  pipeline->objtype = AGX_OBJTYPE_TRIANGLES;
  pipeline->depth_bias_enable = false;
  pipeline->pass_type = AGX_PASS_TYPE_OPAQUE;

  pipeline->depth_compare = AGX_COMPARE_FUNCTION_ALWAYS;
  pipeline->depth_write = false;
  pipeline->two_sided = false;

  Agx_Stencil_State off = {0};
  off.compare = AGX_COMPARE_FUNCTION_ALWAYS;
  pipeline->stencil_front = off;
  pipeline->stencil_back = off;

  pipeline->output_selects = 0;
  pipeline->has_output_selects = false;
  pipeline->point_line_width = 15;
  pipeline->fill_mode = AGX_TRIANGLE_FILL_MODE_FILL;

  pipeline->rasterization_enabled = true;
  pipeline->tile_word = 0x7419;
  pipeline->tagsort_flush_data = 0x1ffff;
}

void
agx_pipeline_desc_set_shader_code(Agx_Pipeline_Desc* pipeline, const Agx_Bo* heap, uint32_t offset)
{
  pipeline->shader_code_offset = (uint32_t)(heap->gpu_va & 0xffffffffull) + offset;
}

void
agx_pipeline_desc_set_varying_components(Agx_Pipeline_Desc* pipeline, uint32_t components)
{
  pipeline->varying_components = components;
}

void
agx_pipeline_desc_set_frag_coord_z(Agx_Pipeline_Desc* pipeline, bool reads)
{
  pipeline->reads_frag_coord_z = reads;
}

void
agx_pipeline_desc_set_vertex_output_size(Agx_Pipeline_Desc* pipeline, uint32_t size)
{
  pipeline->vertex_output_size = size;
}

void
agx_pipeline_desc_set_output_selects(Agx_Pipeline_Desc* pipeline, uint32_t selects)
{
  pipeline->output_selects = selects;
  pipeline->has_output_selects = true;
}

void
agx_pipeline_desc_set_clip_distance_count(Agx_Pipeline_Desc* pipeline, uint32_t count)
{
  pipeline->clip_distance_count = count < AGX_MAX_CLIP_DISTANCES ? count : AGX_MAX_CLIP_DISTANCES;
}

void
agx_pipeline_desc_set_varying_groups(Agx_Pipeline_Desc* pipeline, uint32_t groups)
{
  pipeline->varying_groups = groups;
}

uint32_t
agx_pipeline_interpolator_groups(uint32_t varying_groups, uint32_t varying_components)
{
  if (varying_components == 0 || varying_groups == 0)
  {
    return 0;
  }

  return varying_groups + 1u;
}

void
agx_pipeline_desc_set_objtype(Agx_Pipeline_Desc* pipeline, Agx_Objtype objtype)
{
  pipeline->objtype = objtype;
}

void
agx_pipeline_desc_set_scissor_enable(Agx_Pipeline_Desc* pipeline, bool scissor_enable)
{
  pipeline->scissor_enable = scissor_enable;
}

void
agx_pipeline_desc_set_depth_bias_enable(Agx_Pipeline_Desc* pipeline, bool depth_bias_enable)
{
  pipeline->depth_bias_enable = depth_bias_enable;
}

void
agx_pipeline_desc_set_pass_type(Agx_Pipeline_Desc* pipeline, Agx_Pass_Type pass_type)
{
  pipeline->pass_type = pass_type;
}

void
agx_pipeline_desc_set_rasterization_enabled(Agx_Pipeline_Desc* pipeline, bool enabled)
{
  pipeline->rasterization_enabled = enabled;
}

void
agx_pipeline_desc_set_depth_compare(Agx_Pipeline_Desc* pipeline, Agx_Compare_Function compare)
{
  pipeline->depth_compare = compare;
}

void
agx_pipeline_desc_set_depth_write(Agx_Pipeline_Desc* pipeline, bool depth_write)
{
  pipeline->depth_write = depth_write;
}

void
agx_pipeline_desc_set_stencil(Agx_Pipeline_Desc* pipeline, Agx_Stencil_State state)
{
  pipeline->stencil_front = state;
  pipeline->stencil_back = state;
  pipeline->two_sided = false;
}

void
agx_pipeline_desc_set_stencil_two_sided(Agx_Pipeline_Desc* pipeline, Agx_Stencil_State front, Agx_Stencil_State back)
{
  pipeline->stencil_front = front;
  pipeline->stencil_back = back;
  pipeline->two_sided = true;
}

void
agx_pipeline_desc_set_point_line_width(Agx_Pipeline_Desc* pipeline, uint32_t width)
{
  pipeline->point_line_width = width;
}

void
agx_pipeline_desc_set_fill_mode(Agx_Pipeline_Desc* pipeline, Agx_Triangle_Fill_Mode mode)
{
  pipeline->fill_mode = mode;
}

void
agx_depth_stencil_desc_init(Agx_Depth_Stencil_Desc* desc)
{
  memset(desc, 0, sizeof(*desc));

  desc->depth_compare = AGX_COMPARE_FUNCTION_ALWAYS;
  desc->depth_write = false;
  desc->stencil_front.compare = AGX_COMPARE_FUNCTION_ALWAYS;
  desc->stencil_front.read_mask = 0xff;
  desc->stencil_front.write_mask = 0xff;
  desc->stencil_back = desc->stencil_front;
}

void
agx_depth_stencil_desc_set_depth_compare(Agx_Depth_Stencil_Desc* desc, Agx_Compare_Function compare)
{
  desc->depth_compare = compare;
}

void
agx_depth_stencil_desc_set_depth_write(Agx_Depth_Stencil_Desc* desc, bool depth_write)
{
  desc->depth_write = depth_write;
}

void
agx_depth_stencil_desc_set_stencil(Agx_Depth_Stencil_Desc* desc, Agx_Stencil_State state)
{
  desc->stencil_front = state;
  desc->stencil_back = state;
  desc->two_sided = false;
}

void
agx_depth_stencil_desc_set_stencil_two_sided(Agx_Depth_Stencil_Desc* desc, Agx_Stencil_State front, Agx_Stencil_State back)
{
  desc->stencil_front = front;
  desc->stencil_back = back;
  desc->two_sided = true;
}

static bool
agx_tri_merge_disable(Agx_Objtype objtype, Agx_Pass_Type pass_type)
{
  return objtype != AGX_OBJTYPE_TRIANGLES || pass_type != AGX_PASS_TYPE_OPAQUE;
}

static bool
agx_stencil_face_inert(const Agx_Stencil_State* face)
{
  return face->compare == AGX_COMPARE_FUNCTION_ALWAYS && face->fail == AGX_STENCIL_OPERATION_KEEP &&
         face->depth_fail == AGX_STENCIL_OPERATION_KEEP && face->pass == AGX_STENCIL_OPERATION_KEEP;
}

static uint32_t
agx_stencil_mask_effective(const Agx_Stencil_State* face, uint8_t mask)
{
  return agx_stencil_face_inert(face) ? 0u : (uint32_t)mask;
}

static size_t
agx_face_state_write(
  const Agx_Cmd*                cmd,
  const Agx_Depth_Stencil_Desc* desc,
  Agx_Triangle_Fill_Mode        fill_mode,
  Agx_Objtype                   objtype,
  void*                         state_cpu
)
{
  uint8_t* p = (uint8_t*)state_cpu;

  agx_cmd(p, ppp_header, cfg)
  {
    cfg.pres_frg_face_ctl_f = true;
    cfg.pres_frg_face_stencil_f = true;
    cfg.pres_frg_face_ctl_b = true;
    cfg.pres_frg_face_stencil_b = true;
    cfg.pres_frg_vis_query_tag_flush = true;
    cfg.viewport_count = 1;
  }
  agx_cmd(p + 4, ppp_frg_common_ctl, cfg)
  {
    cfg.tagsort_flush_ctl = 2;
    cfg.face_stencil_pres = true;
    cfg.two_sided = desc->two_sided;

    cfg.dbenable = cmd->depth_bias_enable;
    cfg.scenable = cmd->scissor_enable;

    cfg.tri_merge_disable = objtype != AGX_OBJTYPE_TRIANGLES;
  }

  const Agx_Stencil_State* faces[2] = {&desc->stencil_front, &desc->stencil_back};
  for (uint32_t i = 0; i < 2; ++i)
  {
    uint8_t* face = p + 8 + i * 8;
    agx_cmd(face, ppp_frg_face_ctl_pso, cfg)
    {
      cfg.sref = faces[i]->reference;
      cfg.pointlinewidth = cmd->point_line_width;
      cfg.dwritedisable = !desc->depth_write;
      cfg.dcmpmode = (uint32_t)desc->depth_compare;

      cfg.fill_mode_override = (uint32_t)fill_mode;
    }
    agx_cmd(face + 4, ppp_frg_face_stencil, cfg)
    {
      cfg.swmask = agx_stencil_mask_effective(faces[i], faces[i]->write_mask);
      cfg.scmpmask = agx_stencil_mask_effective(faces[i], faces[i]->read_mask);
      cfg.sop_3 = (uint32_t)faces[i]->pass;
      cfg.sop_2 = (uint32_t)faces[i]->depth_fail;
      cfg.sop_1 = (uint32_t)faces[i]->fail;
      cfg.scmpmode = (uint32_t)faces[i]->compare;
    }
  }

  *(uint32_t*)(p + 0x18) = 0;

  return AGX_DEPTH_STENCIL_STATE_LENGTH;
}

size_t
agx_pipeline_state_write(const Agx_Pipeline_Desc* pipeline, void* state_cpu)
{
  uint8_t* p = (uint8_t*)state_cpu;
  memset(p, 0, AGX_PIPELINE_LENGTH);

  const uint32_t perspective_components = pipeline->varying_components;

  const uint32_t interpolator_groups = agx_pipeline_interpolator_groups(pipeline->varying_groups, perspective_components);

  if (pipeline->rasterization_enabled)
  {
    agx_cmd(p + 0x00, ppp_header, cfg)
    {
      cfg.pres_frg_shader_words = true;
      cfg.viewport_count = 1;
    }

    agx_cmd(p + 0x04, fragment_shader_word_0, cfg)
    {
      cfg.coefficients_present = pipeline->varying_components != 0;
      cfg.cf_binding_count = interpolator_groups;
    }

    *(uint32_t*)(p + 0x18) = (3u * perspective_components >= 24u) ? 1u : 0u;

    *(uint32_t*)(p + 0x08) = pipeline->shader_code_offset;

    agx_cmd(p + 0x10, fragment_shader_word_3, cfg)
    {
      cfg.enable = true;
    }
    *(uint32_t*)(p + 0x14) = pipeline->tile_word;
  }
  else
  {
    p -= AGX_PIPELINE_FRG_SHADER_WORDS_BYTES;
  }

  agx_cmd(p + 0x1c, ppp_header, cfg)
  {
    cfg.pres_outselects = true;
    cfg.pres_ms_prim_output = true;
    cfg.pres_vs_amplify_ctrl = true;
    cfg.pres_vs_output_size = true;
    cfg.viewport_count = 1;
  }

  {
    uint32_t selects = pipeline->output_selects;

    if (!pipeline->has_output_selects)
    {
      agx_cmd(&selects, output_select, cfg)
      {
        cfg.varyings = pipeline->varying_components != 0;
        cfg.frag_coord_z = pipeline->reads_frag_coord_z;

        cfg.clip_distance_plane_0 = pipeline->clip_distance_count > 0u;
        cfg.clip_distance_plane_1 = pipeline->clip_distance_count > 1u;
        cfg.clip_distance_plane_2 = pipeline->clip_distance_count > 2u;
        cfg.clip_distance_plane_3 = pipeline->clip_distance_count > 3u;
        cfg.clip_distance_plane_4 = pipeline->clip_distance_count > 4u;
        cfg.clip_distance_plane_5 = pipeline->clip_distance_count > 5u;
        cfg.clip_distance_plane_6 = pipeline->clip_distance_count > 6u;
        cfg.clip_distance_plane_7 = pipeline->clip_distance_count > 7u;
      }
    }
    memcpy(p + 0x20, &selects, sizeof(selects));
  }

  {
    uint32_t output_size = pipeline->vertex_output_size != 0
                           ? pipeline->vertex_output_size
                           : pipeline->varying_components + 4 + pipeline->clip_distance_count;

    memcpy(p + 0x2c, &output_size, sizeof(output_size));
  }

  agx_cmd(p + 0x30, ppp_header, cfg)
  {
    cfg.pres_frg_face_ctl_f = true;
    cfg.pres_frg_face_stencil_f = true;
    cfg.pres_frg_face_ctl_b = true;
    cfg.pres_frg_face_stencil_b = true;
    cfg.pres_frg_vis_query_tag_flush = true;
    cfg.viewport_count = 1;
  }

  agx_cmd(p + 0x34, ppp_frg_common_ctl, cfg)
  {
    cfg.tagsort_flush_ctl = 2;
    cfg.face_stencil_pres = true;
    cfg.scenable = pipeline->scissor_enable;
    cfg.dbenable = pipeline->depth_bias_enable;
    cfg.two_sided = pipeline->two_sided;

    cfg.tri_merge_disable = pipeline->objtype != AGX_OBJTYPE_TRIANGLES;
  }

  const Agx_Stencil_State* faces[2] = {&pipeline->stencil_front, &pipeline->stencil_back};
  for (uint32_t i = 0; i < 2; ++i)
  {
    uint8_t* face = p + 0x38 + i * 8;
    agx_cmd(face, ppp_frg_face_ctl_pso, cfg)
    {
      cfg.sref = faces[i]->reference;
      cfg.pointlinewidth = pipeline->point_line_width;

      cfg.fill_mode_override = (uint32_t)pipeline->fill_mode;
      cfg.dwritedisable = !pipeline->depth_write;
      cfg.dcmpmode = (uint32_t)pipeline->depth_compare;
    }
    agx_cmd(face + 4, ppp_frg_face_stencil, cfg)
    {
      cfg.swmask = agx_stencil_mask_effective(faces[i], faces[i]->write_mask);
      cfg.scmpmask = agx_stencil_mask_effective(faces[i], faces[i]->read_mask);
      cfg.sop_3 = (uint32_t)faces[i]->pass;
      cfg.sop_2 = (uint32_t)faces[i]->depth_fail;
      cfg.sop_1 = (uint32_t)faces[i]->fail;
      cfg.scmpmode = (uint32_t)faces[i]->compare;
    }
  }

  agx_cmd(p + 0x4c, ppp_header, cfg)
  {
    cfg.pres_frg_face_ctl_f_pso = true;
    cfg.pres_frg_face_ctl_b_pso = true;
    cfg.pres_frg_vis_query_tag_flush_pso = true;
    cfg.viewport_count = 1;
  }
  agx_cmd(p + 0x50, ppp_frg_common_ctl, cfg)
  {
    cfg.tagsort_flush_ctl = 2;
    cfg.tri_merge_disable = agx_tri_merge_disable(pipeline->objtype, pipeline->pass_type);

    cfg.passtype = (uint32_t)pipeline->pass_type;
  }

  for (uint32_t i = 0; i < 2; ++i)
  {
    agx_cmd(p + 0x54 + i * 4, ppp_frg_face_ctl_pso, cfg)
    {
      cfg.dwritedisable = true;
      cfg.fs_depth_dir_qual = 3;
      cfg.dcmpmode = 7;
      cfg.objtype = (uint32_t)pipeline->objtype;
    }
  }
  agx_cmd(p + 0x5c, ppp_frg_vis_query_tag_flush_pso, cfg)
  {
    cfg.tagsort_flush_data = pipeline->tagsort_flush_data;
  }

  agx_cmd(p + 0x60, ppp_header, cfg)
  {
    cfg.pres_wclamp = true;
    cfg.pres_pppctrl_pso = true;
    cfg.viewport_count = 1;
  }

  agx_cmd(p + 0x68, ppp_control, cfg)
  {
    cfg.flatshade_vtx = 1;
    cfg.clip_mode = 0;
  }

  return AGX_PIPELINE_LENGTH;
}

Agx_Bo
agx_bo_alloc(Agx_Device device, const Agx_Bo_Desc* desc)
{
  Agx_Bo_Desc        req = *desc;
  Agx_Create_Bo_Resp resp = {0};
  memset(&resp, 0, sizeof(resp));
  size_t resp_size = sizeof(resp);

  kern_return_t ret =
    IOConnectCallMethod(device.agx, AGX_SELECTOR_ALLOCATE_MEM, NULL, 0, &req, sizeof(req), NULL, 0, &resp, &resp_size);

  if (ret != 0)
  {
    fprintf(stderr, "agx_bo_alloc: IOConnectCallMethod failed, kern_return_t=0x%x (%d)\n", ret, ret);
    return (Agx_Bo) {0};
  }
  if (resp_size != sizeof(resp))
  {
    agx_refuse(
      "agx_bo_alloc: ALLOCATE_MEM returned %zu bytes, not the %zu Agx_Allocate_Mem_Resp declares", resp_size, sizeof(resp)
    );
    return (Agx_Bo) {0};
  }

  return (Agx_Bo) {
    .type = AGX_ALLOC_TYPE_REGULAR,
    .guid = resp.guid,
    .index = resp.index,
    .gpu_va = resp.gpu_va,
    .cpu = (void*)resp.cpu,
    .size = resp.sub_size,

    .memory_type = req.type,
    .bits = req.bits,
    .width = req.width_height & 0xffffu,
    .height = req.width_height >> 16,
    .surface_id = req.sub_cpu,
    .shared_guid = req.shared_guid,
    .sub_offset = resp.root_size - resp.sub_size,
    .parent_index = req.heap_index,
  };
}

Agx_Bo
agx_alloc_iosurface(Agx_Device device, uint32_t surface_id, uint32_t width, uint32_t height)
{
  Agx_Bo_Desc desc = {0};
  agx_bo_desc_init(&desc);
  agx_bo_desc_set_iosurface(&desc, surface_id, width, height);

  Agx_Bo alloc = agx_bo_alloc(device, &desc);
  if (!alloc.gpu_va)
  {
    fprintf(stderr, "agx_alloc_iosurface: failed to map IOSurface id %u (%ux%u)\n", surface_id, width, height);
  }
  return alloc;
}

bool
agx_free_mem(Agx_Device device, Agx_Bo* allocation)
{
  kern_return_t ret = IOConnectTrap1(agx_trap_connection(device.agx), AGX_SELECTOR_TRAP1_FREE_MEM, allocation->index);

  if (ret)
  {
    return agx_refuse("agx_free_mem: FREE_MEM(index=%u) failed, kern_return_t=0x%x", (unsigned)allocation->index, ret);
  }

  return true;
}

Agx_Bo
agx_alloc_shmem(Agx_Device device, size_t size, bool cmd)
{
  Agx_Create_Shmem_Resp out = {0};
  size_t                out_sz = sizeof(out);

  uint64_t inputs[2] = {size, cmd ? 1 : 0};

  kern_return_t ret =
    IOConnectCallMethod(device.agx, AGX_SELECTOR_ALLOCATE_SHMEM, inputs, 2, NULL, 0, NULL, NULL, &out, &out_sz);

  if (ret)
  {
    agx_refuse(
      "agx_alloc_shmem: ALLOCATE_SHMEM(%zu bytes, %s) failed, kern_return_t=0x%x",
      size,
      cmd ? "command memory" : "segment list",
      ret
    );
    return (Agx_Bo) {0};
  }

  if (out_sz != sizeof(out))
  {
    agx_refuse(
      "agx_alloc_shmem: ALLOCATE_SHMEM returned %zu bytes, not the %zu Agx_Create_Shmem_Resp declares", out_sz, sizeof(out)
    );
    return (Agx_Bo) {0};
  }

  if (out.size != size)
  {
    agx_refuse("agx_alloc_shmem: asked for %zu bytes and the kernel allocated %u", size, (unsigned)out.size);
    return (Agx_Bo) {0};
  }

  return (Agx_Bo) {
    .type = cmd ? AGX_ALLOC_TYPE_CMD : AGX_ALLOC_TYPE_SEGMENT_LIST,
    .index = out.index,
    .cpu = out.map,
    .size = out.size,
    .guid = 0,
  };
}

bool
agx_free_shmem(Agx_Device device, Agx_Bo* allocation)
{
  uint64_t input = allocation->index;

  kern_return_t ret = IOConnectCallScalarMethod(device.agx, AGX_SELECTOR_FREE_SHMEM, &input, 1, NULL, NULL);

  if (ret)
  {
    return agx_refuse("agx_free_shmem: FREE_SHMEM(index=%u) failed, kern_return_t=0x%x", (unsigned)allocation->index, ret);
  }

  return true;
}

static inline Agx_Notification_Queue
agx_create_notification_queue(mach_port_t connection)
{
  Agx_Create_Notification_Queue_Resp resp = {0};
  size_t                             resp_size = sizeof(resp);

  uint64_t input[] = {
    0x100,
    0x28,
  };

  kern_return_t ret = IOConnectCallMethod(
    connection, AGX_SELECTOR_CREATE_NOTIFICATION_QUEUE, input, 2, NULL, 0, NULL, NULL, &resp, &resp_size
  );

  if (ret)
  {
    agx_refuse("agx_create_notification_queue: CREATE_NOTIFICATION_QUEUE failed, kern_return_t=0x%x", ret);
    return (Agx_Notification_Queue) {0};
  }

  if (resp_size != sizeof(resp))
  {
    agx_refuse(
      "agx_create_notification_queue: CREATE_NOTIFICATION_QUEUE returned %zu bytes, not the %zu "
      "Agx_Create_Notification_Queue_Resp declares",
      resp_size,
      sizeof(resp)
    );
    return (Agx_Notification_Queue) {0};
  }

  mach_port_t notif_port = IODataQueueAllocateNotificationPort();
  IOConnectSetNotificationPort(connection, 0, notif_port, resp.id);

  return (Agx_Notification_Queue) {
    .port = notif_port,
    .queue = resp.queue,
    .id = resp.id,
  };
}

Agx_Command_Queue
agx_create_command_queue(Agx_Device device, const Agx_Command_Queue_Desc* desc)
{
  Agx_Command_Queue queue = {};

  {
    uint8_t buffer[1024 + 16] = {0};

    char        path_storage[1024] = {0};
    uint32_t    path_size = (uint32_t)sizeof(path_storage);
    const char* path = NULL;

    if (_NSGetExecutablePath(path_storage, &path_size) != 0 || path_storage[0] == 0)
    {
      snprintf(path_storage, sizeof(path_storage), "%s", "/tmp/a.out");
    }
    path = path_storage;

    if (strlen(path) >= 1022)
    {
      agx_refuse(
        "agx_create_command_queue: the executable's path is %zu bytes and the kernel's request field "
        "holds 1021",
        strlen(path)
      );
      return (Agx_Command_Queue) {0};
    }

    memcpy(buffer + 0, path, strlen(path));

    uint32_t path_len = (uint32_t)strlen(path);
    uint32_t END_LEN = (path_len < 1024 - path_len) ? path_len : 1024 - path_len;
    uint32_t SKIP = path_len - END_LEN;
    uint32_t OFFS = 1024 - END_LEN;
    memcpy(buffer + OFFS, path + SKIP, END_LEN);

    if (desc->queue_type > 4u)
    {
      fprintf(
        stderr, "agx_create_command_queue: queue_type %u is out of range; the kernel accepts 0 through 4\n", desc->queue_type
      );
    }
    if (desc->unknown_policed_0x0c & 0x10u)
    {
      fprintf(
        stderr, "agx_create_command_queue: unk_c 0x%x has bit 4 set; the kernel rejects every such value\n", desc->unknown_policed_0x0c
      );
    }

    memcpy(buffer + 1024, desc, sizeof(*desc));

    Agx_Create_Command_Queue_Resp out = {};
    size_t                        out_sz = sizeof(out);

    kern_return_t ret =
      IOConnectCallStructMethod(device.agx, AGX_SELECTOR_CREATE_COMMAND_QUEUE, buffer, sizeof(buffer), &out, &out_sz);

    if (ret)
    {
      agx_refuse(
        "agx_create_command_queue: CREATE_COMMAND_QUEUE failed, kern_return_t=0x%x; queue type %u, "
        "unk_c 0x%x",
        ret,
        desc->queue_type,
        desc->unknown_policed_0x0c
      );
      return (Agx_Command_Queue) {0};
    }

    if (out_sz != sizeof(out))
    {
      agx_refuse(
        "agx_create_command_queue: CREATE_COMMAND_QUEUE returned %zu bytes, not the %zu "
        "Agx_Create_Command_Queue_Resp declares",
        out_sz,
        sizeof(out)
      );
      return (Agx_Command_Queue) {0};
    }

    queue.id = out.id;
    queue.agx = device.agx;

    if (queue.id == 0)
    {
      agx_refuse(
        "agx_create_command_queue: the kernel returned queue id 0, which this library reserves for "
        "a queue it could not create"
      );
      return (Agx_Command_Queue) {0};
    }
  }

  queue.notif = agx_create_notification_queue(device.agx);

  queue.progress = (Agx_Queue_Progress*)calloc(1, sizeof(*queue.progress));

  agx_queue_bind_notification_queue(queue);

  return queue;
}

bool
agx_queue_bind_notification_queue(Agx_Command_Queue queue)
{
  uint64_t scalars[2] = {
    queue.id,
    queue.notif.id,
  };

  kern_return_t ret = IOConnectCallScalarMethod(queue.agx, AGX_SELECTOR_UNK1C, scalars, 2, NULL, NULL);
  if (ret)
  {
    fprintf(
      stderr,
      "agx_queue_bind_notification_queue: queue %u, notifications %u failed, "
      "kern_return_t=0x%x\n",
      queue.id,
      queue.notif.id,
      ret
    );
    return false;
  }
  return true;
}

bool
agx_destroy_command_queue(Agx_Device device, Agx_Command_Queue* queue)
{
  {
    uint64_t input = queue->id;

    kern_return_t ret = IOConnectCallScalarMethod(device.agx, AGX_SELECTOR_DESTROY_COMMAND_QUEUE, &input, 1, NULL, NULL);

    if (ret)
    {
      free(queue->progress);
      queue->progress = NULL;
      return agx_refuse(
        "agx_destroy_command_queue: DESTROY_COMMAND_QUEUE(id=%u) failed, kern_return_t=0x%x", (unsigned)queue->id, ret
      );
    }
  }

  {
    uint64_t input = queue->notif.id;

    kern_return_t ret =
      IOConnectCallScalarMethod(device.agx, AGX_SELECTOR_DESTROY_NOTIFICATION_QUEUE, &input, 1, NULL, NULL);

    if (ret)
    {
      free(queue->progress);
      queue->progress = NULL;
      return agx_refuse(
        "agx_destroy_command_queue: DESTROY_NOTIFICATION_QUEUE(id=%u) failed, kern_return_t=0x%x",
        (unsigned)queue->notif.id,
        ret
      );
    }
  }

  free(queue->progress);
  queue->progress = NULL;

  return true;
}

void
agx_cmd_begin(Agx_Cmd* cmd, const Agx_Bo* memory, const Agx_Bo* stream, const Agx_Bo* segment_list)
{
  memset(cmd, 0, sizeof(*cmd));

  cmd->depth_clip_mode = AGX_DEPTH_CLIP_MODE_CLIP;

  cmd->point_line_width = 15;
  cmd->tagsort_flush_data = 0x1ffff;
  agx_depth_stencil_desc_init(&cmd->depth_stencil);

  cmd->memory = *memory;
  cmd->stream = *stream;
  cmd->segment_list = *segment_list;

  if (memory->cpu)
  {
    memset(memory->cpu, 0, memory->size);
  }
  if (stream->cpu)
  {
    memset(stream->cpu, 0, stream->size);
  }

  if (memory->cpu)
  {
    agx_cmd((uint8_t*)memory->cpu, cmd_header, cfg)
    {
      cfg.unk_0 = 0xf;
      cfg.unk_4 = 0xac;
      cfg.unk_ac = 0x10000;
    }
    cmd->cursor = AGX_CMD_HEADER_LENGTH;
  }
}

uint8_t*
agx_cmd_begin_encoder(Agx_Cmd* cmd, size_t record_bytes)
{
  if (!cmd->memory.cpu || cmd->open_encoder || cmd->cursor + record_bytes > cmd->memory.size)
  {
    return NULL;
  }

  if (cmd->last_record)
  {
    uint16_t stated = 0;
    memcpy(&stated, cmd->last_record, sizeof(stated));

    uint16_t trailer = 0;
    memcpy(&trailer, cmd->last_record + stated - 2, sizeof(trailer));
    trailer |= 1;
    memcpy(cmd->last_record + stated - 2, &trailer, sizeof(trailer));
  }

  cmd->open_encoder = (uint8_t*)cmd->memory.cpu + cmd->cursor;
  return cmd->open_encoder;
}

static void
agx_copy_shaped_record_write(
  uint8_t*             record,
  uint32_t             run_token,
  const Agx_Copy_Desc* desc,
  uint64_t             stream,
  uint32_t             stages,
  Agx_Stage            barrier_after,
  Agx_Stage            barrier_before
);

void
agx_cmd_end_encoder(Agx_Cmd* cmd)
{
  if (!cmd->open_encoder)
  {
    return;
  }

  uint16_t stated = 0;
  memcpy(&stated, cmd->open_encoder, sizeof(stated));

  if (cmd->record_count < AGX_CMD_MAX_RECORDS)
  {
    cmd->record_offset[cmd->record_count] = (uint32_t)cmd->cursor;
    cmd->record_size[cmd->record_count] = stated;
    cmd->record_count++;
  }
  cmd->last_record = cmd->open_encoder;

  cmd->cursor += stated;
  cmd->open_encoder = NULL;

  if (cmd->alias_barrier_pending)
  {
    Agx_Stage after = cmd->alias_barrier_after;
    Agx_Stage before = cmd->alias_barrier_before;
    cmd->alias_barrier_pending = false;

    if (cmd->last_copy_desc_valid)
    {
      if (agx_cmd_begin_copy_encoder(cmd, &cmd->last_copy_desc))
      {
        uint8_t* record = cmd->open_encoder;
        uint32_t stages = (uint32_t)after | (uint32_t)before;
        memcpy(record + AGX_CMD_RECORD_STAGES_OFFSET, &stages, sizeof(stages));

        uint32_t masks[4] = {(uint32_t)after, (uint32_t)before, (uint32_t)after, (uint32_t)before};
        memcpy(record + 0xa8, masks, sizeof(masks));

        uint32_t unk_164 = 2;
        memcpy(record + 0x164, &unk_164, sizeof(unk_164));

        agx_cmd_end_copy_encoder(cmd);
      }
    }
    else
    {
      uint8_t* record = agx_cmd_begin_encoder(cmd, AGX_COPY_RECORD_LENGTH);
      if (record)
      {
        agx_copy_shaped_record_write(
          record, 0x124f716du + cmd->copy_records, NULL, 0, (uint32_t)after | (uint32_t)before, after, before
        );
        agx_cmd_end_encoder(cmd);
      }
    }
  }
}

void
agx_sampler_desc_init(Agx_Sampler_Desc* desc)
{
  memset(desc, 0, sizeof(*desc));
  desc->mag_filter = AGX_SAMPLER_FILTER_NEAREST;
  desc->min_filter = AGX_SAMPLER_FILTER_NEAREST;
  desc->mip_filter = AGX_SAMPLER_MIP_FILTER_NOT_MIPMAPPED;
  desc->wrap_s = AGX_ADDRESS_MODE_CLAMP_TO_EDGE;
  desc->wrap_t = AGX_ADDRESS_MODE_CLAMP_TO_EDGE;
  desc->wrap_r = AGX_ADDRESS_MODE_CLAMP_TO_EDGE;
  desc->min_lod = 0;

  desc->max_lod = 896;
  desc->max_aniso = 0;
  desc->normalized_coords = true;

  desc->compare_mode = true;
  desc->compare_func = AGX_SAMPLER_COMPARE_FUNC_NEVER;
  desc->border_mode = AGX_SAMPLER_BORDER_MODE_TRANSPARENT_BLACK;
}

void
agx_sampler_desc_set_max_anisotropy(Agx_Sampler_Desc* desc, uint32_t samples)
{
  uint32_t log2 = 0;

  if (desc == NULL)
  {
    return;
  }
  while ((samples >> (log2 + 1u)) != 0u && log2 < AGX_SAMPLER_MAX_ANISOTROPY_LOG2)
  {
    log2 += 1u;
  }
  desc->max_aniso = log2;
}

void
agx_sampler_desc_set_normalized_coordinates(Agx_Sampler_Desc* desc, bool normalized)
{
  if (desc != NULL)
  {
    desc->normalized_coords = normalized;
  }
}

void
agx_sampler_desc_set_compare(Agx_Sampler_Desc* desc, Agx_Sampler_Compare_Func compare)
{
  if (desc != NULL)
  {
    desc->compare_func = compare;
  }
}

void
agx_sampler_desc_set_border_color(Agx_Sampler_Desc* desc, Agx_Sampler_Border_Mode border)
{
  if (desc != NULL)
  {
    desc->border_mode = border;
  }
}

void
agx_sampler_desc_set_filter(Agx_Sampler_Desc* desc, Agx_Sampler_Filter min, Agx_Sampler_Filter mag)
{
  desc->min_filter = min;
  desc->mag_filter = mag;
}

void
agx_sampler_desc_set_address_mode(Agx_Sampler_Desc* desc, Agx_Address_Mode s, Agx_Address_Mode t, Agx_Address_Mode r)
{
  desc->wrap_s = s;
  desc->wrap_t = t;
  desc->wrap_r = r;
}

void
agx_sampler_desc_set_mip_filter(Agx_Sampler_Desc* desc, Agx_Sampler_Mip_Filter filter)
{
  desc->mip_filter = filter;
}

void
agx_sampler_desc_set_lod_clamp(Agx_Sampler_Desc* desc, uint32_t min_lod, uint32_t max_lod)
{
  desc->min_lod = min_lod;
  desc->max_lod = max_lod;
}

void
agx_argument_table_set_sampler(const Agx_Bo* table, const Agx_Sampler_Desc* sampler)
{
  if (!table->cpu || table->size < 0x660)
  {
    return;
  }

  uint8_t* at = (uint8_t*)table->cpu;

  agx_cmd(at + 0x640, sampler, cfg)
  {
    cfg.min_lod = sampler->min_lod;
    cfg.max_lod = sampler->max_lod;
    cfg.max_aniso = sampler->max_aniso;
    cfg.mag_filter = sampler->mag_filter;
    cfg.min_filter = sampler->min_filter;
    cfg.mip_filter = sampler->mip_filter;
    cfg.wrap_s = sampler->wrap_s;
    cfg.wrap_t = sampler->wrap_t;
    cfg.wrap_r = sampler->wrap_r;
    cfg.non_normalized_coords = !sampler->normalized_coords;

    cfg.compare_mode = sampler->compare_mode;
    cfg.compare_func = sampler->compare_func;
    cfg.border_mode = sampler->border_mode;
  }

  uint64_t address = table->gpu_va + 0x640;
  memcpy(at + 0x608, &address, sizeof(address));
}

void
agx_driver_arguments_init(const Agx_Bo* arguments)
{
  if (!arguments->cpu || arguments->size < 0x900)
  {
    return;
  }

  uint8_t* at = (uint8_t*)arguments->cpu;

  uint64_t block_1_table = arguments->gpu_va + 0x468;
  uint64_t block_2_table = arguments->gpu_va + 0x620;
  uint64_t tail = arguments->gpu_va + 0x828;
  memcpy(at + 0x460, &block_1_table, sizeof(block_1_table));
  memcpy(at + 0x600, &block_2_table, sizeof(block_2_table));
  memcpy(at + 0x820, &tail, sizeof(tail));

  {
    const struct
    {
      uint32_t at;
      uint32_t points_to;
    } k_self_pointers[] = {
      {0x000, 0x020},
      {0x008, 0x120},
      {0x160, 0x168},
      {0x300, 0x320},
      {0x308, 0x420},
    };
    uint32_t i = 0;

    for (i = 0; i < sizeof(k_self_pointers) / sizeof(k_self_pointers[0]); ++i)
    {
      uint64_t value = arguments->gpu_va + k_self_pointers[i].points_to;

      memcpy(at + k_self_pointers[i].at, &value, sizeof(value));
    }
  }

  agx_driver_arguments_set_attachment_count(arguments, 1);

  static const struct
  {
    size_t   offset;
    uint32_t value;
  } k_end_of_tile_table[] = {
    {0x82c, 0x10020300},
    {0x874, 0x1},
  };
  for (unsigned i = 0; i < sizeof(k_end_of_tile_table) / sizeof(k_end_of_tile_table[0]); ++i)
  {
    memcpy(at + k_end_of_tile_table[i].offset, &k_end_of_tile_table[i].value, sizeof(k_end_of_tile_table[i].value));
  }
}

void
agx_argument_table_set_image(const Agx_Bo* table, const Agx_Image* image)
{
  if (!table->cpu || table->size < 0x640)
  {
    return;
  }

  uint8_t* at = (uint8_t*)table->cpu;
  agx_image_write_descriptor(at + 0x620, image);

  uint64_t address = table->gpu_va + 0x620;
  memcpy(at + 0x600, &address, sizeof(address));
}

static void
agx_sampler_write_descriptor(void* at, const Agx_Sampler_Desc* sampler)
{
  agx_cmd(at, sampler, cfg)
  {
    cfg.min_lod = sampler->min_lod;
    cfg.max_lod = sampler->max_lod;
    cfg.max_aniso = sampler->max_aniso;
    cfg.mag_filter = sampler->mag_filter;
    cfg.min_filter = sampler->min_filter;
    cfg.mip_filter = sampler->mip_filter;
    cfg.wrap_s = sampler->wrap_s;
    cfg.wrap_t = sampler->wrap_t;
    cfg.wrap_r = sampler->wrap_r;
    cfg.non_normalized_coords = !sampler->normalized_coords;
    cfg.compare_mode = true;
    cfg.compare_func = sampler->compare_func;
    cfg.border_mode = sampler->border_mode;
  }
}

void
agx_argument_table_set_compute_sampler(const Agx_Bo* table, const Agx_Sampler_Desc* sampler)
{
  if (!table->cpu || table->size < 0x1520)
  {
    return;
  }

  uint8_t* at = (uint8_t*)table->cpu;

  agx_sampler_write_descriptor(at + 0x1500, sampler);

  uint64_t address = table->gpu_va + 0x1500;
  memcpy(at + 0x14c8, &address, sizeof(address));
}

void
agx_argument_table_set_compute_image(const Agx_Bo* table, const Agx_Image* image)
{
  if (!table->cpu || table->size < 0x1500)
  {
    return;
  }

  uint8_t* at = (uint8_t*)table->cpu;
  agx_image_write_descriptor(at + 0x14e0, image);

  uint64_t address = table->gpu_va + 0x14e0;
  memcpy(at + 0x14c0, &address, sizeof(address));
}

void
agx_pool_arguments_set_vertex_buffer(const Agx_Bo* arguments, uint32_t index, const Agx_Bo* buffer)
{
  if (arguments == NULL || arguments->cpu == NULL || buffer == NULL)
  {
    agx_refuse(
      "agx_pool_arguments_set_vertex_buffer: the arguments buffer is not mapped, or no "
      "buffer was given"
    );
    return;
  }
  if (index > AGX_VERTEX_BUFFER_INDEX_MAX)
  {
    agx_refuse(
      "agx_pool_arguments_set_vertex_buffer: index %u, and this hardware reads index %u "
      "whatever base the program encodes -- a second binding is not available, so an "
      "address written higher would be read by nothing",
      index,
      (uint32_t)AGX_VERTEX_BUFFER_INDEX_MAX
    );
    return;
  }
  agx_pool_arguments_set_geometry_pair(arguments->cpu, index, buffer->gpu_va);
}

void
agx_driver_arguments_set_render_target(const Agx_Bo* arguments, uint32_t block, uint32_t attachment, const Agx_Image* image)
{
  size_t offset = (size_t)block * 0x300u + 0x20u + (size_t)attachment * 0x20u;
  if (!arguments->cpu || arguments->size < offset + 0x20)
  {
    return;
  }

  agx_image_write_descriptor((uint8_t*)arguments->cpu + offset, image);
}

void
agx_driver_arguments_set_clear_color(const Agx_Bo* arguments, uint32_t block, uint32_t attachment, const float rgba[4])
{
  size_t offset = (size_t)block * 0x300u + 0x170u + (size_t)attachment * 0x10u;

  if (!arguments->cpu || arguments->size < offset + 4 * sizeof(float))
  {
    return;
  }

  memcpy((uint8_t*)arguments->cpu + offset, rgba, 4 * sizeof(float));
}

void
agx_render_pass_desc_init(Agx_Render_Pass_Desc* desc)
{
  memset(desc, 0, sizeof(*desc));
  desc->attachment_count = 1;
  desc->color_load_action = AGX_ATTACHMENT_LOAD_ACTION_CLEAR;
  desc->color_store_action = AGX_ATTACHMENT_STORE_ACTION_STORE;
  desc->depth_load_action = AGX_ATTACHMENT_LOAD_ACTION_CLEAR;
  desc->depth_clear = 1.0f;
  desc->depth_format = AGX_DEPTH_FORMAT_FLOAT32;
  desc->depth_store_action = AGX_ATTACHMENT_STORE_ACTION_DISCARD;
  desc->stencil_store_action = AGX_ATTACHMENT_STORE_ACTION_DISCARD;
}

void
agx_render_pass_desc_set_depth_format(Agx_Render_Pass_Desc* desc, Agx_Depth_Format format)
{
  if (desc != NULL)
  {
    desc->depth_format = format;
  }
}

void
agx_render_pass_desc_set_clear_color(Agx_Render_Pass_Desc* desc, const float rgba[4], Agx_Image_Channels channels)
{
  uint32_t packed = 0;
  uint32_t channel = 0;

  if (desc == NULL || rgba == NULL)
  {
    return;
  }
  if (channels != AGX_IMAGE_CHANNELS_R10G10B10A2)
  {
    for (channel = 0; channel < 4u; channel += 1)
    {
      desc->clear_color[channel] = rgba[channel];
    }
    return;
  }

  for (channel = 0; channel < 3u; channel += 1)
  {
    float    value = rgba[channel];
    uint32_t level = 0;

    value = value < 0.0f ? 0.0f : (value > 1.0f ? 1.0f : value);
    level = (uint32_t)(value * (float)AGX_UNORM10_MAX + 0.5f);
    packed |= (level & AGX_UNORM10_MAX) << (AGX_RGB10A2_COLOR_BITS * channel);
  }
  {
    float    alpha = rgba[3] < 0.0f ? 0.0f : (rgba[3] > 1.0f ? 1.0f : rgba[3]);
    uint32_t level = (uint32_t)(alpha * (float)AGX_UNORM2_MAX + 0.5f);

    packed |= (level & AGX_UNORM2_MAX) << AGX_RGB10A2_ALPHA_SHIFT;
  }

  for (channel = 0; channel < 4u; channel += 1)
  {
    desc->clear_color[channel] = (float)((packed >> (8u * channel)) & AGX_UNORM8_MAX) / (float)AGX_UNORM8_MAX;
  }
}

bool
agx_cmd_begin_render_pass(Agx_Cmd* cmd, const Agx_Render_Pass_Desc* desc)
{
  uint32_t length = AGX_RENDER_PASS_LENGTH +
                    (desc->attachment_count ? desc->attachment_count - 1u : 0u) * AGX_RENDER_PASS_ATTACHMENT_LENGTH;

  if (desc->threadgroup_memory_length != 0)
  {
    return false;
  }

  cmd->pass_attachments = desc->attachment_count;
  cmd->pass_samples = desc->sample_count ? desc->sample_count : 1u;
  cmd->pass_primitives = 0;

  if (cmd->driver_arguments_cpu)
  {
    size_t offset = cmd->render_passes == 0 ? 0x470u : 0x9e0u + (size_t)(cmd->render_passes - 1u) * 0x180u;

    uint32_t colors = desc->attachment_count ? desc->attachment_count : 1u;
    for (uint32_t i = 0; i < colors; ++i)
    {
      size_t at = offset + (size_t)i * 0x10u;
      if (at + 4 * sizeof(float) <= cmd->driver_arguments_bytes)
      {
        memcpy((uint8_t*)cmd->driver_arguments_cpu + at, desc->clear_color, 4 * sizeof(float));
      }
    }
  }
  cmd->render_passes++;

  uint8_t* record = agx_cmd_begin_encoder(cmd, length);

  if (cmd->stream.cpu)
  {
    cmd->cs = (uint8_t*)cmd->stream.cpu + cmd->stream_cursor;
    cmd->stream_gpu = cmd->stream.gpu_va + cmd->stream_cursor;
  }

  if (!record)
  {
    if (cmd->memory.cpu != NULL)
    {
      return agx_refuse(
        "agx_cmd_begin_render_pass: no room for a %u-byte record at %zu of %zu%s",
        length,
        cmd->cursor,
        cmd->memory.size,
        cmd->open_encoder ? ", and an encoder is still open" : ""
      );
    }

    return cmd->stream.cpu != NULL;
  }

  agx_cmd(record, render_pass, cfg)
  {
    cfg.stages = AGX_RECORD_STAGE_VERTEX;

    cfg.inline_command_bytes = 672u + 2368u * cmd->render_passes;
    cfg.inline_commands_present = true;
    cfg.kind_tag = 0x4;
    cfg.render_target_descriptors = 0x160000;

    cfg.few_primitives = true;
    cfg.tile_stores = 1;
    cfg.partial_load_action_mask = 0xffff80;

    const bool clearing = desc->color_load_action == AGX_ATTACHMENT_LOAD_ACTION_CLEAR;
    const bool loading = desc->color_load_action == AGX_ATTACHMENT_LOAD_ACTION_LOAD;
    const bool storing = desc->color_store_action == AGX_ATTACHMENT_STORE_ACTION_STORE;

    cfg.load_action = (clearing || loading) ? AGX_LOAD_ACTION_CLEAR_OR_LOAD : AGX_LOAD_ACTION_DONT_CARE;
    cfg.color_load_action_mask = (clearing || loading) ? 0xffff80 : 0;
    cfg.clear = clearing;
    cfg.any_load = loading;
    cfg.clear_and_store = clearing && storing;

    cfg.uniform_bind_count = 2;
    cfg.sample_count = desc->sample_count ? desc->sample_count : 1u;

    cfg.imageblock_sample_length = AGX_IMAGEBLOCK_SAMPLE_LENGTH;

    cfg.tile_width = AGX_TILE_WIDTH;
    cfg.tile_height = cfg.sample_count > 2u ? AGX_TILE_HEIGHT / 2u : AGX_TILE_HEIGHT;

    const uint32_t tile_memory_bytes =
      cfg.tile_width * cfg.tile_height * AGX_IMAGEBLOCK_SAMPLE_LENGTH * cfg.sample_count + desc->threadgroup_memory_length;

    const uint32_t tile_memory_blocks = tile_memory_bytes / 2048u;
    const uint32_t tile_memory_steps = tile_memory_blocks > 4u ? tile_memory_blocks - 4u : 0u;

    cfg.tile_memory_request = false;
    cfg.tile_memory_request_copy = false;
    cfg.tile_memory_blocks = tile_memory_blocks;

    cfg.tile_memory_samples = cfg.sample_count > 1u ? cfg.sample_count : 0u;
    cfg.unk_624 = 640u + 6u * tile_memory_steps;

    cfg.zls_control = desc->stencil_buffer ? 0x154u : 0x44u;

    if (desc->stencil_buffer && desc->stencil_store_action == AGX_ATTACHMENT_STORE_ACTION_STORE)
    {
      cfg.zls_control |= 0x40000u;
    }
    if (desc->depth_buffer && desc->depth_store_action == AGX_ATTACHMENT_STORE_ACTION_STORE)
    {
      cfg.zls_control |= 0x80000u;
    }
    if (desc->depth_buffer && desc->depth_load_action == AGX_ATTACHMENT_LOAD_ACTION_LOAD)
    {
      cfg.zls_control |= 0x8000u;
    }

    if (desc->depth_format == AGX_DEPTH_FORMAT_UNORM16)
    {
      float    clamped = desc->depth_clear < 0.0f ? 0.0f : (desc->depth_clear > 1.0f ? 1.0f : desc->depth_clear);
      uint32_t level = (uint32_t)(clamped * (float)AGX_UNORM16_MAX + 0.5f);
      float    carried = 0.0f;

      memcpy(&carried, &level, sizeof(carried));
      cfg.depth_clear = carried;
    }
    else
    {
      cfg.depth_clear = desc->depth_clear;
    }
    cfg.depth_format = desc->depth_format;
    cfg.stencil_clear = desc->stencil_clear;
    cfg.depth_unk_4a4 = 0x007f80ff;

    static const uint8_t k_sample_positions[3][8] = {
      {8, 8},
      {12, 12, 4, 4},
      {6, 2, 14, 6, 2, 10, 10, 14},
    };
    const uint8_t* positions = k_sample_positions[cfg.sample_count == 4u ? 2 : cfg.sample_count == 2u ? 1 : 0];
    cfg.sample_position_0 = positions[0];
    cfg.sample_position_1 = positions[1];
    cfg.sample_position_2 = positions[2];
    cfg.sample_position_3 = positions[3];
    cfg.sample_position_4 = positions[4];
    cfg.sample_position_5 = positions[5];
    cfg.sample_position_6 = positions[6];
    cfg.sample_position_7 = positions[7];

    cfg.attachment_count = desc->attachment_count;

    cfg.run_token = 0x124f716e + cmd->render_passes - 1u;

    cfg.width = desc->width;
    cfg.height = desc->height;

    cfg.width_copy = desc->width;
    cfg.height_copy = desc->height;

    cfg.background_program = desc->background_program;
    cfg.store_program = desc->store_program;

    cfg.store_program_copy = desc->store_program;

    cfg.partial_background_program = desc->background_program;
    cfg.partial_store_program = desc->store_program;
    cfg.partial_store_program_copy = desc->store_program;
    cfg.unknown_program = desc->tile_program_heap;

    cfg.uniform_slice = desc->uniform_slice;

    cfg.scissor_buffer = cmd->scissor_buffer_gpu;
    cfg.depth_bias_table = cmd->depth_bias_table_gpu;

    if (desc->depth_buffer)
    {
      uint64_t meta = desc->depth_buffer + AGX_IMAGE_COLOR_BYTES(desc->width, desc->height);
      cfg.depth_buffer = desc->depth_buffer;
      cfg.depth_buffer_copy = desc->depth_buffer;
      cfg.depth_buffer_partial_render = desc->depth_buffer;
      cfg.depth_meta = meta;
      cfg.depth_meta_copy = meta;
      cfg.depth_meta_partial_render = meta;
    }

    if (desc->stencil_buffer)
    {
      uint64_t meta = desc->stencil_buffer + AGX_STENCIL_PLANE_BYTES(desc->width, desc->height);
      cfg.stencil_buffer = desc->stencil_buffer;
      cfg.stencil_buffer_copy = desc->stencil_buffer;
      cfg.stencil_buffer_partial_render = desc->stencil_buffer;
      cfg.stencil_meta = meta;
      cfg.stencil_meta_copy = meta;
      cfg.stencil_meta_partial_render = meta;
    }

    {
      const uint64_t run = cmd->stream.gpu_va + cmd->stream_cursor;

      cfg.encoder_stream = run & ~(uint64_t)0xff;
      cfg.encoder_stream_low = (uint32_t)(run & 0xffu);
    }

    cfg.unk_84 = 0x4;
    cfg.unk_a0 = 0x9d0;
    cfg.unk_a4 = 0x1;

    cfg.unk_bc = AGX_RECORD_STAGE_FRAGMENT;
    cfg.unk_12c = 0x68020;
    cfg.unk_17c = 0x6b0003;
    cfg.unk_180 = 0x3a0012;
    cfg.unk_184 = 0x1;
    cfg.unk_206 = 0xad;
    cfg.unk_214 = 0x1fc;
    cfg.unk_2ed = 0x680;
    cfg.unk_2f1 = 0x1000000;
    cfg.unk_2f9 = 0x1c000000;
    cfg.unk_310 = 0xffffffff;
    cfg.unk_314 = 0xffffffff;
    cfg.unk_318 = 0xffffffff;
    cfg.unk_380 = 0x8;
    cfg.unk_434 = 0x40;
    cfg.unk_494 = 0x4040404;
    cfg.unk_555 = 0xc0;
    cfg.unk_566 = 0x10;

    cfg.unk_615 = 0xca80u + 0x40u * tile_memory_steps;

    cfg.unk_61d = 128u * (1012u + 7u * tile_memory_steps + (tile_memory_steps + 1u) / 2u);
    cfg.unk_621 = 0x0;
    cfg.unk_71c = 0xffffffff;
    cfg.unk_720 = 0xffffffff;
    cfg.unk_724 = 0xffffffff;
    cfg.unk_770 = 0x8;
    cfg.unk_7ac = 0x40;
    cfg.unk_8cc = 0x1c;
    cfg.unk_8e0 = 0xffffffff;
    cfg.unk_95c = 0x1;
    cfg.unk_974 = 0x1;
    cfg.unk_a9c = 0x1;
  }

  return true;
}

static uint32_t
agx_cmd_attachment_samples(const Agx_Cmd* cmd, uint32_t index)
{
  if (cmd->pass_samples <= 1u)
  {
    return 1u;
  }
  return (index & 1u) ? cmd->pass_samples : 1u;
}

static uint32_t
agx_cmd_attachment_share(const Agx_Cmd* cmd, uint32_t index)
{
  uint32_t entries = cmd->pass_attachments ? cmd->pass_attachments : 1u;
  uint32_t total = 0;
  for (uint32_t a = 0; a < entries; ++a)
  {
    total += agx_cmd_attachment_samples(cmd, a);
  }
  if (total == 0)
  {
    total = 1u;
  }
  return 0x01000000u | ((100u * agx_cmd_attachment_samples(cmd, index) + total / 2u) / total);
}

void
agx_cmd_set_render_target(Agx_Cmd* cmd, uint32_t index, const Agx_Bo* image, uint32_t width, uint32_t height, Agx_Image_Channels channels)
{
  if (!cmd->open_encoder)
  {
    return;
  }

  if (index == 0)
  {
    uint32_t low = (uint32_t)image->gpu_va;
    memcpy(cmd->open_encoder + 0xaa4, &low, sizeof(low));
  }

  uint8_t* at = cmd->open_encoder + 0xaa8 + (size_t)index * AGX_RENDER_PASS_ATTACHMENT_LENGTH;

  agx_cmd(at, render_pass_attachment, cfg)
  {
    cfg.unk_0 = 0x100;
    cfg.slot_tag = image->index;
    cfg.block_bytes =
      AGX_IMAGE_BLOCK_BYTES(width, height, agx_image_channels_bytes(channels)) * agx_cmd_attachment_samples(cmd, index);
    cfg.unk_c = 0xc;
    cfg.share = agx_cmd_attachment_share(cmd, index);

    cfg.next_slot = index == 0 ? 8 : 0;
  }
}

size_t
agx_cmd_end_render_pass(Agx_Cmd* cmd)
{
  if (cmd->open_encoder)
  {
    uint16_t size = (uint16_t)(AGX_RENDER_PASS_LENGTH + AGX_RENDER_PASS_ATTACHMENT_LENGTH *
                                                          (cmd->pass_attachments ? cmd->pass_attachments - 1u : 0u));

    uint8_t attachment_bytes = (uint8_t)(AGX_RENDER_PASS_ATTACHMENT_LENGTH * cmd->pass_attachments);
    memcpy(cmd->open_encoder + 0x00, &size, sizeof(size));
    memcpy(cmd->open_encoder + 0x94, &attachment_bytes, sizeof(attachment_bytes));

    cmd->open_encoder[0x554] = (uint8_t)((cmd->open_encoder[0x554] & ~1u) | (cmd->pass_primitives <= 4u ? 1u : 0u));
  }

  if (!cmd->cs)
  {
    agx_cmd_end_encoder(cmd);
    return 0;
  }

  static const uint32_t k_terminator = 0xC0000000u;
  uint8_t*              base = (uint8_t*)cmd->stream.cpu + cmd->stream_cursor;
  if ((size_t)(cmd->cs - (uint8_t*)cmd->stream.cpu) + sizeof(k_terminator) <= cmd->stream.size)
  {
    memcpy(cmd->cs, &k_terminator, sizeof(k_terminator));
    cmd->cs += sizeof(k_terminator);
  }

  size_t pushed = (size_t)(cmd->cs - base);
  cmd->stream_cursor += pushed;

  if (cmd->render_passes)
  {
    cmd->stream_cursor = (size_t)cmd->render_passes * 136u;
  }

  cmd->cs = NULL;

  agx_cmd_end_encoder(cmd);
  return pushed;
}

size_t
agx_cmd_end(Agx_Cmd* cmd)
{
  agx_cmd_end_encoder(cmd);
  return cmd->cursor;
}

bool
agx_queue_submit(Agx_Command_Queue queue, Agx_Cmd* cmds, uint32_t cmd_count)
{
  if (cmd_count == 0 || cmd_count > AGX_MAX_SUBMIT_COMMAND_BUFFERS)
  {
    return false;
  }

  Agx_Submit_Cmd_Req reqs[AGX_MAX_SUBMIT_COMMAND_BUFFERS] = {};
  for (uint32_t i = 0; i < cmd_count; ++i)
  {
    reqs[i].command_buffer_shmem_id = cmds[i].memory.index;
    reqs[i].segment_list_shmem_id = cmds[i].segment_list.index;

    reqs[i].completion_token = cmds[i].completion_token;
  }

  if (cmd_count > 1)
  {
    uint32_t ids[4] = {0};
    size_t   ids_size = sizeof(ids);
    if (IOConnectCallStructMethod(queue.agx, AGX_SELECTOR_GET_GLOBAL_IDS, NULL, 0, ids, &ids_size) == 0 &&
        ids_size == sizeof(ids))
    {
      for (uint32_t i = 0; i < cmd_count; ++i)
      {
        reqs[i].unknown_0x28 = ids[0] + 3u * i;
        reqs[i].unknown_0x2c = ids[1];
      }
    }
  }

  kern_return_t ret;
  if (cmd_count == 1)
  {
    ret = IOConnectTrap4(
      agx_trap_connection(queue.agx), AGX_SELECTOR_TRAP4_SUBMIT_COMMAND_BUFFERS, queue.id, sizeof(reqs[0]), (uintptr_t)reqs, 0
    );
  }
  else
  {
    uint64_t scalars[4] = {queue.id, 0, cmd_count, sizeof(reqs[0])};
    ret = IOConnectCallMethod(
      queue.agx, AGX_SELECTOR_SUBMIT_COMMAND_BUFFERS, scalars, 4, reqs, cmd_count * sizeof(reqs[0]), NULL, NULL, NULL, NULL
    );
  }

  if (ret)
  {
    fprintf(stderr, "agx_queue_submit: submit failed, kern_return_t=0x%x (%u command buffer(s))\n", ret, cmd_count);
    return false;
  }

  if (queue.progress)
  {
    queue.progress->submitted++;
  }

  return true;
}

void
agx_cmd_residency_begin(Agx_Cmd* cmd, Agx_Segment_List_Builder* builder)
{
  agx_segment_list_begin(builder, &cmd->segment_list);
}

static void
agx_cmd_push_vertex_pipeline_section(Agx_Cmd* cmd, uint32_t pipeline, uint64_t program_buffer)
{
  agx_push(cmd->cs, vdm_state_pipeline, cfg)
  {
    cfg.pipeline = pipeline;

    cfg.vertex_caller_count = AGX_VERTEX_PIPELINE_CALLER_COUNT;

    cfg.program_buffer = program_buffer - AGX_MAIN_SPACE_BASE;
    cfg.vertex_amplify = cmd->vertex_amplify;
  }
}

void
agx_cmd_bind_vertex_pipeline(Agx_Cmd* cmd, Agx_Vertex_Pipeline pipeline, uint64_t program_buffer, uint32_t varying_components)
{
  agx_push(cmd->cs, vdm_state_header, cfg)
  {
    cfg.present = 0x2e;
  }

  agx_cmd_push_vertex_pipeline_section(cmd, (uint32_t)pipeline, program_buffer);

  agx_push(cmd->cs, vdm_state_varyings, cfg)
  {
    cfg.vertex_output_count = varying_components + 4 + cmd->clip_distance_count;
    cfg.vertex_output_count_copy = cmd->varying_components_copy != 0
                                   ? cmd->varying_components_copy + 4 + cmd->clip_distance_count
                                   : varying_components + 4 + cmd->clip_distance_count;
  }

  agx_push(cmd->cs, vdm_state_unk_word, cfg)
  {
  }

  agx_push(cmd->cs, vdm_state_vertex_unknown, cfg)
  {
    cfg.unk_4 = cmd->rasterizer_discard;
    cfg.unk_5 = cmd->rasterizer_discard;
    cfg.generate_primitive_id = cmd->reads_primitive_id;
  }
}

void
agx_cmd_rebind_vertex_pipeline(Agx_Cmd* cmd, Agx_Vertex_Pipeline pipeline, uint64_t program_buffer)
{
  agx_push(cmd->cs, vdm_state_header, cfg)
  {
    cfg.present = 0x02;
  }

  agx_cmd_push_vertex_pipeline_section(cmd, (uint32_t)pipeline, program_buffer);
}

void
agx_cmd_set_vertex_amplification(Agx_Cmd* cmd, bool enabled)
{
  cmd->vertex_amplify = enabled;
}

void
agx_cmd_set_varying_components_copy(Agx_Cmd* cmd, uint32_t components)
{
  cmd->varying_components_copy = components;
}

void
agx_cmd_push_state(Agx_Cmd* cmd, uint64_t address, uint32_t words)
{
  agx_push(cmd->cs, ppp_state, cfg)
  {
    cfg.pointer_hi = (address >> 32) & 0xff;
    cfg.size_words = words;
    cfg.pointer = (uint32_t)address;
  }
}

void
agx_driver_state_write(const Agx_Bo* state, uint32_t varying_components)
{
  enum
  {
    AGX_DRIVER_STATE_OFFSET = 0x40,
  };
  uint8_t* at = NULL;

  if (state == NULL || state->cpu == NULL || state->size < AGX_DRIVER_STATE_OFFSET + 8u)
  {
    return;
  }
  at = (uint8_t*)state->cpu + AGX_DRIVER_STATE_OFFSET;

  agx_cmd(at, ppp_header, cfg)
  {
    cfg.viewport_count = 1;
    cfg.pres_varying_words = true;
    cfg.pres_amplify_varying_words = true;
  }
  memcpy(at + 4, &varying_components, sizeof(varying_components));
}

void
agx_cmd_push_driver_state(Agx_Cmd* cmd, const Agx_Bo* state)
{
  enum
  {
    AGX_DRIVER_STATE_OFFSET = 0x40,
    AGX_DRIVER_STATE_WORDS = 5,
  };

  agx_cmd_push_state(cmd, state->gpu_va + AGX_DRIVER_STATE_OFFSET, AGX_DRIVER_STATE_WORDS);
}

void
agx_cmd_set_pipeline_state(Agx_Cmd* cmd, const Agx_Bo* state, uint64_t offset)
{
  static const struct
  {
    uint32_t offset;
    uint32_t words;
  } k_blocks[] = {
    {0x00, 7},
    {0x1c, 5},
    {0x30, 7},
    {0x4c, 5},
    {0x60, 3},
  };

  const uint32_t first = cmd->rasterizer_discard ? 1u : 0u;
  const uint32_t shift = cmd->rasterizer_discard ? AGX_PIPELINE_FRG_SHADER_WORDS_BYTES : 0u;

  for (uint32_t i = first; i < sizeof(k_blocks) / sizeof(k_blocks[0]); ++i)
  {
    agx_cmd_push_state(cmd, state->gpu_va + offset + k_blocks[i].offset - shift, k_blocks[i].words);
  }
}

void
agx_cmd_set_depth_stencil_state(Agx_Cmd* cmd, const Agx_Depth_Stencil_Desc* desc)
{
  cmd->depth_stencil = *desc;
}

void
agx_cmd_set_depth_compare_function(Agx_Cmd* cmd, Agx_Compare_Function compare)
{
  cmd->depth_stencil.depth_compare = compare;
}

void
agx_cmd_set_depth_write_enabled(Agx_Cmd* cmd, bool enabled)
{
  cmd->depth_stencil.depth_write = enabled;
}

void
agx_cmd_set_stencil_state(Agx_Cmd* cmd, Agx_Stencil_State state)
{
  agx_depth_stencil_desc_set_stencil(&cmd->depth_stencil, state);
}

void
agx_cmd_set_stencil_state_two_sided(Agx_Cmd* cmd, Agx_Stencil_State front, Agx_Stencil_State back)
{
  agx_depth_stencil_desc_set_stencil_two_sided(&cmd->depth_stencil, front, back);
}

void
agx_cmd_set_stencil_reference_value(Agx_Cmd* cmd, uint8_t reference)
{
  cmd->depth_stencil.stencil_front.reference = reference;
  cmd->depth_stencil.stencil_back.reference = reference;
}

void
agx_cmd_set_stencil_reference_values(Agx_Cmd* cmd, uint8_t front, uint8_t back)
{
  cmd->depth_stencil.stencil_front.reference = front;
  cmd->depth_stencil.stencil_back.reference = back;

  cmd->depth_stencil.two_sided = cmd->depth_stencil.two_sided || front != back;
}

void
agx_cmd_set_triangle_fill_mode(Agx_Cmd* cmd, Agx_Triangle_Fill_Mode mode)
{
  cmd->fill_mode = mode;
}

static Agx_Objtype
agx_objtype_for_primitive(Agx_Primitive primitive)
{
  switch (primitive)
  {
  case AGX_PRIMITIVE_LINES:
  case AGX_PRIMITIVE_LINE_STRIP:
    return AGX_OBJTYPE_LINES;
  case AGX_PRIMITIVE_POINTS:
    return AGX_OBJTYPE_POINTS;
  default:
    return AGX_OBJTYPE_TRIANGLES;
  }
}

static size_t
agx_face_control_write(const Agx_Cmd* cmd, Agx_Objtype objtype, void* state_cpu)
{
  uint8_t* p = (uint8_t*)state_cpu;

  agx_cmd(p, ppp_header, cfg)
  {
    cfg.pres_frg_face_ctl_f_pso = true;
    cfg.pres_frg_face_ctl_b_pso = true;
    cfg.pres_frg_vis_query_tag_flush_pso = true;
    cfg.viewport_count = 1;
  }

  agx_cmd(p + 4, ppp_frg_common_ctl, cfg)
  {
    cfg.tagsort_flush_ctl = 2;

    cfg.tri_merge_disable = agx_tri_merge_disable(objtype, cmd->pass_type);

    cfg.passtype = (uint32_t)cmd->pass_type;
  }

  for (uint32_t i = 0; i < 2; ++i)
  {
    agx_cmd(p + 8 + i * 4, ppp_frg_face_ctl_pso, cfg)
    {
      cfg.dwritedisable = true;
      cfg.fs_depth_dir_qual = 3;
      cfg.dcmpmode = 7;
      cfg.objtype = (uint32_t)objtype;
    }
  }
  agx_cmd(p + 16, ppp_frg_vis_query_tag_flush_pso, cfg)
  {
    cfg.tagsort_flush_data = cmd->tagsort_flush_data;
  }

  return 20;
}

static void
agx_cmd_push_arena_block(Agx_Cmd* cmd, size_t most, size_t (*write)(Agx_Cmd*, Agx_Primitive, void*), Agx_Primitive primitive)
{
  if (!cmd->state_arena_cpu || cmd->state_arena_cursor + most > cmd->state_arena_bytes)
  {
    return;
  }

  size_t length = write(cmd, primitive, (uint8_t*)cmd->state_arena_cpu + cmd->state_arena_cursor);
  if (length == 0)
  {
    return;
  }

  agx_cmd_push_state(cmd, cmd->state_arena_gpu + cmd->state_arena_cursor, (uint32_t)(length / 4));
  cmd->state_arena_cursor += length;
}

static size_t
agx_cmd_write_face_state(Agx_Cmd* cmd, Agx_Primitive primitive, void* block)
{
  return agx_face_state_write(cmd, &cmd->depth_stencil, cmd->fill_mode, agx_objtype_for_primitive(primitive), block);
}

static size_t
agx_cmd_write_face_control(Agx_Cmd* cmd, Agx_Primitive primitive, void* block)
{
  return agx_face_control_write(cmd, agx_objtype_for_primitive(primitive), block);
}

static size_t
agx_cmd_write_raster_state(Agx_Cmd* cmd, Agx_Primitive primitive, void* block)
{
  (void)primitive;

  if (!cmd->has_viewport && !cmd->has_scissor)
  {
    return 0;
  }

  uint8_t* p = (uint8_t*)block;
  uint32_t count = cmd->viewport_count ? cmd->viewport_count : 1;

  agx_push(p, ppp_header, cfg)
  {
    cfg.pres_region_clip = cmd->has_scissor;
    cfg.pres_viewport = cmd->has_viewport;
    cfg.viewport_count = count;
  }

  if (cmd->has_scissor)
  {
    for (uint32_t v = 0; v < count; ++v)
    {
      Agx_Scissor_Desc sc = cmd->scissors[v];
      if (!cmd->scissor_set[v])
      {
        const Agx_Viewport_Desc* vp = &cmd->viewports[v];
        sc.x = (uint32_t)(vp->x < 0.0f ? 0.0f : vp->x);
        sc.y = (uint32_t)(vp->y < 0.0f ? 0.0f : vp->y);
        sc.width = (uint32_t)(vp->width < 0.0f ? 0.0f : vp->width);
        sc.height = (uint32_t)(vp->height < 0.0f ? 0.0f : vp->height);
      }

      agx_push(p, region_clip, cfg)
      {
        cfg.enable = true;
        cfg.x_min = sc.x / AGX_SCISSOR_TILE;
        cfg.y_min = sc.y / AGX_SCISSOR_TILE;
        cfg.x_max = (sc.x + sc.width + AGX_SCISSOR_TILE - 1) / AGX_SCISSOR_TILE - 1;
        cfg.y_max = (sc.y + sc.height + AGX_SCISSOR_TILE - 1) / AGX_SCISSOR_TILE - 1;
      }
    }
  }

  if (cmd->has_viewport)
  {
    agx_push(p, viewport_control, cfg)
    {
      cfg.last_viewport = count;
    }
    for (uint32_t v = 0; v < count; ++v)
    {
      const Agx_Viewport_Desc* vp = &cmd->viewports[v];
      agx_push(p, viewport, cfg)
      {
        cfg.translate_x = vp->x + vp->width / 2.0f;
        cfg.scale_x = vp->width / 2.0f;
        cfg.translate_y = vp->y + vp->height / 2.0f;
        cfg.scale_y = -vp->height / 2.0f;
        cfg.near_z = vp->near_z;
        cfg.far_z = vp->far_z;
      }
    }
  }

  return (size_t)(p - (uint8_t*)block);
}

static size_t
agx_cmd_write_control_state(Agx_Cmd* cmd, Agx_Primitive primitive, void* block)
{
  (void)primitive;
  uint8_t* p = (uint8_t*)block;

  agx_push(p, ppp_header, cfg)
  {
    cfg.pres_pppctrl = true;

    cfg.viewport_count = 1;
  }
  agx_push(p, ppp_control, cfg)
  {
    cfg.cullmode = (uint32_t)cmd->cull_mode;
    cfg.front_face_dir = cmd->front_face_winding == AGX_WINDING_COUNTER_CLOCKWISE;
    cfg.clip_mode = (uint32_t)cmd->depth_clip_mode;
    cfg.wbuffen = cmd->depth_source == AGX_DEPTH_SOURCE_W;
    cfg.rasterizer_discard = cmd->rasterizer_discard;
    cfg.pretransform = cmd->pretransform;

    cfg.primitive_id_pres = cmd->reads_primitive_id;
  }

  return (size_t)(p - (uint8_t*)block);
}

static size_t
agx_cmd_write_dbsc_state(Agx_Cmd* cmd, Agx_Primitive primitive, void* block)
{
  (void)primitive;
  uint8_t* p = (uint8_t*)block;

  agx_push(p, ppp_header, cfg)
  {
    cfg.pres_frg_dbsc = true;
    cfg.viewport_count = 1;
  }
  agx_push(p, ppp_frg_dbsc, cfg)
  {
    cfg.scissor_index = cmd->scissor_index;
    cfg.depth_bias_index = cmd->depth_bias_index;
  }

  return (size_t)(p - (uint8_t*)block);
}

void
agx_cmd_set_viewport(Agx_Cmd* cmd, uint32_t index, Agx_Viewport_Desc viewport)
{
  if (index >= AGX_MAX_VIEWPORTS)
  {
    return;
  }
  cmd->viewports[index] = viewport;
  cmd->has_viewport = true;

  cmd->has_scissor = true;
  if (index + 1 > cmd->viewport_count)
  {
    cmd->viewport_count = index + 1;
  }
}

void
agx_cmd_set_scissor_rect(Agx_Cmd* cmd, uint32_t index, Agx_Scissor_Desc scissor)
{
  if (index >= AGX_MAX_VIEWPORTS)
  {
    return;
  }
  cmd->scissors[index] = scissor;
  cmd->scissor_set[index] = true;
  cmd->has_scissor = true;
  if (index + 1 > cmd->viewport_count)
  {
    cmd->viewport_count = index + 1;
  }
}

static void
agx_cmd_push_draw_state(Agx_Cmd* cmd, Agx_Primitive primitive)
{
  size_t raster_max = 4 + 4 + (size_t)(cmd->viewport_count ? cmd->viewport_count : 1) * (8 + 24);
  agx_cmd_push_arena_block(cmd, raster_max, agx_cmd_write_raster_state, primitive);
  agx_cmd_push_arena_block(cmd, 8, agx_cmd_write_control_state, primitive);
  agx_cmd_push_arena_block(cmd, 8, agx_cmd_write_dbsc_state, primitive);
  agx_cmd_push_arena_block(cmd, AGX_DEPTH_STENCIL_STATE_LENGTH, agx_cmd_write_face_state, primitive);
  agx_cmd_push_arena_block(cmd, 20, agx_cmd_write_face_control, primitive);
}

void
agx_cmd_set_dynamic_state(Agx_Cmd* cmd, const Agx_Bo* memory, uint64_t offset, size_t bytes)
{
  cmd->state_arena_cpu = memory->cpu ? (uint8_t*)memory->cpu + offset : NULL;
  cmd->state_arena_gpu = memory->gpu_va ? memory->gpu_va + offset : 0;
  cmd->state_arena_bytes = bytes;
  cmd->state_arena_cursor = 0;
}

void
agx_cmd_set_point_line_width(Agx_Cmd* cmd, uint32_t width)
{
  cmd->point_line_width = width;
}

void
agx_work_extent_write(void* geometry_cpu, Agx_Work_Extent primary, Agx_Work_Extent secondary)
{
  uint8_t* g = (uint8_t*)geometry_cpu;
  memcpy(g + 0x3c, &primary.x, sizeof(uint32_t));
  memcpy(g + 0x40, &primary.y, sizeof(uint32_t));
  memcpy(g + 0x44, &primary.z, sizeof(uint32_t));
  memcpy(g + 0x48, &secondary.x, sizeof(uint32_t));
  memcpy(g + 0x4c, &secondary.y, sizeof(uint32_t));
  memcpy(g + 0x50, &secondary.z, sizeof(uint32_t));
}

void
agx_work_extent_for_buffer_copy(uint64_t bytes, Agx_Work_Extent* source, Agx_Work_Extent* tile)
{
  uint32_t blocks = (uint32_t)(bytes / 16u);
  *source = (Agx_Work_Extent) {blocks, 1u, 1u};
  *tile = (Agx_Work_Extent) {blocks, 1u, 1u};
}

static const uint32_t k_ops_open = 0x60000160u;

static bool
agx_ops_program_granule_used(const uint8_t* granule)
{
  for (size_t i = 0; i < 0x40u; ++i)
  {
    if (granule[i])
    {
      return true;
    }
  }
  return false;
}

static size_t
agx_ops_program_length(const uint8_t* program)
{
  for (size_t i = 0; i + 2 <= AGX_OPS_PROGRAM_SCAN_BYTES; i += 2)
  {
    if (program[i] == 0x0eu && program[i + 1] == 0x00u)
    {
      return i + 2;
    }
  }
  return 0;
}

static void
agx_ops_program_ensure(Agx_Cmd* cmd)
{
  if (!cmd->ops_programs_cpu || !cmd->ops_cursor)
  {
    return;
  }

  uint8_t* base = (uint8_t*)cmd->ops_programs_cpu;
  uint8_t* at = base + (size_t)cmd->ops_cursor * 0x40u;
  if (agx_ops_program_granule_used(at))
  {
    return;
  }

  size_t   source = 0;
  bool     found = false;
  uint32_t granule = 0;
  while (granule < cmd->ops_cursor)
  {
    if (!agx_ops_program_granule_used(base + (size_t)granule * 0x40u))
    {
      granule += 1;
      continue;
    }
    source = (size_t)granule * 0x40u;
    found = true;
    size_t length = agx_ops_program_length(base + source);
    if (!length)
    {
      break;
    }
    granule += (uint32_t)((length + 0x3fu) / 0x40u);
  }
  if (!found)
  {
    return;
  }

  size_t length = agx_ops_program_length(base + source);
  if (!length)
  {
    return;
  }

  if (cmd->ops_operands_gpu == cmd->ops_operands_first)
  {
    memcpy(at, base + source, length);
    return;
  }

  if (length < AGX_OPS_PROGRAM_OPERAND_IMMEDIATE_OFFSET + 2u)
  {
    return;
  }

  {
    const uint16_t want = (uint16_t)((cmd->ops_operands_first >> AGX_OPS_PROGRAM_OPERAND_IMMEDIATE_SHIFT) & 0xffffu);
    const uint16_t now = (uint16_t)(base[source + AGX_OPS_PROGRAM_OPERAND_IMMEDIATE_OFFSET] |
                                    ((uint16_t)base[source + AGX_OPS_PROGRAM_OPERAND_IMMEDIATE_OFFSET + 1u] << 8));
    uint16_t       relocated = 0;

    if (now != want)
    {
      agx_refuse(
        "agx_ops_program_ensure: the program at +0x%zx does not carry the first operand "
        "buffer's immediate at +0x%x -- it reads 0x%04x where 0x%04x was expected -- so "
        "its shape is not the one relocation was measured on. The cursor is left empty",
        source,
        (unsigned)AGX_OPS_PROGRAM_OPERAND_IMMEDIATE_OFFSET,
        now,
        want
      );
      return;
    }

    relocated = (uint16_t)((cmd->ops_operands_gpu >> AGX_OPS_PROGRAM_OPERAND_IMMEDIATE_SHIFT) & 0xffffu);
    memcpy(at, base + source, length);
    at[AGX_OPS_PROGRAM_OPERAND_IMMEDIATE_OFFSET] = (uint8_t)(relocated & 0xffu);
    at[AGX_OPS_PROGRAM_OPERAND_IMMEDIATE_OFFSET + 1u] = (uint8_t)((relocated >> 8) & 0xffu);
  }
}

static void
agx_ops_append(Agx_Cmd* cmd, uint32_t marker, uint32_t kind_word, uint32_t cursor_step, Agx_Work_Extent primary, Agx_Work_Extent secondary)
{
  if (!cmd->ops)
  {
    return;
  }

  agx_ops_program_ensure(cmd);

  uint8_t* at = cmd->ops;
  agx_cmd(at, encoder_operation, cfg)
  {
    cfg.marker = marker;
    cfg.kind = kind_word;
    cfg.program = cmd->ops_program;
    cfg.cursor = cmd->ops_cursor;
    cfg.tag = 0x40000001u;
    cfg.primary_x = primary.x;
    cfg.primary_y = primary.y;
    cfg.primary_z = primary.z;
    cfg.secondary_x = secondary.x;
    cfg.secondary_y = secondary.y;
    cfg.secondary_z = secondary.z;
  }

  cmd->ops += AGX_ENCODER_OPERATION_LENGTH;
  cmd->ops_cursor += cursor_step;
}

static void
agx_copy_shaped_record_write(
  uint8_t*             record,
  uint32_t             run_token,
  const Agx_Copy_Desc* desc,
  uint64_t             stream,
  uint32_t             stages,
  Agx_Stage            barrier_after,
  Agx_Stage            barrier_before
)
{
  agx_cmd(record, copy_record, cfg)
  {
    cfg.size = AGX_COPY_RECORD_LENGTH;
    cfg.stages = stages;
    cfg.run_token = run_token;

    if (desc)
    {
      cfg.pointer_c4 = stream;
      cfg.source_address_bits_15_to_23 = (uint32_t)((stream >> 15) & 0x1ffu);
      cfg.source_address_bits_32_to_63 = (uint32_t)(stream >> 32);

      cfg.pointer_11c = desc->operands;
      cfg.pointer_134 = desc->operands + 0x1480;
      cfg.pointer_13c = desc->operands + 0x1488;
      cfg.pointer_144 = desc->operands + 0x1490;
      cfg.pointer_14c = desc->operands + 0x1498;
      cfg.pointer_15c = desc->descriptors;
    }

    cfg.barrier_consumer_after_stages = (uint32_t)barrier_after;
    cfg.barrier_consumer_before_stages = (uint32_t)barrier_before;
    cfg.barrier_producer_after_stages = (uint32_t)barrier_after;
    cfg.barrier_producer_before_stages = (uint32_t)barrier_before;

    cfg.unk_84 = 0x4;
    cfg.unk_a0 = 0x268;
    cfg.unk_a4 = 0x3;
    cfg.unk_154 = 0x1;
    cfg.unk_164 = 0x4;
    cfg.unk_168 = 0x1c;
    cfg.unk_190 = 0xffffffff;
    cfg.unk_214 = 0x1318000;
    cfg.unk_21c = 0x1f28000;
    cfg.unk_224 = 0x274;
    cfg.unk_2f8 = 0x8;
    cfg.unk_30c = 0x10000;
    cfg.unk_318 = 0xffffffff;
    cfg.unk_31c = 0xffffffff;
    cfg.unk_320 = 0xffffffff;

    cfg.encoder_follows = false;
  }
}

bool
agx_cmd_begin_copy_encoder(Agx_Cmd* cmd, const Agx_Copy_Desc* desc)
{
  uint8_t* copy = agx_cmd_begin_encoder(cmd, AGX_COPY_RECORD_LENGTH);
  if (!copy)
  {
    return false;
  }

  cmd->copy_records++;

  agx_copy_shaped_record_write(
    copy, 0x124f716du + (cmd->copy_records - 1u), desc, desc->geometry + cmd->ops_offset, AGX_RECORD_STAGE_BLIT, AGX_STAGE_NONE, AGX_STAGE_NONE
  );

  cmd->ops = NULL;
  cmd->ops_base = NULL;
  cmd->ops_program = (uint32_t)((desc->programs & 0xffffffull) >> 14);
  cmd->ops_programs_cpu = desc->programs_cpu;

  cmd->ops_cursor += 4;
  cmd->ops_operands = desc->operands_cpu;
  cmd->ops_operands_gpu = desc->operands;
  if (!cmd->ops_operands_first)
  {
    cmd->ops_operands_first = desc->operands;
  }
  cmd->last_copy_desc = *desc;
  cmd->last_copy_desc_valid = true;
  cmd->ops_blocks = 0;
  if (desc->geometry_cpu)
  {
    cmd->ops_base = (uint8_t*)desc->geometry_cpu + cmd->ops_offset;
    memcpy(cmd->ops_base, &k_ops_open, sizeof(k_ops_open));
    cmd->ops = cmd->ops_base + sizeof(k_ops_open);
  }

  return true;
}

bool
agx_cmd_begin_compute_encoder(Agx_Cmd* cmd, const Agx_Copy_Desc* desc)
{
  if (!agx_cmd_begin_copy_encoder(cmd, desc))
  {
    return false;
  }

  *(uint32_t*)((uint8_t*)cmd->open_encoder + AGX_CMD_RECORD_STAGES_OFFSET) = (uint32_t)AGX_RECORD_STAGE_DISPATCH;

  cmd->ops_cursor -= 4;
  return true;
}

void
agx_cmd_end_compute_encoder(Agx_Cmd* cmd)
{
  agx_cmd_end_copy_encoder(cmd);
}

static void
agx_ops_operands(Agx_Cmd* cmd, uint64_t source, uint64_t destination)
{
  if (!cmd->ops_operands)
  {
    return;
  }

  uint32_t pair = 0x14d0u;
  if (cmd->ops_programs_cpu)
  {
    uint32_t word0 = 0;
    memcpy(&word0, (const uint8_t*)cmd->ops_programs_cpu + (size_t)cmd->ops_cursor * 0x40u, sizeof(word0));
    pair = (((word0 >> 8) & 0xffffu) + 0x1200u) + 0x10u;
  }

  uint8_t* ops = (uint8_t*)cmd->ops_operands;
  if (source)
  {
    memcpy(ops + pair, &source, sizeof(source));
  }
  if (destination)
  {
    memcpy(ops + pair + 8, &destination, sizeof(destination));
  }
}

void
agx_image_init(Agx_Image* desc)
{
  memset(desc, 0, sizeof(*desc));
  desc->tiles_per_row = 1;
  desc->dimension = AGX_IMAGE_DIMENSION_2D;
  desc->slices = 1;
  desc->slice = 0;
  desc->sample_count = 1;
  desc->base_level = 0;
  desc->levels = 1;
  desc->swizzle[0] = AGX_CHANNEL_R;
  desc->swizzle[1] = AGX_CHANNEL_G;
  desc->swizzle[2] = AGX_CHANNEL_B;
  desc->swizzle[3] = AGX_CHANNEL_A;

  desc->channels = AGX_IMAGE_CHANNELS_R8G8B8A8;
  desc->is_float = false;
  desc->transpose = false;
}

void
agx_image_set_image(Agx_Image* desc, uint64_t buffer, uint32_t width, uint32_t height)
{
  desc->buffer = buffer;
  desc->width = width;
  desc->height = height;
}

void
agx_image_set_layout(Agx_Image* desc, uint32_t layout)
{
  desc->layout = layout;
}

void
agx_image_set_channels(Agx_Image* desc, Agx_Image_Channels channels)
{
  desc->channels = channels;
}

void
agx_image_set_float(Agx_Image* desc, bool is_float)
{
  desc->is_float = is_float;
}

void
agx_image_set_transpose(Agx_Image* desc, bool transpose)
{
  if (desc != NULL)
  {
    desc->transpose = transpose;
  }
}

uint32_t
agx_image_channels_bytes(Agx_Image_Channels channels)
{
  switch (channels)
  {
  case AGX_IMAGE_CHANNELS_R8:
    return 1u;
  case AGX_IMAGE_CHANNELS_R16:
  case AGX_IMAGE_CHANNELS_R8G8:
  case AGX_IMAGE_CHANNELS_R5G6B5:
  case AGX_IMAGE_CHANNELS_R4G4B4A4:
  case AGX_IMAGE_CHANNELS_A1R5G5B5:
  case AGX_IMAGE_CHANNELS_R5G5B5A1:
    return 2u;
  case AGX_IMAGE_CHANNELS_R32:
  case AGX_IMAGE_CHANNELS_R16G16:
  case AGX_IMAGE_CHANNELS_R11G11B10:
  case AGX_IMAGE_CHANNELS_R10G10B10A2:
  case AGX_IMAGE_CHANNELS_R9G9B9E5:
  case AGX_IMAGE_CHANNELS_R8G8B8A8:
    return 4u;
  case AGX_IMAGE_CHANNELS_R32G32:
  case AGX_IMAGE_CHANNELS_R16G16B16A16:
    return 8u;
  case AGX_IMAGE_CHANNELS_R32G32B32A32:
    return 16u;
  default:
    break;
  }

  return 4u;
}

void
agx_image_set_swizzle(Agx_Image* desc, Agx_Channel r, Agx_Channel g, Agx_Channel b, Agx_Channel a)
{
  desc->swizzle[0] = r;
  desc->swizzle[1] = g;
  desc->swizzle[2] = b;
  desc->swizzle[3] = a;
}

void
agx_image_set_tiles_per_row(Agx_Image* desc, uint32_t tiles)
{
  desc->tiles_per_row = tiles;
}

void
agx_image_set_dimension(Agx_Image* desc, uint32_t dimension)
{
  desc->dimension = dimension;
}

void
agx_image_set_slices(Agx_Image* desc, uint32_t slices)
{
  desc->slices = slices ? slices : 1u;
}

void
agx_image_set_slice(Agx_Image* desc, uint32_t slice)
{
  desc->slice = slice;
}

void
agx_image_set_srgb_decode(Agx_Image* desc, bool decode)
{
  desc->srgb_decode = decode;
}

void
agx_image_set_compression(Agx_Image* desc, uint64_t metadata)
{
  desc->metadata = metadata;
  desc->compress = metadata != 0;
}

void
agx_image_set_extended(Agx_Image* desc, bool extended)
{
  desc->extended = extended;
}

void
agx_image_set_levels(Agx_Image* desc, uint32_t levels)
{
  agx_image_set_level_range(desc, 0, levels);
}

void
agx_image_set_level_range(Agx_Image* desc, uint32_t base_level, uint32_t levels)
{
  desc->base_level = base_level;
  desc->levels = levels < 1u ? 1u : levels;
}

uint64_t
agx_image_level_offset(uint32_t width, uint32_t height, uint32_t level)
{
  uint64_t offset = 0;

  for (uint32_t n = 0; n < level; ++n)
  {
    uint32_t w = width >> n, h = height >> n;
    if (!w && !h)
    {
      break;
    }
    offset += AGX_IMAGE_LEVEL_BYTES(w, h);
  }
  return offset;
}

uint64_t
agx_image_chain_bytes(uint32_t width, uint32_t height)
{
  uint64_t bytes = 0;

  for (uint32_t n = 0;; ++n)
  {
    uint32_t w = width >> n, h = height >> n;
    if (!w && !h)
    {
      break;
    }
    bytes += AGX_IMAGE_LEVEL_BYTES(w, h);
  }
  return bytes;
}

uint64_t
agx_image_level_header_offset(uint32_t width, uint32_t height, uint32_t level)
{
  uint64_t offset = 0;

  for (uint32_t n = 0; n < level; ++n)
  {
    uint32_t w = width >> n, h = height >> n;
    uint64_t block = 0;

    if (!w && !h)
    {
      break;
    }

    block = AGX_IMAGE_HEADER_BYTES(w ? w : 1u, h ? h : 1u);
    offset += (block + 15u) & ~(uint64_t)15u;
  }
  return offset;
}

void
agx_image_set_sample_count(Agx_Image* desc, uint32_t samples)
{
  desc->sample_count = samples;
}

void
agx_image_set_grid(Agx_Image* desc, Agx_Image_Grid grid)
{
  desc->grid = grid;
}

static void
agx_image_write_descriptor(void* at, const Agx_Image* desc)
{
  uint8_t* out = (uint8_t*)at;

  uint64_t slice_color_offset = (uint64_t)desc->slice * AGX_IMAGE_COLOR_BYTES(desc->width, desc->height);
  uint64_t slice_header_offset = (uint64_t)desc->slice * AGX_IMAGE_HEADER_BYTES(desc->width, desc->height);

  agx_cmd(out, render_target, cfg)
  {
    cfg.dimension = desc->sample_count > 1u ? AGX_IMAGE_DIMENSION_2D_MULTISAMPLED : desc->dimension;

    if (desc->grid == AGX_IMAGE_GRID_SAMPLE)
    {
      cfg.layered = false;
      cfg.unk_126 = desc->slices > 1u;
    }
    else
    {
      cfg.layered = desc->slices > 1u || desc->dimension == AGX_IMAGE_DIMENSION_2D_ARRAY ||
                    desc->dimension == AGX_IMAGE_DIMENSION_1D_ARRAY || desc->dimension == AGX_IMAGE_DIMENSION_CUBE ||
                    desc->dimension == AGX_IMAGE_DIMENSION_CUBE_ARRAY || desc->dimension == AGX_IMAGE_DIMENSION_3D;
      cfg.unk_126 = 0u;
    }
    cfg.multisample_4x = desc->sample_count > 2u;
    cfg.layout = desc->layout;
    cfg.channels = desc->channels;
    cfg.is_float = desc->is_float;
    cfg.transpose = desc->transpose;
    cfg.unk_13 = desc->mode_bits;
    cfg.swizzle_r = desc->swizzle[0];
    cfg.swizzle_g = desc->swizzle[1];
    cfg.swizzle_b = desc->swizzle[2];
    cfg.swizzle_a = desc->swizzle[3];

    uint32_t fine_w = desc->width * 16u - 9u;
    uint32_t fine_h = desc->height * 16u - 15u;
    uint32_t packed = (fine_w - 1u) + ((fine_h - 1u) << 14);

    cfg.width = desc->grid == AGX_IMAGE_GRID_SAMPLE ? ((packed & 0x3fffu) + 1u) : desc->width;
    cfg.height = desc->grid == AGX_IMAGE_GRID_SAMPLE ? (((packed >> 14) & 0x3fffu) + 1u) : desc->height;

    if (desc->grid == AGX_IMAGE_GRID_SAMPLE && desc->tiles_per_row > 1024u)
    {
      fprintf(
        stderr,
        "agx_image: tiles per row %u exceeds the field's ten bits; a fine-grid image stops at "
        "1026 texels wide. The texel grid is how Metal goes further.\n",
        desc->tiles_per_row
      );
    }

    if (desc->layout == AGX_IMAGE_LAYOUT_GPU)
    {
      cfg.tiles_per_row = desc->grid == AGX_IMAGE_GRID_SAMPLE ? AGX_IMAGE_FINE_SLICES(desc->slices) : desc->slices;
    }
    else
    {
      cfg.tiles_per_row = desc->tiles_per_row;
    }

    if (desc->grid == AGX_IMAGE_GRID_SAMPLE)
    {
      cfg.tiles_per_row += desc->srgb_decode ? 1u : 0u;
      cfg.srgb = false;
    }
    else
    {
      cfg.srgb = desc->srgb_decode;
    }
    cfg.extended = desc->extended;
    cfg.buffer = desc->buffer + slice_color_offset;
    cfg.compress = desc->compress;

    cfg.mipmapped = desc->base_level + desc->levels > 1u;
  }

  agx_cmd(out + 0x10, image_descriptor_trailer, trailer)
  {
    trailer.metadata_shifted = desc->metadata ? desc->metadata + slice_header_offset : desc->metadata;
    trailer.base_level = desc->base_level;
    trailer.last_level = desc->base_level + (desc->levels > 1u ? desc->levels - 1u : 0);
  }
}

static void
agx_cmd_copy_texture(
  Agx_Cmd*        cmd,
  uint64_t        descriptor_block,
  uint64_t        destination_in_texels,
  Agx_Work_Extent source_extent,
  Agx_Work_Extent tile,
  uint32_t        cursor_step
);

void
agx_cmd_copy_image(Agx_Cmd* cmd, const Agx_Image* source, const Agx_Image* destination, Agx_Work_Extent extent, Agx_Work_Extent tile)
{
  if (!cmd->ops_operands)
  {
    return;
  }

  uint32_t offset = 0x1500u + 0x140u * cmd->ops_blocks;
  uint8_t* block = (uint8_t*)cmd->ops_operands + offset;

  Agx_Image fine = *destination;
  fine.grid = AGX_IMAGE_GRID_SAMPLE;

  fine.levels = 1;

  agx_image_write_descriptor(block + 0x00, &fine);
  agx_image_write_descriptor(block + 0x40, source);
  agx_image_write_descriptor(block + 0x80, destination);
  cmd->ops_blocks++;

  if (cmd->ops_blocks > 1)
  {
    agx_ops_append(cmd, 0x00880000u, 0x01000060u, 7u, extent, tile);
  }
  else
  {
    agx_cmd_copy_texture(
      cmd,
      cmd->ops_operands_gpu + offset,
      cmd->ops_operands_gpu + offset + 0x80,
      extent,
      tile,
      source->layout == AGX_IMAGE_LAYOUT_LINEAR ? 7u : 9u
    );
  }
}

void
agx_cmd_copy_buffer(Agx_Cmd* cmd, uint64_t source, uint64_t destination, uint64_t bytes)
{
  Agx_Work_Extent extent = {0u, 0u, 0u};
  Agx_Work_Extent tile = {0u, 0u, 0u};

  if (bytes % 16u)
  {
    fprintf(
      stderr,
      "agx_cmd_copy_buffer: %llu bytes is not a multiple of 16, which is the block a "
      "buffer copy works in; nothing was recorded\n",
      (unsigned long long)bytes
    );
    return;
  }

  agx_work_extent_for_buffer_copy(bytes, &extent, &tile);

  agx_buffer_copy_program_write(cmd->ops_programs_cpu, cmd->ops_cursor, cmd->ops_operands_gpu);

  agx_ops_operands(cmd, source, destination);

  agx_ops_append(cmd, 0x00880000u, 0x01000040u, 8u, extent, tile);
}

void
agx_cmd_set_driver_arguments(Agx_Cmd* cmd, const Agx_Bo* arguments)
{
  cmd->driver_arguments_cpu = arguments->cpu;
  cmd->driver_arguments_bytes = arguments->size;
  cmd->render_passes = 0;
}

void
agx_cmd_set_scratch(Agx_Cmd* cmd, const Agx_Bo* memory, uint64_t offset, size_t bytes)
{
  cmd->scratch_cpu = memory->cpu ? (uint8_t*)memory->cpu + offset : NULL;
  cmd->scratch_gpu = memory->gpu_va ? memory->gpu_va + offset : 0;
  cmd->scratch_bytes = bytes;
  cmd->scratch_cursor = 0;
}

void
agx_cmd_fill_buffer(Agx_Cmd* cmd, uint64_t destination, uint64_t bytes, uint32_t value)
{
  size_t pattern_bytes = (size_t)bytes;

  if (!cmd->scratch_cpu || bytes == 0 || (bytes % 16u) != 0)
  {
    return;
  }

  if (cmd->scratch_cursor + pattern_bytes > cmd->scratch_bytes)
  {
    return;
  }

  uint8_t* pattern = (uint8_t*)cmd->scratch_cpu + cmd->scratch_cursor;
  uint64_t pattern_gpu = cmd->scratch_gpu + cmd->scratch_cursor;

  for (size_t i = 0; i < pattern_bytes; i += sizeof(uint32_t))
  {
    memcpy(pattern + i, &value, sizeof(value));
  }
  cmd->scratch_cursor += pattern_bytes;

  agx_cmd_copy_buffer(cmd, pattern_gpu, destination, bytes);
}

static void
agx_cmd_copy_texture(
  Agx_Cmd*        cmd,
  uint64_t        descriptor_block,
  uint64_t        destination_in_texels,
  Agx_Work_Extent source_extent,
  Agx_Work_Extent tile,
  uint32_t        cursor_step
)
{
  agx_ops_operands(cmd, descriptor_block, destination_in_texels);
  agx_ops_append(cmd, 0x00880000u, 0x01000060u, cursor_step, source_extent, tile);
}

void
agx_cmd_dispatch(Agx_Cmd* cmd, Agx_Work_Extent grid, Agx_Work_Extent threadgroup)
{
  agx_ops_append(cmd, 0x00080000u, 0x01000000u, 3u, grid, threadgroup);

  if (cmd->open_encoder)
  {
    uint32_t stages = 0;
    memcpy(&stages, (uint8_t*)cmd->open_encoder + AGX_CMD_RECORD_STAGES_OFFSET, sizeof(stages));
    stages |= (uint32_t)AGX_RECORD_STAGE_DISPATCH;
    memcpy((uint8_t*)cmd->open_encoder + AGX_CMD_RECORD_STAGES_OFFSET, &stages, sizeof(stages));
  }
}

void
agx_cmd_end_copy_encoder(Agx_Cmd* cmd)
{
  if (cmd->open_encoder && cmd->ops && cmd->ops_base && (size_t)(cmd->ops - cmd->ops_base) == sizeof(k_ops_open) &&
      cmd->record_count == 0u)
  {
    fprintf(
      stderr,
      "agx_cmd_end_copy_encoder: the FIRST record of this command buffer is an encoder that "
      "recorded no operations, so its inline command stream is empty. That submission does "
      "not fail cleanly -- it renders differently on every run and faults. Record an "
      "operation in it, or open it after another record.\n"
    );
  }

  if (cmd->ops && cmd->open_encoder)
  {
    uint32_t terminator = 0x40000000u;
    memcpy(cmd->ops, &terminator, sizeof(terminator));

    size_t length = cmd->ops_offset + (size_t)(cmd->ops - cmd->ops_base);

    cmd->ops_offset = (uint32_t)length + (uint32_t)sizeof(terminator);

    uint16_t field = 0;
    memcpy(&field, cmd->open_encoder + 0x124, sizeof(field));
    field = (uint16_t)((field & 0x8000u) | (uint16_t)(length & 0x7fffu));
    memcpy(cmd->open_encoder + 0x124, &field, sizeof(field));
  }

  cmd->ops = NULL;
  cmd->ops_base = NULL;
  agx_cmd_end_encoder(cmd);
}

void
agx_cmd_set_cull_mode(Agx_Cmd* cmd, Agx_Cull_Mode mode)
{
  cmd->cull_mode = mode;
}

void
agx_cmd_set_front_facing_winding(Agx_Cmd* cmd, Agx_Winding winding)
{
  cmd->front_face_winding = winding;
}

static void
agx_cmd_barrier_write(Agx_Cmd* cmd, size_t offset, Agx_Stage after, Agx_Stage before)
{
  if (!cmd->open_encoder)
  {
    return;
  }

  uint32_t after_mask = (uint32_t)after;
  uint32_t before_mask = (uint32_t)before;
  memcpy(cmd->open_encoder + offset, &after_mask, sizeof(after_mask));
  memcpy(cmd->open_encoder + offset + 4, &before_mask, sizeof(before_mask));
}

void
agx_cmd_barrier_after_queue_stages(Agx_Cmd* cmd, Agx_Stage after, Agx_Stage before, Agx_Visibility_Option visibility)
{
  agx_cmd_barrier_write(cmd, 0xa8, after, before);

  if (visibility != AGX_VISIBILITY_OPTION_RESOURCE_ALIAS || !cmd->open_encoder)
  {
    return;
  }

  agx_cmd_barrier_write(cmd, 0xb0, after, before);

  for (size_t offset = AGX_CMD_RECORD_STAGES_OFFSET; offset <= AGX_CMD_RECORD_STAGES_OFFSET + 4; offset += 4)
  {
    uint32_t stages = 0;
    memcpy(&stages, cmd->open_encoder + offset, sizeof(stages));
    stages |= (uint32_t)after | (uint32_t)before;
    memcpy(cmd->open_encoder + offset, &stages, sizeof(stages));
  }
}

void
agx_cmd_barrier_after_stages(Agx_Cmd* cmd, Agx_Stage after, Agx_Stage before_queue, Agx_Visibility_Option visibility)
{
  if (visibility != AGX_VISIBILITY_OPTION_RESOURCE_ALIAS)
  {
    agx_cmd_barrier_write(cmd, 0xb0, after, before_queue);
    return;
  }

  if (!cmd->open_encoder)
  {
    return;
  }

  cmd->alias_barrier_pending = true;
  cmd->alias_barrier_after = after;
  cmd->alias_barrier_before = before_queue;
}

void
agx_depth_bias_write(void* table_cpu, uint32_t index, Agx_Depth_Bias_Desc bias)
{
  uint8_t* entry = (uint8_t*)table_cpu + (size_t)index * 12u;
  agx_cmd(entry, depth_bias, cfg)
  {
    cfg.constant = bias.constant;
    cfg.slope_scale = bias.slope_scale;
    cfg.clamp = bias.clamp;
  }
}

void
agx_scissor_write(void* buffer_cpu, uint32_t index, Agx_Scissor_Desc scissor)
{
  uint8_t* entry = (uint8_t*)buffer_cpu + (size_t)index * 16u;
  agx_cmd(entry, scissor, cfg)
  {
    cfg.x_min = scissor.x;
    cfg.x_max = scissor.x + scissor.width;
    cfg.y_min = scissor.y;
    cfg.y_max = scissor.y + scissor.height;
    cfg.z_min = 0.0f;
    cfg.z_max = 1.0f;
  }
}

void
agx_cmd_set_depth_clip_mode(Agx_Cmd* cmd, Agx_Depth_Clip_Mode mode)
{
  cmd->depth_clip_mode = mode;
}

void
agx_cmd_set_depth_source(Agx_Cmd* cmd, Agx_Depth_Source source)
{
  cmd->depth_source = source;
}

void
agx_cmd_set_clip_distance_count(Agx_Cmd* cmd, uint32_t count)
{
  cmd->clip_distance_count = count < AGX_MAX_CLIP_DISTANCES ? count : AGX_MAX_CLIP_DISTANCES;
}

void
agx_cmd_set_rasterization_enabled(Agx_Cmd* cmd, bool enabled)
{
  cmd->rasterizer_discard = !enabled;
}

void
agx_cmd_set_primitive_id(Agx_Cmd* cmd, bool reads)
{
  cmd->reads_primitive_id = reads;
}

void
agx_cmd_set_pretransform(Agx_Cmd* cmd, bool pretransformed)
{
  cmd->pretransform = pretransformed;
}

static void
agx_cmd_draw_common(
  Agx_Cmd*       cmd,
  Agx_Primitive  primitive,
  uint32_t       count,
  uint32_t       instance_count,
  uint32_t       start,
  bool           indexed,
  uint64_t       index_buffer,
  size_t         index_buffer_bytes,
  Agx_Index_Size index_size,
  uint64_t       arguments
)
{
  bool indirect = arguments != 0;

  if (indirect)
  {
    cmd->pass_primitives = UINT32_MAX;
  }
  else
  {
    uint32_t primitives = 0;

    switch (primitive)
    {
    case AGX_PRIMITIVE_POINTS:
      primitives = count;
      break;
    case AGX_PRIMITIVE_LINES:
      primitives = count / 2u;
      break;
    case AGX_PRIMITIVE_LINE_STRIP:
      primitives = count > 1u ? count - 1u : 0u;
      break;
    case AGX_PRIMITIVE_TRIANGLES:
      primitives = count / 3u;
      break;
    case AGX_PRIMITIVE_TRIANGLE_STRIP:
      primitives = count > 2u ? count - 2u : 0u;
      break;
    default:
      primitives = count;
      break;
    }

    if (instance_count > 1u)
    {
      primitives *= instance_count;
    }

    if (cmd->pass_primitives > UINT32_MAX - primitives)
    {
      cmd->pass_primitives = UINT32_MAX;
    }
    else
    {
      cmd->pass_primitives += primitives;
    }
  }

  agx_cmd_push_draw_state(cmd, primitive);

  agx_push(cmd->cs, index_list, cfg)
  {
    cfg.primitive = primitive;
    cfg.index_size = indexed ? index_size : AGX_INDEX_SIZE_U32;
    cfg.index_buffer_present = indexed;
    cfg.index_buffer_size_present = indexed;
    cfg.index_buffer_hi = indexed ? ((index_buffer >> 32) & 0xff) : 0;

    cfg.index_count_present = !indirect;
    cfg.instance_count_present = !indirect;
    cfg.start_present = !indirect;
    cfg.indirect_present = indirect;
  }

  if (indexed)
  {
    agx_push(cmd->cs, index_list_buffer, cfg)
    {
      cfg.buffer_lo = (uint32_t)index_buffer;
    }
  }

  if (indirect)
  {
    agx_push(cmd->cs, index_list_indirect, cfg)
    {
      cfg.arguments_hi = (uint32_t)(arguments >> 32);
      cfg.arguments_lo = (uint32_t)arguments;
    }
  }
  else
  {
    agx_push(cmd->cs, index_list_count, cfg)
    {
      cfg.count = count;
    }
    agx_push(cmd->cs, index_list_instances, cfg)
    {
      cfg.count = instance_count;
    }
    agx_push(cmd->cs, index_list_start, cfg)
    {
      cfg.start = start;
    }
  }

  if (indexed)
  {
    agx_push(cmd->cs, index_list_buffer_size, cfg)
    {
      cfg.size = indirect ? (uint32_t)index_buffer_bytes - 1 : (uint32_t)(index_buffer_bytes / 4) - 1;
    }
    agx_push(cmd->cs, index_list_unk, cfg)
    {
    }
  }
}

void
agx_cmd_draw(Agx_Cmd* cmd, Agx_Primitive primitive, uint32_t vertex_count, uint32_t instance_count, uint32_t vertex_start)
{
  agx_cmd_draw_common(cmd, primitive, vertex_count, instance_count, vertex_start, false, 0, 0, AGX_INDEX_SIZE_U32, 0);
}

void
agx_cmd_draw_indexed(
  Agx_Cmd*       cmd,
  Agx_Primitive  primitive,
  uint32_t       index_count,
  uint32_t       instance_count,
  uint32_t       base_vertex,
  uint64_t       index_buffer,
  size_t         index_buffer_bytes,
  Agx_Index_Size index_size
)
{
  agx_cmd_draw_common(
    cmd, primitive, index_count, instance_count, base_vertex, true, index_buffer, index_buffer_bytes, index_size, 0
  );
}

void
agx_cmd_draw_indirect(Agx_Cmd* cmd, Agx_Primitive primitive, uint64_t arguments)
{
  agx_cmd_draw_common(cmd, primitive, 0, 0, 0, false, 0, 0, AGX_INDEX_SIZE_U32, arguments);
}

void
agx_cmd_draw_indexed_indirect(
  Agx_Cmd*       cmd,
  Agx_Primitive  primitive,
  uint64_t       index_buffer,
  size_t         index_buffer_bytes,
  Agx_Index_Size index_size,
  uint64_t       arguments
)
{
  agx_cmd_draw_common(cmd, primitive, 0, 0, 0, true, index_buffer, index_buffer_bytes, index_size, arguments);
}

uint32_t
agx_drawable_tile_header_index(uint32_t tile_x, uint32_t tile_y)
{
  uint32_t index = 0;

  for (uint32_t bit = 0; bit < 16; ++bit)
  {
    index |= ((tile_x >> bit) & 1u) << (2u * bit);
    index |= ((tile_y >> bit) & 1u) << (2u * bit + 1u);
  }

  return index;
}

uint32_t
agx_drawable_metadata_tiles(const void* surface_base, uint32_t width, uint32_t height, uint32_t* tile_count)
{
  const uint8_t* base = (const uint8_t*)surface_base;
  uint32_t       tiles_x = AGX_TILES_X(width);
  uint32_t       tiles_y = AGX_TILES_Y(height);
  const uint8_t* headers = base + AGX_DRAWABLE_METADATA_BYTES(width, height);
  uint32_t       total = tiles_x * tiles_y;
  uint32_t       written = 0;

  for (uint32_t y = 0; y < tiles_y; ++y)
  {
    for (uint32_t x = 0; x < tiles_x; ++x)
    {
      uint64_t entry = 0;
      memcpy(&entry, headers + (size_t)agx_drawable_tile_header_index(x, y) * 8u, sizeof(entry));
      written += entry != 0;
    }
  }

  if (tile_count)
  {
    *tile_count = total;
  }
  return written;
}

uint32_t
agx_drawable_uniform_tiles(const void* surface_base, uint32_t width, uint32_t height, uint32_t* color, uint32_t* tile_count)
{
  const uint8_t* base = (const uint8_t*)surface_base;
  uint32_t       tiles_x = AGX_TILES_X(width);
  uint32_t       tiles_y = AGX_TILES_Y(height);
  uint32_t       tiles = tiles_x * tiles_y;

  uint32_t uniform = 0;
  uint32_t first = 0;
  bool     agreed = true;

  for (uint32_t t = 0; t < tiles; ++t)
  {
    const uint8_t* payload = base + (size_t)t * 1024u;

    uint32_t c = 0;
    memcpy(&c, payload, sizeof(c));

    bool payload_uniform = true;
    for (uint32_t i = 1; i < 8; ++i)
    {
      uint32_t other = 0;
      memcpy(&other, payload + i * sizeof(other), sizeof(other));
      if (other != c)
      {
        payload_uniform = false;
        break;
      }
    }
    for (uint32_t i = 32; payload_uniform && i < 1024; ++i)
    {
      if (payload[i])
      {
        payload_uniform = false;
      }
    }
    if (!payload_uniform)
    {
      continue;
    }

    if (uniform == 0)
    {
      first = c;
    }
    else if (c != first)
    {
      agreed = false;
    }
    uniform++;
  }

  if (color)
  {
    *color = agreed ? first : 0;
  }
  if (tile_count)
  {
    *tile_count = tiles;
  }
  return uniform;
}

void
agx_cmd_link_stream(Agx_Cmd* cmd, uint64_t target)
{
  agx_push(cmd->cs, stream_link, cfg)
  {
    cfg.target_lo = (uint32_t)target;
  }
}

void
agx_cmd_continue_stream(Agx_Cmd* cmd, const Agx_Bo* stream)
{
  agx_cmd_link_stream(cmd, stream->gpu_va);

  cmd->stream = *stream;
  cmd->stream_cursor = 0;
  cmd->cs = (uint8_t*)stream->cpu;
}

mach_port_t
agx_trap_connection(mach_port_t connection)
{
  volatile mach_port_t held = connection;

  return (mach_port_t)held;
}

bool
agx_queue_signal_fence(Agx_Command_Queue queue, Agx_Fence fence, uint64_t value)
{
  kern_return_t ret =
    IOConnectTrap4(agx_trap_connection(fence.agx), AGX_SELECTOR_TRAP4_SIGNAL_EVENT, queue.id, fence.id, value, 0);

  if (ret)
  {
    fprintf(
      stderr, "agx_queue_signal_fence: SIGNAL_EVENT(id=0x%llx, value=%llu) failed, kern_return_t=0x%x\n", fence.id, value, ret
    );
    return false;
  }

  return true;
}

bool
agx_queue_wait_fence(Agx_Command_Queue queue, Agx_Fence fence, uint64_t value)
{
  kern_return_t ret =
    IOConnectTrap5(agx_trap_connection(fence.agx), AGX_SELECTOR_TRAP5_WAIT_EVENT, queue.id, fence.id, value, 0, 0xffffffff);

  if (ret)
  {
    fprintf(
      stderr, "agx_queue_wait_fence: WAIT_EVENT(id=0x%llx, value=%llu) failed, kern_return_t=0x%x\n", fence.id, value, ret
    );
    return false;
  }

  return true;
}

bool
agx_create_fence(Agx_Device device, Agx_Fence* fence)
{
  {
    uint64_t input = 0;
    uint64_t scalars[2] = {0};
    uint32_t scalar_count = 2;

    kern_return_t ret =
      IOConnectCallScalarMethod(device.iosrf, IOSURFACE_SELECTOR_CREATE_SHARED_EVENT, &input, 1, scalars, &scalar_count);

    if (ret)
    {
      fprintf(stderr, "agx_create_fence: CREATE_SHARED_EVENT failed, kern_return_t=0x%x\n", ret);
      return false;
    }

    fence->agx = device.agx;
    fence->iosrf = device.iosrf;
    fence->id = scalars[0];
  }

  {
    uint64_t input = fence->id;

    uint64_t scalars[4] = {0};
    uint32_t scalar_count = 4;

    kern_return_t ret =
      IOConnectCallScalarMethod(device.iosrf, IOSURFACE_SELECTOR_CREATE_SHARED_EVENT2, &input, 1, scalars, &scalar_count);

    if (ret)
    {
      fprintf(stderr, "agx_create_fence: CREATE_SHARED_EVENT2 failed, kern_return_t=0x%x\n", ret);
      return false;
    }

    (void)scalars;
  }

  return true;
}

bool
agx_signal_fence(Agx_Fence fence, uint64_t value)
{
  kern_return_t ret =
    IOConnectTrap2(agx_trap_connection(fence.iosrf), IOSURFACE_SELECTOR_TRAP2_SET_SIGNALED_VALUE, (uint64_t)fence.id, value);

  if (ret)
  {
    fprintf(
      stderr, "agx_signal_fence: SET_SIGNALED_VALUE(id=0x%llx, value=%llu) failed, kern_return_t=0x%x\n", fence.id, value, ret
    );
    return false;
  }

  return true;
}

bool
agx_wait_fence(Agx_Fence fence, uint64_t value, uint32_t timeout_ms)
{
  kern_return_t ret = IOConnectTrap3(
    agx_trap_connection(fence.iosrf), IOSURFACE_SELECTOR_TRAP3_WAIT_SIGNALED_VALUE, (uint64_t)fence.id, value, timeout_ms
  );

  if (ret)
  {
    fprintf(
      stderr,
      "agx_wait_fence: WAIT_SIGNALED_VALUE(id=0x%llx, value=%llu, timeout=%ums) returned 0x%x\n",
      fence.id,
      value,
      timeout_ms,
      ret
    );
    return false;
  }

  return true;
}

void
agx_destroy_fence(Agx_Device device, Agx_Fence fence)
{
}

bool
agx_resource_group_update(Agx_Device device, uint64_t group, const uint32_t* indices, uint32_t count)
{
  kern_return_t ret = IOConnectTrap6(
    agx_trap_connection(device.agx),
    AGX_SELECTOR_TRAP6_RESOURCE_GROUP_UPDATE_RESOURCES,
    (uintptr_t)group,
    (uintptr_t)count,
    (uintptr_t)indices,
    0,
    0,
    0
  );
  if (ret)
  {
    fprintf(
      stderr, "agx_resource_group_update: group %" PRIu64 ", %u resource(s) failed, kern_return_t=0x%x\n", group, count, ret
    );
    return false;
  }
  return true;
}

Agx_Purgeable_State
agx_bo_set_purgeable_state(Agx_Device device, const Agx_Bo* bo, Agx_Purgeable_State state)
{
  Agx_Purgeable_State previous = AGX_PURGEABLE_STATE_KEEP_CURRENT;
  kern_return_t       ret = IOConnectTrap3(
    agx_trap_connection(device.agx),
    AGX_SELECTOR_TRAP3_SET_PURGEABLE_STATE,
    (uintptr_t)bo->index,
    (uintptr_t)state,
    (uintptr_t)&previous
  );
  if (ret)
  {
    fprintf(
      stderr,
      "agx_bo_set_purgeable_state: index %u state %u failed, kern_return_t=0x%x\n",
      (unsigned)bo->index,
      (unsigned)state,
      ret
    );
    return AGX_PURGEABLE_STATE_KEEP_CURRENT;
  }
  return previous;
}

bool
agx_queue_set_resource_groups(Agx_Command_Queue queue, uint64_t group)
{
  kern_return_t ret = IOConnectTrap3(
    agx_trap_connection(queue.agx), AGX_SELECTOR_TRAP3_QUEUE_SET_RESOURCE_GROUPS, queue.id, 1, (uintptr_t)group
  );
  if (ret)
  {
    fprintf(stderr, "agx_queue_set_resource_groups: group %" PRIu64 " failed, kern_return_t=0x%x\n", group, ret);
    return false;
  }
  return true;
}

static bool
agx_notify_dump_enabled(void)
{
  static int enabled = -1;
  if (enabled < 0)
  {
    const char* env = getenv("AGX_NOTIFY_DUMP");
    enabled = (env && atoi(env)) ? 1 : 0;
  }
  return enabled != 0;
}

static void
agx_queue_hold_status(Agx_Command_Queue queue, Agx_Notification message)
{
  Agx_Queue_Progress* progress = queue.progress;
  enum
  {
    PENDING_MAX = sizeof(progress->pending) / sizeof(progress->pending[0])
  };

  if (!progress)
  {
    return;
  }

  if (progress->pending_count < PENDING_MAX)
  {
    progress->pending[progress->pending_count++] = message;
    return;
  }

  if (!message.status)
  {
    return;
  }
  for (uint32_t i = 0; i < PENDING_MAX; ++i)
  {
    if (!progress->pending[i].status)
    {
      progress->pending[i] = message;
      return;
    }
  }
}

static uint32_t
agx_queue_dequeue(Agx_Command_Queue queue, Agx_Notification* messages, uint32_t max)
{
  uint32_t count = 0;
  if (!queue.notif.queue)
  {
    return 0;
  }

  while (IODataQueueDataAvailable(queue.notif.queue))
  {
    uint8_t  entry[256] = {0};
    uint32_t size = sizeof(entry);
    if (IODataQueueDequeue(queue.notif.queue, entry, &size) != kIOReturnSuccess)
    {
      break;
    }
    if (size >= 0x20)
    {
      if (queue.progress)
      {
        queue.progress->retired++;
      }

      Agx_Notification message = {0};
      memcpy(&message.start_ns, entry + 0x08, sizeof(message.start_ns));
      memcpy(&message.end_ns, entry + 0x10, sizeof(message.end_ns));
      memcpy(&message.status, entry + 0x18, sizeof(message.status));

      if (messages && count < max)
      {
        messages[count] = message;
      }
      else
      {
        agx_queue_hold_status(queue, message);
      }
    }

    if (size > 0x18 && entry[0x18] && agx_notify_dump_enabled())
    {
      fprintf(stderr, "agx notification (%u bytes):", size);
      for (uint32_t i = 0; i < size && i < sizeof(entry); ++i)
      {
        fprintf(stderr, "%s%02x", (i % 8) ? " " : "  ", entry[i]);
      }
      fprintf(stderr, "\n");
    }
    count++;
  }

  return count > max ? max : count;
}

uint32_t
agx_drain_notifications(Agx_Command_Queue queue, Agx_Notification* messages, uint32_t max)
{
  uint32_t count = 0;

  Agx_Queue_Progress* progress = queue.progress;
  if (progress && progress->pending_count && messages)
  {
    count = progress->pending_count < max ? progress->pending_count : max;
    memcpy(messages, progress->pending, count * sizeof(*messages));
    memmove(progress->pending, progress->pending + count, (progress->pending_count - count) * sizeof(*progress->pending));
    progress->pending_count -= count;
  }

  return count + agx_queue_dequeue(queue, messages ? messages + count : NULL, max > count ? max - count : 0);
}

uint64_t
agx_queue_submitted(Agx_Command_Queue queue)
{
  return queue.progress ? queue.progress->submitted : 0;
}

uint64_t
agx_queue_retired(Agx_Command_Queue queue)
{
  return queue.progress ? queue.progress->retired : 0;
}

bool
agx_queue_wait_retired(Agx_Command_Queue queue, uint64_t submissions, uint32_t timeout_ms, Agx_Notification* messages, uint32_t max)
{
  if (!queue.progress)
  {
    return false;
  }

  uint32_t drained = agx_drain_notifications(queue, messages, max);
  for (uint32_t waited_us = 0; queue.progress->retired < submissions;)
  {
    if (waited_us >= timeout_ms * 1000u)
    {
      return false;
    }
    usleep(50);
    waited_us += 50;
    drained += agx_drain_notifications(queue, messages ? messages + drained : NULL, max > drained ? max - drained : 0);
  }

  return true;
}

void
agx_cmd_set_scissor_buffer(Agx_Cmd* cmd, const Agx_Bo* buffer)
{
  cmd->scissor_buffer_gpu = buffer->gpu_va;
}

void
agx_cmd_set_depth_bias_table(Agx_Cmd* cmd, const Agx_Bo* table, uint32_t entries)
{
  cmd->depth_bias_table_cpu = table->cpu;
  cmd->depth_bias_table_gpu = table->gpu_va;
  cmd->depth_bias_entries = entries;
  cmd->depth_bias_next = 0;
}

void
agx_cmd_set_depth_bias(Agx_Cmd* cmd, Agx_Depth_Bias_Desc bias)
{
  if (!cmd->depth_bias_table_cpu || cmd->depth_bias_next >= cmd->depth_bias_entries)
  {
    return;
  }

  agx_depth_bias_write(cmd->depth_bias_table_cpu, cmd->depth_bias_next, bias);
  cmd->depth_bias_index = cmd->depth_bias_next;
  cmd->depth_bias_next++;

  cmd->depth_bias_enable = true;
}

void
agx_cmd_set_pass_type(Agx_Cmd* cmd, Agx_Pass_Type pass_type)
{
  cmd->pass_type = pass_type;
}

void
agx_cmd_set_depth_bias_enable(Agx_Cmd* cmd, bool enable)
{
  cmd->depth_bias_enable = enable;
}

void
agx_cmd_set_depth_bias_index(Agx_Cmd* cmd, uint32_t index)
{
  cmd->depth_bias_index = index;
}

void
agx_cmd_set_scissor_enable(Agx_Cmd* cmd, bool enable)
{
  cmd->scissor_enable = enable;
}

void
agx_cmd_set_scissor_index(Agx_Cmd* cmd, uint32_t index)
{
  cmd->scissor_index = index;
}

void
agx_compute_bindings_desc_init(Agx_Compute_Bindings_Desc* desc)
{
  memset(desc, 0, sizeof(*desc));

  desc->table_offset = 0x14c0;
  desc->descriptor_offset = 0x14e0;
  desc->reserved_slots = 0;
}

void
agx_compute_bindings_desc_set_table(Agx_Compute_Bindings_Desc* desc, uint32_t table_offset, uint32_t descriptor_offset)
{
  desc->table_offset = table_offset;
  desc->descriptor_offset = descriptor_offset;
}

void
agx_compute_bindings_desc_set_reserved_slots(Agx_Compute_Bindings_Desc* desc, uint32_t slots)
{
  desc->reserved_slots = slots;
}

uint32_t
agx_compute_bindings_write(
  const Agx_Compute_Bindings_Desc* desc,
  const Agx_Bo*                    table,
  const Agx_Image*                 image,
  const Agx_Sampler_Desc*          sampler,
  const uint64_t*                  buffers,
  uint32_t                         buffer_count
)
{
  uint8_t* at = NULL;
  uint32_t slots = 0;
  uint32_t which = 0;
  uint64_t address = 0;

  if (desc == NULL || table == NULL || table->cpu == NULL)
  {
    return 0;
  }
  slots = desc->reserved_slots + buffer_count;

  if ((uint64_t)desc->table_offset + (uint64_t)slots * 8u > table->size ||
      (uint64_t)desc->descriptor_offset + 0x40u > table->size)
  {
    return 0;
  }

  at = (uint8_t*)table->cpu;

  if (image != NULL)
  {
    agx_image_write_descriptor(at + desc->descriptor_offset, image);
    address = table->gpu_va + desc->descriptor_offset;
    memcpy(at + desc->table_offset, &address, sizeof(address));
  }
  if (sampler != NULL)
  {
    agx_sampler_write_descriptor(at + desc->descriptor_offset + 0x20u, sampler);
    address = table->gpu_va + desc->descriptor_offset + 0x20u;
    memcpy(at + desc->table_offset + 8u, &address, sizeof(address));
  }

  for (which = 0; which < buffer_count; which += 1)
  {
    address = buffers[which];
    memcpy(at + desc->table_offset + (desc->reserved_slots + which) * 8u, &address, sizeof(address));
  }

  return slots;
}
