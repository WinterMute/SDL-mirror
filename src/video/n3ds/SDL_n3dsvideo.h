/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997-2012 Sam Lantinga

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

    Sam Lantinga
    slouken@libsdl.org
*/
#include "SDL_config.h"

#ifndef _SDL_n3dsvideo_h
#define _SDL_n3dsvideo_h

#include <3ds.h>
#include <citro3d.h>

#include "SDL_mouse.h"
#include "../SDL_sysvideo.h"

/* Hidden "this" pointer for the video functions */
#define _THIS	SDL_VideoDevice *this

#define SCREEN_VECTOR(x1, y1, x2, y2) \
    { { x2, y2, 1.0f}, {1.0f, 1.0f}, {0.0f, 0.0f, +1.0f} }, \
    { { x1, y2, 1.0f}, {0.0f, 1.0f}, {0.0f, 0.0f, +1.0f} }, \
    { { x1, y1, 1.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, +1.0f} }, \
    { { x1, y1, 1.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, +1.0f} }, \
    { { x2, y1, 1.0f}, {1.0f, 0.0f}, {0.0f, 0.0f, +1.0f} }, \
    { { x2, y2, 1.0f}, {1.0f, 1.0f}, {0.0f, 0.0f, +1.0f} }

typedef struct { 
    uint32_t x1;
    uint32_t y1;
    uint32_t x2;
    uint32_t y2;
    uint32_t width;
    uint32_t height;
    C3D_Tex texture;
} screen_segment;

/* Private display data */
struct SDL_PrivateVideoData {
    int w, h, pitch;
    void *buffer;
    short ndsmode;
    short secondbufferallocd;
    
    C3D_Tex screen_texture_front;
    int buffer_format;
    
    screen_segment *screen_segments;
    int screen_seg_count;
};

#endif /* _SDL_ndsvideo_h */
