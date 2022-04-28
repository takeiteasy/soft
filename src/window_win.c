#include "window-private.c"
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

struct win32_window_t {
  WNDCLASS wnd;
  HWND hwnd;
  HDC hdc;
  BITMAPINFO *bmpinfo;
  TRACKMOUSEEVENT tme;
  HICON icon;
  HCURSOR cursor;
  int cursor_lx, cursor_ly;
  bool mouse_inside, cursor_vis, cursor_locked, closed, refresh_tme, custom_icon, custom_cursor;
  struct surface_t *buffer;
};

static void close_win32_window(struct win32_window_t *window) {
  if (window->closed)
    return;
  window->closed = true;
  if (window->cursor_locked)
    ClipCursor(NULL);
  if (!window->cursor_vis)
    ShowCursor(TRUE);
  WINDOW_FREE(window->bmpinfo);
  if (window->custom_icon && window->icon)
    DeleteObject(window->icon);
  if (window->custom_cursor && window->cursor)
    DeleteObject(window->cursor);
  ReleaseDC(window->hwnd, window->hdc);
  DestroyWindow(window->hwnd);
}

static void clip_win32_cursor(HWND hwnd) {
  static RECT r = { 0 };
  GetClientRect(hwnd, &r);
  ClientToScreen(hwnd, (LPPOINT)&r);
  ClientToScreen(hwnd, (LPPOINT)&r + 1);
  ClipCursor(&r);
}

LINKEDLIST(window, struct win32_window_t);
static struct window_node_t *windows = NULL;

static int translate_mod() {
  int mods = 0;

  if (GetKeyState(VK_SHIFT) & 0x8000)
    mods |= KB_MOD_SHIFT;
  if (GetKeyState(VK_CONTROL) & 0x8000)
    mods |= KB_MOD_CONTROL;
  if (GetKeyState(VK_MENU) & 0x8000)
    mods |= KB_MOD_ALT;
  if ((GetKeyState(VK_LWIN) | GetKeyState(VK_RWIN)) & 0x8000)
    mods |= KB_MOD_SUPER;
  if (GetKeyState(VK_CAPITAL) & 1)
    mods |= KB_MOD_CAPS_LOCK;
  if (GetKeyState(VK_NUMLOCK) & 1)
    mods |= KB_MOD_NUM_LOCK;

  return mods;
}

static int translate_key(WPARAM wParam, LPARAM lParam) {
  if (wParam == VK_CONTROL) {
    static MSG next;
    static DWORD time;

    if (lParam & 0x01000000)
      return KB_KEY_RIGHT_CONTROL;

    ZeroMemory(&next, sizeof(MSG));
    time = GetMessageTime();
    if (PeekMessageW(&next, NULL, 0, 0, PM_NOREMOVE))
      if (next.message == WM_KEYDOWN || next.message == WM_SYSKEYDOWN || next.message == WM_KEYUP || next.message == WM_SYSKEYUP)
        if (next.wParam == VK_MENU && (next.lParam & 0x01000000) && next.time == time)
          return KB_KEY_UNKNOWN;

    return KB_KEY_LEFT_CONTROL;
  }

  if (wParam == VK_PROCESSKEY)
    return KB_KEY_UNKNOWN;

  return keycodes[HIWORD(lParam) & 0x1FF];
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
  static struct win32_window_t *e_data = NULL;
  static struct window_t *e_window = NULL;
  e_window = (struct window_t*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
  if (!e_window || !e_window->window)
    return DefWindowProc(hWnd, message, wParam, lParam);
  e_data = (struct win32_window_t*)e_window->window;
  
  switch (message) {
    case WM_PAINT:
      if (!e_data->buffer)
        break;
      e_data->bmpinfo->bmiHeader.biWidth = e_data->buffer->w;
      e_data->bmpinfo->bmiHeader.biHeight = -e_data->buffer->h;
      StretchDIBits(e_data->hdc, 0, 0, e_window->w, e_window->h, 0, 0, e_data->buffer->w, e_data->buffer->h, e_data->buffer->buf, e_data->bmpinfo, DIB_RGB_COLORS, SRCCOPY);
      ValidateRect(hWnd, NULL);
      break;
    case WM_DESTROY:
    case WM_CLOSE:
      close_win32_window(e_data);
      windows = window_pop(windows, e_data);
      e_data->closed = true;
      if (e_window && e_window->closed_callback)
        e_window->closed_callback(e_window->parent);
      break;
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
    case WM_KEYUP:
    case WM_SYSKEYUP: {
      static int kb_key = 0;
      static bool kb_action = false;
      kb_key = translate_key(wParam, lParam);
      kb_action = !((lParam >> 31) & 1);

      if (kb_key == KB_KEY_UNKNOWN)
        break;
      if (!kb_action && wParam == VK_SHIFT) {
        CBCALL(keyboard_callback, KB_KEY_LEFT_SHIFT, translate_mod(), kb_action);
      } else if (wParam == VK_SNAPSHOT) {
        CBCALL(keyboard_callback, kb_key, translate_mod(), false);
      } else {
        CBCALL(keyboard_callback, kb_key, translate_mod(), kb_action);
      }
      break;
    }
    case WM_SETCURSOR:
      if (LOWORD(lParam) == HTCLIENT) {
        SetCursor(e_data->cursor);
        return TRUE;
      }
      break;
    case WM_CHAR:
    case WM_SYSCHAR:
    case WM_UNICHAR:
      // I don't know if this is important or not
      break;
    case WM_LBUTTONUP:
    case WM_RBUTTONUP:
    case WM_MBUTTONUP:
    case WM_XBUTTONUP:
    case WM_LBUTTONDOWN:
    case WM_LBUTTONDBLCLK:
    case WM_RBUTTONDOWN:
    case WM_RBUTTONDBLCLK:
    case WM_MBUTTONDOWN:
    case WM_MBUTTONDBLCLK:
    case WM_XBUTTONDOWN:
    case WM_XBUTTONDBLCLK: {
      static int m_button, m_action = 0;
      switch (message) {
      case WM_LBUTTONDOWN:
        m_action = 1;
      case WM_LBUTTONUP:
        m_button = enum button_1;
        break;
      case WM_RBUTTONDOWN:
        m_action = 1;
      case WM_RBUTTONUP:
        m_button = enum button_2;
        break;
      case WM_MBUTTONDOWN:
        m_action = 1;
      case WM_MBUTTONUP:
        m_button = enum button_3;
        break;
      default:
        m_button = (GET_XBUTTON_WPARAM(wParam) == XBUTTON1 ? enum button_5 : enum button_6);
        if (message == WM_XBUTTONDOWN)
          m_action = 1;
      }
      CBCALL(mouse_button_callback, (enum button)m_button, translate_mod(), m_action);
      break;
    }
    case WM_MOUSEWHEEL:
      CBCALL(scroll_callback, translate_mod(), 0.f, (SHORT)HIWORD(wParam) / (float)WHEEL_DELTA);
      break;
    case WM_MOUSEHWHEEL:
      CBCALL(scroll_callback, translate_mod(), -((SHORT)HIWORD(wParam) / (float)WHEEL_DELTA), 0.f);
      break;
    case WM_MOUSEMOVE: {
      if (e_data->refresh_tme) {
        e_data->tme.cbSize = sizeof(e_data->tme);
        e_data->tme.hwndTrack = e_data->hwnd;
        e_data->tme.dwFlags = TME_HOVER | TME_LEAVE;
        e_data->tme.dwHoverTime = 1;
        TrackMouseEvent(&e_data->tme);
      }
      static int cx, cy;
      cx = ((int)(short)LOWORD(lParam));
      cy = ((int)(short)HIWORD(lParam));
      CBCALL(mouse_move_callback, cx, cy, cx - e_data->cursor_lx, cy - e_data->cursor_ly);
      e_data->cursor_lx = cx;
      e_data->cursor_ly = cy;
      break;
    }
    case WM_MOUSEHOVER:
      if (!e_data->mouse_inside) {
        e_data->refresh_tme = true;
        e_data->mouse_inside = true;
        if (!e_data->cursor_vis)
          ShowCursor(FALSE);
      }
      break;
    case WM_MOUSELEAVE:
      if (e_data->mouse_inside) {
        e_data->refresh_tme = true;
        e_data->mouse_inside = false;
        ShowCursor(TRUE);
      }
      break;
    case WM_SIZE:
      e_window->w = LOWORD(lParam);
      e_window->h = HIWORD(lParam);
      CBCALL(resize_callback, e_window->w, e_window->h);
      break;
    case WM_SETFOCUS:
      if (e_data->cursor_locked)
        clip_win32_cursor(e_data->hwnd);
      CBCALL(focus_callback, true);
      break;
    case WM_KILLFOCUS:
      if (e_data->cursor_locked)
        ClipCursor(NULL);
      CBCALL(focus_callback, false);
      break;
    default:
      break;
  }
  return DefWindowProc(hWnd, message, wParam, lParam);
}

static void windows_error(enum window_error err, const char *msg) {
  DWORD id = GetLastError();
  LPSTR buf = NULL;
  FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&buf, 0, NULL);
  WINDOW_ERROR(err, "%s (%d): %s", msg, id, buf);
}

bool window(struct window_t *s, const char *t, int w, int h, short flags) {
  if (!keycodes_init) {
    memset(keycodes, -1, sizeof(keycodes));
    
    keycodes[0x00B] = KB_KEY_0;
    keycodes[0x002] = KB_KEY_1;
    keycodes[0x003] = KB_KEY_2;
    keycodes[0x004] = KB_KEY_3;
    keycodes[0x005] = KB_KEY_4;
    keycodes[0x006] = KB_KEY_5;
    keycodes[0x007] = KB_KEY_6;
    keycodes[0x008] = KB_KEY_7;
    keycodes[0x009] = KB_KEY_8;
    keycodes[0x00A] = KB_KEY_9;
    keycodes[0x01E] = KB_KEY_A;
    keycodes[0x030] = KB_KEY_B;
    keycodes[0x02E] = KB_KEY_C;
    keycodes[0x020] = KB_KEY_D;
    keycodes[0x012] = KB_KEY_E;
    keycodes[0x021] = KB_KEY_F;
    keycodes[0x022] = KB_KEY_G;
    keycodes[0x023] = KB_KEY_H;
    keycodes[0x017] = KB_KEY_I;
    keycodes[0x024] = KB_KEY_J;
    keycodes[0x025] = KB_KEY_K;
    keycodes[0x026] = KB_KEY_L;
    keycodes[0x032] = KB_KEY_M;
    keycodes[0x031] = KB_KEY_N;
    keycodes[0x018] = KB_KEY_O;
    keycodes[0x019] = KB_KEY_P;
    keycodes[0x010] = KB_KEY_Q;
    keycodes[0x013] = KB_KEY_R;
    keycodes[0x01F] = KB_KEY_S;
    keycodes[0x014] = KB_KEY_T;
    keycodes[0x016] = KB_KEY_U;
    keycodes[0x02F] = KB_KEY_V;
    keycodes[0x011] = KB_KEY_W;
    keycodes[0x02D] = KB_KEY_X;
    keycodes[0x015] = KB_KEY_Y;
    keycodes[0x02C] = KB_KEY_Z;
    
    keycodes[0x028] = KB_KEY_APOSTROPHE;
    keycodes[0x02B] = KB_KEY_BACKSLASH;
    keycodes[0x033] = KB_KEY_COMMA;
    keycodes[0x00D] = KB_KEY_EQUALS;
    keycodes[0x029] = KB_KEY_GRAVE_ACCENT;
    keycodes[0x01A] = KB_KEY_LEFT_BRACKET;
    keycodes[0x00C] = KB_KEY_MINUS;
    keycodes[0x034] = KB_KEY_PERIOD;
    keycodes[0x01B] = KB_KEY_RIGHT_BRACKET;
    keycodes[0x027] = KB_KEY_SEMICOLON;
    keycodes[0x035] = KB_KEY_SLASH;
    keycodes[0x056] = KB_KEY_WORLD_2;
    
    keycodes[0x00E] = KB_KEY_BACKSPACE;
    keycodes[0x153] = KB_KEY_DELETE;
    keycodes[0x14F] = KB_KEY_END;
    keycodes[0x01C] = KB_KEY_ENTER;
    keycodes[0x001] = KB_KEY_ESCAPE;
    keycodes[0x147] = KB_KEY_HOME;
    keycodes[0x152] = KB_KEY_INSERT;
    keycodes[0x15D] = KB_KEY_MENU;
    keycodes[0x151] = KB_KEY_PAGE_DOWN;
    keycodes[0x149] = KB_KEY_PAGE_UP;
    keycodes[0x045] = KB_KEY_PAUSE;
    keycodes[0x146] = KB_KEY_PAUSE;
    keycodes[0x039] = KB_KEY_SPACE;
    keycodes[0x00F] = KB_KEY_TAB;
    keycodes[0x03A] = KB_KEY_CAPS_LOCK;
    keycodes[0x145] = KB_KEY_NUM_LOCK;
    keycodes[0x046] = KB_KEY_SCROLL_LOCK;
    keycodes[0x03B] = KB_KEY_F1;
    keycodes[0x03C] = KB_KEY_F2;
    keycodes[0x03D] = KB_KEY_F3;
    keycodes[0x03E] = KB_KEY_F4;
    keycodes[0x03F] = KB_KEY_F5;
    keycodes[0x040] = KB_KEY_F6;
    keycodes[0x041] = KB_KEY_F7;
    keycodes[0x042] = KB_KEY_F8;
    keycodes[0x043] = KB_KEY_F9;
    keycodes[0x044] = KB_KEY_F10;
    keycodes[0x057] = KB_KEY_F11;
    keycodes[0x058] = KB_KEY_F12;
    keycodes[0x064] = KB_KEY_F13;
    keycodes[0x065] = KB_KEY_F14;
    keycodes[0x066] = KB_KEY_F15;
    keycodes[0x067] = KB_KEY_F16;
    keycodes[0x068] = KB_KEY_F17;
    keycodes[0x069] = KB_KEY_F18;
    keycodes[0x06A] = KB_KEY_F19;
    keycodes[0x06B] = KB_KEY_F20;
    keycodes[0x06C] = KB_KEY_F21;
    keycodes[0x06D] = KB_KEY_F22;
    keycodes[0x06E] = KB_KEY_F23;
    keycodes[0x076] = KB_KEY_F24;
    keycodes[0x038] = KB_KEY_LEFT_ALT;
    keycodes[0x01D] = KB_KEY_LEFT_CONTROL;
    keycodes[0x02A] = KB_KEY_LEFT_SHIFT;
    keycodes[0x15B] = KB_KEY_LEFT_SUPER;
    keycodes[0x137] = KB_KEY_PRINT_SCREEN;
    keycodes[0x138] = KB_KEY_RIGHT_ALT;
    keycodes[0x11D] = KB_KEY_RIGHT_CONTROL;
    keycodes[0x036] = KB_KEY_RIGHT_SHIFT;
    keycodes[0x15C] = KB_KEY_RIGHT_SUPER;
    keycodes[0x150] = KB_KEY_DOWN;
    keycodes[0x14B] = KB_KEY_LEFT;
    keycodes[0x14D] = KB_KEY_RIGHT;
    keycodes[0x148] = KB_KEY_UP;
    
    keycodes[0x052] = KB_KEY_KP_0;
    keycodes[0x04F] = KB_KEY_KP_1;
    keycodes[0x050] = KB_KEY_KP_2;
    keycodes[0x051] = KB_KEY_KP_3;
    keycodes[0x04B] = KB_KEY_KP_4;
    keycodes[0x04C] = KB_KEY_KP_5;
    keycodes[0x04D] = KB_KEY_KP_6;
    keycodes[0x047] = KB_KEY_KP_7;
    keycodes[0x048] = KB_KEY_KP_8;
    keycodes[0x049] = KB_KEY_KP_9;
    keycodes[0x04E] = KB_KEY_KP_ADD;
    keycodes[0x053] = KB_KEY_KP_DECIMAL;
    keycodes[0x135] = KB_KEY_KP_DIVIDE;
    keycodes[0x11C] = KB_KEY_KP_ENTER;
    keycodes[0x037] = KB_KEY_KP_MULTIPLY;
    keycodes[0x04A] = KB_KEY_KP_SUBTRACT;
    
    keycodes_init = true;
  }

  struct win32_window_t *win_data = WINDOW_MALLOC(sizeof(struct win32_window_t));
  if (!win_data) {
    WINDOW_ERROR(OUT_OF_MEMEORY, "malloc() failed");
    return false;
  }

  RECT rect = {0};
  long window_flags = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX;
  if (flags & FULLSCREEN) {
    flags = FULLSCREEN;
    rect.right = GetSystemMetrics(SM_CXSCREEN);
    rect.bottom = GetSystemMetrics(SM_CYSCREEN);
    window_flags = WS_POPUP & ~(WS_CAPTION | WS_THICKFRAME | WS_MINIMIZE | WS_MAXIMIZE | WS_SYSMENU);

    DEVMODE settings = { 0 };
    EnumDisplaySettings(0, 0, &settings);
    settings.dmPelsWidth = GetSystemMetrics(SM_CXSCREEN);
    settings.dmPelsHeight = GetSystemMetrics(SM_CYSCREEN);
    settings.dmBitsPerPel = 32;
    settings.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;

    if (ChangeDisplaySettings(&settings, CDS_FULLSCREEN) != DISP_CHANGE_SUCCESSFUL)
      flags = FULLSCREEN_DESKTOP;
  }

  if (flags & BORDERLESS)
    window_flags = WS_POPUP;
  if (flags & RESIZABLE)
    window_flags |= WS_MAXIMIZEBOX | WS_SIZEBOX;
  if (flags & FULLSCREEN_DESKTOP) {
    window_flags = WS_OVERLAPPEDWINDOW;

    int width = GetSystemMetrics(SM_CXFULLSCREEN);
    int height = GetSystemMetrics(SM_CYFULLSCREEN);

    rect.right = width;
    rect.bottom = height;
    AdjustWindowRect(&rect, window_flags, 0);
    if (rect.left < 0) {
      width += rect.left * 2;
      rect.right += rect.left;
      rect.left = 0;
    }
    if (rect.bottom > (LONG)height) {
      height -= (rect.bottom - height);
      rect.bottom += (rect.bottom - height);
      rect.top = 0;
    }
  }
  else if (!(flags & FULLSCREEN)) {
    rect.right = w;
    rect.bottom = h;

    AdjustWindowRect(&rect, window_flags, 0);

    rect.right -= rect.left;
    rect.bottom -= rect.top;

    rect.left = (GetSystemMetrics(SM_CXSCREEN) - rect.right) / 2;
    rect.top = (GetSystemMetrics(SM_CYSCREEN) - rect.bottom + rect.top) / 2;
  }

  memset(&win_data->wnd, 0, sizeof(win_data->wnd));
  win_data->wnd.style = CS_OWNDC | CS_VREDRAW | CS_HREDRAW;
  win_data->wnd.lpfnWndProc = WndProc;
  win_data->wnd.hCursor = LoadCursor(0, IDC_ARROW);
  win_data->wnd.lpszClassName = t;
  if (!RegisterClass(&win_data->wnd)) {
    windows_error(WIN_WINDOW_CREATION_FAILED, "RegisterClass() failed");
    return false;
  }

  if (!(win_data->hwnd = CreateWindowEx(0, t, t, window_flags, rect.left, rect.top, rect.right, rect.bottom, 0, 0, 0, 0))) {
    windows_error(WIN_WINDOW_CREATION_FAILED, "CreateWindowEx() failed");
    return false;
  }
  if (!(win_data->hdc = GetDC(win_data->hwnd))) {
    windows_error(WIN_WINDOW_CREATION_FAILED, "GetDC() failed");
    return false;
  }
  SetWindowLongPtr(win_data->hwnd, GWLP_USERDATA, (LONG_PTR)s);

  if (flags & ALWAYS_ON_TOP)
    SetWindowPos(win_data->hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

  ShowWindow(win_data->hwnd, SW_NORMAL);
  SetFocus(win_data->hwnd);

  size_t bmpinfo_sz = sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD) * 3;
  if (!(win_data->bmpinfo = WINDOW_MALLOC(bmpinfo_sz))) {
    WINDOW_ERROR(OUT_OF_MEMEORY, "malloc() failed");
    return false;
  }
  memset(win_data->bmpinfo, 0, bmpinfo_sz);
  win_data->bmpinfo->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
  win_data->bmpinfo->bmiHeader.biPlanes = 1;
  win_data->bmpinfo->bmiHeader.biBitCount = 32;
  win_data->bmpinfo->bmiHeader.biCompression = BI_BITFIELDS;
  win_data->bmpinfo->bmiHeader.biWidth = w;
  win_data->bmpinfo->bmiHeader.biHeight = -(LONG)h;
  win_data->bmpinfo->bmiColors[0].rgbRed = 0xFF;
  win_data->bmpinfo->bmiColors[1].rgbGreen = 0xFF;
  win_data->bmpinfo->bmiColors[2].rgbBlue = 0xff;

  win_data->tme.cbSize = sizeof(win_data->tme);
  win_data->tme.hwndTrack = win_data->hwnd;
  win_data->tme.dwFlags = TME_HOVER | TME_LEAVE;
  win_data->tme.dwHoverTime = HOVER_DEFAULT;
  TrackMouseEvent(&win_data->tme);

  win_data->buffer = NULL;
  win_data->mouse_inside = false;
  win_data->closed = false;
  win_data->refresh_tme = true;

  win_data->icon = NULL;
  win_data->cursor = win_data->wnd.hCursor;
  win_data->custom_icon = false;
  win_data->custom_cursor = false;
  win_data->cursor_vis = true;
  win_data->cursor_locked = false;
  POINT p;
  GetCursorPos(&p);
  win_data->cursor_lx = p.x;
  win_data->cursor_ly = p.y;

  windows = window_push(windows, win_data);
  static int window_id = 0;
  s->w = rect.right;
  s->h = rect.bottom;
  s->id = window_id++;
  s->window = win_data;

  return true;
}

void window_icon(struct window_t *s, struct surface_t *b) {
  struct win32_window_t *win = (struct win32_window_t*)s->window;
  HBITMAP hbmp = NULL, bmp_mask = NULL;

  if (!(hbmp = CreateBitmap(b->w, b->h, 1, 32, b->buf)))
    goto FAILED;
  if (!(bmp_mask = CreateCompatibleBitmap(GetDC(NULL), b->w / 2, b->h / 2)))
    goto FAILED;
  ICONINFO ii = { 0 };
  ii.fIcon = TRUE;
  ii.hbmColor = hbmp;
  ii.hbmMask = bmp_mask;
  win->icon = CreateIconIndirect(&ii);

FAILED:
  if (bmp_mask)
    DeleteObject(bmp_mask);
  if (hbmp)
    DeleteObject(hbmp);

  if (!win->icon) {
    windows_error(WINDOW_ICON_FAILED, "create_windows_icon() failed");
    win->icon = LoadIcon(NULL, IDI_APPLICATION);
    win->custom_icon = false;
  } else
    win->custom_icon = true;

  SetClassLong(win->hwnd, GCLP_HICON, win->icon);
  SendMessage(win->hwnd, WM_SETICON, ICON_SMALL, win->icon);
  SendMessage(win->hwnd, WM_SETICON, ICON_BIG, win->icon);
  SendMessage(GetWindow(win->hwnd, GW_OWNER), WM_SETICON, ICON_SMALL, win->icon);
  SendMessage(GetWindow(win->hwnd, GW_OWNER), WM_SETICON, ICON_BIG, win->icon);
}

void window_title(struct window_t *s, const char *t) {
  SetWindowTextA(((struct win32_window_t*)s->window)->hwnd, t);
}

void window_position(struct window_t *s, int *x, int * y) {
  static RECT rect = { 0 };
  GetWindowRect(((struct win32_window_t*)s->window)->hwnd, &rect);
  if (x)
    *x = rect.left;
  if (y)
    *y = rect.top;
}

void screen_size(struct window_t *s, int *w, int *h) {
  if (w)
    *w = GetSystemMetrics(SM_CXFULLSCREEN);
  if (h)
    *h = GetSystemMetrics(SM_CYFULLSCREEN);
}

void window_destroy(struct window_t *s) {
  struct win32_window_t *win = (struct win32_window_t*)s->window;
  close_win32_window(win);
  WINDOW_SAFE_FREE(win);
  s->window = NULL;
}

bool closed(struct window_t *s) {
  return ((struct win32_window_t*)s->window)->closed;
}

bool closed_va(int n, ...) {
  va_list args;
  va_start(args, n);
  bool ret = true;
  for (int i = 0; i < n; ++i) {
    struct window_t *w = va_arg(args, struct window_t*);
    if (!((struct win32_window_t*)w->window)->closed) {
      ret = false;
      break;
    }
  }
  va_end(args);
  return ret;
}

bool closed_all() {
  return windows == NULL;
}

void cursor_lock(struct window_t *s, bool locked) {
  struct win32_window_t *win = (struct win32_window_t*)s->window;
  if (!s || !locked) {
    ClipCursor(NULL);
    win->cursor_locked = false;
  } else {
    clip_win32_cursor(win->hwnd);
    win->cursor_locked = true;
  }
}

void cursor_visible(struct window_t *s, bool shown) {
  ((struct win32_window_t*)s->window)->cursor_vis = shown;
}

void cursor_icon(struct window_t *s, enum cursor_type t) {
  HCURSOR tmp = NULL;
  switch (t) {
  default:
  case CURSOR_ARROW:
    tmp = LoadCursor(NULL, IDC_ARROW);
    break;
  case CURSOR_WAIT:
    tmp = LoadCursor(NULL, IDC_WAIT);
    break;
  case CURSOR_WAITARROW:
    tmp = LoadCursor(NULL, IDC_APPSTARTING);
    break;
  case CURSOR_IBEAM:
    tmp = LoadCursor(NULL, IDC_IBEAM);
    break;
  case CURSOR_CROSSHAIR:
    tmp = LoadCursor(NULL, IDC_CROSS);
    break;
  case CURSOR_SIZENWSE:
    tmp = LoadCursor(NULL, IDC_SIZENWSE);
    break;
  case CURSOR_SIZENESW:
    tmp = LoadCursor(NULL, IDC_SIZENESW);
    break;
  case CURSOR_SIZEWE:
    tmp = LoadCursor(NULL, IDC_SIZENWSE);
    break;
  case CURSOR_SIZENS:
    tmp = LoadCursor(NULL, IDC_SIZENS);
    break;
  case CURSOR_SIZEALL:
    tmp = LoadCursor(NULL, IDC_SIZEALL);
    break;
  case CURSOR_NO:
    tmp = LoadCursor(NULL, IDC_NO);
    break;
  case CURSOR_HAND:
    tmp = LoadCursor(NULL, IDC_HAND);
    break;
  }
  struct win32_window_t *win = (struct win32_window_t*)s->window;
  if (win->cursor && win->custom_cursor)
    DeleteObject(win->cursor);
  win->custom_cursor = false;
  win->cursor = tmp;
  SetCursor(win->cursor);
}

void cursor_icon_custom(struct window_t *s, struct surface_t *b) {
  struct win32_window_t *win = (struct win32_window_t*)s->window;
  HBITMAP hbmp = NULL, bmp_mask = NULL;

  if (!(hbmp = CreateBitmap(b->w, b->h, 1, 32, b->buf)))
    goto FAILED;
  if (!(bmp_mask = CreateCompatibleBitmap(GetDC(NULL), b->w, b->h)))
    goto FAILED;

  ICONINFO ii;
  ii.fIcon = TRUE;
  ii.xHotspot = 0;
  ii.yHotspot = -b->h;
  ii.hbmMask = bmp_mask;
  ii.hbmColor = hbmp;
  win->cursor = (HCURSOR)CreateIconIndirect(&ii);

FAILED:
  if (bmp_mask)
    DeleteObject(bmp_mask);
  if (hbmp)
    DeleteObject(hbmp);

  if (!win->cursor) {
    windows_error(WINDOW_ICON_FAILED, "cursor_icon_custom() failed");
    win->icon = LoadCursor(NULL, IDC_ARROW);
    win->custom_cursor = false;
  } else
    win->custom_icon = true;
  
  SetCursor(win->cursor);
}

void cursor_pos(int *x, int *y) {
  static POINT p;
  GetCursorPos(&p);
  if (x)
    *x = p.x;
  if (y)
    *y = p.y;
}

void cursor_set_pos(int x, int y) {
  SetCursorPos(x, y);
}

void events() {
  static MSG msg;
  ZeroMemory(&msg, sizeof(MSG));
  if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }
}

void flush(struct window_t *s, struct surface_t *b) {
  if (!s)
    return;
  struct win32_window_t *tmp = (struct win32_window_t*)s->window;
  if (!tmp || tmp->closed)
    return;
  tmp->buffer = b;
  InvalidateRect(tmp->hwnd, NULL, TRUE);
  SendMessage(tmp->hwnd, WM_PAINT, 0, 0);
}

void release() {
  struct window_node_t *tmp = NULL, *cursor = windows;
  while (cursor) {
    tmp = cursor->next;
    close_win32_window(tmp->data);
    WINDOW_SAFE_FREE(cursor->data);
    WINDOW_SAFE_FREE(cursor);
    cursor = tmp;
  }
}
