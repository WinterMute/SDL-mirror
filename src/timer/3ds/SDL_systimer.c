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

#include "SDL_thread.h"
#include "SDL_timer.h"
#include "SDL_error.h"
#include "../SDL_timer_c.h"

#include <3ds.h>

#define TICKS_PER_MSEC 268123.480

u64 SDL_start_tick = 0;

void SDL_StartTicks(void) 
{
    SDL_start_tick = svcGetSystemTick();
}

Uint32 SDL_GetTicks(void)
{
    u64 delta = svcGetSystemTick() - SDL_start_tick;
	return delta / TICKS_PER_MSEC;
}

void SDL_Delay(Uint32 ms)
{
    svcSleepThread(ms * 1000000);
}

/* This is only called if the event thread is not running */
int SDL_SYS_TimerInit(void)
{
	return 0;
}

void SDL_SYS_TimerQuit(void)
{
    
}

int SDL_SYS_StartTimer(void)
{
	SDL_SetError("Timers not implemented on 3DS");
	return -1;
}

void SDL_SYS_StopTimer(void)
{
    
}
