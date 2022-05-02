#pragma message WARN("TODO: X11 support not yet fully implemented")
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/XKBlib.h>
#include <X11/keysymdef.h>
#include <X11/keysym.h>
#include <X11/Xatom.h>
#include <X11/cursorfont.h>
#if defined(WINDOW_HAS_X11VMEXT)
#include <X11/extensions/xf86vmode.h>
#endif

static Display *display = None;
static int screen = None;
static Window root_window = None;
static Cursor empty_cursor = None;

struct nix_window_t {
  Window window;
  Atom wm_del;
  GC gc;
  XImage *img;
  Cursor cursor;
  bool mouse_inside, cursor_locked, cursor_vis, closed;
  int depth, cursor_lx, cursor_ly;
  Surface scaler;
  Window *parent;
};

static void close_nix_window(struct nix_window_t *w) {
  if (w->closed)
    return;
  w->closed = true;
  if (w->scaler.buf)
    surface_destroy(&w->scaler);
  w->img->data = NULL;
  XDestroyImage(w->img);
  XDestroyWindow(display, w->window);
  XFlush(display);
}

LINKEDLIST(window, struct nix_window_t);
static struct window_node_t *windows = NULL;

#define Button6 6
#define Button7 7

static int translate_key_b(int sym) {
  switch (sym) {
    case XK_KP_0:           return KB_KEY_KP_0;
    case XK_KP_1:           return KB_KEY_KP_1;
    case XK_KP_2:           return KB_KEY_KP_2;
    case XK_KP_3:           return KB_KEY_KP_3;
    case XK_KP_4:           return KB_KEY_KP_4;
    case XK_KP_5:           return KB_KEY_KP_5;
    case XK_KP_6:           return KB_KEY_KP_6;
    case XK_KP_7:           return KB_KEY_KP_7;
    case XK_KP_8:           return KB_KEY_KP_8;
    case XK_KP_9:           return KB_KEY_KP_9;
    case XK_KP_Separator:
    case XK_KP_Decimal:     return KB_KEY_KP_DECIMAL;
    case XK_KP_Equal:       return KB_KEY_KP_EQUALS;
    case XK_KP_Enter:       return KB_KEY_KP_ENTER;
  }
  return KB_KEY_UNKNOWN;
}

static int translate_key_a(int sym) {
  switch (sym) {
    case XK_Escape:         return KB_KEY_ESCAPE;
    case XK_Tab:            return KB_KEY_TAB;
    case XK_Shift_L:        return KB_KEY_LEFT_SHIFT;
    case XK_Shift_R:        return KB_KEY_RIGHT_SHIFT;
    case XK_Control_L:      return KB_KEY_LEFT_CONTROL;
    case XK_Control_R:      return KB_KEY_RIGHT_CONTROL;
    case XK_Meta_L:
    case XK_Alt_L:          return KB_KEY_LEFT_ALT;
    case XK_Mode_switch:      // Mapped to Alt_R on many keyboards
    case XK_ISO_Level3_Shift: // AltGr on at least some machines
    case XK_Meta_R:
    case XK_Alt_R:          return KB_KEY_RIGHT_ALT;
    case XK_Super_L:        return KB_KEY_LEFT_SUPER;
    case XK_Super_R:        return KB_KEY_RIGHT_SUPER;
    case XK_Menu:           return KB_KEY_MENU;
    case XK_Num_Lock:       return KB_KEY_NUM_LOCK;
    case XK_Caps_Lock:      return KB_KEY_CAPS_LOCK;
    case XK_Print:          return KB_KEY_PRINT_SCREEN;
    case XK_Scroll_Lock:    return KB_KEY_SCROLL_LOCK;
    case XK_Pause:          return KB_KEY_PAUSE;
    case XK_Delete:         return KB_KEY_DELETE;
    case XK_BackSpace:      return KB_KEY_BACKSPACE;
    case XK_Return:         return KB_KEY_ENTER;
    case XK_Home:           return KB_KEY_HOME;
    case XK_End:            return KB_KEY_END;
    case XK_Page_Up:        return KB_KEY_PAGE_UP;
    case XK_Page_Down:      return KB_KEY_PAGE_DOWN;
    case XK_Insert:         return KB_KEY_INSERT;
    case XK_Left:           return KB_KEY_LEFT;
    case XK_Right:          return KB_KEY_RIGHT;
    case XK_Down:           return KB_KEY_DOWN;
    case XK_Up:             return KB_KEY_UP;
    case XK_F1:             return KB_KEY_F1;
    case XK_F2:             return KB_KEY_F2;
    case XK_F3:             return KB_KEY_F3;
    case XK_F4:             return KB_KEY_F4;
    case XK_F5:             return KB_KEY_F5;
    case XK_F6:             return KB_KEY_F6;
    case XK_F7:             return KB_KEY_F7;
    case XK_F8:             return KB_KEY_F8;
    case XK_F9:             return KB_KEY_F9;
    case XK_F10:            return KB_KEY_F10;
    case XK_F11:            return KB_KEY_F11;
    case XK_F12:            return KB_KEY_F12;
    case XK_F13:            return KB_KEY_F13;
    case XK_F14:            return KB_KEY_F14;
    case XK_F15:            return KB_KEY_F15;
    case XK_F16:            return KB_KEY_F16;
    case XK_F17:            return KB_KEY_F17;
    case XK_F18:            return KB_KEY_F18;
    case XK_F19:            return KB_KEY_F19;
    case XK_F20:            return KB_KEY_F20;
    case XK_F21:            return KB_KEY_F21;
    case XK_F22:            return KB_KEY_F22;
    case XK_F23:            return KB_KEY_F23;
    case XK_F24:            return KB_KEY_F24;
    case XK_F25:            return KB_KEY_F25;

    // Numeric keypad
    case XK_KP_Divide:      return KB_KEY_KP_DIVIDE;
    case XK_KP_Multiply:    return KB_KEY_KP_MULTIPLY;
    case XK_KP_Subtract:    return KB_KEY_KP_SUBTRACT;
    case XK_KP_Add:         return KB_KEY_KP_ADD;

    // These should have been detected in secondary keysym test above!
    case XK_KP_Insert:      return KB_KEY_KP_0;
    case XK_KP_End:         return KB_KEY_KP_1;
    case XK_KP_Down:        return KB_KEY_KP_2;
    case XK_KP_Page_Down:   return KB_KEY_KP_3;
    case XK_KP_Left:        return KB_KEY_KP_4;
    case XK_KP_Right:       return KB_KEY_KP_6;
    case XK_KP_Home:        return KB_KEY_KP_7;
    case XK_KP_Up:          return KB_KEY_KP_8;
    case XK_KP_Page_Up:     return KB_KEY_KP_9;
    case XK_KP_Delete:      return KB_KEY_KP_DECIMAL;
    case XK_KP_Equal:       return KB_KEY_KP_EQUALS;
    case XK_KP_Enter:       return KB_KEY_KP_ENTER;

    // Last resort: Check for printable keys (should not happen if the XKB
    // extension is available). This will give a layout dependent mapping
    // (which is wrong, and we may miss some keys, especially on non-US
    // keyboards), but it's better than nothing...
    case XK_a:              return KB_KEY_A;
    case XK_b:              return KB_KEY_B;
    case XK_c:              return KB_KEY_C;
    case XK_d:              return KB_KEY_D;
    case XK_e:              return KB_KEY_E;
    case XK_f:              return KB_KEY_F;
    case XK_g:              return KB_KEY_G;
    case XK_h:              return KB_KEY_H;
    case XK_i:              return KB_KEY_I;
    case XK_j:              return KB_KEY_J;
    case XK_k:              return KB_KEY_K;
    case XK_l:              return KB_KEY_L;
    case XK_m:              return KB_KEY_M;
    case XK_n:              return KB_KEY_N;
    case XK_o:              return KB_KEY_O;
    case XK_p:              return KB_KEY_P;
    case XK_q:              return KB_KEY_Q;
    case XK_r:              return KB_KEY_R;
    case XK_s:              return KB_KEY_S;
    case XK_t:              return KB_KEY_T;
    case XK_u:              return KB_KEY_U;
    case XK_v:              return KB_KEY_V;
    case XK_w:              return KB_KEY_W;
    case XK_x:              return KB_KEY_X;
    case XK_y:              return KB_KEY_Y;
    case XK_z:              return KB_KEY_Z;
    case XK_1:              return KB_KEY_1;
    case XK_2:              return KB_KEY_2;
    case XK_3:              return KB_KEY_3;
    case XK_4:              return KB_KEY_4;
    case XK_5:              return KB_KEY_5;
    case XK_6:              return KB_KEY_6;
    case XK_7:              return KB_KEY_7;
    case XK_8:              return KB_KEY_8;
    case XK_9:              return KB_KEY_9;
    case XK_0:              return KB_KEY_0;
    case XK_space:          return KB_KEY_SPACE;
    case XK_minus:          return KB_KEY_MINUS;
    case XK_equal:          return KB_KEY_EQUALS;
    case XK_bracketleft:    return KB_KEY_LEFT_BRACKET;
    case XK_bracketright:   return KB_KEY_RIGHT_BRACKET;
    case XK_backslash:      return KB_KEY_BACKSLASH;
    case XK_semicolon:      return KB_KEY_SEMICOLON;
    case XK_apostrophe:     return KB_KEY_APOSTROPHE;
    case XK_grave:          return KB_KEY_GRAVE_ACCENT;
    case XK_comma:          return KB_KEY_COMMA;
    case XK_period:         return KB_KEY_PERIOD;
    case XK_slash:          return KB_KEY_SLASH;
    case XK_less:           return KB_KEY_WORLD_1; // At least in some layouts...
    default:                break;
  }
  return KB_KEY_UNKNOWN;
}

static int translate_key(int scancode) {
  return scancode < 0 || scancode > 255 ? KB_KEY_UNKNOWN : keycodes[scancode];
}

static int translate_mod(int state) {
  int mod_keys = 0;
  if (state & ShiftMask)
      mod_keys |= KB_MOD_SHIFT;
  if (state & ControlMask)
      mod_keys |= KB_MOD_CONTROL;
  if (state & Mod1Mask)
      mod_keys |= KB_MOD_ALT;
  if (state & Mod4Mask)
      mod_keys |= KB_MOD_SUPER;
  if (state & LockMask)
      mod_keys |= KB_MOD_CAPS_LOCK;
  if (state & Mod2Mask)
      mod_keys |= KB_MOD_NUM_LOCK;
  return mod_keys;
}

static int translate_mod_ex(int key, int state, int is_pressed) {
  int mod_keys = translate_mod(state);
  switch (key) {
    case KB_KEY_LEFT_SHIFT:
    case KB_KEY_RIGHT_SHIFT:
      if (is_pressed)
        mod_keys |= KB_MOD_SHIFT;
      else
        mod_keys &= ~KB_MOD_SHIFT;
      break;
    case KB_KEY_LEFT_CONTROL:
    case KB_KEY_RIGHT_CONTROL:
      if (is_pressed)
        mod_keys |= KB_MOD_CONTROL;
      else
        mod_keys &= ~KB_MOD_CONTROL;
      break;
    case KB_KEY_LEFT_ALT:
    case KB_KEY_RIGHT_ALT:
      if (is_pressed)
        mod_keys |= KB_MOD_ALT;
      else
        mod_keys &= ~KB_MOD_ALT;
      break;
    case KB_KEY_LEFT_SUPER:
    case KB_KEY_RIGHT_SUPER:
      if (is_pressed)
        mod_keys |= KB_MOD_SUPER;
      else
        mod_keys &= ~KB_MOD_SUPER;
      break;
  }
  return mod_keys;
}

static void get_cursor_pos(int *x, int *y) {
  Window in_win, in_child_win;
  Atom type_prop;
  int root_x, root_y, child_x, child_y;
  unsigned int mask, format;
  unsigned long n, sz;
  Window *props;
  XGetWindowProperty(display, root_window, XInternAtom(display, "_NET_ACTIVE_WINDOW", True), 0, 1, False, AnyPropertyType, &type_prop, &format, &n, &sz, (unsigned char**)&props);
  XQueryPointer(display, props[0], &in_win, &in_child_win, &root_x, &root_y, &child_x, &child_y, &mask);
  XFree(props);
  if (x)
    *x = root_x;
  if (y)
    *y = root_y;
}

struct Hints {
  unsigned long flags;
  unsigned long functions;
  unsigned long decorations;
  long input_mode;
  unsigned long status;
};

bool NewWindow(Window *s, const char *t, int w, int h, short flags) {
  if (!keycodes_init) {
    if (!(display = XOpenDisplay(NULL))) {
      WINDOW_ERROR(NIX_WINDOW_CREATION_FAILED, "XOpenDisplay() failed");
      return false;
    }
    root_window = DefaultRootWindow(display);
    screen = DefaultScreen(display);

    memset(keycodes, -1, sizeof(keycodes));
    for (int i = 0; i < 512; ++i)
      keycodes[i] = KB_KEY_UNKNOWN;
    for (int i = 8; i < 256; ++i)
      if ((keycodes[i] = translate_key_b(XkbKeycodeToKeysym(display, i, 0, 1))) == KB_KEY_UNKNOWN)
        keycodes[i] = translate_key_a(XkbKeycodeToKeysym(display, i, 0, 0));

    char data[1] = { 0 };
    XColor color;
    color.red = color.green = color.blue = 0;
    Pixmap pixmap = XCreateBitmapFromData(display, root_window, data, 1, 1);
    if (!pixmap) {
      WINDOW_ERROR(NIX_CURSOR_PIXMAP_ERROR, "XCreateBitmapFromData() failed");
      return false;
    }
    empty_cursor = XCreatePixmapCursor(display, pixmap, pixmap, &color, &color, 0, 0);
    XFreePixmap(display, pixmap);

    keycodes_init = true;
  }

  struct nix_window_t *win_data = WINDOW_MALLOC(sizeof(struct nix_window_t));
  if (!win_data) {
    WINDOW_ERROR(OUT_OF_MEMEORY, "malloc() failed");
    return false;
  }

  int screen_w = DisplayWidth(display, screen);
  int screen_h = DisplayHeight(display, screen);

  if (flags & FULLSCREEN)
    flags = FULLSCREEN | BORDERLESS;

  int x = 0, y = 0;
  if (flags & FULLSCREEN || flags & FULLSCREEN_DESKTOP) {
    w = screen_w;
    h = screen_h;
  } else {
    x = screen_w / 2 - w / 2;
    y = screen_h / 2 - h / 2;
  }

  Visual *visual = DefaultVisual(display, screen);
  int format_c = 0;
  XPixmapFormatValues *formats = XListPixmapFormats(display, &format_c);
  int depth = DefaultDepth(display, screen);
  int depth_c;
  for (int i = 0; i < format_c; ++i)
    if (depth == formats[i].depth) {
      depth_c = formats[i].bits_per_pixel;
      break;
    }
  XFree(formats);

  if (depth_c != 32) {
    WINDOW_ERROR(NIX_WINDOW_CREATION_FAILED, "Invalid display depth: %d", depth_c);
    return false;
  }

  XSetWindowAttributes swa;
  swa.override_redirect = True;
  swa.border_pixel = BlackPixel(display, screen);
  swa.background_pixel = BlackPixel(display, screen);
  swa.backing_store = NotUseful;
  if (!(win_data->window = XCreateWindow(display, root_window, x, y, w, h, 0, depth, InputOutput, visual, CWBackPixel | CWBorderPixel | CWBackingStore, &swa))) {
    WINDOW_ERROR(NIX_WINDOW_CREATION_FAILED, "XCreateWindow() failed");
    return false;
  }

  win_data->wm_del = XInternAtom(display, "WM_DELETE_WINDOW", False);
  XSetWMProtocols(display, win_data->window, &win_data->wm_del, 1);
  
  XSelectInput(display, win_data->window, StructureNotifyMask | KeyPressMask | KeyReleaseMask | PointerMotionMask | ButtonPressMask | ButtonReleaseMask | ExposureMask | FocusChangeMask | EnterWindowMask | LeaveWindowMask);
  XStoreName(display, win_data->window, t);

  if (flags & FULLSCREEN) {
#if defined(WINDOW_HAS_X11VMEXT)
    int modes_n, best_mode = 0;
    XF86VidModeModeInfo **modes;
    XF86VidModeGetAllModeLines(display, screen, &modes_n, &modes);
    for (int i = 0; i < modes_n; ++i) {
      if (modes[i]->hdisplay == w && modes[i]->vdisplay == h)
        best_mode = i;
    }
    XF86VidModeSwitchToMode(display, screen, modes[best_mode]);
    XF86VidModeSetViewPort(display, screen, 0, 0);
    XMoveResizeWindow(display, win_data->win, 0, 0, w, h);
    XMapRaised(display, win_data->win);
    XGrabPointer(display, win_data->win, True, 0, GrabModeAsync, GrabModeAsync, win_data->win, 0L, CurrentTime);
    XGrabKeyboard(display, win_data->win, False, GrabModeAsync, GrabModeAsync, CurrentTime);
#else
    Atom p = XInternAtom(display, "_NET_WM_STATE_FULLSCREEN", True);
    XChangeProperty(display, win_data->window, XInternAtom(display, "_NET_WM_STATE", True), XA_ATOM, 32, PropModeReplace, (unsigned char*)&p, 1);
#endif
  }

  if (flags & BORDERLESS) {
    struct Hints hints;
    hints.flags = 2;
    hints.decorations = 0;
    Atom p = XInternAtom(display, "_MOTIF_WM_HINTS", True);
    XChangeProperty(display, win_data->window, p, p, 32, PropModeReplace, (unsigned char*)&hints, 5);
  }

  if (flags & ALWAYS_ON_TOP) {
    Atom p = XInternAtom(display, "_NET_WM_STATE_ABOVE", False);
    XChangeProperty(display, win_data->window, XInternAtom(display, "_NET_WM_STATE", False), XA_ATOM, 32, PropModeReplace, (unsigned char *)&p, 1);
  }

  XSizeHints hints;
  hints.flags = PPosition | PMinSize | PMaxSize;
  hints.x = 0;
  hints.y = 0;
  if (flags & RESIZABLE) {
    hints.min_width = 0;
    hints.min_height = 0;
    hints.max_width = screen_w;
    hints.max_height = screen_h;
  } else {
    hints.min_width = w;
    hints.min_height = h;
    hints.max_width = w;
    hints.max_height = h;
  }
  XSetWMNormalHints(display, win_data->window, &hints);
  XClearWindow(display, win_data->window);
  XMapRaised(display, win_data->window);
  XFlush(display);
  win_data->gc = DefaultGC(display, screen);
  win_data->cursor = XCreateFontCursor(display, XC_left_ptr);
  get_cursor_pos(&win_data->cursor_lx, &win_data->cursor_ly);
  win_data->img = XCreateImage(display, CopyFromParent, depth, ZPixmap, 0, NULL, w, h, 32, w * 4);
  win_data->depth = depth;
  memset(&win_data->scaler, 0, sizeof(Surface));
  win_data->closed = false;

  windows = window_push(windows, win_data);
  s->w = w;
  s->h = h;
  s->id = (int)win_data->window;
  s->window = win_data;
  win_data->parent = s;

  return true;
}

void SetWindowIcon(Window *w, Surface *b) {
  return;
}

void SetWindowTitle(Window *w, const char *t) {
  struct nix_window_t *win = (struct nix_window_t*)w->window;
  XStoreName(display, win->window, t);
}

void GetWindowPosition(Window *w, int *x, int *y) {
  struct nix_window_t *win = (struct nix_window_t*)w->window;
  static int wx, wy;
  static XWindowAttributes xwa;
  static Window child;
  XTranslateCoordinates(display, win->window, root_window, 0, 0, &wx, &wy, &child);
  XGetWindowAttributes(display, win->window, &xwa);
  if (x)
    *x = wx - xwa.x;
  if (y)
    *y = wy - xwa.y;
}

void GetScreenSize(Window *s, int *w, int *h) {
  if (w)
    *w = DisplayWidth(display, screen);
  if (h)
    *h = DisplayHeight(display, screen);
}

void DestroyWindow(Window *w) {
  struct nix_window_t *win = (struct nix_window_t*)w->window;
  close_nix_window(win);
  WINDOW_SAFE_FREE(win);
  w->window = NULL;
}

bool IsWindowClosed(Window *w) {
  return ((struct nix_window_t*)w->window)->closed;
}

bool AreWindowsClosed(int n, ...) {
  va_list args;
  va_start(args, n);
  bool ret = true;
  for (int i = 0; i < n; ++i) {
    Window *w = va_arg(args, Window*);
    if (!((struct nix_window_t*)w->window)->closed) {
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

void ToggleCursorLock(Window *w, bool lock) {
  return;
}

void ToggleCursorVisiblity(Window *w, bool visible) {
  return;
}

void SetCursorIcon(Window *w, Cursor type) {
  return;
}

void SetCustomCursorIcon(Window *w, Surface *b) {
  return;
}

void GetCursorPosition(int *x, int *y) {
  get_cursor_pos(x, y);
}

void SetCursorPosition(int x, int y) {
  return;
}

Window *event_window(Window w) {
  struct window_node_t *cursor = windows;
  while (cursor) {
    if (cursor->data->window == w)
      return cursor->data->parent;
    cursor = cursor->next;
  }
  return NULL;
}

void PollEvents() {
  static XEvent e;
  static Window *e_window = NULL;
  static struct nix_window_t *e_data = NULL;
  while (XPending(display)) {
    XNextEvent(display, &e);
    if (!(e_window = event_window(e.xclient.window)))
      continue;
    if (!(e_data = (struct nix_window_t*)e_window->window))
      continue;
    if (e_data->closed)
      continue;
    switch (e.type) {
      case KeyPress:
      case KeyRelease: {
        static bool pressed = false;
        pressed = e.type == KeyPress;
        CBCALL(keyboard_callback, translate_key(e.xkey.keycode), translate_mod_ex(e.xkey.keycode, e.xkey.state, pressed), pressed);
        break;
      }
      case ButtonPress:
      case ButtonRelease:
        switch (e.xbutton.Button) {
          case Button1:
          case Button2:
          case Button3:
            CBCALL(mouse_button_callback, (Button)e.xbutton.Button, translate_mod(e.xkey.state), e.type == ButtonPress);
            break;
          case Button4:
            CBCALL(scroll_callback, translate_mod(e.xkey.state), 0.f, 1.f);
            break;
          case Button5:
            CBCALL(scroll_callback, translate_mod(e.xkey.state), 0.f, -1.f);
            break;
          case Button6:
            CBCALL(scroll_callback, translate_mod(e.xkey.state), 1.f, 0.f);
            break;
          case Button7:
            CBCALL(scroll_callback, translate_mod(e.xkey.state), -1.f, 0.f);
            break;
          default:
            CBCALL(mouse_button_callback, (Button)(e.xbutton.Button - 4), translate_mod(e.xkey.state), e.type == ButtonPress);
            break;
        }
        break;
      case ConfigureNotify: {
        static int w = 0, h = 0;
        w = e.xconfigure.width;
        h = e.xconfigure.height;
        if (e_window->w == w && e_window->h == h)
          break;
        CBCALL(resize_callback, w, h);
        e_window->w = w;
        e_window->h = h;
        if (e_data->img) {
          e_data->img->data = NULL;
          XDestroyImage(e_data->img);
        }
        if (e_data->scaler.buf)
          surface_destroy(&e_data->scaler);
        e_data->img = XCreateImage(display, CopyFromParent, e_data->depth, ZPixmap, 0, NULL, w, h, 32, w * 4);
        surface(&e_data->scaler, w, h);
        break;
      }
      case EnterNotify:
      case LeaveNotify:
        e_data->mouse_inside = e.type == EnterNotify;
        break;
      case FocusIn:
      case FocusOut:
        CBCALL(focus_callback, e.type == FocusIn);
        break;
      case MotionNotify: {
        static int cx = 0, cy = 0;
        cx = e.xmotion.x;
        cy = e.xmotion.y;
        CBCALL(mouse_move_callback, cx, cy, cx - e_data->cursor_lx, cy - e_data->cursor_ly);
        e_data->cursor_lx = cx;
        e_data->cursor_ly = cy;
        break;
      }
      case ClientMessage:
        if (e.xclient.data.l[0] != e_data->wm_del)
          break;
        close_nix_window(e_data);
        windows = window_pop(windows, e_data);
        if (e_window && e_window->closed_callback)
          e_window->closed_callback(e_window->parent);
        break;
    }
  }
}

void Flush(Window *w, Surface *b) {
  if (!w)
    return;
  struct nix_window_t *tmp = (struct nix_window_t*)w->window;
  if (!tmp || tmp->closed)
    return;
  if (b->w != w->w || b->h != w->h) {
    __resize(b, &tmp->scaler);
    tmp->img->data = (char*)tmp->scaler.buf;
  } else
    tmp->img->data = (char*)b->buf;
  XPutImage(display, tmp->window, tmp->gc, tmp->img, 0, 0, 0, 0, w->w, w->h);
  XFlush(display);
}

void CloseAllWindows() {
  struct window_node_t *tmp = NULL, *cursor = windows;
  while (cursor) {
    tmp = cursor->next;
    close_nix_window(tmp->data);
    WINDOW_SAFE_FREE(cursor->data);
    WINDOW_SAFE_FREE(cursor);
    cursor = tmp;
  }
  if (display)
    XCloseDisplay(display);
}
