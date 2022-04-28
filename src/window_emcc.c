#include "window-private.c"
#if !defined(WINDOW_CANVAS_NAME)
#if defined(WINDOW_CANVAS_ID)
#error WINDOW_CANVAS_ID cannot be pre-defined unless WINDOW_CANVAS_NAME is also pre-defined
#endif
#define WINDOW_CANVAS_NAME "canvas"
#endif
#define WINDOW_CANVAS_ID "#" WINDOW_CANVAS_NAME
#include <emscripten.h>
#include <emscripten/html5.h>

static struct window_t *e_window = NULL;
static int window_w, window_h, canvas_w, canvas_h, canvas_x, canvas_y, cursor_x, cursor_y;
static bool mouse_in_canvas = true, fullscreen = false;

static enum key_mod translate_mod(bool ctrl, bool shift, bool alt, bool meta) {
  return (enum key_mod)((ctrl ? KB_MOD_CONTROL : 0) | (shift ? KB_MOD_SHIFT : 0) | (alt ? KB_MOD_ALT : 0) | (meta ? KB_MOD_SUPER : 0));
}

static enum key_sym translate_key(int key) {
  return (key > 222 || key < 8 ? KB_KEY_UNKNOWN : keycodes[key]);
}

static EM_BOOL key_callback(int type, const EmscriptenKeyboardEvent *e, void *user_data) {
  static enum key_mod mod;
  static enum key_sym sym;
  mod = translate_mod(e->ctrlKey, e->shiftKey, e->altKey, e->metaKey);
  sym = translate_key(e->keyCode);
  CBCALL(keyboard_callback, sym, mod, (type == EMSCRIPTEN_EVENT_KEYDOWN));

  switch (sym) {
    case KB_KEY_R: // Reload
    case KB_KEY_W: // Close tab (Just in case)
    case KB_KEY_Q: // Close window (Just in case)
#if defined(WINDOW_OSX)
      return (mod == KB_MOD_SUPER);
#else
      return (mod == KB_MOD_CONTROL);
#endif
#if defined(WINDOW_DEBUG) && defined(WINDOW_EMCC_HTML)
    case KB_KEY_F12: // Developer tools Edge
      return true;
    case KB_KEY_I: // Developer tools Chrome/Firefox
    case KB_KEY_J: // Developer tools Chrome
    case KB_KEY_C: // Developer tools Safari/Firefox
    case KB_KEY_K: // Developer tools Firefox
#if defined(WINDOW_OSX)
      return (mod == (KB_MOD_SUPER & KB_MOD_ALT));
#else
      return (mod == (KB_MOD_SUPER & KB_MOD_SHIFT));
#endif
#endif
    default:
      return false;
  }
}

static EM_BOOL mouse_callback(int type, const EmscriptenMouseEvent *e, void *user_data) {
  switch (type) {
    case EMSCRIPTEN_EVENT_MOUSEDOWN:
      if (mouse_in_canvas && e->buttons != 0)
        CBCALL(mouse_button_callback, (enum button)(e->button + 1), translate_mod(e->ctrlKey, e->shiftKey, e->altKey, e->metaKey), true);
      break;
    case EMSCRIPTEN_EVENT_MOUSEUP:
      if (mouse_in_canvas)
        CBCALL(mouse_button_callback, (enum button)(e->button + 1), translate_mod(e->ctrlKey, e->shiftKey, e->altKey, e->metaKey), false);
      break;
    case EMSCRIPTEN_EVENT_MOUSEMOVE:
      cursor_x = e->clientX;
      cursor_y = e->clientY;
      if (mouse_in_canvas)
        CBCALL(mouse_move_callback, e->clientX, e->clientY, e->movementX, e->movementY);
      break;
    case EMSCRIPTEN_EVENT_MOUSEENTER:
      mouse_in_canvas = true;
      return true;
    case EMSCRIPTEN_EVENT_MOUSELEAVE:
      mouse_in_canvas = false;
      return false;
    case EMSCRIPTEN_EVENT_CLICK:
    case EMSCRIPTEN_EVENT_DBLCLICK:
    default:
      return true;
  }
  return true;
}

static EM_BOOL wheel_callback(int type, const EmscriptenWheelEvent *e, void *user_data) {
  if (!mouse_in_canvas)
    return false;
  CBCALL(scroll_callback, translate_mod(e->mouse.ctrlKey, e->mouse.shiftKey, e->mouse.altKey, e->mouse.metaKey), e->deltaX, e->deltaY);
  return true;
}

static EM_BOOL uievent_callback(int type, const EmscriptenUiEvent *e, void *user_data) {
  window_w = EM_ASM_INT_V({ return window.innerWidth; });
  window_h = EM_ASM_INT_V({ return window.innerHeight; });
  canvas_x = EM_ASM_INT({ return Module['canvas'].getBoundingClientRect().left });
  canvas_y = EM_ASM_INT({ return Module['canvas'].getBoundingClientRect().top });
  if (fullscreen) {
    canvas_w  = window_w;
    canvas_h = window_h;
  } else {
    static double css_w, css_h;
    emscripten_get_element_css_size(WINDOW_CANVAS_ID, &css_w, &css_h);
    canvas_w  = (int)css_w;
    canvas_h = (int)css_h;
  }
  return true;
}

static EM_BOOL focusevent_callback(int type, const EmscriptenFocusEvent *e, void *user_data) {
  CBCALL(focus_callback, (type == EMSCRIPTEN_EVENT_FOCUS));
  return true;
}

static EM_BOOL fullscreenchange_callback(int type, const EmscriptenFullscreenChangeEvent *e, void *user_data) {
  fullscreen = e->isFullscreen;
  return true;
}

static EM_BOOL pointerlockchange_callback(int type, const EmscriptenPointerlockChangeEvent *e, void *user_data) {
  return true;
}

static const char* beforeunload_callback(int type, const void *reserved, void *user_data) {
  return "Do you really want to leave the page?";
}

bool window(struct window_t *s, const char *t, int w, int h, short flags) {
  if (e_window) {
#pragma message WARN("TODO: window() handle error")
    return false;
  }
  
  for (int i = 0; i < KB_KEY_LAST; ++i)
    keycodes[i] = KB_KEY_UNKNOWN;
  
  keycodes[8] = KB_KEY_BACKSPACE;
  keycodes[9] = KB_KEY_TAB;
  keycodes[13] = KB_KEY_ENTER;
  keycodes[16] = KB_KEY_LEFT_SHIFT;
  keycodes[17] = KB_KEY_LEFT_CONTROL;
  keycodes[18] = KB_KEY_LEFT_ALT;
  keycodes[19] = KB_KEY_PAUSE;
  keycodes[20] = KB_KEY_CAPS_LOCK;
  keycodes[27] = KB_KEY_ESCAPE;
  keycodes[32] = KB_KEY_SPACE;
  keycodes[33] = KB_KEY_PAGE_UP;
  keycodes[34] = KB_KEY_PAGE_DOWN;
  keycodes[35] = KB_KEY_END;
  keycodes[36] = KB_KEY_HOME;
  keycodes[37] = KB_KEY_LEFT;
  keycodes[38] = KB_KEY_UP;
  keycodes[39] = KB_KEY_RIGHT;
  keycodes[40] = KB_KEY_DOWN;
  keycodes[45] = KB_KEY_INSERT;
  keycodes[46] = KB_KEY_DELETE;
  keycodes[48] = KB_KEY_0;
  keycodes[49] = KB_KEY_1;
  keycodes[50] = KB_KEY_2;
  keycodes[51] = KB_KEY_3;
  keycodes[52] = KB_KEY_4;
  keycodes[53] = KB_KEY_5;
  keycodes[54] = KB_KEY_6;
  keycodes[55] = KB_KEY_7;
  keycodes[56] = KB_KEY_8;
  keycodes[57] = KB_KEY_9;
  keycodes[59] = KB_KEY_SEMICOLON;
  keycodes[61] = KB_KEY_EQUALS;
  keycodes[65] = KB_KEY_A;
  keycodes[66] = KB_KEY_B;
  keycodes[67] = KB_KEY_C;
  keycodes[68] = KB_KEY_D;
  keycodes[69] = KB_KEY_E;
  keycodes[70] = KB_KEY_F;
  keycodes[71] = KB_KEY_G;
  keycodes[72] = KB_KEY_H;
  keycodes[73] = KB_KEY_I;
  keycodes[74] = KB_KEY_J;
  keycodes[75] = KB_KEY_K;
  keycodes[76] = KB_KEY_L;
  keycodes[77] = KB_KEY_M;
  keycodes[78] = KB_KEY_N;
  keycodes[79] = KB_KEY_O;
  keycodes[80] = KB_KEY_P;
  keycodes[81] = KB_KEY_Q;
  keycodes[82] = KB_KEY_R;
  keycodes[83] = KB_KEY_S;
  keycodes[84] = KB_KEY_T;
  keycodes[85] = KB_KEY_U;
  keycodes[86] = KB_KEY_V;
  keycodes[87] = KB_KEY_W;
  keycodes[88] = KB_KEY_X;
  keycodes[89] = KB_KEY_Y;
  keycodes[90] = KB_KEY_Z;
  keycodes[96] = KB_KEY_KP_0;
  keycodes[97] = KB_KEY_KP_1;
  keycodes[98] = KB_KEY_KP_2;
  keycodes[99] = KB_KEY_KP_3;
  keycodes[100] = KB_KEY_KP_4;
  keycodes[101] = KB_KEY_KP_5;
  keycodes[102] = KB_KEY_KP_6;
  keycodes[103] = KB_KEY_KP_7;
  keycodes[104] = KB_KEY_KP_8;
  keycodes[105] = KB_KEY_KP_9;
  keycodes[106] = KB_KEY_KP_MULTIPLY;
  keycodes[107] = KB_KEY_KP_ADD;
  keycodes[109] = KB_KEY_KP_SUBTRACT;
  keycodes[110] = KB_KEY_KP_DECIMAL;
  keycodes[111] = KB_KEY_KP_DIVIDE;
  keycodes[112] = KB_KEY_F1;
  keycodes[113] = KB_KEY_F2;
  keycodes[114] = KB_KEY_F3;
  keycodes[115] = KB_KEY_F4;
  keycodes[116] = KB_KEY_F5;
  keycodes[117] = KB_KEY_F6;
  keycodes[118] = KB_KEY_F7;
  keycodes[119] = KB_KEY_F8;
  keycodes[120] = KB_KEY_F9;
  keycodes[121] = KB_KEY_F10;
  keycodes[122] = KB_KEY_F11;
  keycodes[123] = KB_KEY_F12;
  keycodes[124] = KB_KEY_F13;
  keycodes[125] = KB_KEY_F14;
  keycodes[126] = KB_KEY_F15;
  keycodes[127] = KB_KEY_F16;
  keycodes[128] = KB_KEY_F17;
  keycodes[129] = KB_KEY_F18;
  keycodes[130] = KB_KEY_F19;
  keycodes[131] = KB_KEY_F20;
  keycodes[132] = KB_KEY_F21;
  keycodes[133] = KB_KEY_F22;
  keycodes[134] = KB_KEY_F23;
  keycodes[135] = KB_KEY_F24;
  keycodes[144] = KB_KEY_NUM_LOCK;
  keycodes[145] = KB_KEY_SCROLL_LOCK;
  keycodes[173] = KB_KEY_MINUS;
  keycodes[186] = KB_KEY_SEMICOLON;
  keycodes[187] = KB_KEY_EQUALS;
  keycodes[188] = KB_KEY_COMMA;
  keycodes[189] = KB_KEY_MINUS;
  keycodes[190] = KB_KEY_PERIOD;
  keycodes[191] = KB_KEY_SLASH;
  keycodes[192] = KB_KEY_GRAVE_ACCENT;
  keycodes[219] = KB_KEY_LEFT_BRACKET;
  keycodes[220] = KB_KEY_BACKSLASH;
  keycodes[221] = KB_KEY_RIGHT_BRACKET;
  keycodes[222] = KB_KEY_APOSTROPHE;
  
  emscripten_set_keypress_callback(0, 0, 1, key_callback);
  emscripten_set_keydown_callback(0, 0, 1, key_callback);
  emscripten_set_keyup_callback(0, 0, 1, key_callback);
  
  emscripten_set_click_callback(WINDOW_CANVAS_ID, 0, 1, mouse_callback);
  emscripten_set_mousedown_callback(WINDOW_CANVAS_ID, 0, 1, mouse_callback);
  emscripten_set_mouseup_callback(WINDOW_CANVAS_ID, 0, 1, mouse_callback);
  emscripten_set_dblclick_callback(WINDOW_CANVAS_ID, 0, 1, mouse_callback);
  emscripten_set_mousemove_callback(WINDOW_CANVAS_ID, 0, 1, mouse_callback);
  emscripten_set_mouseenter_callback(WINDOW_CANVAS_ID, 0, 1, mouse_callback);
  emscripten_set_mouseleave_callback(WINDOW_CANVAS_ID, 0, 1, mouse_callback);
  emscripten_set_mouseover_callback(WINDOW_CANVAS_ID, 0, 1, mouse_callback);
  emscripten_set_mouseout_callback(WINDOW_CANVAS_ID, 0, 1, mouse_callback);
  
  emscripten_set_wheel_callback(0, 0, 1, wheel_callback);
  
  emscripten_set_resize_callback(0, 0, 1, uievent_callback);
  emscripten_set_scroll_callback(0, 0, 1, uievent_callback);
  
  emscripten_set_blur_callback(0, 0, 1, focusevent_callback);
  emscripten_set_focus_callback(0, 0, 1, focusevent_callback);
  emscripten_set_focusin_callback(0, 0, 1, focusevent_callback);
  emscripten_set_focusout_callback(0, 0, 1, focusevent_callback);
  
  emscripten_set_fullscreenchange_callback(WINDOW_CANVAS_ID, 0, 1, fullscreenchange_callback);
  
  emscripten_set_pointerlockchange_callback(0, 0, 1, pointerlockchange_callback);
  
  emscripten_set_beforeunload_callback(0, beforeunload_callback);
  
  emscripten_set_webglcontextlost_callback(0, 0, 1, webglcontext_callback);
  emscripten_set_webglcontextrestored_callback(0, 0, 1, webglcontext_callback);
  
  EM_ASM(Module['noExitRuntime'] = true);
  
  if (t)
    window_title(NULL, t);
  
  emscripten_set_canvas_element_size(WINDOW_CANVAS_ID, w, h);
  EMSCRIPTEN_FULLSCREEN_SCALE fs_scale = EMSCRIPTEN_FULLSCREEN_SCALE_DEFAULT;
  bool fs = false, soft_fs = false;
  if (flags & FULLSCREEN_DESKTOP) {
    fs_scale = EMSCRIPTEN_FULLSCREEN_SCALE_STRETCH;
    fs = true;
  }
  if (flags & FULLSCREEN) {
    fs_scale = EMSCRIPTEN_FULLSCREEN_SCALE_STRETCH;
    fs = soft_fs = true;
  }
  if (fs) {
    EmscriptenFullscreenStrategy fsf;
    memset(&fsf, 0, sizeof(fsf));
    fsf.scaleMode = fs_scale;
    fsf.canvasResolutionScaleMode = EMSCRIPTEN_FULLSCREEN_CANVAS_SCALE_NONE;
    fsf.filteringMode = EMSCRIPTEN_FULLSCREEN_FILTERING_NEAREST;
    fsf.canvasResizedCallback = uievent_callback;
    if (soft_fs)
      emscripten_enter_soft_fullscreen(0, &fsf);
    else
      emscripten_request_fullscreen_strategy(0, 1, &fsf);
  }
  uievent_callback(0, NULL, NULL);
  
  e_window = s;
  return true;
}

void window_icon(struct window_t *_, struct surface_t *__) {
#pragma message WARN("window_icon() unsupported on emscripten")
}

void window_title(struct window_t *_, const char *t) {
  EM_ASM({
    setWindowTitle(UTF8ToString($0));
  }, t);
}

void window_position(struct window_t *_, int *x, int * y) {
  if (x)
    *x = canvas_x;
  if (y)
    *y = canvas_y;
}

void screen_size(struct window_t *_, int *w, int *h) {
  if (w)
    *w = window_w;
  if (h)
    *h = window_h;
}

void window_destroy(struct window_t *s) {
  memset(s, 0, sizeof(struct window_t));
}

bool closed(struct window_t *_) {
  return false;
}

bool closed_va(int n, ...) {
  return false;
}

bool closed_all() {
  return false;
}

void cursor_lock(struct window_t *s, bool locked) {
#pragma message WARN("cursor_lock() unsupported on emscripten")
  if (locked)
    emscripten_request_pointerlock(NULL, 1);
  else
    emscripten_exit_pointerlock();
}

static const char *cursor = "default";
static bool cursor_custom = false;

void cursor_visible(struct window_t *s, bool show) {
  EM_ASM({
    if (Module['canvas']) {
      Module['canvas'].style['cursor'] = ($1 ? UTF8ToString($0) : 'none');
    }
  }, cursor, show);
}

void cursor_icon(struct window_t *w, enum cursor_type t) {
  if (cursor_custom && cursor)
    WINDOW_SAFE_FREE(cursor);
  cursor_custom = false;
  
  switch (t) {
    default:
    case CURSOR_ARROW:
      cursor = "default";
    case CURSOR_WAIT:
      cursor = "wait";
      break;
    case CURSOR_WAITARROW:
      cursor = "progress";
      break;
    case CURSOR_IBEAM:
      cursor = "text";
      break;
    case CURSOR_CROSSHAIR:
      cursor = "crosshair";
      break;
    case CURSOR_SIZENWSE:
      cursor = "nwse-resize";
      break;
    case CURSOR_SIZENESW:
      cursor = "nesw-resize";
      break;
    case CURSOR_SIZEWE:
      cursor = "ew-resize";
      break;
    case CURSOR_SIZENS:
      cursor = "ns-resize";
      break;
    case CURSOR_SIZEALL:
      cursor = "move";
      break;
    case CURSOR_NO:
      cursor = "not-allowed";
      break;
    case CURSOR_HAND:
      cursor = "pointer";
      break;
  }
  cursor_visible(w, true);
}

void cursor_icon_custom(struct window_t *w, struct surface_t *b) {
  if (cursor_custom && cursor)
    WINDOW_SAFE_FREE(cursor);
  
  cursor = (const char*)EM_ASM_INT({
    var w = $0;
    var h = $1;
    var pixels = $2;
    var ctx = canvas.getContext("2d");
    var canvas = document.createElement("canvas");
    canvas.width = w;
    canvas.height = h;
    var image = ctx.createImageData(w, h);
    var data = image.data;
    var src = pixels >> 2;
    var dst = 0;
    var num = data.length;
    while (dst < num) {
      var val = HEAP32[src];
      data[dst  ] = (val >> 16) & 0xFF;
      data[dst+1] = (val >> 8) & 0xFF;
      data[dst+2] = val & 0xFF;
      data[dst+3] = 0xFF;
      src++;
      dst += 4;
    }
    
    ctx.putImageData(image, 0, 0);
    var url = "url(" + canvas.toDataURL() + "), auto";
    var url_buf = _malloc(url.length + 1);
    stringToUTF8(url, url_buf, url.length + 1);
    
    return url_buf;
  }, b->w, b->h, b->buf);
  if (!cursor) {
    WINDOW_ERROR(UNKNOWN_ERROR, "cursor_custom_icon() failed");
    cursor = "default";
    cursor_custom = false;
    return;
  }
  cursor_custom = true;
  cursor_visible(w, true);
}

void cursor_pos(int *x, int *y) {
  if (x)
    *x = cursor_x;
  if (y)
    *y = cursor_y;
}

void cursor_set_pos(int _, int __) {
#pragma message WARN("cursor_set_pos() unsupported on emscripten")
}

void events(void) {
#if defined(WINDOW_DEBUG) && defined(WINDOW_EMCC_HTML)
  EM_ASM({
    stats.begin();
  });
#endif
}

void flush(struct window_t *_, struct surface_t *b) {
  EM_ASM({
    var w = $0;
    var h = $1;
    var buf = $2;
    var src = buf >> 2;
    var canvas = document.getElementById("canvas");
    var ctx = canvas.getContext("2d");
    var img = ctx.createImageData(w, h);
    var data = img.data;
    
    var i = 0;
    var j = data.length;
    while (i < j) {
      var val = HEAP32[src];
      data[i  ] = (val >> 16) & 0xFF;
      data[i+1] = (val >> 8) & 0xFF;
      data[i+2] = val & 0xFF;
      data[i+3] = 0xFF;
      src++;
      i += 4;
    }

    ctx.putImageData(img, 0, 0);
#if defined(WINDOW_DEBUG) && defined(WINDOW_EMCC_HTML)
    stats.end();
#endif
  }, b->w, b->h, b->buf);
}

void release(void) {
  return;
}
