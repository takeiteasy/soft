/* window.c
 *
 * Created by George Watson on 26/11/2017.
 * Copyright � 2017-2019 George Watson. All rights reserved.

 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * *   Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * *   Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * *   Neither the name of the <organization> nor the
 *     names of its contributors may be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL GEORGE WATSON BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "window-private.c"
#include <Cocoa/Cocoa.h>
#include <mach/mach_time.h>

unsigned long long TimerTicks(void) {
    return mach_absolute_time();
}

static bool timerInit = false;
unsigned long long TimerFrequency(void) {
    static long long freq = 0; 
    if (!timerInit) {
        mach_timebase_info_data_t info;
        mach_timebase_info(&info);
        freq= (info.denom * 1e9) / info.numer;
        timerInit = true;
    }
    return freq; 
}

#if MAC_OS_X_VERSION_MAX_ALLOWED < MAC_OS_X_VERSION_10_12
#define NSWindowStyleMaskBorderless NSBorderlessWindowMask
#define NSWindowStyleMaskClosable NSClosableWindowMask
#define NSWindowStyleMaskMiniaturizable NSMiniaturizableWindowMask
#define NSWindowStyleMaskResizable NSResizableWindowMask
#define NSWindowStyleMaskTitled NSTitledWindowMask
#define NSEventModifierFlagCommand NSCommandKeyMask
#define NSEventModifierFlagControl NSControlKeyMask
#define NSEventModifierFlagOption NSAlternateKeyMask
#define NSEventModifierFlagShift NSShiftKeyMask
#define NSEventModifierFlagDeviceIndependentFlagsMask NSDeviceIndependentModifierFlagsMask
#define NSEventMaskAny NSAnyEventMask
#define NSEventTypeApplicationDefined NSApplicationDefined
#define NSEventTypeKeyUp NSKeyUp
#define NSBitmapFormatAlphaNonpremultiplied NSAlphaNonpremultipliedBitmapFormat
#define CGContext graphicsPort
#endif

static inline int translate_mod(NSUInteger flags) {
  int mods = 0;
  
  if (flags & NSEventModifierFlagShift)
    mods |= KB_MOD_SHIFT;
  if (flags & NSEventModifierFlagControl)
    mods |= KB_MOD_CONTROL;
  if (flags & NSEventModifierFlagOption)
    mods |= KB_MOD_ALT;
  if (flags & NSEventModifierFlagCommand)
    mods |= KB_MOD_SUPER;
  if (flags & NSEventModifierFlagCapsLock)
    mods |= KB_MOD_CAPS_LOCK;
  
  return mods;
}

static inline int translate_key(unsigned int key) {
  return (key >= sizeof(keycodes) / sizeof(keycodes[0]) ?  KB_KEY_UNKNOWN : keycodes[key]);
}

static inline NSImage *create_cocoa_image(Surface *s) {
  NSImage *nsi = [[[NSImage alloc] initWithSize:NSMakeSize(s->w, s->h)] autorelease];
  if (!nsi)
    return nil;
  
  NSBitmapImageRep *nsbir = [[[NSBitmapImageRep alloc] initWithBitmapDataPlanes:NULL
                                                                     pixelsWide:s->w
                                                                     pixelsHigh:s->h
                                                                  bitsPerSample:8
                                                                samplesPerPixel:4
                                                                       hasAlpha:YES
                                                                       isPlanar:NO
                                                                 colorSpaceName:NSDeviceRGBColorSpace
                                                                   bitmapFormat:NSBitmapFormatAlphaNonpremultiplied
                                                                    bytesPerRow:0
                                                                   bitsPerPixel:0] autorelease];
  if (!nsbir)
    return nil;
  
  char *rgba = WINDOW_MALLOC(s->w * s->h * 4);
  int offset = 0, c;
  for(int i = 0; i < s->h; ++i) {
    for (int j = 0; j < s->w; j++) {
      c = GetPixel(s, j, i);
      rgba[4 * offset]     = r_channel(c);
      rgba[4 * offset + 1] = g_channel(c);
      rgba[4 * offset + 2] = b_channel(c);
      rgba[4 * offset + 3] = a_channel(c);
      offset++;
    }
  }
  memcpy([nsbir bitmapData], rgba, s->w * s->h * sizeof(char) * 4);
  WINDOW_SAFE_FREE(rgba);
  
  [nsi addRepresentation:nsbir];
  return nsi;
}

@protocol AppViewDelegate;

@interface AppView : NSView
@property (nonatomic, strong) id<AppViewDelegate> delegate;
@property (strong) NSTrackingArea *track;
@property (atomic) Surface *buffer;
@property BOOL mouse_in_window;
@property (nonatomic, strong) NSCursor *cursor;
@property BOOL custom_cursor;
@property BOOL cursor_vis;
@end

@implementation AppView
@synthesize delegate = _delegate;
@synthesize track = _track;
@synthesize buffer = _buffer;
@synthesize mouse_in_window = _mouse_in_window;
@synthesize cursor = _cursor;
@synthesize custom_cursor = _custom_cursor;
@synthesize cursor_vis = _cursor_vis;

-(id)initWithFrame:(NSRect)frameRect {
  _mouse_in_window = NO;
  _cursor = [NSCursor arrowCursor];
  _custom_cursor = NO;
  _cursor_vis = YES;
  
  self = [super initWithFrame:frameRect];
  [self updateTrackingAreas];
  return self;
}

-(void)updateTrackingAreas {
  if (_track) {
    [self removeTrackingArea:_track];
    [_track release];
  }
  _track = [[NSTrackingArea alloc] initWithRect:[self visibleRect]
                                        options:NSTrackingMouseEnteredAndExited
                                               |NSTrackingActiveInKeyWindow
                                               |NSTrackingEnabledDuringMouseDrag
                                               |NSTrackingCursorUpdate
                                               |NSTrackingInVisibleRect
                                               |NSTrackingAssumeInside
                                               |NSTrackingActiveInActiveApp
                                               |NSTrackingActiveAlways
                                               |NSTrackingMouseMoved
                                          owner:self
                                       userInfo:nil];
  [self addTrackingArea:_track];
  [super updateTrackingAreas];
}

-(BOOL)acceptsFirstResponder {
  return YES;
}

-(BOOL)performKeyEquivalent:(NSEvent*)event {
  return YES;
}

-(void)resetCursorRects {
  [super resetCursorRects];
  [self addCursorRect:[self visibleRect] cursor:(_cursor ? _cursor : [NSCursor arrowCursor])];
}

-(void)setCustomCursor:(NSImage*)img {
  if (!img) {
    if (_custom_cursor && _cursor)
      [_cursor release];
    _cursor = [NSCursor arrowCursor];
    return;
  }
  if (_custom_cursor && _cursor)
    [_cursor release];
  _custom_cursor = YES;
  _cursor = [[NSCursor alloc] initWithImage:img hotSpot:NSMakePoint(0.f, 0.f)];
  [_cursor retain];
}

-(void)setRegularCursor:(Cursor)type {
  NSCursor *tmp = nil;
  switch (type) {
    default:
    case CURSOR_ARROW:
    case CURSOR_WAIT:
    case CURSOR_WAITARROW:
      tmp = [NSCursor arrowCursor];
      break;
    case CURSOR_IBEAM:
      tmp = [NSCursor IBeamCursor];
      break;
    case CURSOR_CROSSHAIR:
      tmp = [NSCursor crosshairCursor];
      break;
    case CURSOR_SIZENWSE:
    case CURSOR_SIZENESW:
      tmp = [NSCursor closedHandCursor];
      break;
    case CURSOR_SIZEWE:
      tmp = [NSCursor resizeLeftRightCursor];
      break;
    case CURSOR_SIZENS:
      tmp = [NSCursor resizeUpDownCursor];
      break;
    case CURSOR_SIZEALL:
      tmp = [NSCursor closedHandCursor];
      break;
    case CURSOR_NO:
      tmp = [NSCursor operationNotAllowedCursor];
      break;
    case CURSOR_HAND:
      tmp = [NSCursor pointingHandCursor];
      break;
  }
  if (_custom_cursor && _cursor)
    [_cursor release];
  _custom_cursor = NO;
  _cursor = tmp;
  [_cursor retain];
}

-(void)setCursorVisibility:(BOOL)visibility {
  _cursor_vis = visibility;
}

-(void)mouseEntered:(NSEvent*)event {
  _mouse_in_window = YES;
  if (!_cursor_vis)
    [NSCursor hide];
}

-(void)mouseExited:(NSEvent*)event {
  _mouse_in_window = NO;
  if (!_cursor_vis)
    [NSCursor unhide];
}

-(void)mouseMoved:(NSEvent*)event {
  if (_cursor && _mouse_in_window)
    [_cursor set];
}

-(BOOL)preservesContentDuringLiveResize {
  return NO;
}

-(void)drawRect:(NSRect)dirtyRect {
  if (!_buffer)
    return;
  
  CGContextRef ctx = (CGContextRef)[[NSGraphicsContext currentContext] CGContext];
  CGColorSpaceRef s = CGColorSpaceCreateDeviceRGB();
  CGDataProviderRef p = CGDataProviderCreateWithData(NULL, _buffer->buf, _buffer->w * _buffer->h * 4, NULL);
  CGContextSetInterpolationQuality(ctx, kCGInterpolationNone);
  CGImageRef img = CGImageCreate(_buffer->w, _buffer->h, 8, 32, _buffer->w * 4, s, kCGImageAlphaNoneSkipFirst | kCGBitmapByteOrder32Little, p, NULL, 0, kCGRenderingIntentDefault);
  /* This line causes Visual Studio to crash if uncommented. I don't know why and I don't want to know.
   Not the whole line though, just the `[self frame]` parts. This has caused me an issue for over a month.
   `CGContextDrawImage(ctx, CGRectMake(0, 0, [self frame].size.width, [self frame].size.height), img);` */
  CGSize wh = [self frame].size;
  CGContextDrawImage(ctx, CGRectMake(0, 0, wh.width, wh.height), img);
  CGColorSpaceRelease(s);
  CGDataProviderRelease(p);
  CGImageRelease(img);
}

-(void)dealloc {
  [_track release];
  if (_custom_cursor && _cursor)
    [_cursor release];
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wobjc-missing-super-calls"
}
#pragma clang diagnostic pop
@end

@protocol AppViewDelegate <NSObject>
@end

@interface AppDelegate : NSObject <NSApplicationDelegate, NSWindowDelegate, AppViewDelegate>
@property (unsafe_unretained) NSWindow *window;
@property (unsafe_unretained) AppView *view;
@property (nonatomic) Window *parent;
@property BOOL closed;
@end

struct osx_window_t {
  AppDelegate *delegate;
  NSInteger window_id;
};

LINKEDLIST(window, struct osx_window_t);
static struct window_node_t *windows = NULL;

@implementation AppDelegate
@synthesize window = _window;
@synthesize view = _view;
@synthesize parent = _parent;
@synthesize closed = _closed;

-(id)initWithSize:(NSSize)windowSize styleMask:(short)flags title:(const char*)windowTitle {
  NSWindowStyleMask styleMask = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskMiniaturizable;
  flags |= (flags & FULLSCREEN ? (BORDERLESS | RESIZABLE | FULLSCREEN_DESKTOP) : 0);
  styleMask |= (flags & RESIZABLE ? NSWindowStyleMaskResizable : 0);
  styleMask |= (flags & BORDERLESS ? NSWindowStyleMaskFullSizeContentView : 0);
  if (flags & FULLSCREEN_DESKTOP) {
    NSRect f = [[NSScreen mainScreen] frame];
    windowSize.width = f.size.width;
    windowSize.height = f.size.height;
    styleMask |= NSWindowStyleMaskFullSizeContentView;
  }
  NSRect frameRect = NSMakeRect(0, 0, windowSize.width, windowSize.height);
  
  _window = [[NSWindow alloc] initWithContentRect:frameRect
                                        styleMask:styleMask
                                          backing:NSBackingStoreBuffered
                                            defer:NO];
  if (!_window) {
    CloseAllWindows();
    WINDOW_ERROR(OSX_WINDOW_CREATION_FAILED, "[_window initWithContentRect] failed");
    return nil;
  }
  
  if (flags & ALWAYS_ON_TOP)
    [_window setLevel:NSFloatingWindowLevel];
  
#if MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_7
  if (flags & FULLSCREEN) {
    [_window setCollectionBehavior:NSWindowCollectionBehaviorFullScreenPrimary];
    [_window performSelectorOnMainThread: @selector(toggleFullScreen:) withObject:_window waitUntilDone:NO];
  }
#else
#pragma message WARN("Fullscreen is unsupported on OSX versions < 10.7")
#endif
  
  [_window setAcceptsMouseMovedEvents:YES];
  [_window setRestorable:NO];
  [_window setTitle:(windowTitle ? @(windowTitle) : [[NSProcessInfo processInfo] processName])];
  [_window setReleasedWhenClosed:NO];
  
  if (flags & BORDERLESS && flags & ~FULLSCREEN) {
    [_window setTitle:@""];
    [_window setTitlebarAppearsTransparent:YES];
    [[_window standardWindowButton:NSWindowZoomButton] setHidden:YES];
    [[_window standardWindowButton:NSWindowCloseButton] setHidden:YES];
    [[_window standardWindowButton:NSWindowMiniaturizeButton] setHidden:YES];
  }
  
  [_window center];
  _view = [[AppView alloc] initWithFrame:frameRect];
  if (!_view) {
    CloseAllWindows();
    WINDOW_ERROR(OSX_WINDOW_CREATION_FAILED, "[_view initWithFrame] failed");
    return nil;
  }
  [_view setDelegate:self];
  [_view setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];
  
  [_window setContentView:_view];
  [_window setDelegate:self];
  [_window performSelectorOnMainThread:@selector(makeKeyAndOrderFront:)
                            withObject:nil
                         waitUntilDone:YES];
  
  _closed = NO;
  return self;
}

-(void)setParent:(Window*)screen {
  _parent = screen;
}

-(void)windowWillClose:(NSNotification*)notification {
  _closed = YES;
  if (_parent->Closed_callback)
    _parent->Closed_callback(_parent->parent);
  [[self view] dealloc];
  
  struct window_node_t *head = windows, *cursor = windows, *prev = NULL;
  while (cursor) {
    if (cursor->data->delegate != self) {
      prev = cursor;
      cursor = cursor->next;
      continue;
    }
    
    if (!prev)
      head = cursor->next;
    else
      prev->next = cursor->next;
    break;
  }
  if (cursor) {
    cursor->next = NULL;
    WINDOW_FREE(cursor);
  }
  windows = head;
}

-(void)windowDidBecomeKey:(NSNotification*)notification {
  if (_parent->Focus_callback)
    _parent->Focus_callback(_parent->parent, true);
}

-(void)windowDidResignKey:(NSNotification*)notification {
  if (_parent->Focus_callback)
    _parent->Focus_callback(_parent->parent, false);
}

-(void)windowDidResize:(NSNotification*)notification {
  static CGSize size;
  size = [_view frame].size;
  _parent->w = (int)roundf(size.width);
  _parent->h = (int)roundf(size.height);
  if (_parent->Resize_callback)
  _parent->Resize_callback(_parent->Resize_callback, _parent->w, _parent->h);
}
@end

bool NewWindow(Window *s, const char *t, int w, int h, short flags) {
  if (!keycodes_init) {
    memset(keycodes,  -1, sizeof(keycodes));
    
    keycodes[0x1D] = KB_KEY_0;
    keycodes[0x12] = KB_KEY_1;
    keycodes[0x13] = KB_KEY_2;
    keycodes[0x14] = KB_KEY_3;
    keycodes[0x15] = KB_KEY_4;
    keycodes[0x17] = KB_KEY_5;
    keycodes[0x16] = KB_KEY_6;
    keycodes[0x1A] = KB_KEY_7;
    keycodes[0x1C] = KB_KEY_8;
    keycodes[0x19] = KB_KEY_9;
    keycodes[0x00] = KB_KEY_A;
    keycodes[0x0B] = KB_KEY_B;
    keycodes[0x08] = KB_KEY_C;
    keycodes[0x02] = KB_KEY_D;
    keycodes[0x0E] = KB_KEY_E;
    keycodes[0x03] = KB_KEY_F;
    keycodes[0x05] = KB_KEY_G;
    keycodes[0x04] = KB_KEY_H;
    keycodes[0x22] = KB_KEY_I;
    keycodes[0x26] = KB_KEY_J;
    keycodes[0x28] = KB_KEY_K;
    keycodes[0x25] = KB_KEY_L;
    keycodes[0x2E] = KB_KEY_M;
    keycodes[0x2D] = KB_KEY_N;
    keycodes[0x1F] = KB_KEY_O;
    keycodes[0x23] = KB_KEY_P;
    keycodes[0x0C] = KB_KEY_Q;
    keycodes[0x0F] = KB_KEY_R;
    keycodes[0x01] = KB_KEY_S;
    keycodes[0x11] = KB_KEY_T;
    keycodes[0x20] = KB_KEY_U;
    keycodes[0x09] = KB_KEY_V;
    keycodes[0x0D] = KB_KEY_W;
    keycodes[0x07] = KB_KEY_X;
    keycodes[0x10] = KB_KEY_Y;
    keycodes[0x06] = KB_KEY_Z;
    
    keycodes[0x27] = KB_KEY_APOSTROPHE;
    keycodes[0x2A] = KB_KEY_BACKSLASH;
    keycodes[0x2B] = KB_KEY_COMMA;
    keycodes[0x18] = KB_KEY_EQUALS;
    keycodes[0x32] = KB_KEY_GRAVE_ACCENT;
    keycodes[0x21] = KB_KEY_LEFT_BRACKET;
    keycodes[0x1B] = KB_KEY_MINUS;
    keycodes[0x2F] = KB_KEY_PERIOD;
    keycodes[0x1E] = KB_KEY_RIGHT_BRACKET;
    keycodes[0x29] = KB_KEY_SEMICOLON;
    keycodes[0x2C] = KB_KEY_SLASH;
    keycodes[0x0A] = KB_KEY_WORLD_1;
    
    keycodes[0x33] = KB_KEY_BACKSPACE;
    keycodes[0x39] = KB_KEY_CAPS_LOCK;
    keycodes[0x75] = KB_KEY_DELETE;
    keycodes[0x7D] = KB_KEY_DOWN;
    keycodes[0x77] = KB_KEY_END;
    keycodes[0x24] = KB_KEY_ENTER;
    keycodes[0x35] = KB_KEY_ESCAPE;
    keycodes[0x7A] = KB_KEY_F1;
    keycodes[0x78] = KB_KEY_F2;
    keycodes[0x63] = KB_KEY_F3;
    keycodes[0x76] = KB_KEY_F4;
    keycodes[0x60] = KB_KEY_F5;
    keycodes[0x61] = KB_KEY_F6;
    keycodes[0x62] = KB_KEY_F7;
    keycodes[0x64] = KB_KEY_F8;
    keycodes[0x65] = KB_KEY_F9;
    keycodes[0x6D] = KB_KEY_F10;
    keycodes[0x67] = KB_KEY_F11;
    keycodes[0x6F] = KB_KEY_F12;
    keycodes[0x69] = KB_KEY_F13;
    keycodes[0x6B] = KB_KEY_F14;
    keycodes[0x71] = KB_KEY_F15;
    keycodes[0x6A] = KB_KEY_F16;
    keycodes[0x40] = KB_KEY_F17;
    keycodes[0x4F] = KB_KEY_F18;
    keycodes[0x50] = KB_KEY_F19;
    keycodes[0x5A] = KB_KEY_F20;
    keycodes[0x73] = KB_KEY_HOME;
    keycodes[0x72] = KB_KEY_INSERT;
    keycodes[0x7B] = KB_KEY_LEFT;
    keycodes[0x3A] = KB_KEY_LEFT_ALT;
    keycodes[0x3B] = KB_KEY_LEFT_CONTROL;
    keycodes[0x38] = KB_KEY_LEFT_SHIFT;
    keycodes[0x37] = KB_KEY_LEFT_SUPER;
    keycodes[0x6E] = KB_KEY_MENU;
    keycodes[0x47] = KB_KEY_NUM_LOCK;
    keycodes[0x79] = KB_KEY_PAGE_DOWN;
    keycodes[0x74] = KB_KEY_PAGE_UP;
    keycodes[0x7C] = KB_KEY_RIGHT;
    keycodes[0x3D] = KB_KEY_RIGHT_ALT;
    keycodes[0x3E] = KB_KEY_RIGHT_CONTROL;
    keycodes[0x3C] = KB_KEY_RIGHT_SHIFT;
    keycodes[0x36] = KB_KEY_RIGHT_SUPER;
    keycodes[0x31] = KB_KEY_SPACE;
    keycodes[0x30] = KB_KEY_TAB;
    keycodes[0x7E] = KB_KEY_UP;
    
    keycodes[0x52] = KB_KEY_KP_0;
    keycodes[0x53] = KB_KEY_KP_1;
    keycodes[0x54] = KB_KEY_KP_2;
    keycodes[0x55] = KB_KEY_KP_3;
    keycodes[0x56] = KB_KEY_KP_4;
    keycodes[0x57] = KB_KEY_KP_5;
    keycodes[0x58] = KB_KEY_KP_6;
    keycodes[0x59] = KB_KEY_KP_7;
    keycodes[0x5B] = KB_KEY_KP_8;
    keycodes[0x5C] = KB_KEY_KP_9;
    keycodes[0x45] = KB_KEY_KP_ADD;
    keycodes[0x41] = KB_KEY_KP_DECIMAL;
    keycodes[0x4B] = KB_KEY_KP_DIVIDE;
    keycodes[0x4C] = KB_KEY_KP_ENTER;
    keycodes[0x51] = KB_KEY_KP_EQUALS;
    keycodes[0x43] = KB_KEY_KP_MULTIPLY;
    keycodes[0x4E] = KB_KEY_KP_SUBTRACT;
    keycodes_init = true;
  }
  
  NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
  [NSApplication sharedApplication];
  [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
  
  AppDelegate *app = [[AppDelegate alloc] initWithSize:NSMakeSize(w, h) styleMask:flags title:t];
  if (!app) {
    CloseAllWindows();
    WINDOW_ERROR(OSX_WINDOW_CREATION_FAILED, "[AppDelegate alloc] failed");
    return false;
  }
  
  struct osx_window_t *win_data = WINDOW_MALLOC(sizeof(struct osx_window_t));
  win_data->delegate = app;
  win_data->window_id = [[app window] windowNumber];
  windows = window_push(windows, win_data);
  
  memset(s, 0, sizeof(Window));
  s->id = (int)[[app window] windowNumber];
  s->w  = w;
  s->h  = h;
  s->window = (void*)app;
  [app setParent:s];
  
  [NSApp activateIgnoringOtherApps:YES];
  [pool drain];
  return true;
}

#define SET_DEFAULT_APP_ICON [NSApp setApplicationIconImage:[NSImage imageNamed:@"NSApplicationIcon"]]

void SetWindowIcon(Window *s, Surface *b) {
  if (!b || !b->buf) {
    SET_DEFAULT_APP_ICON;
    return;
  }
  
  NSImage *img = create_cocoa_image(b);
  if (!img)  {
    WINDOW_ERROR(WINDOW_ICON_FAILED, "window_icon_b() failed: Couldn't set window icon");
    SET_DEFAULT_APP_ICON;
    return;
  }
  [NSApp setApplicationIconImage:img];
}

void SetWindowTitle(Window *s, const char *t) {
  [[(AppDelegate*)s->window window] setTitle:@(t)];
}

void GetWindowPosition(Window *s, int *x, int * y) {
  static NSRect frame;
  frame = [[(AppDelegate*)s->window window] frame];
  if (x)
    *x = frame.origin.x;
  if (y)
    *y = frame.origin.y;
}

void GetScreenSize(Window *s, int *w, int *h) {
  static NSRect frame;
  frame = [[[(AppDelegate*)s->window window] screen] frame];
  if (w)
    *w = frame.size.width;
  if (h)
    *h = frame.size.height;
}


void DestroyWindow(Window *s) {
  NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
  AppDelegate *app = (AppDelegate*)s->window;
  [[app view] dealloc];
  [app dealloc];
  memset(s, 0, sizeof(Window));
  [pool drain];
}

bool IsWindowClosed(Window *s) {
  return (bool)[(AppDelegate*)s->window closed];
}

bool AreWindowsClosed(int n, ...) {
  va_list args;
  va_start(args, n);
  bool ret = true;
  for (int i = 0; i < n; ++i) {
    Window *w = va_arg(args, Window*);
    if (![(AppDelegate*)w->window closed]) {
      ret = false;
      break;
    }
  }
  va_end(args);
  return ret;
}

bool AreAllWindowsClosed() {
  return windows == NULL;
}

void ToggleCursorLock(Window *s, bool locked) {
  return;
}

void ToggleCursorVisiblity(Window *s, bool shown) {
  [[(AppDelegate*)s->window view] setCursorVisibility:shown];
}

void SetCursorIcon(Window *s, Cursor t) {
  AppDelegate *app = (AppDelegate*)s->window;
  if (!app) {
    WINDOW_ERROR(CURSOR_MOD_FAILED, "cursor_icon() failed: Invalid window");
    return;
  }
  [[app view] setRegularCursor:t];
}

void SetCustomCursorIcon(Window *s, Surface *b) {
  NSImage *img = create_cocoa_image(b);
  if (!img) {
    WINDOW_ERROR(CURSOR_MOD_FAILED, "cursor_icon_custom_buf() failed: Couldn't set cursor from buffer");
    return;
  }
  
  AppDelegate *app = (AppDelegate*)s->window;
  if (!app) {
    [img release];
    WINDOW_ERROR(CURSOR_MOD_FAILED, "cursor_icon_custom_buf() failed: Invalid window");
    return;
  }
  [[app view] setCustomCursor:img];
  [img release];
}

void GetCursorPosition(int *x, int *y) {
  static NSPoint _p = {0,0};
  _p = [NSEvent mouseLocation];
  if (x)
    *x = _p.x;
  if (y)
    *y = [NSScreen mainScreen].frame.size.height - _p.y;
}

void SetCursorPosition(int x, int y) {
  CGWarpMouseCursorPosition((CGPoint){ x, y });
}

Window *event_delegate(NSInteger window_id) {
  struct window_node_t *cursor = windows;
  while (cursor) {
    if (cursor->data->window_id == window_id)
      return [cursor->data->delegate parent];
    cursor = cursor->next;
  }
  return NULL;
}

void PollEvents() {
  NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
  NSEvent *e = nil;
  while ((e = [NSApp nextEventMatchingMask:NSEventMaskAny
                                 untilDate:[NSDate distantPast]
                                    inMode:NSDefaultRunLoopMode
                                   dequeue:YES])) {
    Window *e_window = event_delegate([e windowNumber]);
    if (!e_window) {
      [NSApp sendEvent:e];
      continue;
    }
    switch ([e type]) {
      case NSEventTypeKeyUp:
      case NSEventTypeKeyDown:
        CBCALL(Keyboard_callback, translate_key([e keyCode]), translate_mod([e modifierFlags]), ([e type] == NSEventTypeKeyDown));
        break;
      case NSEventTypeLeftMouseUp:
      case NSEventTypeRightMouseUp:
      case NSEventTypeOtherMouseUp:
        CBCALL(MouseButton_callback, (Button)([e buttonNumber] + 1), translate_mod([e modifierFlags]), false);
        break;
      case NSEventTypeLeftMouseDown:
      case NSEventTypeRightMouseDown:
      case NSEventTypeOtherMouseDown:
        CBCALL(MouseButton_callback, (Button)([e buttonNumber] + 1), translate_mod([e modifierFlags]), true);
        break;
      case NSEventTypeScrollWheel:
        CBCALL(Scroll_callback, translate_mod([e modifierFlags]), [e deltaX], [e deltaY]);
        break;
      case NSEventTypeLeftMouseDragged:
      case NSEventTypeRightMouseDragged:
      case NSEventTypeOtherMouseDragged:
        CBCALL(MouseButton_callback, (Button)([e buttonNumber] + 1), translate_mod([e modifierFlags]), true);
      case NSEventTypeMouseMoved: {
        AppDelegate *app = (AppDelegate*)e_window->window;
        if ([[app view] mouse_in_window])
          CBCALL(MouseMove_callback, [e locationInWindow].x, (int)([[app view] frame].size.height - roundf([e locationInWindow].y)), [e deltaX], [e deltaY]);
        break;
      }
      default:
        break;
    }
    [NSApp sendEvent:e];
  }
  [pool release];
}

void Flush(Window *s, Surface *b) {
  if (!s)
    return;
  AppDelegate *tmp = (AppDelegate*)s->window;
  if (!tmp)
    return;
  [tmp view].buffer = b;
  [[tmp view] setNeedsDisplay:YES];
}

void CloseAllWindows() {
  struct window_node_t *cursor = windows, *tmp = NULL;
  while (cursor) {
    tmp = cursor->next;
    [cursor->data->delegate dealloc];
    WINDOW_SAFE_FREE(cursor->data);
    WINDOW_SAFE_FREE(cursor);
    cursor = tmp;
  }
  [NSApp terminate:nil];
}
