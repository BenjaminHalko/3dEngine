// MetalBridge.mm - Provides macdrv symbols for DXMT native builds
#if defined(__APPLE__)

#import <Cocoa/Cocoa.h>
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>

// Opaque types matching DXMT expectations
typedef void* macdrv_metal_device;
typedef void* macdrv_metal_view;
typedef void* macdrv_metal_layer;
typedef void* macdrv_view;

struct macdrv_win_data {
    void* hwnd;
    void* cocoa_window;
    void* cocoa_view;
    void* client_cocoa_view;  // This is what DXMT uses
};

// Storage for win_data - simple approach for single window
static struct macdrv_win_data g_win_data;
static CAMetalLayer* g_metal_layer = nil;
static NSView* g_metal_view = nil;

extern "C" {

__attribute__((visibility("default")))
struct macdrv_win_data* get_win_data(void* hwnd) {
    // hwnd is actually NSWindow* from SDL3
    NSWindow* window = (__bridge NSWindow*)hwnd;
    g_win_data.hwnd = hwnd;
    g_win_data.cocoa_window = hwnd;
    g_win_data.cocoa_view = (__bridge void*)[window contentView];
    g_win_data.client_cocoa_view = g_win_data.cocoa_view;
    return &g_win_data;
}

__attribute__((visibility("default")))
void release_win_data(struct macdrv_win_data* data) {
    // Nothing to release in our simple implementation
}

__attribute__((visibility("default")))
macdrv_metal_view macdrv_view_create_metal_view(macdrv_view v, macdrv_metal_device d) {
    NSView* contentView = (__bridge NSView*)v;
    id<MTLDevice> device = (__bridge id<MTLDevice>)d;

    if (!contentView || !device) {
        return nil;
    }

    // Create and configure CAMetalLayer
    g_metal_layer = [[CAMetalLayer alloc] init];
    g_metal_layer.device = device;
    g_metal_layer.pixelFormat = MTLPixelFormatBGRA8Unorm;
    g_metal_layer.framebufferOnly = NO;

    NSWindow* window = [contentView window];
    if (window) {
        g_metal_layer.contentsScale = [window backingScaleFactor];
    }

    // Make view layer-backed and set our Metal layer
    [contentView setWantsLayer:YES];
    [contentView setLayer:g_metal_layer];

    // Set drawable size
    NSSize viewSize = [contentView bounds].size;
    CGFloat scale = window ? [window backingScaleFactor] : 1.0;
    g_metal_layer.drawableSize = CGSizeMake(viewSize.width * scale, viewSize.height * scale);

    g_metal_view = contentView;
    return (__bridge macdrv_metal_view)contentView;
}

__attribute__((visibility("default")))
macdrv_metal_layer macdrv_view_get_metal_layer(macdrv_metal_view v) {
    return (__bridge macdrv_metal_layer)g_metal_layer;
}

__attribute__((visibility("default")))
void macdrv_view_release_metal_view(macdrv_metal_view v) {
    g_metal_layer = nil;
    g_metal_view = nil;
}

} // extern "C"

#endif // __APPLE__
