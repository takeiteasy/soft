
#include "hwsurface_private.c"

@interface GLContext : NSView {
    NSOpenGLContext *ctx;
    NSOpenGLPixelFormat *fmt;
}
-(id)initWithWidth:(int)w andHeight:(int)h;
-(void)dealloc;
@end

@implementation GLContext
-(id)initWithWidth:(int)w andHeight:(int)h {
    self = [super initWithFrame:(NSRect) { 0, 0, w, h }];
    if (self) {
        NSOpenGLPixelFormatAttribute attribs[] = {
            NSOpenGLPFADoubleBuffer,
            NSOpenGLPFAAccelerated,
            NSOpenGLPFABackingStore, YES,
            NSOpenGLPFAColorSize, 24,
            NSOpenGLPFADepthSize, 24,
            NSOpenGLPFAAlphaSize, 8,
            NSOpenGLPFAOpenGLProfile,
            NSOpenGLProfileVersion3_2Core,
            0
        };
        fmt = [[NSOpenGLPixelFormat alloc] initWithAttributes:attribs];
        ctx = [[NSOpenGLContext alloc] initWithFormat:fmt
                                         shareContext:nil];
        [ctx setView:self];
        [ctx makeCurrentContext];
    }
    return self;
}

-(void)dealloc {
    [NSOpenGLContext clearCurrentContext];
    [ctx clearDrawable];
    [fmt dealloc];
    [ctx dealloc];
    [super dealloc];
}
@end

static GLContext *context = nil;

bool InitGLRenderer(int w, int h) {
    if (context)
        return true;
    return (context = [[GLContext alloc] initWithWidth:w andHeight:h]) && InitRenderBuffer(w, h);
}

void DestroyGLRenderer(void) {
    if (!context)
        return;
    DestroyRenderBuffer();
    [context dealloc];
} 