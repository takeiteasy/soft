#include "surface.h"
#include "window.h"
#include <stdio.h> // printf()
#include <stdlib.h> // abort()

void error_cb(enum window_error e, const char *file, const char *func, const char *msg, int line) {
    printf("ERROR! (%s, %s:%d) %s\n", file, func, line, msg);
    abort();
}

int main(int argc, const char *argv[]) {
    struct window_t w;
    window(&w, "soft example", 640, 480, NONE);
    window_error_callback(error_cb);
    
    struct surface_t s;
    surface_create(&s, 640, 480);
    surface_fill(&s, RED);
    
    while (!closed(&w)) {
        events();
        flush(&w, &s);
    }
    release();
    return 0;
}
