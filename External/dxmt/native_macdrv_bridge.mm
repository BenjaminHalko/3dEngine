// Native macdrv bridge for DXMT
// Provides the macdrv_functions structure that DXMT expects via dlsym to create Metal views
// This bridges GLFW/NSWindow to Metal layer creation in native (non-Wine) builds

#import <Cocoa/Cocoa.h>
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>
#include <unordered_map>
#include <mutex>

// Opaque types matching DXMT's expectations
typedef struct macdrv_opaque_metal_device *macdrv_metal_device;
typedef struct macdrv_opaque_metal_view *macdrv_metal_view;
typedef struct macdrv_opaque_metal_layer *macdrv_metal_layer;
typedef struct macdrv_opaque_view *macdrv_view;
typedef struct macdrv_opaque_window *macdrv_window;
typedef void *HWND;

// Structure that DXMT expects from get_win_data
struct macdrv_win_data {
    HWND hwnd;
    macdrv_window cocoa_window;
    macdrv_view cocoa_view;
    macdrv_view client_cocoa_view;  // This is the key - the view to attach Metal to
};

// Global state
static std::mutex g_mutex;
static std::unordered_map<HWND, macdrv_win_data*> g_windowData;

extern "C" {

// Get or create win_data for an HWND (which is really an NSWindow* for GLFW)
__attribute__((visibility("default")))
struct macdrv_win_data* get_win_data(HWND hwnd) {
    std::lock_guard<std::mutex> lock(g_mutex);

    auto it = g_windowData.find(hwnd);
    if (it != g_windowData.end()) {
        return it->second;
    }

    // hwnd is actually an NSWindow* when using GLFW on macOS
    NSWindow* nsWindow = (__bridge NSWindow*)hwnd;
    if (!nsWindow || ![nsWindow isKindOfClass:[NSWindow class]]) {
        fprintf(stderr, "native_macdrv_bridge: Invalid HWND (not an NSWindow): %p\n", hwnd);
        return nullptr;
    }

    NSView* contentView = [nsWindow contentView];

    // Create new win_data
    macdrv_win_data* data = new macdrv_win_data();
    data->hwnd = hwnd;
    data->cocoa_window = (macdrv_window)hwnd;
    data->cocoa_view = (macdrv_view)(__bridge void*)contentView;
    data->client_cocoa_view = (macdrv_view)(__bridge void*)contentView;

    g_windowData[hwnd] = data;

    fprintf(stderr, "native_macdrv_bridge: Created win_data for HWND %p (NSWindow: %s)\n",
            hwnd, [[nsWindow title] UTF8String] ?: "untitled");

    return data;
}

// Release win_data (we don't actually free it, just mark it can be reused)
__attribute__((visibility("default")))
void release_win_data(struct macdrv_win_data* data) {
    // For now, we keep the data around - DXMT may call get_win_data again
}

// Create a Metal view for a Cocoa view
// v = macdrv_view (which is an NSView*)
// d = macdrv_metal_device (which is an id<MTLDevice>)
// Returns: macdrv_metal_view (which is the Metal layer or a wrapper)
__attribute__((visibility("default")))
macdrv_metal_view macdrv_view_create_metal_view(macdrv_view v, macdrv_metal_device d) {
    if (!v) {
        fprintf(stderr, "native_macdrv_bridge: macdrv_view_create_metal_view called with null view\n");
        return nullptr;
    }

    NSView* view = (__bridge NSView*)v;
    id<MTLDevice> device = (__bridge id<MTLDevice>)d;

    if (!device) {
        device = MTLCreateSystemDefaultDevice();
    }

    // Make the view layer-backed if it isn't already
    if (![view wantsLayer]) {
        [view setWantsLayer:YES];
    }

    // Check if there's already a Metal layer
    CAMetalLayer* metalLayer = nil;
    if ([view.layer isKindOfClass:[CAMetalLayer class]]) {
        metalLayer = (CAMetalLayer*)view.layer;
    } else {
        // Create a new Metal layer
        metalLayer = [CAMetalLayer layer];
        metalLayer.device = device;
        metalLayer.pixelFormat = MTLPixelFormatBGRA8Unorm;
        metalLayer.framebufferOnly = YES;
        metalLayer.contentsScale = view.window.backingScaleFactor;

        // Set up autoresizing
        metalLayer.frame = view.bounds;
        metalLayer.needsDisplayOnBoundsChange = YES;

        [view setLayer:metalLayer];
    }

    fprintf(stderr, "native_macdrv_bridge: Created Metal view for NSView %p, layer %p, device %s\n",
            v, metalLayer, [[device name] UTF8String]);

    // Return the layer as the "metal view" - DXMT will call get_metal_layer on this
    return (macdrv_metal_view)(__bridge_retained void*)metalLayer;
}

// Get the Metal layer from a metal view
__attribute__((visibility("default")))
macdrv_metal_layer macdrv_view_get_metal_layer(macdrv_metal_view v) {
    if (!v) return nullptr;

    // v is actually a CAMetalLayer* that we returned from create_metal_view
    CAMetalLayer* layer = (__bridge CAMetalLayer*)v;

    if (![layer isKindOfClass:[CAMetalLayer class]]) {
        fprintf(stderr, "native_macdrv_bridge: get_metal_layer - not a CAMetalLayer: %p\n", v);
        return nullptr;
    }

    return (macdrv_metal_layer)v;
}

// Release a metal view
__attribute__((visibility("default")))
void macdrv_view_release_metal_view(macdrv_metal_view v) {
    if (!v) return;

    // Release the retained reference from create_metal_view
    CAMetalLayer* layer = (__bridge_transfer CAMetalLayer*)v;
    (void)layer; // Let ARC handle the release

    fprintf(stderr, "native_macdrv_bridge: Released Metal view %p\n", v);
}

// Additional functions that might be needed (stubs for now)
__attribute__((visibility("default")))
void macdrv_init_display_devices(int force) {
    fprintf(stderr, "native_macdrv_bridge: macdrv_init_display_devices called (stub)\n");
}

__attribute__((visibility("default")))
macdrv_window macdrv_get_cocoa_window(HWND hwnd, int require_on_screen) {
    return (macdrv_window)hwnd;
}

__attribute__((visibility("default")))
macdrv_metal_device macdrv_create_metal_device(void) {
    id<MTLDevice> device = MTLCreateSystemDefaultDevice();
    return (macdrv_metal_device)(__bridge_retained void*)device;
}

__attribute__((visibility("default")))
void macdrv_release_metal_device(macdrv_metal_device d) {
    if (d) {
        id<MTLDevice> device = (__bridge_transfer id<MTLDevice>)d;
        (void)device;
    }
}

__attribute__((visibility("default")))
void on_main_thread(void (^block)(void)) {
    if ([NSThread isMainThread]) {
        block();
    } else {
        dispatch_sync(dispatch_get_main_queue(), block);
    }
}

// The macdrv_functions structure that DXMT looks for via dlsym
struct macdrv_functions_t {
    void (*macdrv_init_display_devices)(int);
    struct macdrv_win_data* (*get_win_data)(HWND hwnd);
    void (*release_win_data)(struct macdrv_win_data* data);
    macdrv_window (*macdrv_get_cocoa_window)(HWND hwnd, int require_on_screen);
    macdrv_metal_device (*macdrv_create_metal_device)(void);
    void (*macdrv_release_metal_device)(macdrv_metal_device d);
    macdrv_metal_view (*macdrv_view_create_metal_view)(macdrv_view v, macdrv_metal_device d);
    macdrv_metal_layer (*macdrv_view_get_metal_layer)(macdrv_metal_view v);
    void (*macdrv_view_release_metal_view)(macdrv_metal_view v);
    void (*on_main_thread)(void (^block)(void));
};

__attribute__((visibility("default")))
struct macdrv_functions_t macdrv_functions = {
    .macdrv_init_display_devices = macdrv_init_display_devices,
    .get_win_data = get_win_data,
    .release_win_data = release_win_data,
    .macdrv_get_cocoa_window = macdrv_get_cocoa_window,
    .macdrv_create_metal_device = macdrv_create_metal_device,
    .macdrv_release_metal_device = macdrv_release_metal_device,
    .macdrv_view_create_metal_view = macdrv_view_create_metal_view,
    .macdrv_view_get_metal_layer = macdrv_view_get_metal_layer,
    .macdrv_view_release_metal_view = macdrv_view_release_metal_view,
    .on_main_thread = on_main_thread,
};

} // extern "C"
