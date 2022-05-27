/* surface.h
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

#ifndef surface_h
#define surface_h
#if defined(__cplusplus)
extern "C" {
#endif

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
 * @discussion Convert RGBA to packed integer
 * @param r R channel
 * @param g G channel
 * @param b B channel
 * @param a A channel
 * @return Packed RGBA colour
 */
int rgba(unsigned char r, unsigned char g, unsigned char b, unsigned char a);
/*!
 * @discussion Convert RGB to packed integer
 * @param r R channel
 * @param g G channel
 * @param b B channel
 * @return Packed RGB colour
 */
int rgb(unsigned char r, unsigned char g, unsigned char b);
/*!
 * @discussion Convert channel to packed RGBA integer
 * @param c R channel
 * @return Packed RGBA colour
 */
int rgba1(unsigned char c);
/*!
 * @discussion Convert channel to packed RGB integer
 * @param c R channel
 * @return Packed RGBA colour
 */
int rgb1(unsigned char c);
/*!
 * @discussion Retrieve R channel from packed RGBA integer
 * @param c Packed RGBA integer
 * @return R channel value
 */
unsigned char r_channel(int c);
/*!
 * @discussion Retrieve G channel from packed RGBA integer
 * @param c Packed RGBA integer
 * @return G channel value
 */
unsigned char g_channel(int c);
/*!
 * @discussion Retrieve B channel from packed RGBA integer
 * @param c Packed RGBA integer
 * @return B channel value
 */
unsigned char b_channel(int c);
/*!
 * @discussion Retrieve A channel from packed RGBA integer
 * @param c Packed RGBA integer
 * @return A channel value
 */
unsigned char a_channel(int c);
/*!
 * @discussion Modify R channel of packed RGBA integer
 * @param c Packed RGBA integer
 * @param r New R channel
 * @return Packed RGBA integer
 */
int rgba_r(int c, unsigned char r);
/*!
 * @discussion Modify G channel of packed RGBA integer
 * @param c Packed RGBA integer
 * @param g New G channel
 * @return Packed RGBA integer
 */
int rgba_g(int c, unsigned char g);
/*!
 * @discussion Modify B channel of packed RGBA integer
 * @param c Packed RGBA integer
 * @param b New B channel
 * @return Packed RGBA integer
 */
int rgba_b(int c, unsigned char b);
/*!
 * @discussion Modify A channel of packed RGBA integer
 * @param c Packed RGBA integer
 * @param a New A channel
 * @return Packed RGBA integer
 */
int rgba_a(int c, unsigned char a);

/*!
 * @typedef colours
 * @brief A list of colours with names
 */
typedef enum {
    BLACK = -16777216,
    BLUE = -16776961,
    CYAN = -16711681,
    GRAY = -8355712,
    GREEN = -16744448,
    LIME = -16711936,
    MAGENTA = -65281,
    MAROON = -8388608,
    NAVY = -16777088,
    PURPLE = -8388480,
    RED = -65536,
    TEAL = -16744320,
    WHITE = -1,
    YELLOW = -256,
    
    ALICE_BLUE = -984833,
    ANTIQUE_WHITE = -332841,
    AQUA = -16711681,
    AQUA_MARINE = -8388652,
    AZURE = -983041,
    BEIGE = -657956,
    BISQUE = -6972,
    BLANCHED_ALMOND = -5171,
    BLUE_VIOLET = -7722014,
    BROWN = -5952982,
    BURLY_WOOD = -2180985,
    CADET_BLUE = -10510688,
    CHART_REUSE = -8388864,
    CHOCOLATE = -2987746,
    CORAL = -32944,
    CORN_FLOWER_BLUE = -10185235,
    CORN_SILK = -1828,
    CRIMSON = -2354116,
    DARK_BLUE = -16777077,
    DARK_CYAN = -16741493,
    DARK_GOLDEN_ROD = -4684277,
    DARK_GRAY = -5658199,
    DARK_GREEN = -16751616,
    DARK_KHAKI = -4343957,
    DARK_MAGENTA = -7667573,
    DARK_OLIVE_GREEN = -11179217,
    DARK_ORANGE = -29696,
    DARK_ORCHID = -6737204,
    DARK_RED = -7667712,
    DARK_SALMON = -1468806,
    DARK_SEA_GREEN = -7357297,
    DARK_SLATE_BLUE = -12042869,
    DARK_SLATE_GRAY = -13676721,
    DARK_TURQUOISE = -16724271,
    DARK_VIOLET = -7077677,
    DEEP_PINK = -60269,
    DEEP_SKY_BLUE = -16728065,
    DIM_GRAY = -9868951,
    DODGER_BLUE = -14774017,
    FIREBRICK = -5103070,
    FLORAL_WHITE = -1296,
    FOREST_GREEN = -14513374,
    GAINSBORO = -2302756,
    GHOST_WHITE = -460545,
    GOLD = -10496,
    GOLDEN_ROD = -2448096,
    GREEN_YELLOW = -5374161,
    HONEYDEW = -983056,
    HOT_PINK = -38476,
    INDIAN_RED = -3318692,
    INDIGO = -11861886,
    IVORY = -16,
    KHAKI = -989556,
    LAVENDER = -1644806,
    LAVENDER_BLUSH = -3851,
    LAWN_GREEN = -8586240,
    LEMON_CHIFFON = -1331,
    LIGHT_BLUE = -5383962,
    LIGHT_CORAL = -1015680,
    LIGHT_CYAN = -2031617,
    LIGHT_GOLDEN_ROD = -329006,
    LIGHT_GRAY = -2894893,
    LIGHT_GREEN = -7278960,
    LIGHT_PINK = -18751,
    LIGHT_SALMON = -24454,
    LIGHT_SEA_GREEN = -14634326,
    LIGHT_SKY_BLUE = -7876870,
    LIGHT_SLATE_GRAY = -8943463,
    LIGHT_STEEL_BLUE = -5192482,
    LIGHT_YELLOW = -32,
    LIME_GREEN = -13447886,
    LINEN = -331546,
    MEDIUM_AQUA_MARINE = -10039894,
    MEDIUM_BLUE = -16777011,
    MEDIUM_ORCHID = -4565549,
    MEDIUM_PURPLE = -7114533,
    MEDIUM_SEA_GREEN = -12799119,
    MEDIUM_SLATE_BLUE = -8689426,
    MEDIUM_SPRING_GREEN = -16713062,
    MEDIUM_TURQUOISE = -12004916,
    MEDIUM_VIOLET_RED = -3730043,
    MIDNIGHT_BLUE = -15132304,
    MINT_CREAM = -655366,
    MISTY_ROSE = -6943,
    MOCCASIN = -6987,
    NAVAJO_WHITE = -8531,
    OLD_LACE = -133658,
    OLIVE_DRAB = -9728477,
    ORANGE = -23296,
    ORANGE_RED = -47872,
    ORCHID = -2461482,
    PALE_GOLDEN_ROD = -1120086,
    PALE_GREEN = -6751336,
    PALE_TURQUOISE = -5247250,
    PALE_VIOLET_RED = -2396013,
    PAPAYA_WHIP = -4139,
    PEACH_PUFF = -9543,
    PERU = -3308225,
    PINK = -16181,
    PLUM = -2252579,
    POWDER_BLUE = -5185306,
    ROSY_BROWN = -4419697,
    ROYAL_BLUE = -12490271,
    SADDLE_BROWN = -7650029,
    SALMON = -360334,
    SANDY_BROWN = -744352,
    SEA_GREEN = -13726889,
    SEA_SHELL = -2578,
    SIENNA = -6270419,
    SKY_BLUE = -7876885,
    SLATE_BLUE = -9807155,
    SLATE_GRAY = -9404272,
    SNOW = -1286,
    SPRING_GREEN = -16711809,
    STEEL_BLUE = -12156236,
    TAN = -2968436,
    THISTLE = -2572328,
    TOMATO = -40121,
    TURQUOISE = -12525360,
    VIOLET = -1146130,
    WHEAT = -663885,
    WHITE_SMOKE = -657931,
    YELLOW_GREEN = -6632142
} Colour;

/*!
 * @typedef Surface
 * @brief An object to hold image data
 * @constant buf Buffer holding pixel data
 * @constant w Width of image
 * @constant h Height of image
 */
typedef struct {
    int *buf, w, h;
} Surface;

/*!
 * @discussion Create a new surface
 * @param s Pointer to surface object to create
 * @param w Width of new surface
 * @param h Height of new surface
 * @return Boolean for success
 */
bool NewSurface(Surface* s, unsigned int w, unsigned int h);
/*!
 * @discussion Destroy a surface
 * @param s Pointer to pointer to surface object
 */
void DestroySurface(Surface* s);

/*!
 * @discussion Fill a surface with a given colour
 * @param s Surface object
 * @param col Colour to set
 */
void FillSurface(Surface* s, int col);
/*!
 * @discussion Flood portion of surface with given colour
 * @param s Surface object
 * @param x X position
 * @param y Y position
 * @param col Colour to set
 */
void FloodSurface(Surface* s, int x, int y, int col);
/*!
 * @discussion Clear a surface, zero the buffer
 * @param s Surface object
 */
void ClearSurface(Surface *s);
/*!
 * @discussion Set surface pixel colour
 * @param s Surface object
 * @param x X position
 * @param y Y position
 * @param col Colour to set
 */
void BlendPixel(Surface *s, int x, int y, int col);
/*!
 * @discussion Set surface pixel colour (without blending)
 * @param s Surface object
 * @param x X position
 * @param y Y position
 * @param col Colour to set
 */
void SetPixel(Surface *s, int x, int y, int col);
/*!
 * @discussion Get surface pixel colour
 * @param s Surface object
 * @param x X position
 * @param y Y position
 * @return Pixel colour
 */
int GetPixel(Surface *s, int x, int y);
/*!
 * @discussion Blit one surface onto another at point
 * @param dst Surface to blit to
 * @param src Surface to blit
 * @param x X position
 * @param y Y position
 * @return Boolean of success
 */
bool PasteSurface(Surface *dst, Surface *src, int x, int y);
/*!
 * @discussion Blit one surface onto another at point with clipping rect
 * @param dst Surface to blit to
 * @param src Surface to blit
 * @param x X position
 * @param y Y position
 * @param rx Clip rect X
 * @param ry Clip rect Y
 * @param rw Clip rect width
 * @param rh Clip rect height
 * @return Boolean of success
 */
bool PasteSurfaceClip(Surface *dst, Surface *src, int x, int y, int rx, int ry, int rw, int rh);
/*!
 * @discussion Reallocate a surface
 * @param s Surface object
 * @param nw New width
 * @param nh New height
 * @return Boolean of success
 */
bool ReuseSurface(Surface *s, int nw, int nh);
/*!
 * @discussion Create a copy of a surface
 * @param a Original surface object
 * @param b New surface object to be allocated
 * @return Boolean of success
 */
bool CopySurface(Surface *a, Surface *b);
/*!
 * @discussion Loop through each pixel of surface and run position and colour through a callback. Return value of the callback is the new colour at the position
 * @param s Surface object
 * @param fn Callback function
 */
void PassthruSurface(Surface *s, int(*fn)(int x, int y, int col));
/*!
 * @discussion Scale surface to given size
 * @param a Original surface object
 * @param nw New width
 * @param nh New height
 * @param b New surface object to be allocated
 * @return Boolean of success
 */
bool ScaleSurface(Surface *a, int nw, int nh, Surface *b);
/*!
 * @discussion Rotate a surface by a given degree
 * @param a Original surface object
 * @param angle Angle to rotate by
 * @param b New surface object to be allocated
 * @return Boolean of success
 */
bool RotateSurface(Surface *a, float angle, Surface *b);

/*!
 * @discussion Simple Bresenham line
 * @param s Surface object
 * @param x0 Vector A X position
 * @param y0 Vector A Y position
 * @param x1 Vector B X position
 * @param y1 Vector B Y position
 * @param col Colour of line
 */
void DrawLine(Surface *s, int x0, int y0, int x1, int y1, int col);
/*!
 * @discussion Draw a circle
 * @param s Surface object
 * @param xc Centre X position
 * @param yc Centre Y position
 * @param r Circle radius
 * @param col Colour of cricle
 * @param fill Fill circle boolean
 */
void DrawCircle(Surface *s, int xc, int yc, int r, int col, bool fill);
/*!
 * @discussion Draw a rectangle
 * @param x X position
 * @param y Y position
 * @param w Rectangle width
 * @param h Rectangle height
 * @param col Colour of rectangle
 * @param fill Fill rectangle boolean
 */
void DrawRect(Surface *s, int x, int y, int w, int h, int col, bool fill);
/*!
 * @discussion Draw a triangle
 * @param s Surface object
 * @param x0 Vector A X position
 * @param y0 Vector A Y position
 * @param x1 Vector B X position
 * @param y1 Vector B Y position
 * @param x2 Vector C X position
 * @param y2 Vector C Y position
 * @param col Colour of line
 * @param fill Fill triangle boolean
 */
void DrawTri(Surface *s, int x0, int y0, int x1, int y1, int x2, int y2, int col, bool fill);

#if defined(__cplusplus)
}
#endif
#endif // surface_h
