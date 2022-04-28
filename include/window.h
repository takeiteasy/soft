/* window.h
 *
 * Created by George Watson on 26/11/2017.
 * Copyright © 2017-2019 George Watson. All rights reserved.
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

#ifndef window_h
#define window_h
#if defined(__cplusplus)
extern "C" {
#endif
#include "surface.h"

#if defined(_MSC_VER)
#if !defined(bool)
#define bool int
#endif
#if !defined(true)
#define true 1
#endif
#if !defined(false)
#define false 0
#endif
#else
#include <stdbool.h>
#endif
#include <stdarg.h>

/*!
 * @discussion High precision timer
 * @return Number of CPU ticks
 */
unsigned long long ticks(void);

/*!
 * @typedef button
 * @brief A list of mouse buttons
 */
enum button {
  MOUSE_BTN_0 = 0,
  MOUSE_BTN_1,
  MOUSE_BTN_2,
  MOUSE_BTN_3,
  MOUSE_BTN_4,
  MOUSE_BTN_5,
  MOUSE_BTN_6,
  MOUSE_BTN_7,
  MOUSE_BTN_8
};

static const int MOUSE_LAST   = MOUSE_BTN_8;
static const int MOUSE_LEFT   = MOUSE_BTN_0;
static const int MOUSE_RIGHT  = MOUSE_BTN_1;
static const int MOUSE_MIDDLE = MOUSE_BTN_2;

/*!
 * @typedef key_sym
 * @brief A list of key symbols
 */
enum key_sym {
  KB_KEY_SPACE = 32,
  KB_KEY_APOSTROPHE = 39,
  KB_KEY_COMMA = 44,
  KB_KEY_MINUS = 45,
  KB_KEY_PERIOD = 46,
  KB_KEY_SLASH = 47,
  KB_KEY_0 = 48,
  KB_KEY_1 = 49,
  KB_KEY_2 = 50,
  KB_KEY_3 = 51,
  KB_KEY_4 = 52,
  KB_KEY_5 = 53,
  KB_KEY_6 = 54,
  KB_KEY_7 = 55,
  KB_KEY_8 = 56,
  KB_KEY_9 = 57,
  KB_KEY_SEMICOLON = 59,
  KB_KEY_EQUALS = 61,
  KB_KEY_A = 65,
  KB_KEY_B = 66,
  KB_KEY_C = 67,
  KB_KEY_D = 68,
  KB_KEY_E = 69,
  KB_KEY_F = 70,
  KB_KEY_G = 71,
  KB_KEY_H = 72,
  KB_KEY_I = 73,
  KB_KEY_J = 74,
  KB_KEY_K = 75,
  KB_KEY_L = 76,
  KB_KEY_M = 77,
  KB_KEY_N = 78,
  KB_KEY_O = 79,
  KB_KEY_P = 80,
  KB_KEY_Q = 81,
  KB_KEY_R = 82,
  KB_KEY_S = 83,
  KB_KEY_T = 84,
  KB_KEY_U = 85,
  KB_KEY_V = 86,
  KB_KEY_W = 87,
  KB_KEY_X = 88,
  KB_KEY_Y = 89,
  KB_KEY_Z = 90,
  KB_KEY_LEFT_BRACKET = 91,
  KB_KEY_BACKSLASH = 92,
  KB_KEY_RIGHT_BRACKET = 93,
  KB_KEY_GRAVE_ACCENT = 96,
  KB_KEY_WORLD_1 = 161,
  KB_KEY_WORLD_2 = 162,
  KB_KEY_ESCAPE = 256,
  KB_KEY_ENTER = 257,
  KB_KEY_TAB = 258,
  KB_KEY_BACKSPACE = 259,
  KB_KEY_INSERT = 260,
  KB_KEY_DELETE = 261,
  KB_KEY_RIGHT = 262,
  KB_KEY_LEFT = 263,
  KB_KEY_DOWN = 264,
  KB_KEY_UP = 265,
  KB_KEY_PAGE_UP = 266,
  KB_KEY_PAGE_DOWN = 267,
  KB_KEY_HOME = 268,
  KB_KEY_END = 269,
  KB_KEY_CAPS_LOCK = 280,
  KB_KEY_SCROLL_LOCK = 281,
  KB_KEY_NUM_LOCK = 282,
  KB_KEY_PRINT_SCREEN = 283,
  KB_KEY_PAUSE = 284,
  KB_KEY_F1 = 290,
  KB_KEY_F2 = 291,
  KB_KEY_F3 = 292,
  KB_KEY_F4 = 293,
  KB_KEY_F5 = 294,
  KB_KEY_F6 = 295,
  KB_KEY_F7 = 296,
  KB_KEY_F8 = 297,
  KB_KEY_F9 = 298,
  KB_KEY_F10 = 299,
  KB_KEY_F11 = 300,
  KB_KEY_F12 = 301,
  KB_KEY_F13 = 302,
  KB_KEY_F14 = 303,
  KB_KEY_F15 = 304,
  KB_KEY_F16 = 305,
  KB_KEY_F17 = 306,
  KB_KEY_F18 = 307,
  KB_KEY_F19 = 308,
  KB_KEY_F20 = 309,
  KB_KEY_F21 = 310,
  KB_KEY_F22 = 311,
  KB_KEY_F23 = 312,
  KB_KEY_F24 = 313,
  KB_KEY_F25 = 314,
  KB_KEY_KP_0 = 320,
  KB_KEY_KP_1 = 321,
  KB_KEY_KP_2 = 322,
  KB_KEY_KP_3 = 323,
  KB_KEY_KP_4 = 324,
  KB_KEY_KP_5 = 325,
  KB_KEY_KP_6 = 326,
  KB_KEY_KP_7 = 327,
  KB_KEY_KP_8 = 328,
  KB_KEY_KP_9 = 329,
  KB_KEY_KP_DECIMAL = 330,
  KB_KEY_KP_DIVIDE = 331,
  KB_KEY_KP_MULTIPLY = 332,
  KB_KEY_KP_SUBTRACT = 333,
  KB_KEY_KP_ADD = 334,
  KB_KEY_KP_ENTER = 335,
  KB_KEY_KP_EQUALS = 336,
  KB_KEY_LEFT_SHIFT = 340,
  KB_KEY_LEFT_CONTROL = 341,
  KB_KEY_LEFT_ALT = 342,
  KB_KEY_LEFT_SUPER = 343,
  KB_KEY_RIGHT_SHIFT = 344,
  KB_KEY_RIGHT_CONTROL = 345,
  KB_KEY_RIGHT_ALT = 346,
  KB_KEY_RIGHT_SUPER = 347,
  KB_KEY_MENU = 348
};

static const int KB_KEY_UNKNOWN = -1;
static const int KB_KEY_LAST = KB_KEY_MENU;

/*!
 * @typedef key_mod
 * @brief A list of key modifiers
 */
enum key_mod {
  KB_MOD_SHIFT = 0x0001,
  KB_MOD_CONTROL = 0x0002,
  KB_MOD_ALT = 0x0004,
  KB_MOD_SUPER = 0x0008,
  KB_MOD_CAPS_LOCK = 0x0010,
  KB_MOD_NUM_LOCK = 0x0020
};

#define XMAP_SCREEN_CB \
X(keyboard, (void*, enum key_sym, enum key_mod, bool)) \
X(mouse_button, (void*, enum button, enum key_mod, bool)) \
X(mouse_move, (void*, int, int, int, int)) \
X(scroll, (void*, enum key_mod, float, float)) \
X(focus, (void*, bool)) \
X(resize, (void*, int, int)) \
X(closed, (void*))

/*!
 * @typedef window_t
 * @brief An object to hold window data
 * @constant w Width of window
 * @constant h Height of window
 * @constant window Pointer to internet platform specific window data
 * @parent parent Pointer to userdata for callbacks
 */
struct window_t {
  int id, w, h;

#define X(a, b) void(*a##_callback)b;
  XMAP_SCREEN_CB
#undef X
  
  void *window, *parent;
};

/*!
 * @discussion Set "parent" for a window object. The parent pointer will be passed to window callbacks.
 * @param s Window object
 * @param p Pointer to parent
 */
void window_set_parent(struct window_t *s, void *p);
/*!
 * @discussion Get parent point from window object
 * @param s Window object
 * @return Point to parent
 */
void* window_parent(struct window_t *s);

#define X(a, b) \
void(*a##_cb)b,

void window_callbacks(XMAP_SCREEN_CB struct window_t *window);
#undef X
#define X(a, b) \
void a##_callback(struct window_t *window, void(*a##_cb)b);
XMAP_SCREEN_CB
#undef X

  /*!
   * @typedef cursor_type
   * @brief A list of default cursor icons
   */
enum cursor_type {
    CURSOR_ARROW,     // Arrow
    CURSOR_IBEAM,     // I-beam
    CURSOR_WAIT,      // Wait
    CURSOR_CROSSHAIR, // Crosshair
    CURSOR_WAITARROW, // Small wait cursor (or Wait if not available)
    CURSOR_SIZENWSE,  // Double arrow pointing northwest and southeast
    CURSOR_SIZENESW,  // Double arrow pointing northeast and southwest
    CURSOR_SIZEWE,    // Double arrow pointing west and east
    CURSOR_SIZENS,    // Double arrow pointing north and south
    CURSOR_SIZEALL,   // Four pointed arrow pointing north, south, east, and west
    CURSOR_NO,        // Slashed circle or crossbones
    CURSOR_HAND       // Hand
  };

/*!
 * @typedef WINDOW_FLAGS
 * @brief A list of window flag options
 */
enum window_flags {
  NONE = 0,
  RESIZABLE = 0x01,
  FULLSCREEN = 0x02,
  FULLSCREEN_DESKTOP = 0x04,
  BORDERLESS = 0x08,
  ALWAYS_ON_TOP = 0x10,
};

/*!
 * @discussion Create a new window object
 * @param s Window object to be allocated
 * @param t Window title
 * @param w Window width
 * @param h Window height
 * @param flags Window flags
 * @return Boolean of success
 */
bool window(struct window_t *s, const char *t, int w, int h, short flags);
/*!
 * @discussion Set window icon from surface object
 * @param s Window object
 * @param b Surface object
 */
void window_icon(struct window_t *s, struct surface_t *b);
/*!
 * @discussion Set window title
 * @param s Window object
 * @param t New title
 */
void window_title(struct window_t *s, const char *t);
/*!
 * @discussion Get the position of a window object
 * @param s Window object
 * @param x Pointer to int to set
 * @param y Pointer to int to set
 */
void window_position(struct window_t *s, int *x, int * y);
/*!
 * @discussion Get the size of the screen a window is on
 * @param s Window object
 * @param w Pointer to int to set
 * @param h Pointer to int to set
 */
void screen_size(struct window_t *s, int *w, int *h);
/*!
 * @discussion Destroy window object
 * @param s Window object
 */
void window_destroy(struct window_t *s);
/*!
 * @discussion Unique window ID for window object
 * @param s Window object
 * @return Unique ID of window object
 */
int window_id(struct window_t *s);
/*!
 * @discussion Get size of window
 * @param s Window object
 * @param w Pointer to int to set
 * @param h Pointer to int to set
 */
void window_size(struct window_t *s, int *w, int *h);
/*!
 * @discussion Check if a window is still open
 * @param s Window object
 * @return Boolean if window is open
 */
bool closed(struct window_t *s);
/*!
 * @discussion Check if n windows are still open
 * @param n Numbers of arguments
 * @param ... Window objects
 * @return Boolean if any window are still open
 */
bool closed_va(int n, ...);
/*!
 * @discussion Checks if any windows are still open
 * @return Boolean if any windows are still open
 */
bool closed_all(void);

/*!
 * @discussion Lock or unlock cursor movement to active window
 * @param locked Turn on or off
 */
void cursor_lock(struct window_t *s, bool locked);
/*!
 * @discussion Hide or show system cursor
 * @param show Hide or show
 */
void cursor_visible(struct window_t *s, bool show);
/*!
 * @discussion Change cursor icon to system icon
 * @param s Window object
 * @param t Type of cursor
 */
void cursor_icon(struct window_t *s, enum cursor_type t);
/*!
 * @discussion Change cursor icon to icon from surface object
 * @param s Window object
 * @param b Surface object
 */
void cursor_icon_custom(struct window_t *s, struct surface_t *b);
/*!
 * @discussion Get cursor position
 * @param x Integer to set
 * @param y Integer to set
 */
void cursor_pos(int *x, int *y);
/*!
 * @discussion Set cursor position
 * @param x X position
 * @param y Y position
 */
void cursor_set_pos(int x, int y);

/*!
 * @discussion Poll for window events
 */
void events(void);
/*!
 * @discussion Draw surface object to window
 * @param s Window object
 * @param b Surface object
 */
void flush(struct window_t *s, struct surface_t *b);
/*!
 * @discussion Release anything allocated by this library
 */
void release(void);

/*!
 * @typedef window_error
 * @brief A list of different error types the library can generate
 */
enum window_error {
  UNKNOWN_ERROR,
  OUT_OF_MEMEORY,
  FILE_OPEN_FAILED,
  INVALID_BMP,
  UNSUPPORTED_BMP,
  INVALID_PARAMETERS,
  CURSOR_MOD_FAILED,
  OSX_WINDOW_CREATION_FAILED,
  OSX_APPDEL_CREATION_FAILED,
  OSX_FULLSCREEN_FAILED,
  WIN_WINDOW_CREATION_FAILED,
  WIN_FULLSCREEN_FAILED,
  NIX_CURSOR_PIXMAP_ERROR,
  NIX_OPEN_DISPLAY_FAILED,
  NIX_WINDOW_CREATION_FAILED,
  WINDOW_ICON_FAILED,
  CUSTOM_CURSOR_NOT_CREATED
};

/*!
 * @discussion Callback for errors inside library
 * @param cb Function pointer to callback
 */
void window_error_callback(void(*cb)(enum window_error, const char*, const char*, const char*, int));

#if defined(__cplusplus)
}
#endif
#endif // window_h
