#pragma once

#include "agx.h"

#include <IOSurface/IOSurface.h>
#include <stdbool.h>
#include <stdint.h>

#define AGX_SWAPCHAIN_MAX_IMAGES 4

typedef enum Agx_Swapchain_Format
{
  AGX_SWAPCHAIN_FORMAT_BGRA8_UNORM = 0,
  AGX_SWAPCHAIN_FORMAT_BGRA8_UNORM_SRGB,
  AGX_SWAPCHAIN_FORMAT_RGBA16_FLOAT,
  AGX_SWAPCHAIN_FORMAT_RGBA8_UNORM,
  AGX_SWAPCHAIN_FORMAT_RGB10A2_UNORM,
  AGX_SWAPCHAIN_FORMAT_COUNT
} Agx_Swapchain_Format;

typedef struct Agx_Swapchain
{
  void*                layer;
  IOSurfaceRef         images[AGX_SWAPCHAIN_MAX_IMAGES];
  uint32_t             image_count;
  uint32_t             index;
  uint32_t             width;
  uint32_t             height;
  Agx_Swapchain_Format format;
  uint64_t             acquire_timeouts;
} Agx_Swapchain;

bool
agx_swapchain_format_is_compressed(Agx_Swapchain_Format format);

bool
agx_swapchain_create(
  Agx_Swapchain*       swapchain,
  void*                layer,
  uint32_t             width,
  uint32_t             height,
  uint32_t             image_count,
  Agx_Swapchain_Format format
);

void
agx_swapchain_destroy(Agx_Swapchain* swapchain);

IOSurfaceRef
agx_swapchain_acquire(Agx_Swapchain* swapchain, uint32_t* index);

void
agx_swapchain_present(Agx_Swapchain* swapchain);

bool
agx_surface_append_transaction(IOSurfaceRef surface, Agx_Fence fence, uint64_t wait_value, bool is_write);

bool
agx_surface_lookup(Agx_Device device, IOSurfaceRef surface);

IOSurfaceRef
agx_swapchain_create_image(uint32_t width, uint32_t height, Agx_Swapchain_Format format);
