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

/* N3DS SDL video driver implementation; this is just enough to make an
 *  SDL-based application THINK it's got a working video driver, for
 *  applications that call SDL_Init(SDL_INIT_VIDEO) when they don't need it,
 *  and also for use as a collection of stubs when porting SDL to a new
 *  platform for which you haven't yet written a valid video driver.
 *
 * This is also a great way to determine bottlenecks: if you think that SDL
 *  is a performance problem for a given platform, enable this driver, and
 *  then see if your application runs faster without video overhead.
 *
 * Initial work by Ryan C. Gordon (icculus@icculus.org). A good portion
 *  of this was cut-and-pasted from Stephane Peter's work in the AAlib
 *  SDL video driver.  Renamed to "N3DS" by Sam Lantinga.
 */

#include <3ds.h>
#include <citro3d.h>
#include <string.h>
#include "vshader_shbin.h"

#include "SDL_video.h"
#include "SDL_mouse.h"
#include "../SDL_sysvideo.h"
#include "../SDL_pixels_c.h"
#include "../../events/SDL_events_c.h"

#include "SDL_n3dsvideo.h"
#include "SDL_n3dsevents_c.h"
#include "SDL_n3dsmouse_c.h"

#define N3DSVID_DRIVER_NAME "N3DS"

#define CLEAR_COLOR 0x68B0D8FF

#define DISPLAY_TRANSFER_FLAGS \
	(GX_TRANSFER_FLIP_VERT(0) | GX_TRANSFER_OUT_TILED(0) | GX_TRANSFER_RAW_COPY(0) | \
	GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8) | GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGB8) | \
	GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO))

typedef struct { float position[3]; float texcoord[2]; float normal[3]; } vertex;

vertex *vertex_list;

static DVLB_s* vshader_dvlb;
static shaderProgram_s program;
static int uLoc_projection;
static C3D_Mtx projection;

static void* vbo_data;

/* Initialization/Query functions */
static int N3DS_VideoInit(_THIS, SDL_PixelFormat *vformat);
static SDL_Rect **N3DS_ListModes(_THIS, SDL_PixelFormat *format, Uint32 flags);
static SDL_Surface *N3DS_SetVideoMode(_THIS, SDL_Surface *current, int width, int height, int bpp, Uint32 flags);
static int N3DS_SetColors(_THIS, int firstcolor, int ncolors, SDL_Color *colors);
static void N3DS_VideoQuit(_THIS);

/* Hardware surface functions */
static int N3DS_AllocHWSurface(_THIS, SDL_Surface *surface);
static int N3DS_LockHWSurface(_THIS, SDL_Surface *surface);
static void N3DS_UnlockHWSurface(_THIS, SDL_Surface *surface);
static void N3DS_FreeHWSurface(_THIS, SDL_Surface *surface);

/* etc. */
static void N3DS_UpdateRects(_THIS, int numrects, SDL_Rect *rects);

/* N3DS driver bootstrap functions */

static int N3DS_Available(void)
{
	return 1;
}

static void N3DS_DeleteDevice(SDL_VideoDevice *device)
{
	SDL_free(device->hidden);
	SDL_free(device);
}

static SDL_VideoDevice *N3DS_CreateDevice(int devindex)
{
	SDL_VideoDevice *device;

	/* Initialize all variables that we clean on shutdown */
	device = (SDL_VideoDevice *)SDL_malloc(sizeof(SDL_VideoDevice));
	if ( device ) {
		SDL_memset(device, 0, (sizeof *device));
		device->hidden = (struct SDL_PrivateVideoData *)
				SDL_malloc((sizeof *device->hidden));
	}
	if ( (device == NULL) || (device->hidden == NULL) ) {
		SDL_OutOfMemory();
		if ( device ) {
			SDL_free(device);
		}
		return(0);
	}
	SDL_memset(device->hidden, 0, (sizeof *device->hidden));

	/* Set the function pointers */
	device->VideoInit = N3DS_VideoInit;
	device->ListModes = N3DS_ListModes;
	device->SetVideoMode = N3DS_SetVideoMode;
	device->CreateYUVOverlay = NULL;
	device->SetColors = N3DS_SetColors;
	device->UpdateRects = N3DS_UpdateRects;
	device->VideoQuit = N3DS_VideoQuit;
	device->AllocHWSurface = N3DS_AllocHWSurface;
	device->CheckHWBlit = NULL;
	device->FillHWRect = NULL;
	device->SetHWColorKey = NULL;
	device->SetHWAlpha = NULL;
	device->LockHWSurface = N3DS_LockHWSurface;
	device->UnlockHWSurface = N3DS_UnlockHWSurface;
	device->FlipHWSurface = NULL;
	device->FreeHWSurface = N3DS_FreeHWSurface;
	device->SetCaption = NULL;
	device->SetIcon = NULL;
	device->IconifyWindow = NULL;
	device->GrabInput = NULL;
	device->GetWMInfo = NULL;
	device->InitOSKeymap = N3DS_InitOSKeymap;
	device->PumpEvents = N3DS_PumpEvents;

	device->free = N3DS_DeleteDevice;

	return device;
}

VideoBootStrap N3DS_bootstrap = {
	N3DSVID_DRIVER_NAME, "SDL N3DS video driver",
	N3DS_Available, N3DS_CreateDevice
};

static C3D_RenderBuf rb;

int N3DS_VideoInit(_THIS, SDL_PixelFormat *vformat)
{
	/* Determine the screen depth (use default 8-bit depth) */
	/* we change this during the SDL_SetVideoMode implementation... */
	vformat->BitsPerPixel = 24;
	vformat->BytesPerPixel = 3;
    
    vformat->Rmask = 0x00FF0000;
    vformat->Gmask = 0x0000FF00;
    vformat->Bmask = 0x000000FF;
    
	// Initialize graphics
	gfxInitDefault();
    
	C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);

	// Initialize the renderbuffer
	C3D_RenderBufInit(&rb, 240, 400, GPU_RB_RGBA8, GPU_RB_DEPTH24_STENCIL8);
	rb.clearColor = CLEAR_COLOR;
	C3D_RenderBufClear(&rb);
	C3D_RenderBufBind(&rb);

	// Load the vertex shader, create a shader program and bind it
	vshader_dvlb = DVLB_ParseFile((u32*)vshader_shbin, vshader_shbin_size);
	shaderProgramInit(&program);
	shaderProgramSetVsh(&program, &vshader_dvlb->DVLE[0]);
	C3D_BindProgram(&program);

	// Configure attributes for use with the vertex shader
	C3D_AttrInfo* attrInfo = C3D_GetAttrInfo();
	AttrInfo_Init(attrInfo);
	AttrInfo_AddLoader(attrInfo, 0, GPU_FLOAT, 3); // v0=position
	AttrInfo_AddLoader(attrInfo, 1, GPU_FLOAT, 2); // v1=texcoord
	AttrInfo_AddLoader(attrInfo, 2, GPU_FLOAT, 3); // v2=normal
    
    this->hidden->screen_seg_count = 0;
    
	/* We're done! */
	return 0;
}

SDL_Rect **N3DS_ListModes(_THIS, SDL_PixelFormat *format, Uint32 flags)
{
   	 return (SDL_Rect **) -1;
}

uint32_t nearest_pow(uint32_t num) 
{
    uint32_t n = num > 0 ? num - 1 : 0;

    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    n++;

    return n == num ? n : n >> 1;
}

SDL_Surface *N3DS_SetVideoMode(_THIS, SDL_Surface *current,
				int width, int height, int bpp, Uint32 flags)
{
    // Determine texture format for screen texture
    Uint32 Rmask, Gmask, Bmask;

    switch(bpp) 
    {
		case 16:
            this->hidden->buffer_format = GPU_RGB565;
            Rmask = 0x0000f800;
            Gmask = 0x000007e0;
            Bmask = 0x0000001f;
			break;
		case 24:
            this->hidden->buffer_format = GPU_RGB8;
            Rmask = 0x00FF0000;
            Gmask = 0x0000FF00;
            Bmask = 0x000000FF;
            break;
		case 32:
            this->hidden->buffer_format = GPU_RGBA8;
            Rmask = 0xFF000000;
            Gmask = 0x00FF0000;
            Bmask = 0x0000FF00;
            break;
		default:
            SDL_SetError("Invalid SetVideoMode bpp type");
			return NULL;
	}
    
    // TODO-lovemhz: Clean up any previous screen segments
    
	// Compute the projection matrix
	Mtx_OrthoTilt(&projection, 0.0, width, 0.0, height, 0.0, 1.0);    
    
    // Figure out how many screen segments we will need
    Uint32 temp_width  = width;
    Uint32 temp_height = height;    
    Uint32 vertex_x = 0, vertex_y = 0;    
    
    do { vertex_x++; } while(temp_width  -= nearest_pow(temp_width));    
    do { vertex_y++; } while(temp_height -= nearest_pow(temp_height));    
        
    // Allocate memory for our screen segment index
    this->hidden->screen_seg_count = vertex_x * vertex_y;    
    this->hidden->screen_segments  = malloc(this->hidden->screen_seg_count * sizeof(*this->hidden->screen_segments));    
    
    // Create VBO
	vbo_data = linearAlloc((vertex_x * vertex_y) * sizeof(vertex) * 6);    
    
    // Update our index of screen segements with information about each one
    Uint32 screen_index = 0;
    temp_height = height;
    vertex_y = 0;
    
    do { 
        temp_width = width;
        vertex_x = 0;
        do {
            screen_segment *segment = &this->hidden->screen_segments[screen_index];
            segment->x1 = width - temp_width;
            segment->y1 = height - temp_height;    
            segment->x2 = segment->x1 + nearest_pow(temp_width);
            segment->y2 = segment->y1 + nearest_pow(temp_height);
            segment->width  = segment->x2 - segment->x1;
            segment->height = segment->y2 - segment->y1;

            /* Create vertex array for this segment and copy it to the VOB. 
               Flip the Y axis due the 3DS screen rotation. */
            vertex screen_vertex[6] = { 
                SCREEN_VECTOR(segment->x1, height - segment->y2, segment->x2, height - segment->y1)
            };
            memcpy(vbo_data + (screen_index * sizeof(vertex) * 6), screen_vertex, sizeof(screen_vertex));    
            
            // Create screen front texture
            if(!C3D_TexInit(&segment->texture, segment->width, segment->height, this->hidden->buffer_format)) 
            {
                // TODO-lovemhz: Handle failure better
                SDL_SetError("Couldn't allocate buffer for requested mode");    
                return NULL;
            }
            
            // Set texture filtering
            C3D_TexSetFilter(&segment->texture, GPU_LINEAR, GPU_NEAREST);    
            
            // Clear texture
            SDL_memset(segment->texture.data, 0, segment->width * segment->height * (bpp / 8));
            
            vertex_x++;
            screen_index++;
        } while(temp_width -= nearest_pow(temp_width));    
            
        vertex_y++;
    } while(temp_height -= nearest_pow(temp_height));    
        
    // Configure buffers
	C3D_BufInfo* bufInfo = C3D_GetBufInfo();    
	BufInfo_Init(bufInfo);
	BufInfo_Add(bufInfo, vbo_data, sizeof(vertex), 3, 0x210);
    
    C3D_TexSetFilter(&this->hidden->screen_segments[0].texture, GPU_LINEAR, GPU_NEAREST);
    C3D_TexBind(0, &this->hidden->screen_segments[0].texture);
    
    // Set texture envoriment?
    C3D_TexEnv* env = C3D_GetTexEnv(0);
    C3D_TexEnvSrc(env, C3D_Both, GPU_TEXTURE0, 0, 0);
    C3D_TexEnvOp(env, C3D_Both, 0, 0, 0);
    C3D_TexEnvFunc(env, C3D_Both, GPU_REPLACE);
    
    // Create screen front SDL texture
    this->hidden->buffer = SDL_malloc(width * height * (bpp / 8));
	if(!this->hidden->buffer) 
    {
		SDL_SetError("Couldn't allocate buffer for requested mode");
		return NULL;
	}

    // Clear screen front SDL texture
	SDL_memset(this->hidden->buffer, 0, width * height * (bpp / 8));
    
	// Allocate the new pixel format for the screen
	if(!SDL_ReallocFormat(current, bpp, Rmask, Gmask, Bmask, 0))
    {
		C3D_TexDelete(&this->hidden->screen_texture_front);
		SDL_SetError("Couldn't allocate new pixel format for requested mode");
		return NULL;
	}

	// Set up the new mode framebuffer
	current->flags = flags & SDL_FULLSCREEN;
	this->hidden->w = current->w = width;
	this->hidden->h = current->h = height;
	this->hidden->pitch = current->pitch = current->w * (bpp / 8);
	current->pixels = this->hidden->buffer;

	// We're done
	return current;
}

/* We don't actually allow hardware surfaces other than the main one */
static int N3DS_AllocHWSurface(_THIS, SDL_Surface *surface)
{
	return -1;
}

static void N3DS_FreeHWSurface(_THIS, SDL_Surface *surface)
{
	return;
}

/* We need to wait for vertical retrace on page flipped displays */
static int N3DS_LockHWSurface(_THIS, SDL_Surface *surface)
{
	return 0;
}

static void N3DS_UnlockHWSurface(_THIS, SDL_Surface *surface)
{
	return;
}

// Grabbed from Citra Emulator (citra/src/video_core/utils.h)
static inline u32 morton_interleave(u32 x, u32 y)
{
	u32 i = (x & 7) | ((y & 7) << 8); // ---- -210
	i = (i ^ (i << 2)) & 0x1313;      // ---2 --10
	i = (i ^ (i << 1)) & 0x1515;      // ---2 -1-0
	i = (i | (i >> 7)) & 0x3F;
	return i;
}

// Grabbed from Citra Emulator (citra/src/video_core/utils.h)
static inline u32 get_morton_offset(u32 x, u32 y, u32 bytes_per_pixel)
{
    u32 i = morton_interleave(x, y);
    unsigned int offset = (x & ~7) * 8;
    return (i + offset) * bytes_per_pixel;
}

static void N3DS_UpdateRects(_THIS, int numrects, SDL_Rect *rects)
{
    // VSync
    C3D_VideoSync();
    hidScanInput();
    
    if(this->hidden->screen_seg_count == 0) 
        return;
        
	// Update the uniforms
	C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, uLoc_projection, &projection); 

	// Draw the VBO
    for(int s = 0; s < this->hidden->screen_seg_count; s++) {
        screen_segment *segment = &this->hidden->screen_segments[s];
            
        // Update texture
        void *segment_buffer = this->hidden->buffer + (segment->y1 * this->hidden->pitch) + (segment->x1 * 4);
        
        // This can probably be greatly optimized!
        for(int j = 0; j < segment->height; j++)
        {
            u32 coarse_y = j & ~7;
                
            for(int i = 0; i < segment->width; i++) 
            {
                u32 dst_offset = get_morton_offset(i, j, 4) + coarse_y * segment->width * 4;
                u32 v = ((u32 *)segment_buffer)[i + j * this->hidden->w];
                *(u32 *)(segment->texture.data + dst_offset) = v | 0x000000FF;
            }
        }

        C3D_TexBind(0, &segment->texture);
        C3D_DrawArrays(GPU_TRIANGLES, (s * 6), 6);
    }
    
    // Update Screen Buffer
    C3D_Flush();
    C3D_RenderBufTransfer(&rb, (u32*)gfxGetFramebuffer(GFX_TOP, GFX_LEFT, NULL, NULL), DISPLAY_TRANSFER_FLAGS);
    C3D_RenderBufClear(&rb);
}

int N3DS_SetColors(_THIS, int firstcolor, int ncolors, SDL_Color *colors)
{
	return 1;
}

/* Note:  If we are terminated, this could be called in the middle of
   another SDL video routine -- notably UpdateRects.
*/
void N3DS_VideoQuit(_THIS)
{
	if (this->screen->pixels != NULL)
	{
		SDL_free(this->screen->pixels);
		this->screen->pixels = NULL;
	}
}
