/* sftwrend.c
 *
 * Created by George Watson on 26/11/2017.
 * Copyright © 2013-2021 George Watson. All rights reserved.
 
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

#include "sftwrend.h"

#if defined(__EMSCRIPTEN__)
#include "emscripten.h"
#define EXPORT EMSCRIPTEN_KEEPALIVE
#else
#define EXPORT
#endif

EXPORT int rgba(unsigned char r, unsigned char g, unsigned char b, unsigned char a) {
    return ((unsigned int)a << 24) | ((unsigned int)r << 16) | ((unsigned int)g << 8) | b;
}

EXPORT int rgb(unsigned char r, unsigned char g, unsigned char b) {
    return rgba(r, g, b, 255);
}

EXPORT int rgba1(unsigned char c) {
    return rgba(c, c, c, c);
}

EXPORT int rgb1(unsigned char c) {
    return rgb(c, c, c);
}

EXPORT unsigned char r_channel(int c) {
    return (unsigned char)((c >> 16) & 0xFF);
}

EXPORT unsigned char g_channel(int c) {
    return (unsigned char)((c >>  8) & 0xFF);
}

EXPORT unsigned char b_channel(int c) {
    return (unsigned char)(c & 0xFF);
}

EXPORT unsigned char a_channel(int c) {
    return (unsigned char)((c >> 24) & 0xFF);
}

EXPORT int rgba_r(int c, unsigned char r) {
    return (c & ~0x00FF0000) | (r << 16);
}

EXPORT int rgba_g(int c, unsigned char g) {
    return (c & ~0x0000FF00) | (g << 8);
}

EXPORT int rgba_b(int c, unsigned char b) {
    return (c & ~0x000000FF) | b;
}

EXPORT int rgba_a(int c, unsigned char a) {
    return (c & ~0x00FF0000) | (a << 24);
}

EXPORT bool surface_create(struct surface_t *s, unsigned int w, unsigned int h) {
    s->w = w;
    s->h = h;
    size_t sz = w * h * sizeof(unsigned int) + 1;
    s->buf = malloc(sz);
    memset(s->buf, 0, sz);
    return true;
}

EXPORT void surface_destroy(struct surface_t *s) {
    if (s->buf)
        free(s->buf);
    memset(s, 0, sizeof(struct surface_t));
}

EXPORT void surface_fill(struct surface_t *s, int col) {
    for (int i = 0; i < s->w * s->h; ++i)
        s->buf[i] = col;
}

static inline void flood_fn(struct surface_t *s, int x, int y, int new, int old) {
    if (new == old || surface_pget(s, x, y) != old)
        return;
    
    int x1 = x;
    while (x1 < s->w && surface_pget(s, x1, y) == old) {
        surface_pset(s, x1, y, new);
        x1++;
    }
    
    x1 = x - 1;
    while (x1 >= 0 && surface_pget(s, x1, y) == old) {
        surface_pset(s, x1, y, new);
        x1--;
    }
    
    x1 = x;
    while (x1 < s->w && surface_pget(s, x1, y) == new) {
        if(y > 0 && surface_pget(s, x1, y - 1) == old)
            flood_fn(s, x1, y - 1, new, old);
        x1++;
    }
    
    x1 = x - 1;
    while(x1 >= 0 && surface_pget(s, x1, y) == new) {
        if(y > 0 && surface_pget(s, x1, y - 1) == old)
            flood_fn(s, x1, y - 1, new, old);
        x1--;
    }
    
    x1 = x;
    while(x1 < s->w && surface_pget(s, x1, y) == new) {
        if(y < s->h - 1 && surface_pget(s, x1, y + 1) == old)
            flood_fn(s, x1, y + 1, new, old);
        x1++;
    }
    
    x1 = x - 1;
    while(x1 >= 0 && surface_pget(s, x1, y) == new) {
        if(y < s->h - 1 && surface_pget(s, x1, y + 1) == old)
            flood_fn(s, x1, y + 1, new, old);
        x1--;
    }
}

EXPORT void surface_flood(struct surface_t *s, int x, int y, int col) {
    if (x < 0 || y < 0 || x >= s->w || y >= s->h)
        return;
    flood_fn(s, x, y, col, surface_pget(s, x, y));
}

EXPORT void surface_cls(struct surface_t *s) {
    memset(s->buf, 0, s->w * s->h * sizeof(int));
}

#define BLEND(c0, c1, a0, a1) (c0 * a0 / 255) + (c1 * a1 * (255 - a0) / 65025)

EXPORT void surface_pset(struct surface_t *s, int x, int y, int c) {
    unsigned char a = a_channel(c);
    if (!a || x < 0 || y < 0 || x >= s->w || y >= s->h)
        return;
    int *p = &s->buf[y * s->w + x];
    unsigned char b = a_channel(*p);
    *p = (a == 255 || !b) ? c : rgba(BLEND(r_channel(c), r_channel(*p), a, b),
                                     BLEND(g_channel(c), g_channel(*p), a, b),
                                     BLEND(b_channel(c), b_channel(*p), a, b),
                                     a + (b * (255 - a) >> 8));
}

EXPORT int surface_pget(struct surface_t *s, int x, int y) {
    return (x >= 0 && y >= 0 && x < s->w && y < s->h) ? s->buf[y * s->w + x] : 0;
}

EXPORT bool surface_paste(struct surface_t *dst, struct surface_t *src, int x, int y) {
    int ox, oy, c;
    for (ox = 0; ox < src->w; ++ox) {
        for (oy = 0; oy < src->h; ++oy) {
            if (oy > dst->h)
                break;
            c = surface_pget(src, ox, oy);
            surface_pset(dst, x + ox, y + oy, c);
        }
        if (ox > dst->w)
            break;
    }
    return true;
}

EXPORT bool surface_clip_paste(struct surface_t *dst, struct surface_t *src, int x, int y, int rx, int ry, int rw, int rh) {
    for (int ox = 0; ox < rw; ++ox)
        for (int oy = 0; oy < rh; ++oy)
            surface_pset(dst, ox + x, oy + y, surface_pget(src, ox + rx, oy + ry));
    return true;
}

EXPORT bool surface_reset(struct surface_t *s, int nw, int nh) {
    size_t sz = nw * nh * sizeof(unsigned int) + 1;
    int *tmp = realloc(s->buf, sz);
    s->buf = tmp;
    s->w = nw;
    s->h = nh;
    memset(s->buf, 0, sz);
    return true;
}

EXPORT bool surface_copy(struct surface_t *a, struct surface_t *b) {
    if (!surface_create(b, a->w, a->h))
        return false;
    memcpy(b->buf, a->buf, a->w * a->h * sizeof(unsigned int) + 1);
    return !!b->buf;
}

EXPORT void surface_passthru(struct surface_t *s, int (*fn)(int x, int y, int col)) {
    int x, y;
    for (x = 0; x < s->w; ++x)
        for (y = 0; y < s->h; ++y)
            s->buf[y * s->w + x] = fn(x, y, surface_pget(s, x, y));
}

EXPORT bool surface_resize(struct surface_t *a, int nw, int nh, struct surface_t *b) {
    if (!surface_create(b, nw, nh))
        return false;
    
    int x_ratio = (int)((a->w << 16) / b->w) + 1;
    int y_ratio = (int)((a->h << 16) / b->h) + 1;
    int x2, y2, i, j;
    for (i = 0; i < b->h; ++i) {
        int *t = b->buf + i * b->w;
        y2 = ((i * y_ratio) >> 16);
        int *p = a->buf + y2 * a->w;
        int rat = 0;
        for (j = 0; j < b->w; ++j) {
            x2 = (rat >> 16);
            *t++ = p[x2];
            rat += x_ratio;
        }
    }
    return true;
}

#define __MIN(a, b) (((a) < (b)) ? (a) : (b))
#define __MAX(a, b) (((a) > (b)) ? (a) : (b))
#define __CLAMP(x, low, high) (((x) > (high)) ? (high) : (((x) < (low)) ? (low) : (x)))

#define __PI 3.14159265358979323846264338327950288f
#define __D2R(a) ((a) * __PI / 180.0)
#define __R2D(a) ((a) * 180.0 / __PI)

EXPORT bool surface_rotate(struct surface_t *a, float angle, struct surface_t *b) {
    float theta = __D2R(angle);
    float c = cosf(theta), s = sinf(theta);
    float r[3][2] = {
        { -a->h * s, a->h * c },
        {  a->w * c - a->h * s, a->h * c + a->w * s },
        {  a->w * c, a->w * s }
    };
    
    float mm[2][2] = {{
        __MIN(0, __MIN(r[0][0], __MIN(r[1][0], r[2][0]))),
        __MIN(0, __MIN(r[0][1], __MIN(r[1][1], r[2][1])))
    }, {
        (theta > 1.5708  && theta < 3.14159 ? 0.f : __MAX(r[0][0], __MAX(r[1][0], r[2][0]))),
        (theta > 3.14159 && theta < 4.71239 ? 0.f : __MAX(r[0][1], __MAX(r[1][1], r[2][1])))
    }};
    
    int dw = (int)ceil(fabsf(mm[1][0]) - mm[0][0]);
    int dh = (int)ceil(fabsf(mm[1][1]) - mm[0][1]);
    if (!surface_create(b, dw, dh))
        return false;
    
    int x, y, sx, sy;
    for (x = 0; x < dw; ++x)
        for (y = 0; y < dh; ++y) {
            sx = ((x + mm[0][0]) * c + (y + mm[0][1]) * s);
            sy = ((y + mm[0][1]) * c - (x + mm[0][0]) * s);
            if (sx < 0 || sx >= a->w || sy < 0 || sy >= a->h)
                continue;
            surface_pset(b, x, y, surface_pget(a, sx, sy));
        }
    return true;
}

static inline void vline(struct surface_t *s, int x, int y0, int y1, int col) {
    if (y1 < y0) {
        y0 += y1;
        y1  = y0 - y1;
        y0 -= y1;
    }
    
    if (x < 0 || x >= s->w || y0 >= s->h)
        return;
    
    if (y0 < 0)
        y0 = 0;
    if (y1 >= s->h)
        y1 = s->h - 1;
    
    for(int y = y0; y <= y1; y++)
        surface_pset(s, x, y, col);
}

static inline void hline(struct surface_t *s, int y, int x0, int x1, int col) {
    if (x1 < x0) {
        x0 += x1;
        x1  = x0 - x1;
        x0 -= x1;
    }
    
    if (y < 0 || y >= s->h || x0 >= s->w)
        return;
    
    if (x0 < 0)
        x0 = 0;
    if (x1 >= s->w)
        x1 = s->w - 1;
    
    for(int x = x0; x <= x1; x++)
        surface_pset(s, x, y, col);
}

EXPORT void surface_line(struct surface_t *s, int x0, int y0, int x1, int y1, int col) {
    if (x0 == x1)
        vline(s, x0, y0, y1, col);
    if (y0 == y1)
        hline(s, y0, x0, x1, col);
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = (dx > dy ? dx : -dy) / 2;
    
    while (surface_pset(s, x0, y0, col), x0 != x1 || y0 != y1) {
        int e2 = err;
        if (e2 > -dx) { err -= dy; x0 += sx; }
        if (e2 <  dy) { err += dx; y0 += sy; }
    }
}

EXPORT void surface_circle(struct surface_t *s, int xc, int yc, int r, int col, bool fill) {
    int x = -r, y = 0, err = 2 - 2 * r; /* II. Quadrant */
    do {
        surface_pset(s, xc - x, yc + y, col);    /*   I. Quadrant */
        surface_pset(s, xc - y, yc - x, col);    /*  II. Quadrant */
        surface_pset(s, xc + x, yc - y, col);    /* III. Quadrant */
        surface_pset(s, xc + y, yc + x, col);    /*  IV. Quadrant */
        
        if (fill) {
            hline(s, yc - y, xc - x, xc + x, col);
            hline(s, yc + y, xc - x, xc + x, col);
        }
        
        r = err;
        if (r <= y)
            err += ++y * 2 + 1; /* e_xy+e_y < 0 */
        if (r > x || err > y)
            err += ++x * 2 + 1; /* e_xy+e_x > 0 or no 2nd y-step */
    } while (x < 0);
}

EXPORT void surface_rect(struct surface_t *s, int x, int y, int w, int h, int col, bool fill) {
    if (x < 0) {
        w += x;
        x  = 0;
    }
    if (y < 0) {
        h += y;
        y  = 0;
    }
    
    w += x;
    h += y;
    if (w < 0 || h < 0 || x > s->w || y > s->h)
        return;
    
    if (w > s->w)
        w = s->w;
    if (h > s->h)
        h = s->h;
    
    if (fill) {
        for (; y < h; ++y)
            hline(s, y, x, w, col);
    } else {
        hline(s, y, x, w, col);
        hline(s, h, x, w, col);
        vline(s, x, y, h, col);
        vline(s, w, y, h, col);
    }
}

#define GRAPHICS_SWAP(a, b) \
    do { \
        int temp = a; \
        a = b; \
        b = temp; \
    } while(0)

EXPORT void surface_tri(struct surface_t *s, int x0, int y0, int x1, int y1, int x2, int y2, int col, bool fill) {
    if (y0 ==  y1 && y0 ==  y2)
        return;
    if (fill) {
        if (y0 > y1) {
            GRAPHICS_SWAP(x0, x1);
            GRAPHICS_SWAP(y0, y1);
        }
        if (y0 > y2) {
            GRAPHICS_SWAP(x0, x2);
            GRAPHICS_SWAP(y0, y2);
        }
        if (y1 > y2) {
            GRAPHICS_SWAP(x1, x2);
            GRAPHICS_SWAP(y1, y2);
        }
        
        int total_height = y2 - y0, i, j;
        for (i = 0; i < total_height; ++i) {
            bool second_half = i > y1 - y0 || y1 == y0;
            int segment_height = second_half ? y2 - y1 : y1 - y0;
            float alpha = (float)i / total_height;
            float beta  = (float)(i - (second_half ? y1 - y0 : 0)) / segment_height;
            int ax = x0 + (x2 - x0) * alpha;
            int ay = y0 + (y2 - y0) * alpha;
            int bx = second_half ? x1 + (x2 - x1) : x0 + (x1 - x0) * beta;
            int by = second_half ? y1 + (y2 - y1) : y0 + (y1 - y0) * beta;
            if (ax > bx) {
                GRAPHICS_SWAP(ax, bx);
                GRAPHICS_SWAP(ay, by);
            }
            for (j = ax; j <= bx; ++j)
                surface_pset(s, j, y0 + i, col);
        }
    } else {
        surface_line(s, x0, y0, x1, y1, col);
        surface_line(s, x1, y1, x2, y2, col);
        surface_line(s, x2, y2, x0, y0, col);
    }
}
