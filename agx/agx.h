#pragma once

#include <stdbool.h>
#include <mach/mach.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/IODataQueueClient.h>

#include "agx_refusal.h"
#include "agx_cmd.h"

#define AGX_BO_BITS_COMMON 0x0430

#define AGX_VERTEX_BUFFER_INDEX_MAX 0u
#define IOSURFACE_SERVICE_TYPE      0x0
#define AGX_SERVICE_TYPE            0x100005
#define AGX_TILES_X(w)              (((w) + 15u) / 16u)
#define AGX_TILES_Y(h)              (((h) + 15u) / 16u)

#define AGX_IMAGE_TILE_COLUMNS(bytes) ((bytes) <= 2u ? 32u : 16u)
#define AGX_IMAGE_TILE_ROWS(bytes)    (1024u / ((bytes) * AGX_IMAGE_TILE_COLUMNS(bytes)))
#define AGX_IMAGE_TILES_X(w, bytes)   (((w) + AGX_IMAGE_TILE_COLUMNS(bytes) - 1u) / AGX_IMAGE_TILE_COLUMNS(bytes))
#define AGX_IMAGE_TILES_Y(h, bytes)   (((h) + AGX_IMAGE_TILE_ROWS(bytes) - 1u) / AGX_IMAGE_TILE_ROWS(bytes))
#define AGX_IMAGE_COLOR_BYTES_AT(w, h, bytes) \
  ((uint64_t)((AGX_IMAGE_TILES_X(w, bytes) + 3u) / 4u) * ((AGX_IMAGE_TILES_Y(h, bytes) + 3u) / 4u) * 16u * 1024u)
#define AGX_IMAGE_COLOR_BYTES(w, h) AGX_IMAGE_COLOR_BYTES_AT(w, h, 4u)
#define AGX_IMAGE_HEADER_BYTES(w, h) \
  ((uint64_t)agx_round_up_power_of_two(AGX_TILES_X(w)) * agx_round_up_power_of_two(AGX_TILES_Y(h)) * 8u)
#define AGX_IMAGE_BYTES(w, h) (AGX_IMAGE_COLOR_BYTES(w, h) + AGX_IMAGE_HEADER_BYTES(w, h))

#define AGX_RENDER_TARGET_BYTES(w, h, bytes) \
  (AGX_IMAGE_COLOR_BYTES_AT(w, h, bytes) +   \
   (AGX_IMAGE_HEADER_BYTES(w, h) > 16u * 1024u ? AGX_IMAGE_HEADER_BYTES(w, h) : (uint64_t)(16u * 1024u)))

#define AGX_DEPTH_BYTES_AT(w, h, bytes) AGX_RENDER_TARGET_BYTES(w, h, bytes)
#define AGX_DEPTH_BYTES(w, h)           AGX_DEPTH_BYTES_AT(w, h, 4u)

#define AGX_STENCIL_PLANE_BYTES(w, h) ((uint64_t)(w) * (h))
#define AGX_IMAGE_LEVEL_BYTES(w, h)   (((uint64_t)((w) ? (w) : 1u) * ((h) ? (h) : 1u) * 4u + 127u) & ~(uint64_t)127u)

#define AGX_IMAGE_BLOCK_ALIGN(x) (((uint32_t)(x) + 63u) & ~63u)
#define AGX_IMAGE_BLOCK_BYTES(w, h, bytes) \
  ((uint32_t)((uint64_t)AGX_IMAGE_BLOCK_ALIGN(w) * AGX_IMAGE_BLOCK_ALIGN(h) * (bytes) / 128u))

#define AGX_IMAGE_FINE_SLICES(slices) (4u * (slices) - 3u)

#define AGX_DRAWABLE_METADATA_BYTES(w, h) ((uint64_t)AGX_TILES_X(w) * AGX_TILES_Y(h) * 1024u)

#define AGX_SCISSOR_TILE 32

#define AGX_PIPELINE_LENGTH 0x80

#define AGX_PIPELINE_FRG_SHADER_WORDS_BYTES 0x1c

#define AGX_DEPTH_STENCIL_STATE_LENGTH 0x1c

#define AGX_CMD_RECORD_STAGES_OFFSET 0xb8

#define AGX_MAX_CLIP_DISTANCES 8
#define AGX_MAX_VIEWPORTS      16

#define AGX_SEGMENT_LIST_SLOTS_PER_ENTRY 6

#define AGX_RESIDENCY_GRANULE 16384

#define AGX_TILE_PIXELS                16
#define AGX_BLOCK_PIXELS               64
#define AGX_BLOCK_BYTES                16384
#define AGX_MAIN_SPACE_BASE            0x10000000000ull
#define AGX_LOW_SPACE_BASE             0x0ull
#define AGX_MAX_SUBMIT_COMMAND_BUFFERS 64
#define AGX_CMD_MAX_RECORDS            8
#define AGX_TILE_WIDTH                 32
#define AGX_TILE_HEIGHT                32
#define AGX_IMAGEBLOCK_SAMPLE_LENGTH   8
#define AGX_DRIVER_STATE_BYTES         0x48u
#define AGX_OPS_PROGRAM_SCAN_BYTES     0x400u

#define AGX_OPS_PROGRAM_OPERAND_IMMEDIATE_OFFSET 0x06u
#define AGX_OPS_PROGRAM_OPERAND_IMMEDIATE_SHIFT  13u

typedef enum Agx_Selector
{
  AGX_SELECTOR_UNK0 = 0x0,
  AGX_SELECTOR_UNK2 = 0x2,
  AGX_SELECTOR_UNK5 = 0x5,
  AGX_SELECTOR_GET_GLOBAL_IDS = 0x6,
  AGX_SELECTOR_CREATE_COMMAND_QUEUE = 0x7,
  AGX_SELECTOR_DESTROY_COMMAND_QUEUE = 0x8,
  AGX_SELECTOR_ALLOCATE_MEM = 0x9,
  AGX_SELECTOR_FREE_MEM = 0xA,
  AGX_SELECTOR_ALLOCATE_SHMEM = 0xE,
  AGX_SELECTOR_FREE_SHMEM = 0xF,
  AGX_SELECTOR_CREATE_NOTIFICATION_QUEUE = 0x10,
  AGX_SELECTOR_DESTROY_NOTIFICATION_QUEUE = 0x11,
  AGX_SELECTOR_UNK1C = 0x1C,
  AGX_SELECTOR_SUBMIT_COMMAND_BUFFERS = 0x1D,
  AGX_SELECTOR_UNK100 = 0x100,
  AGX_SELECTOR_GET_VERSION = 0x102,
  AGX_SELECTOR_USC_PROFILE_INIT = 0x105,
  AGX_SELECTOR_UNK107 = 0x107,
  AGX_SELECTOR_TRAP1_FREE_MEM = 0x1,
  AGX_SELECTOR_TRAP3_SET_PURGEABLE_STATE = 0x3,
  AGX_SELECTOR_TRAP3_QUEUE_SET_RESOURCE_GROUPS = 0x7,
  AGX_SELECTOR_TRAP4_SUBMIT_COMMAND_BUFFERS = 0x0,
  AGX_SELECTOR_TRAP4_SIGNAL_EVENT = 0x9,
  AGX_SELECTOR_TRAP5_WAIT_EVENT = 0xA,
  AGX_SELECTOR_TRAP6_RESOURCE_GROUP_UPDATE_RESOURCES = 0x6,
} Agx_Selector;

typedef enum Iosurface_Selector
{
  IOSURFACE_SELECTOR_LOOKUP_SURFACE = 0x4,
  IOSURFACE_SELECTOR_UNKD = 0xD,
  IOSURFACE_SELECTOR_GET_GRAPHICS_COMM_PAGE_ADDRESS = 0x20,
  IOSURFACE_SELECTOR_CREATE_SHARED_EVENT = 0x24,
  IOSURFACE_SELECTOR_CREATE_SHARED_EVENT2 = 0x26,
  IOSURFACE_SELECTOR_SHARED_EVENT_NOTIFY_LISTENER = 0x27,
  IOSURFACE_SELECTOR_TRAP2_SET_SIGNALED_VALUE = 0x7,
  IOSURFACE_SELECTOR_TRAP3_WAIT_SIGNALED_VALUE = 0x6,
  IOSURFACE_SELECTOR_TRAP4_APPEND_TRANSACTION = 0xB,
} Iosurface_Selector;

typedef enum Agx_Memory_Type
{
  AGX_MEMORY_TYPE_NORMAL = 0x00000000,
  AGX_MEMORY_TYPE_UNK = 0x08000000,
  AGX_MEMORY_TYPE_CMD_64 = 0x18000000,
  AGX_MEMORY_TYPE_LOW_VA = 0x38000000,
  AGX_MEMORY_TYPE_SHADER = 0x48000000,
  AGX_MEMORY_TYPE_CMD_32 = 0x58000000,
  AGX_MEMORY_TYPE_IMAGE = 0x80888F00,
} Agx_Memory_Type;

typedef enum Agx_Cache_Mode
{
  AGX_CACHE_MODE_DEFAULT = 0,
  AGX_CACHE_MODE_WRITE_COMBINED = 0x400
} Agx_Cache_Mode;

typedef enum Agx_Storage_Mode
{
  AGX_STORAGE_MODE_SHARED = 0,
  AGX_STORAGE_MODE_PRIVATE = 2,
} Agx_Storage_Mode;

typedef enum Agx_Bo_Bit
{
  AGX_BO_BIT_SUBALLOC = 0x0800,
  AGX_BO_BIT_NO_CPU_MAP = 0x2000,
  AGX_BO_BIT_LOW_VA = 0x8000,
  AGX_BO_BIT_REJECTED = 0x10000,
} Agx_Bo_Bit;

typedef uint32_t Agx_Bo_Bits;

typedef enum Agx_Alloc_Type
{
  AGX_ALLOC_TYPE_REGULAR = 0,
  AGX_ALLOC_TYPE_SEGMENT_LIST = 1,
  AGX_ALLOC_TYPE_CMD = 2,
  AGX_ALLOC_TYPE_COUNT,
} Agx_Alloc_Type;

typedef enum Agx_Bo_Space
{
  AGX_BO_SPACE_MAIN,
  AGX_BO_SPACE_USC,
  AGX_BO_SPACE_CMDBUF,
  AGX_BO_SPACE_AUX,
} Agx_Bo_Space;

typedef enum Agx_Objtype
{
  AGX_OBJTYPE_TRIANGLES = 0,
  AGX_OBJTYPE_LINES = 1,
  AGX_OBJTYPE_POINTS = 4,
} Agx_Objtype;

typedef enum Agx_Compare_Function
{
  AGX_COMPARE_FUNCTION_NEVER = 0,
  AGX_COMPARE_FUNCTION_LESS = 1,
  AGX_COMPARE_FUNCTION_EQUAL = 2,
  AGX_COMPARE_FUNCTION_LESS_EQUAL = 3,
  AGX_COMPARE_FUNCTION_GREATER = 4,
  AGX_COMPARE_FUNCTION_NOT_EQUAL = 5,
  AGX_COMPARE_FUNCTION_GREATER_EQUAL = 6,
  AGX_COMPARE_FUNCTION_ALWAYS = 7,
} Agx_Compare_Function;

typedef enum Agx_Stencil_Operation
{
  AGX_STENCIL_OPERATION_KEEP = 0,
  AGX_STENCIL_OPERATION_ZERO = 1,
  AGX_STENCIL_OPERATION_REPLACE = 2,
  AGX_STENCIL_OPERATION_INCREMENT_CLAMP = 3,
  AGX_STENCIL_OPERATION_DECREMENT_CLAMP = 4,
  AGX_STENCIL_OPERATION_INVERT = 5,
  AGX_STENCIL_OPERATION_INCREMENT_WRAP = 6,
  AGX_STENCIL_OPERATION_DECREMENT_WRAP = 7,
} Agx_Stencil_Operation;

typedef enum Agx_Cull_Mode
{
  AGX_CULL_MODE_NONE = 0,
  AGX_CULL_MODE_FRONT = 1,
  AGX_CULL_MODE_BACK = 2,
  AGX_CULL_MODE_FRONT_AND_BACK = 3,
} Agx_Cull_Mode;

typedef enum Agx_Winding
{
  AGX_WINDING_CLOCKWISE = 0,
  AGX_WINDING_COUNTER_CLOCKWISE = 1,
} Agx_Winding;

typedef enum Agx_Triangle_Fill_Mode
{
  AGX_TRIANGLE_FILL_MODE_FILL = 0,
  AGX_TRIANGLE_FILL_MODE_LINES = 1,
  AGX_TRIANGLE_FILL_MODE_POINTS = 2,
} Agx_Triangle_Fill_Mode;

typedef enum Agx_Depth_Clip_Mode
{
  AGX_DEPTH_CLIP_MODE_CLIP = 1,
  AGX_DEPTH_CLIP_MODE_CLAMP = 2,
} Agx_Depth_Clip_Mode;

typedef enum Agx_Depth_Source
{
  AGX_DEPTH_SOURCE_Z = 0,
  AGX_DEPTH_SOURCE_W = 1,
  AGX_DEPTH_SOURCE_COUNT,
} Agx_Depth_Source;

typedef enum Agx_Residency_Class
{
  AGX_RESIDENCY_CLASS_APP = 0x80,
  AGX_RESIDENCY_CLASS_RENDER_TARGET = 0xc,
  AGX_RESIDENCY_CLASS_SHADER_HEAP = 0x400,
  AGX_RESIDENCY_CLASS_DRIVER_A = 0x1,
  AGX_RESIDENCY_CLASS_DRIVER_B = 0x4,
} Agx_Residency_Class;

typedef enum Agx_Bo_Request_Kind
{
  AGX_BO_REQUEST_KIND_ROOT = 0,
  AGX_BO_REQUEST_KIND_SUB_ALLOCATION = 0x80,
  AGX_BO_REQUEST_KIND_IOSURFACE = 0x82,
  AGX_BO_REQUEST_KIND_RESOURCE_GROUP = 3
} Agx_Bo_Request_Kind;

typedef enum Agx_Stage
{
  AGX_STAGE_NONE = 0u,
  AGX_STAGE_DISPATCH = 1u << 0,
  AGX_STAGE_BLIT = 1u << 1,
  AGX_STAGE_ACCELERATION_STRUCTURE = 1u << 2,
  AGX_STAGE_VERTEX = 1u << 3,
  AGX_STAGE_FRAGMENT = 1u << 4,
  AGX_STAGE_TILE = 1u << 5,
  AGX_STAGE_OBJECT = 1u << 6,
  AGX_STAGE_MESH = 1u << 7,
  AGX_STAGE_RESOURCE_STATE = 1u << 8,
  AGX_STAGE_MACHINE_LEARNING = 1u << 9,
  AGX_STAGE_ALL = 0x3ffu,
} Agx_Stage;

typedef enum Agx_Visibility_Option
{
  AGX_VISIBILITY_OPTION_NONE = 0,
  AGX_VISIBILITY_OPTION_DEVICE = 1,
  AGX_VISIBILITY_OPTION_RESOURCE_ALIAS = 2,
} Agx_Visibility_Option;

typedef enum Agx_Attachment_Load_Action
{
  AGX_ATTACHMENT_LOAD_ACTION_DISCARD = 0,
  AGX_ATTACHMENT_LOAD_ACTION_LOAD = 1,
  AGX_ATTACHMENT_LOAD_ACTION_CLEAR = 2,
  AGX_ATTACHMENT_LOAD_ACTION_COUNT,
} Agx_Attachment_Load_Action;

typedef enum Agx_Attachment_Store_Action
{
  AGX_ATTACHMENT_STORE_ACTION_DISCARD = 0,
  AGX_ATTACHMENT_STORE_ACTION_STORE = 1,
  AGX_ATTACHMENT_STORE_ACTION_COUNT,
} Agx_Attachment_Store_Action;

typedef enum Agx_Image_Grid
{
  AGX_IMAGE_GRID_TEXEL = 0,
  AGX_IMAGE_GRID_SAMPLE,
  AGX_IMAGE_GRID_COUNT
} Agx_Image_Grid;

typedef enum Agx_Vertex_Pipeline
{
  AGX_VERTEX_PIPELINE_DRIVER = 0x1000000,
} Agx_Vertex_Pipeline;

typedef enum Agx_Purgeable_State
{
  AGX_PURGEABLE_STATE_KEEP_CURRENT = 0,
  AGX_PURGEABLE_STATE_NON_VOLATILE = 1,
  AGX_PURGEABLE_STATE_VOLATILE = 2,
  AGX_PURGEABLE_STATE_EMPTY = 3,
  AGX_PURGEABLE_STATE_COUNT,
} Agx_Purgeable_State;

typedef struct Agx_Bo
{
  Agx_Alloc_Type  type;
  size_t          size;
  uint32_t        index;
  void*           cpu;
  uint64_t        gpu_va;
  uint64_t        guid;
  Agx_Memory_Type memory_type;
  Agx_Bo_Bits     bits;
  uint32_t        width;
  uint32_t        height;
  uint64_t        surface_id;
  uint64_t        shared_guid;
  uint64_t        sub_offset;
  uint32_t        parent_index;
  char*           name;
  bool            ro;
} Agx_Bo;

typedef struct Agx_Segment_List_Builder
{
  uint8_t* base;
  size_t   capacity;
  size_t   cursor;
  size_t   list_at;
  uint32_t records;
  uint32_t count;
  bool     overflowed;
} Agx_Segment_List_Builder;

typedef struct Agx_Create_Command_Queue_Resp
{
  uint64_t id;
  uint32_t unknown_0x08;
  uint32_t unknown_0x0c;
} __attribute__((packed)) Agx_Create_Command_Queue_Resp;

typedef struct Agx_Command_Queue_Desc
{
  uint32_t queue_type;
  uint32_t unknown_inert_0x04;
  uint32_t unknown_inert_0x08;
  uint32_t unknown_policed_0x0c;
} __attribute__((packed)) Agx_Command_Queue_Desc;

void
agx_command_queue_desc_init(Agx_Command_Queue_Desc* desc);

void
agx_command_queue_desc_set_type(Agx_Command_Queue_Desc* desc, uint32_t queue_type);

typedef struct Agx_Create_Shmem_Req
{
  uint64_t size;
  uint64_t type;
} __attribute__((packed)) Agx_Create_Shmem_Req;

typedef struct Agx_Create_Shmem_Resp
{
  void*    map;
  uint32_t size;
  uint32_t index;
} __attribute__((packed)) Agx_Create_Shmem_Resp;

typedef struct Agx_Create_Notification_Queue_Resp
{
  IODataQueueMemory* queue;
  uint32_t           id;
  uint32_t           unknown_0x0c;
} __attribute__((packed)) Agx_Create_Notification_Queue_Resp;

typedef struct Agx_Submit_Cmd_Req
{
  uint32_t command_buffer_shmem_id;
  uint32_t segment_list_shmem_id;
  uint64_t unknown_0x08;
  uint64_t unknown_0x10;
  uint64_t completion_token;
  uint32_t unknown_0x20;
  uint32_t unknown_0x24;
  uint32_t unknown_0x28;
  uint32_t unknown_0x2c;
  uint32_t unknown_0x30;
  uint32_t unknown_0x34;
  uint32_t unknown_0x38;
  uint32_t unknown_0x3c;
} __attribute__((packed)) Agx_Submit_Cmd_Req;

typedef struct Agx_Bo_Desc
{
  uint32_t    request_kind;
  uint32_t    cache_mode;
  uint32_t    width_height;
  uint32_t    unknown_0x0c;
  uint8_t     unknown_0x10[3];
  uint8_t     bytes_per_pixel;
  Agx_Bo_Bits bits;
  uint32_t    unknown_0x18;
  uint32_t    unknown_0x1c;
  uint32_t    unknown_0x20;
  uint32_t    unknown_0x24;
  uint64_t    short_form_image_bytes;
  uint32_t    unknown_0x30;
  uint32_t    address_window_4gb;
  uint64_t    sub_cpu;
  uint64_t    root_cpu;
  uint64_t    root_size;
  uint32_t    heap_index;
  uint32_t    unknown_0x54;
  uint32_t    type;
  uint32_t    unknown_0x5c;
  uint64_t    shared_guid;
} __attribute__((packed)) Agx_Bo_Desc;

void
agx_bo_desc_init(Agx_Bo_Desc* desc);

void
agx_bo_desc_set_size(Agx_Bo_Desc* desc, uint64_t bytes);

void
agx_bo_desc_set_space(Agx_Bo_Desc* desc, Agx_Bo_Space space);

void
agx_bo_desc_set_memory_type(Agx_Bo_Desc* desc, Agx_Memory_Type type);

void
agx_bo_desc_set_cache_mode(Agx_Bo_Desc* desc, Agx_Cache_Mode mode);

void
agx_bo_desc_set_bits(Agx_Bo_Desc* desc, Agx_Bo_Bits bits);

void
agx_bo_desc_set_width(Agx_Bo_Desc* desc, uint32_t width);

void
agx_bo_desc_set_height(Agx_Bo_Desc* desc, uint32_t height);

void
agx_bo_desc_set_bytes_per_pixel(Agx_Bo_Desc* desc, uint32_t bpp);

void
agx_bo_desc_set_image(Agx_Bo_Desc* desc, uint32_t width, uint32_t height, uint32_t bytes_per_pixel);

void
agx_bo_desc_set_depth_image(Agx_Bo_Desc* desc, uint32_t width, uint32_t height, Agx_Depth_Format format);

void
agx_bo_desc_set_render_target(Agx_Bo_Desc* desc, uint32_t width, uint32_t height, uint32_t bytes);

void
agx_bo_desc_set_iosurface(Agx_Bo_Desc* desc, uint32_t surface_id, uint32_t width, uint32_t height);

void
agx_bo_desc_set_cpu_visible(Agx_Bo_Desc* desc, bool visible);
void
agx_bo_desc_set_storage_mode(Agx_Bo_Desc* desc, Agx_Storage_Mode mode);

void
agx_bo_desc_set_suballocation(Agx_Bo_Desc* desc, const Agx_Bo* parent, uint64_t offset);

void
agx_bo_desc_set_shared_guid(Agx_Bo_Desc* desc, uint64_t guid);

typedef struct Agx_Create_Bo_Resp
{
  uint64_t gpu_va;
  uint64_t cpu;
  uint64_t record_cpu;
  uint32_t unknown_0x18;
  uint32_t unknown_0x1c;
  uint32_t unknown_0x20;
  uint32_t index;
  uint64_t root_size;
  uint64_t guid;
  uint64_t shared_guid;
  uint32_t unknown_0x40;
  uint32_t unknown_0x44;
  uint32_t unknown_0x48;
  uint32_t unknown_0x4c;
  uint64_t sub_size;
} __attribute__((packed)) Agx_Create_Bo_Resp;

typedef struct Agx_Device
{
  mach_port_t iosrf;
  mach_port_t agx;
  uint64_t    comm_page_address;
} Agx_Device;

typedef struct Agx_Fence
{
  mach_port_t agx;
  mach_port_t iosrf;
  uint64_t    id;
} Agx_Fence;

typedef struct Agx_Notification_Queue
{
  mach_port_t        port;
  IODataQueueMemory* queue;
  uint32_t           id;
} Agx_Notification_Queue;

typedef struct Agx_Notification
{
  uint64_t status;
  uint64_t start_ns;
  uint64_t end_ns;
} Agx_Notification;

typedef struct Agx_Queue_Progress
{
  uint64_t         submitted;
  uint64_t         retired;
  Agx_Notification pending[64];
  uint32_t         pending_count;
} Agx_Queue_Progress;

typedef struct Agx_Command_Queue
{
  mach_port_t            agx;
  uint32_t               id;
  Agx_Notification_Queue notif;
  Agx_Queue_Progress*    progress;
} Agx_Command_Queue;

typedef struct Agx_Stencil_State
{
  Agx_Compare_Function  compare;
  uint8_t               reference;
  uint8_t               read_mask;
  uint8_t               write_mask;
  Agx_Stencil_Operation fail;
  Agx_Stencil_Operation depth_fail;
  Agx_Stencil_Operation pass;
} Agx_Stencil_State;

typedef struct Agx_Depth_Stencil_Desc
{
  Agx_Compare_Function depth_compare;
  bool                 depth_write;
  bool                 two_sided;
  Agx_Stencil_State    stencil_front;
  Agx_Stencil_State    stencil_back;
} Agx_Depth_Stencil_Desc;

typedef struct Agx_Viewport_Desc
{
  float x, y, width, height, near_z, far_z;
} Agx_Viewport_Desc;

typedef struct Agx_Scissor_Desc
{
  uint32_t x, y, width, height;
} Agx_Scissor_Desc;

typedef struct Agx_Copy_Desc
{
  uint64_t geometry;
  uint64_t operands;
  uint64_t descriptors;
  void*    operands_cpu;
  void*    geometry_cpu;
  uint64_t programs;
  void*    programs_cpu;
} Agx_Copy_Desc;

typedef struct Agx_Cmd
{
  Agx_Bo                 memory;
  size_t                 cursor;
  uint8_t*               open_encoder;
  Agx_Bo                 stream;
  Agx_Bo                 segment_list;
  size_t                 stream_cursor;
  uint8_t*               cs;
  uint64_t               stream_gpu;
  uint8_t*               ops;
  uint8_t*               ops_base;
  uint32_t               ops_program;
  uint64_t               ops_operands_first;
  uint32_t               ops_cursor;
  void*                  ops_operands;
  uint64_t               ops_operands_gpu;
  uint32_t               ops_blocks;
  void*                  ops_programs_cpu;
  uint32_t               pass_attachments;
  uint32_t               pass_primitives;
  uint32_t               pass_samples;
  uint8_t*               last_record;
  uint32_t               record_count;
  uint32_t               record_offset[AGX_CMD_MAX_RECORDS];
  uint32_t               record_size[AGX_CMD_MAX_RECORDS];
  uint32_t               ops_offset;
  uint32_t               copy_records;
  bool                   alias_barrier_pending;
  Agx_Stage              alias_barrier_after;
  Agx_Stage              alias_barrier_before;
  Agx_Copy_Desc          last_copy_desc;
  bool                   last_copy_desc_valid;
  Agx_Cull_Mode          cull_mode;
  Agx_Winding            front_face_winding;
  Agx_Depth_Clip_Mode    depth_clip_mode;
  Agx_Depth_Source       depth_source;
  bool                   rasterizer_discard;
  bool                   pretransform;
  bool                   reads_primitive_id;
  uint32_t               point_line_width;
  uint32_t               tagsort_flush_data;
  Agx_Triangle_Fill_Mode fill_mode;
  Agx_Depth_Stencil_Desc depth_stencil;
  Agx_Viewport_Desc      viewports[AGX_MAX_VIEWPORTS];
  Agx_Scissor_Desc       scissors[AGX_MAX_VIEWPORTS];
  bool                   scissor_set[AGX_MAX_VIEWPORTS];
  uint32_t               viewport_count;
  bool                   has_viewport;
  bool                   has_scissor;
  uint32_t               depth_bias_index;
  uint32_t               scissor_index;
  bool                   depth_bias_enable;
  bool                   scissor_enable;
  Agx_Pass_Type          pass_type;
  bool                   vertex_amplify;
  uint32_t               varying_components_copy;
  uint32_t               clip_distance_count;
  void*                  depth_bias_table_cpu;
  uint64_t               depth_bias_table_gpu;
  uint64_t               scissor_buffer_gpu;
  uint32_t               depth_bias_entries;
  uint32_t               depth_bias_next;
  void*                  state_arena_cpu;
  uint64_t               state_arena_gpu;
  size_t                 state_arena_bytes;
  size_t                 state_arena_cursor;
  void*                  driver_arguments_cpu;
  size_t                 driver_arguments_bytes;
  uint32_t               render_passes;
  void*                  scratch_cpu;
  uint64_t               scratch_gpu;
  size_t                 scratch_bytes;
  size_t                 scratch_cursor;
  uint64_t               completion_token;
} Agx_Cmd;

typedef struct Agx_Depth_Bias_Desc
{
  float constant;
  float slope_scale;
  float clamp;
} Agx_Depth_Bias_Desc;

typedef struct Agx_Pipeline_Desc
{
  uint32_t               shader_code_offset;
  uint32_t               varying_components;
  uint32_t               varying_groups;
  uint32_t               vertex_output_size;
  uint32_t               output_selects;
  bool                   has_output_selects;
  bool                   reads_frag_coord_z;
  uint32_t               clip_distance_count;
  Agx_Objtype            objtype;
  uint32_t               point_line_width;
  Agx_Triangle_Fill_Mode fill_mode;
  bool                   scissor_enable;
  bool                   depth_bias_enable;
  Agx_Pass_Type          pass_type;
  Agx_Compare_Function   depth_compare;
  bool                   depth_write;
  bool                   two_sided;
  Agx_Stencil_State      stencil_front;
  Agx_Stencil_State      stencil_back;
  bool                   rasterization_enabled;
  uint32_t               tile_word;
  uint32_t               tagsort_flush_data;
} Agx_Pipeline_Desc;

void
agx_pipeline_desc_init(Agx_Pipeline_Desc* pipeline);

void
agx_pipeline_desc_set_shader_code(Agx_Pipeline_Desc* pipeline, const Agx_Bo* heap, uint32_t offset);

void
agx_pipeline_desc_set_varying_components(Agx_Pipeline_Desc* pipeline, uint32_t components);

void
agx_pipeline_desc_set_vertex_output_size(Agx_Pipeline_Desc* pipeline, uint32_t size);

void
agx_pipeline_desc_set_output_selects(Agx_Pipeline_Desc* pipeline, uint32_t selects);

void
agx_pipeline_desc_set_clip_distance_count(Agx_Pipeline_Desc* pipeline, uint32_t count);

void
agx_pipeline_desc_set_frag_coord_z(Agx_Pipeline_Desc* pipeline, bool reads);

void
agx_pipeline_desc_set_varying_groups(Agx_Pipeline_Desc* pipeline, uint32_t groups);

void
agx_pipeline_desc_set_objtype(Agx_Pipeline_Desc* pipeline, Agx_Objtype objtype);

void
agx_pipeline_desc_set_scissor_enable(Agx_Pipeline_Desc* pipeline, bool scissor_enable);

void
agx_pipeline_desc_set_depth_bias_enable(Agx_Pipeline_Desc* pipeline, bool depth_bias_enable);

void
agx_pipeline_desc_set_pass_type(Agx_Pipeline_Desc* pipeline, Agx_Pass_Type pass_type);

void
agx_pipeline_desc_set_depth_compare(Agx_Pipeline_Desc* pipeline, Agx_Compare_Function compare);

void
agx_pipeline_desc_set_depth_write(Agx_Pipeline_Desc* pipeline, bool depth_write);

void
agx_pipeline_desc_set_rasterization_enabled(Agx_Pipeline_Desc* pipeline, bool enabled);

void
agx_pipeline_desc_set_stencil(Agx_Pipeline_Desc* pipeline, Agx_Stencil_State state);

void
agx_pipeline_desc_set_stencil_two_sided(Agx_Pipeline_Desc* pipeline, Agx_Stencil_State front, Agx_Stencil_State back);

void
agx_pipeline_desc_set_point_line_width(Agx_Pipeline_Desc* pipeline, uint32_t width);

void
agx_pipeline_desc_set_fill_mode(Agx_Pipeline_Desc* pipeline, Agx_Triangle_Fill_Mode mode);

typedef struct Agx_Work_Extent
{
  uint32_t x, y, z;
} Agx_Work_Extent;

typedef struct Agx_Render_Pass_Desc
{
  uint32_t                    attachment_count;
  uint32_t                    sample_count;
  uint32_t                    width;
  uint32_t                    height;
  uint64_t                    background_program;
  uint64_t                    store_program;
  uint64_t                    tile_program_heap;
  uint64_t                    uniform_slice;
  uint64_t                    depth_buffer;
  uint64_t                    stencil_buffer;
  Agx_Attachment_Load_Action  depth_load_action;
  Agx_Attachment_Store_Action depth_store_action;
  float                       depth_clear;
  Agx_Depth_Format            depth_format;
  Agx_Attachment_Store_Action stencil_store_action;
  uint8_t                     stencil_clear;
  float                       clear_color[4];
  Agx_Attachment_Load_Action  color_load_action;
  Agx_Attachment_Store_Action color_store_action;
  uint32_t                    threadgroup_memory_length;
} Agx_Render_Pass_Desc;

void
agx_render_pass_desc_init(Agx_Render_Pass_Desc* desc);

void
agx_render_pass_desc_set_clear_color(Agx_Render_Pass_Desc* desc, const float rgba[4], Agx_Image_Channels channels);

void
agx_render_pass_desc_set_depth_format(Agx_Render_Pass_Desc* desc, Agx_Depth_Format format);

typedef struct Agx_Sampler_Desc
{
  Agx_Sampler_Filter       mag_filter;
  Agx_Sampler_Filter       min_filter;
  Agx_Sampler_Mip_Filter   mip_filter;
  Agx_Address_Mode         wrap_s;
  Agx_Address_Mode         wrap_t;
  Agx_Address_Mode         wrap_r;
  uint32_t                 min_lod;
  uint32_t                 max_lod;
  uint32_t                 max_aniso;
  bool                     normalized_coords;
  Agx_Sampler_Compare_Func compare_func;
  bool                     compare_mode;
  Agx_Sampler_Border_Mode  border_mode;
} Agx_Sampler_Desc;

void
agx_sampler_desc_init(Agx_Sampler_Desc* desc);
void
agx_sampler_desc_set_filter(Agx_Sampler_Desc* desc, Agx_Sampler_Filter min, Agx_Sampler_Filter mag);
void
agx_sampler_desc_set_address_mode(Agx_Sampler_Desc* desc, Agx_Address_Mode s, Agx_Address_Mode t, Agx_Address_Mode r);
void
agx_sampler_desc_set_mip_filter(Agx_Sampler_Desc* desc, Agx_Sampler_Mip_Filter filter);

void
agx_sampler_desc_set_lod_clamp(Agx_Sampler_Desc* desc, uint32_t min_lod, uint32_t max_lod);

void
agx_sampler_desc_set_max_anisotropy(Agx_Sampler_Desc* desc, uint32_t samples);

void
agx_sampler_desc_set_normalized_coordinates(Agx_Sampler_Desc* desc, bool normalized);

void
agx_sampler_desc_set_compare(Agx_Sampler_Desc* desc, Agx_Sampler_Compare_Func compare);

void
agx_sampler_desc_set_border_color(Agx_Sampler_Desc* desc, Agx_Sampler_Border_Mode border);

typedef struct Agx_Image
{
  uint64_t           buffer;
  uint64_t           metadata;
  uint32_t           width;
  uint32_t           height;
  uint32_t           tiles_per_row;
  bool               srgb_decode;
  uint32_t           layout;
  uint32_t           dimension;
  uint32_t           slices;
  uint32_t           slice;
  Agx_Image_Grid     grid;
  uint32_t           mode_bits;
  Agx_Channel        swizzle[4];
  Agx_Image_Channels channels;
  bool               is_float;
  bool               transpose;
  bool               extended;
  bool               compress;
  uint32_t           base_level;
  uint32_t           levels;
  uint32_t           sample_count;
} Agx_Image;

void
agx_image_init(Agx_Image* desc);
void
agx_image_set_image(Agx_Image* desc, uint64_t buffer, uint32_t width, uint32_t height);
void
agx_image_set_layout(Agx_Image* desc, uint32_t layout);

void
agx_image_set_channels(Agx_Image* desc, Agx_Image_Channels channels);

void
agx_image_set_float(Agx_Image* desc, bool is_float);

void
agx_image_set_transpose(Agx_Image* desc, bool transpose);

void
agx_image_set_swizzle(Agx_Image* desc, Agx_Channel r, Agx_Channel g, Agx_Channel b, Agx_Channel a);

void
agx_image_set_tiles_per_row(Agx_Image* desc, uint32_t tiles);

void
agx_image_set_dimension(Agx_Image* desc, uint32_t dimension);

void
agx_image_set_slices(Agx_Image* desc, uint32_t slices);

void
agx_image_set_slice(Agx_Image* desc, uint32_t slice);

void
agx_image_set_srgb_decode(Agx_Image* desc, bool decode);

void
agx_image_set_grid(Agx_Image* desc, Agx_Image_Grid grid);
void
agx_image_set_compression(Agx_Image* desc, uint64_t metadata);

void
agx_image_set_extended(Agx_Image* desc, bool extended);

void
agx_image_set_levels(Agx_Image* desc, uint32_t levels);

void
agx_image_set_level_range(Agx_Image* desc, uint32_t base_level, uint32_t levels);

void
agx_image_set_sample_count(Agx_Image* desc, uint32_t samples);

typedef struct Agx_Compute_Bindings_Desc
{
  uint32_t table_offset;
  uint32_t descriptor_offset;
  uint32_t reserved_slots;
} Agx_Compute_Bindings_Desc;

void
agx_compute_bindings_desc_init(Agx_Compute_Bindings_Desc* desc);
void
agx_compute_bindings_desc_set_table(Agx_Compute_Bindings_Desc* desc, uint32_t table_offset, uint32_t descriptor_offset);
void
agx_compute_bindings_desc_set_reserved_slots(Agx_Compute_Bindings_Desc* desc, uint32_t slots);

typedef struct Agx_Draw_Arguments
{
  uint32_t vertex_count;
  uint32_t instance_count;
  uint32_t vertex_start;
  uint32_t base_instance;
} Agx_Draw_Arguments;

typedef struct Agx_Draw_Indexed_Arguments
{
  uint32_t index_count;
  uint32_t instance_count;
  uint32_t index_start;
  uint32_t base_vertex;
  uint32_t base_instance;
} Agx_Draw_Indexed_Arguments;

uint32_t
agx_image_channels_bytes(Agx_Image_Channels channels);

uint32_t
agx_drawable_uniform_tiles(const void* surface_base, uint32_t width, uint32_t height, uint32_t* color, uint32_t* tile_count);

uint32_t
agx_drawable_tile_header_index(uint32_t tile_x, uint32_t tile_y);

uint32_t
agx_drawable_metadata_tiles(const void* surface_base, uint32_t width, uint32_t height, uint32_t* tile_count);

static inline uint32_t
agx_round_up_power_of_two(uint32_t value)
{
  if (value <= 1u)
  {
    return 1u;
  }
  value -= 1u;
  value |= value >> 1;
  value |= value >> 2;
  value |= value >> 4;
  value |= value >> 8;
  value |= value >> 16;
  return value + 1u;
}

uint64_t
agx_image_level_offset(uint32_t width, uint32_t height, uint32_t level);

uint64_t
agx_image_chain_bytes(uint32_t width, uint32_t height);

uint64_t
agx_image_level_header_offset(uint32_t width, uint32_t height, uint32_t level);

bool
agx_create_device(Agx_Device* device);

bool
agx_usc_profile_init(Agx_Device device);

void
agx_destroy_device(Agx_Device device);

Agx_Bo
agx_bo_alloc(Agx_Device device, const Agx_Bo_Desc* desc);

Agx_Bo
agx_alloc_iosurface(Agx_Device device, uint32_t surface_id, uint32_t width, uint32_t height);

bool
agx_free_mem(Agx_Device, Agx_Bo* allocation);

Agx_Bo
agx_alloc_shmem(Agx_Device, size_t size, bool cmd);

bool
agx_free_shmem(Agx_Device device, Agx_Bo* allocation);

Agx_Command_Queue
agx_create_command_queue(Agx_Device device, const Agx_Command_Queue_Desc* desc);

bool
agx_destroy_command_queue(Agx_Device device, Agx_Command_Queue* queue);

void
agx_cmd_begin(Agx_Cmd* cmd, const Agx_Bo* memory, const Agx_Bo* stream, const Agx_Bo* segment_list);

uint8_t*
agx_cmd_begin_encoder(Agx_Cmd* cmd, size_t record_bytes);

void
agx_cmd_end_encoder(Agx_Cmd* cmd);

void
agx_cmd_barrier_after_queue_stages(Agx_Cmd* cmd, Agx_Stage after, Agx_Stage before, Agx_Visibility_Option visibility);
void
agx_cmd_barrier_after_stages(Agx_Cmd* cmd, Agx_Stage after, Agx_Stage before_queue, Agx_Visibility_Option visibility);

bool
agx_cmd_begin_render_pass(Agx_Cmd* cmd, const Agx_Render_Pass_Desc* desc);

void
agx_cmd_set_render_target(Agx_Cmd* cmd, uint32_t index, const Agx_Bo* image, uint32_t width, uint32_t height, Agx_Image_Channels channels);

size_t
agx_cmd_end_render_pass(Agx_Cmd* cmd);

void
agx_argument_table_set_sampler(const Agx_Bo* table, const Agx_Sampler_Desc* sampler);

void
agx_argument_table_set_image(const Agx_Bo* table, const Agx_Image* image);

uint32_t
agx_compute_bindings_write(
  const Agx_Compute_Bindings_Desc* desc,
  const Agx_Bo*                    table,
  const Agx_Image*                 image,
  const Agx_Sampler_Desc*          sampler,
  const uint64_t*                  buffers,
  uint32_t                         buffer_count
);

void
agx_argument_table_set_compute_image(const Agx_Bo* table, const Agx_Image* image);

void
agx_argument_table_set_compute_sampler(const Agx_Bo* table, const Agx_Sampler_Desc* sampler);

void
agx_driver_arguments_init(const Agx_Bo* arguments);

void
agx_driver_arguments_set_clear_color(const Agx_Bo* arguments, uint32_t block, uint32_t attachment, const float rgba[4]);

void
agx_driver_arguments_set_render_target(const Agx_Bo* arguments, uint32_t block, uint32_t attachment, const Agx_Image* image);

size_t
agx_cmd_end(Agx_Cmd* cmd);

void
agx_cmd_residency_begin(Agx_Cmd* cmd, Agx_Segment_List_Builder* builder);

void
agx_cmd_bind_vertex_pipeline(Agx_Cmd* cmd, Agx_Vertex_Pipeline pipeline, uint64_t program_buffer, uint32_t varying_components);

void
agx_cmd_rebind_vertex_pipeline(Agx_Cmd* cmd, Agx_Vertex_Pipeline pipeline, uint64_t program_buffer);

void
agx_cmd_set_vertex_amplification(Agx_Cmd* cmd, bool enabled);

void
agx_cmd_set_varying_components_copy(Agx_Cmd* cmd, uint32_t components);

void
agx_cmd_push_state(Agx_Cmd* cmd, uint64_t address, uint32_t words);

void
agx_cmd_push_driver_state(Agx_Cmd* cmd, const Agx_Bo* state);

void
agx_driver_state_write(const Agx_Bo* state, uint32_t varying_components);

void
agx_cmd_set_pipeline_state(Agx_Cmd* cmd, const Agx_Bo* state, uint64_t offset);

void
agx_cmd_set_depth_stencil_state(Agx_Cmd* cmd, const Agx_Depth_Stencil_Desc* desc);
void
agx_cmd_set_depth_compare_function(Agx_Cmd* cmd, Agx_Compare_Function compare);
void
agx_cmd_set_depth_write_enabled(Agx_Cmd* cmd, bool enabled);

void
agx_cmd_set_stencil_state(Agx_Cmd* cmd, Agx_Stencil_State state);
void
agx_cmd_set_stencil_state_two_sided(Agx_Cmd* cmd, Agx_Stencil_State front, Agx_Stencil_State back);

void
agx_cmd_set_stencil_reference_value(Agx_Cmd* cmd, uint8_t reference);

void
agx_cmd_set_stencil_reference_values(Agx_Cmd* cmd, uint8_t front, uint8_t back);
void
agx_cmd_set_triangle_fill_mode(Agx_Cmd* cmd, Agx_Triangle_Fill_Mode mode);

void
agx_cmd_set_viewport(Agx_Cmd* cmd, uint32_t index, Agx_Viewport_Desc viewport);
void
agx_cmd_set_scissor_rect(Agx_Cmd* cmd, uint32_t index, Agx_Scissor_Desc scissor);

void
agx_cmd_set_dynamic_state(Agx_Cmd* cmd, const Agx_Bo* memory, uint64_t offset, size_t bytes);
void
agx_cmd_set_point_line_width(Agx_Cmd* cmd, uint32_t width);

bool
agx_cmd_begin_copy_encoder(Agx_Cmd* cmd, const Agx_Copy_Desc* desc);

bool
agx_cmd_begin_compute_encoder(Agx_Cmd* cmd, const Agx_Copy_Desc* desc);
void
agx_cmd_end_compute_encoder(Agx_Cmd* cmd);

void
agx_cmd_copy_image(Agx_Cmd* cmd, const Agx_Image* source, const Agx_Image* destination, Agx_Work_Extent extent, Agx_Work_Extent tile);

void
agx_cmd_copy_buffer(Agx_Cmd* cmd, uint64_t source, uint64_t destination, uint64_t bytes);

void
agx_cmd_set_scratch(Agx_Cmd* cmd, const Agx_Bo* memory, uint64_t offset, size_t bytes);

void
agx_pool_arguments_set_vertex_buffer(const Agx_Bo* arguments, uint32_t index, const Agx_Bo* buffer);

void
agx_cmd_set_driver_arguments(Agx_Cmd* cmd, const Agx_Bo* arguments);

void
agx_cmd_fill_buffer(Agx_Cmd* cmd, uint64_t destination, uint64_t bytes, uint32_t value);

void
agx_cmd_dispatch(Agx_Cmd* cmd, Agx_Work_Extent grid, Agx_Work_Extent threadgroup);

void
agx_cmd_end_copy_encoder(Agx_Cmd* cmd);

void
agx_cmd_set_cull_mode(Agx_Cmd* cmd, Agx_Cull_Mode mode);
void
agx_cmd_set_front_facing_winding(Agx_Cmd* cmd, Agx_Winding winding);

void
agx_cmd_set_depth_clip_mode(Agx_Cmd* cmd, Agx_Depth_Clip_Mode mode);

void
agx_cmd_set_depth_source(Agx_Cmd* cmd, Agx_Depth_Source source);

void
agx_cmd_set_clip_distance_count(Agx_Cmd* cmd, uint32_t count);

void
agx_cmd_set_rasterization_enabled(Agx_Cmd* cmd, bool enabled);

void
agx_cmd_set_pretransform(Agx_Cmd* cmd, bool pretransformed);

void
agx_cmd_set_primitive_id(Agx_Cmd* cmd, bool reads);

void
agx_cmd_set_depth_bias_table(Agx_Cmd* cmd, const Agx_Bo* table, uint32_t entries);

void
agx_cmd_set_scissor_buffer(Agx_Cmd* cmd, const Agx_Bo* buffer);
void
agx_cmd_set_depth_bias(Agx_Cmd* cmd, Agx_Depth_Bias_Desc bias);

void
agx_cmd_set_pass_type(Agx_Cmd* cmd, Agx_Pass_Type pass_type);

void
agx_cmd_set_depth_bias_enable(Agx_Cmd* cmd, bool enable);

void
agx_cmd_set_scissor_enable(Agx_Cmd* cmd, bool enable);
void
agx_cmd_set_depth_bias_index(Agx_Cmd* cmd, uint32_t index);
void
agx_cmd_set_scissor_index(Agx_Cmd* cmd, uint32_t index);

void
agx_cmd_draw(Agx_Cmd* cmd, Agx_Primitive primitive, uint32_t vertex_count, uint32_t instance_count, uint32_t vertex_start);

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
);

void
agx_cmd_draw_indirect(Agx_Cmd* cmd, Agx_Primitive primitive, uint64_t arguments);

void
agx_cmd_draw_indexed_indirect(
  Agx_Cmd*       cmd,
  Agx_Primitive  primitive,
  uint64_t       index_buffer,
  size_t         index_buffer_bytes,
  Agx_Index_Size index_size,
  uint64_t       arguments
);

void
agx_cmd_link_stream(Agx_Cmd* cmd, uint64_t target);

void
agx_cmd_continue_stream(Agx_Cmd* cmd, const Agx_Bo* stream);

bool
agx_queue_submit(Agx_Command_Queue queue, Agx_Cmd* cmds, uint32_t cmd_count);

mach_port_t
agx_trap_connection(mach_port_t connection);

bool
agx_queue_signal_fence(Agx_Command_Queue queue, Agx_Fence fence, uint64_t value);

bool
agx_queue_wait_fence(Agx_Command_Queue queue, Agx_Fence fence, uint64_t value);

bool
agx_create_fence(Agx_Device device, Agx_Fence* fence);

bool
agx_signal_fence(Agx_Fence fence, uint64_t value);

bool
agx_wait_fence(Agx_Fence fence, uint64_t value, uint32_t timeout_ms);

void
agx_destroy_fence(Agx_Device device, Agx_Fence fence);

bool
agx_resource_group_update(Agx_Device device, uint64_t group, const uint32_t* indices, uint32_t count);

bool
agx_queue_set_resource_groups(Agx_Command_Queue queue, uint64_t group);

bool
agx_queue_bind_notification_queue(Agx_Command_Queue queue);

Agx_Purgeable_State
agx_bo_set_purgeable_state(Agx_Device device, const Agx_Bo* bo, Agx_Purgeable_State state);

uint32_t
agx_drain_notifications(Agx_Command_Queue queue, Agx_Notification* messages, uint32_t max);

uint64_t
agx_queue_submitted(Agx_Command_Queue queue);

uint64_t
agx_queue_retired(Agx_Command_Queue queue);

bool
agx_queue_wait_retired(Agx_Command_Queue queue, uint64_t submissions, uint32_t timeout_ms, Agx_Notification* messages, uint32_t max);

void
agx_segment_list_begin(Agx_Segment_List_Builder* b, const Agx_Bo* memory);

bool
agx_segment_list_add(Agx_Segment_List_Builder* b, const Agx_Bo* alloc, Agx_Residency_Class cls);

bool
agx_segment_list_add_sized(Agx_Segment_List_Builder* b, uint32_t kernel_index, uint64_t size, Agx_Residency_Class cls);

void
agx_segment_list_record(Agx_Segment_List_Builder* b, uint32_t record_offset, uint32_t record_size);

size_t
agx_segment_list_finish(Agx_Segment_List_Builder* b);

void
agx_depth_bias_write(void* table_cpu, uint32_t index, Agx_Depth_Bias_Desc bias);

void
agx_scissor_write(void* buffer_cpu, uint32_t index, Agx_Scissor_Desc scissor);

void
agx_depth_stencil_desc_init(Agx_Depth_Stencil_Desc* desc);

void
agx_depth_stencil_desc_set_depth_compare(Agx_Depth_Stencil_Desc* desc, Agx_Compare_Function compare);

void
agx_depth_stencil_desc_set_depth_write(Agx_Depth_Stencil_Desc* desc, bool depth_write);

void
agx_depth_stencil_desc_set_stencil(Agx_Depth_Stencil_Desc* desc, Agx_Stencil_State state);

void
agx_depth_stencil_desc_set_stencil_two_sided(Agx_Depth_Stencil_Desc* desc, Agx_Stencil_State front, Agx_Stencil_State back);

uint32_t
agx_pipeline_interpolator_groups(uint32_t varying_groups, uint32_t varying_components);

size_t
agx_pipeline_state_write(const Agx_Pipeline_Desc* pipeline, void* state_cpu);

void
agx_work_extent_write(void* geometry_cpu, Agx_Work_Extent primary, Agx_Work_Extent secondary);

void
agx_work_extent_for_buffer_copy(uint64_t bytes, Agx_Work_Extent* source, Agx_Work_Extent* tile);
