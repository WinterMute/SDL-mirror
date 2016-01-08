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

#include "SDL_3dsvideo.h"

/* Variables and functions exported by SDL_sysevents.c to other parts 
   of the native video subsystem (SDL_sysvideo.c)
*/
extern void X3DS_InitOSKeymap(_THIS);
extern void X3DS_PumpEvents(_THIS);

#define X3DS_NUMKEYS 12

/*
#define N3DS_JOYPADREG 0x4000130
#define N3DS_JOYPAD (*(volatile Uint16*)N3DS_JOYPADREG)

#define N3DS_NUMKEYS 10
#define N3DS_KEYA (0)
#define N3DS_KEYB (1)
#define N3DS_KEYSEL (2)
#define N3DS_KEYSTART (3)
#define N3DS_KEYRIGHT (4)
#define N3DS_KEYLEFT (5)
#define N3DS_KEYUP (6)
#define N3DS_KEYDOWN (7)
#define N3DS_KEYR (8)
#define N3DS_KEYL (9)
*/
/* end of SDL_NDSevents_c.h ... */

