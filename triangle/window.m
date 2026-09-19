#import <Cocoa/Cocoa.h>
#import <QuartzCore/QuartzCore.h>

#include "window.h"

#include <stdlib.h>
#include <string.h>

@interface WindowView : NSView
@end

@implementation WindowView
- (CALayer*)makeBackingLayer
{
  return [CALayer layer];
}

- (instancetype)initWithFrame:(NSRect)frameRect
{
  self = [super initWithFrame:frameRect];
  if (!self)
  {
    return nil;
  }

  self.layer = [CALayer layer];
  self.wantsLayer = YES;
  return self;
}
@end

bool
window_open(Window* window, uint32_t width, uint32_t height, const char* title)
{
  memset(window, 0, sizeof(*window));
  window->width = width;
  window->height = height;

  [NSApplication sharedApplication];

  [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];

  NSRect    frame = NSMakeRect(0, 0, width, height);
  NSWindow* handle = [[NSWindow alloc]
    initWithContentRect:frame
              styleMask:(NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskMiniaturizable)
                backing:NSBackingStoreBuffered
                  defer:NO];
  [handle setTitle:@(title)];

  WindowView* view = [[WindowView alloc] initWithFrame:frame];
  [handle setContentView:view];
  if (!view.layer)
  {
    fprintf(stderr, "window: view has no backing layer\n");
    return false;
  }

  view.layer.contentsGravity = kCAGravityResizeAspect;
  view.layer.magnificationFilter = kCAFilterNearest;

  [handle center];
  [handle makeKeyAndOrderFront:nil];
  [NSApp activateIgnoringOtherApps:YES];

  window->window = [handle retain];
  window->layer = [view.layer retain];

  fprintf(stderr, "window: %ux%u, a plain CALayer -- no Metal in this path\n", width, height);
  return true;
}

void
window_close(Window* window)
{
  if (window->window)
  {
    NSWindow* handle = (NSWindow*)window->window;
    [handle close];
    [handle release];
  }
  if (window->layer)
  {
    [(CALayer*)window->layer release];
  }
  memset(window, 0, sizeof(*window));
}

bool
window_poll(Window* window)
{
  for (;;)
  {
    NSEvent* event = [NSApp nextEventMatchingMask:NSEventMaskAny
                                        untilDate:[NSDate distantPast]
                                           inMode:NSDefaultRunLoopMode
                                          dequeue:YES];
    if (!event)
    {
      break;
    }
    [NSApp sendEvent:event];
  }

  NSWindow* handle = (NSWindow*)window->window;
  return handle != nil && handle.isVisible;
}

void
window_show_image(Window* window, void* image)
{
  CALayer* layer = (CALayer*)window->layer;
  if (!layer || !image)
  {
    return;
  }

  [CATransaction begin];
  [CATransaction setDisableActions:YES];
  layer.contents = (id)image;
  [CATransaction commit];
}
