#include <stdio.h>
#import <Cocoa/Cocoa.h>
#import <IOSurface/IOSurface.h>
#import <QuartzCore/QuartzCore.h>

#include "agx_swapchain.h"

#include <IOKit/IOKitLib.h>
#include <stdlib.h>
#include <string.h>

static uint32_t
agx_swapchain_format_fourcc(Agx_Swapchain_Format format)
{
  if (format == AGX_SWAPCHAIN_FORMAT_RGBA16_FLOAT)
  {
    return 0x26526841u;
  }
  return 0x26424741u;
}

static uint32_t
agx_swapchain_format_tile_bytes(Agx_Swapchain_Format format)
{
  return format == AGX_SWAPCHAIN_FORMAT_RGBA16_FLOAT ? 2048u : 1024u;
}

bool
agx_swapchain_format_is_compressed(Agx_Swapchain_Format format)
{
  return format != AGX_SWAPCHAIN_FORMAT_RGBA8_UNORM && format != AGX_SWAPCHAIN_FORMAT_RGB10A2_UNORM;
}

static IOSurfaceRef
agx_swapchain_create_linear_image(uint32_t width, uint32_t height, Agx_Swapchain_Format format)
{
  size_t        bytes_per_row = (((size_t)width * 4u) + 255u) & ~(size_t)255u;
  uint32_t      fourcc = format == AGX_SWAPCHAIN_FORMAT_RGB10A2_UNORM ? 0x5231306bu : 0x52474241u;
  NSDictionary* props = @{
    (id)kIOSurfaceWidth : @(width),
    (id)kIOSurfaceHeight : @(height),
    (id)kIOSurfaceBytesPerElement : @4,
    (id)kIOSurfaceBytesPerRow : @(bytes_per_row),
    (id)kIOSurfacePixelFormat : @(fourcc),
    (id)kIOSurfacePixelSizeCastingAllowed : @NO,
    @"IOSurfaceCacheMode" : @1792,
    @"IOSurfaceMapCacheAttribute" : @0,
    @"IsDisplayable" : @1,
  };
  IOSurfaceRef surface = IOSurfaceCreate((CFDictionaryRef)props);

  if (!surface)
  {
    fprintf(stderr, "agx_swapchain: IOSurfaceCreate failed for linear %ux%u\n", width, height);
    return NULL;
  }
  IOSurfaceSetValue(surface, CFSTR("IOSurfaceColorPrimaries"), CFSTR("ITU_R_709_2"));
  IOSurfaceSetValue(surface, CFSTR("IOSurfaceTransferFunction"), CFSTR("IEC_sRGB"));
  IOSurfaceSetValue(surface, CFSTR("IOSurfaceColorSpace"), (CFTypeRef) @15);
  return surface;
}

IOSurfaceRef
agx_swapchain_create_image(uint32_t width, uint32_t height, Agx_Swapchain_Format format)
{
  if (!agx_swapchain_format_is_compressed(format))
  {
    return agx_swapchain_create_linear_image(width, height, format);
  }

  unsigned tiles_x = ((unsigned)width + 15) / 16;
  unsigned tiles_y = ((unsigned)height + 15) / 16;
  unsigned pow2_x = 1, pow2_y = 1;
  while (pow2_x < tiles_x)
  {
    pow2_x *= 2;
  }
  while (pow2_y < tiles_y)
  {
    pow2_y *= 2;
  }

  size_t tile_bytes = agx_swapchain_format_tile_bytes(format);
  size_t tile_data = (size_t)tiles_x * tiles_y * tile_bytes;
  size_t header_region = (size_t)pow2_x * pow2_y * 8;
  size_t tiled_bytes = tile_data + header_region;

  NSDictionary* plane = @{
    @"IOSurfaceAddressFormat" : @5,
    @"IOSurfacePlaneBytesPerCompressedTileHeader" : @8,
    @"IOSurfacePlaneBytesPerElement" : @(tile_bytes),
    @"IOSurfacePlaneBytesPerRow" : @(tiles_x * tile_bytes),
    @"IOSurfacePlaneBytesPerRowOfTileData" : @(tiles_x * tile_bytes),
    @"IOSurfacePlaneBytesPerTileData" : @(tile_bytes),
    @"IOSurfacePlaneCompressedTileDataRegionOffset" : @0,
    @"IOSurfacePlaneCompressedTileHeaderRegionOffset" : @(tile_data),
    @"IOSurfacePlaneCompressedTileHeight" : @16,
    @"IOSurfacePlaneCompressedTileWidth" : @16,
    @"IOSurfacePlaneCompressionFootprint" : @0,
    @"IOSurfacePlaneCompressionType" : @3,
    @"IOSurfacePlaneElementHeight" : @16,
    @"IOSurfacePlaneElementWidth" : @16,
    @"IOSurfacePlaneHeight" : @(height),
    @"IOSurfacePlaneHeightInCompressedTiles" : @(tiles_y),
    @"IOSurfacePlaneOffset" : @0,
    @"IOSurfacePlaneSize" : @(tiled_bytes),
    @"IOSurfacePlaneWidth" : @(width),
    @"IOSurfacePlaneWidthInCompressedTiles" : @(tiles_x),
  };
  NSDictionary* props = @{
    (id)kIOSurfaceWidth : @(width),
    (id)kIOSurfaceHeight : @(height),
    (id)kIOSurfacePixelFormat : @(agx_swapchain_format_fourcc(format)),
    (id)kIOSurfaceAllocSize : @(tiled_bytes),
    (id)kIOSurfacePixelSizeCastingAllowed : @NO,
    @"IOSurfaceCacheMode" : @1792,
    @"IOSurfaceMapCacheAttribute" : @0,
    @"IOSurfacePlaneInfo" : @[ plane ],
    @"IsDisplayable" : @1,
  };

  IOSurfaceRef surface = IOSurfaceCreate((CFDictionaryRef)props);
  if (!surface)
  {
    fprintf(stderr, "agx_swapchain: IOSurfaceCreate failed for %ux%u\n", width, height);
    return NULL;
  }

  IOSurfaceSetValue(surface, CFSTR("IOSurfaceColorPrimaries"), CFSTR("ITU_R_709_2"));
  IOSurfaceSetValue(surface, CFSTR("IOSurfaceTransferFunction"), CFSTR("IEC_sRGB"));
  IOSurfaceSetValue(surface, CFSTR("IOSurfaceColorSpace"), (CFTypeRef) @15);
  return surface;
}

bool
agx_swapchain_create(Agx_Swapchain* swapchain, void* layer, uint32_t width, uint32_t height, uint32_t image_count, Agx_Swapchain_Format format)
{
  memset(swapchain, 0, sizeof(*swapchain));
  if (!layer || image_count < 2 || image_count > AGX_SWAPCHAIN_MAX_IMAGES)
  {
    fprintf(
      stderr, "agx_swapchain: %u images requested, 2 to %u supported, and a layer is required\n", image_count, AGX_SWAPCHAIN_MAX_IMAGES
    );
    return false;
  }

  swapchain->layer = [(CALayer*)layer retain];
  swapchain->image_count = image_count;
  swapchain->width = width;
  swapchain->height = height;
  swapchain->format = format;
  swapchain->index = image_count - 1;

  for (uint32_t i = 0; i < image_count; ++i)
  {
    swapchain->images[i] = agx_swapchain_create_image(width, height, format);
    if (!swapchain->images[i])
    {
      agx_swapchain_destroy(swapchain);
      return false;
    }
  }
  return true;
}

void
agx_swapchain_destroy(Agx_Swapchain* swapchain)
{
  for (uint32_t i = 0; i < swapchain->image_count; ++i)
  {
    if (swapchain->images[i])
    {
      CFRelease(swapchain->images[i]);
    }
  }
  if (swapchain->layer)
  {
    [(CALayer*)swapchain->layer release];
  }
  memset(swapchain, 0, sizeof(*swapchain));
}

IOSurfaceRef
agx_swapchain_acquire(Agx_Swapchain* swapchain, uint32_t* index)
{
  swapchain->index = (swapchain->index + 1) % swapchain->image_count;
  IOSurfaceRef image = swapchain->images[swapchain->index];

  NSDate* deadline = [NSDate dateWithTimeIntervalSinceNow:0.1];
  while (IOSurfaceIsInUse(image) && [deadline timeIntervalSinceNow] > 0)
  {
    [[NSRunLoop currentRunLoop] runMode:NSDefaultRunLoopMode beforeDate:[NSDate dateWithTimeIntervalSinceNow:0.001]];
  }

  if (IOSurfaceIsInUse(image))
  {
    swapchain->acquire_timeouts++;
    fprintf(stderr, "agx_swapchain_acquire: image %u still in use after 100 ms; drawing into it anyway\n", swapchain->index);
  }

  if (index)
  {
    *index = swapchain->index;
  }
  return image;
}

bool
agx_surface_lookup(Agx_Device device, IOSurfaceRef surface)
{
  if (!surface)
  {
    return false;
  }

  uint64_t id = IOSurfaceGetID(surface);

  uint8_t       description[3176];
  size_t        description_size = sizeof(description);
  kern_return_t ret = IOConnectCallMethod(
    device.iosrf, IOSURFACE_SELECTOR_LOOKUP_SURFACE, &id, 1, NULL, 0, NULL, NULL, description, &description_size
  );
  if (ret != KERN_SUCCESS)
  {
    fprintf(stderr, "agx_surface_lookup: LOOKUP_SURFACE(%llu) failed, kern_return_t=0x%x\n", id, ret);
    return false;
  }
  return true;
}

bool
agx_surface_append_transaction(IOSurfaceRef surface, Agx_Fence fence, uint64_t wait_value, bool is_write)
{
  if (!surface)
  {
    return false;
  }

  kern_return_t ret = IOConnectTrap4(
    agx_trap_connection(fence.iosrf),
    IOSURFACE_SELECTOR_TRAP4_APPEND_TRANSACTION,
    (uint64_t)IOSurfaceGetID(surface),
    (uint64_t)fence.id,
    wait_value,
    is_write ? 1 : 0
  );
  if (ret != KERN_SUCCESS)
  {
    fprintf(
      stderr,
      "agx_surface_append_transaction: APPEND_TRANSACTION(surface=%u, event=0x%llx, value=%llu) failed, "
      "kern_return_t=0x%x\n",
      IOSurfaceGetID(surface),
      fence.id,
      wait_value,
      ret
    );
    return false;
  }
  return true;
}

void
agx_swapchain_present(Agx_Swapchain* swapchain)
{
  CALayer* layer = (CALayer*)swapchain->layer;
  if (!layer)
  {
    return;
  }

  [CATransaction begin];
  [CATransaction setDisableActions:YES];
  layer.contents = (id)swapchain->images[swapchain->index];
  [CATransaction commit];
}
