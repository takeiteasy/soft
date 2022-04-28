#include "window.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <ctype.h>
#if !defined(WINDOW_WINDOWS)
#include <unistd.h>
#endif
  
#if defined(__gnu_linux__) || defined(__linux__) || defined(__unix__)
#define WINDOW_LINUX
#elif defined(macintosh) || defined(Macintosh) || (defined(__APPLE__) && defined(__MACH__))
#define WINDOW_OSX
#elif defined(_WIN32) || defined(_WIN64) || defined(__WIN32__) || defined(__WINDOWS__)
#define WINDOW_WINDOWS
#else
#define WINDOW_NO_WINDOW
#endif

#if defined(WINDOW_MALLOC) && defined(WINDOW_FREE) && (defined(WINDOW_REALLOC) || defined(WINDOW_REALLOC_SIZED))
#elif !defined(WINDOW_MALLOC) && !defined(WINDOW_FREE) && !defined(WINDOW_REALLOC) && !defined(WINDOW_REALLOC_SIZED)
#else
#error "Must define all or none of WINDOW_MALLOC, WINDOW_FREE, and WINDOW_REALLOC (or WINDOW_REALLOC_SIZED)."
#endif
  
#if defined(DEBUG) && !defined(WINDOW_DEBUG)
#define WINDOW_DEBUG
#endif

#if !defined(WINDOW_MALLOC)
#define WINDOW_MALLOC(sz)       malloc(sz)
#define WINDOW_REALLOC(p,newsz) realloc(p,newsz)
#define WINDOW_FREE(p)          free(p)
#endif
#define WINDOW_SAFE_FREE(x) \
if ((x)) { \
  free((void*)(x)); \
  (x) = NULL; \
}

static short keycodes[512];
static bool keycodes_init = false;

void window_set_parent(struct window_t *s, void *p) {
  s->parent = p;
}

void* window_parent(struct window_t *s) {
  return s->parent;
}

#define X(a, b) void(*a##_cb)b,
void window_callbacks(XMAP_SCREEN_CB struct window_t *window) {
#undef X
#define X(a, b) window->a##_callback = a##_cb;
  XMAP_SCREEN_CB
#undef X
}

#define X(a, b) \
void a##_callback(struct window_t *window, void(*a##_cb)b) { \
  window->a##_callback = a##_cb; \
}
XMAP_SCREEN_CB
#undef X

int window_id(struct window_t *s) {
  return s->id;
}

void window_size(struct window_t *s, int *w, int *h) {
  if (w)
    *w = s->w;
  if (h)
    *h = s->h;
}

#define CBCALL(x, ...) \
  if (e_window && e_window->x) \
    e_window->x(e_window->parent, __VA_ARGS__);

// Taken from: https://stackoverflow.com/a/1911632
#if _MSC_VER
#define STRINGISE_IMPL(x) #x
#define STRINGISE(x) STRINGISE_IMPL(x)
#define FILE_LINE_LINK __FILE__ "(" STRINGISE(__LINE__) ") : "
#define WARN(exp) (FILE_LINE_LINK "WARNING: " exp)
#else
#define WARN(exp) ("WARNING: " exp)
#endif

#define WINDOW_ERROR(A, ...) window_error((A), __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)

#define LINKEDLIST(NAME, TYPE) \
struct NAME##_node_t { \
  TYPE *data; \
  struct NAME##_node_t *next; \
}; \
struct NAME##_node_t* NAME##_push(struct NAME##_node_t *head, TYPE *data) { \
  struct NAME##_node_t *ret = WINDOW_MALLOC(sizeof(struct NAME##_node_t)); \
  if (!ret) \
    return NULL; \
  ret->data = data; \
  ret->next = head; \
  return ret; \
} \
struct NAME##_node_t* NAME##_pop(struct NAME##_node_t *head, TYPE *data) { \
  struct NAME##_node_t *cursor = head, *prev = NULL; \
  while (cursor) { \
    if (cursor->data != data) { \
      prev = cursor; \
      cursor = cursor->next; \
      continue; \
    } \
    if (!prev) \
      head = cursor->next; \
    else \
      prev->next = cursor->next; \
    break; \
  } \
  if (cursor) { \
    cursor->next = NULL; \
    WINDOW_FREE(cursor); \
  } \
  return head; \
}

static void(*__error_callback)(enum window_error, const char*, const char*, const char*, int) = NULL;

void window_error_callback(void(*cb)(enum window_error, const char*, const char*, const char*, int)) {
  __error_callback = cb;
}

void window_error(enum window_error type, const char *file, const char *func, int line, const char *msg, ...) {
  va_list args;
  va_start(args, msg);
  static char error[1024];
  vsprintf((char*)error, msg, args);
  va_end(args);
  
#if defined(WINDOW_DEBUG)
  fprintf(stderr, "[%d] from %s in %s() at %d -- %s\n", type, file, func, line, error);
#endif
  if (__error_callback) {
    __error_callback(type, (const char*)error, __FILE__, __FUNCTION__, __LINE__);
    return;
  }
}
