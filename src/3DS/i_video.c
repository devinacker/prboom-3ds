/* Emacs style mode select   -*- C++ -*-
 *-----------------------------------------------------------------------------
 *
 *
 *  PrBoom: a Doom port merged with LxDoom and LSDLDoom
 *  based on BOOM, a modified and improved DOOM engine
 *  Copyright (C) 1999 by
 *  id Software, Chi Hoang, Lee Killough, Jim Flynn, Rand Phares, Ty Halderman
 *  Copyright (C) 1999-2006 by
 *  Jess Haas, Nicolas Kalkhof, Colin Phipps, Florian Schulze
 *  Copyright 2005, 2006 by
 *  Florian Schulze, Colin Phipps, Neil Stevens, Andrey Budko
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 *  02111-1307, USA.
 *
 * DESCRIPTION:
 *  DOOM graphics stuff for 3DS
 *
 *-----------------------------------------------------------------------------
 */

#include <stdlib.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <3ds.h>

#include "m_argv.h"
#include "doomstat.h"
#include "doomdef.h"
#include "doomtype.h"
#include "v_video.h"
#include "r_draw.h"
#include "d_main.h"
#include "d_event.h"
#include "i_joy.h"
#include "i_video.h"
#include "z_zone.h"
#include "s_sound.h"
#include "sounds.h"
#include "w_wad.h"
#include "st_stuff.h"
#include "lprintf.h"

extern void M_QuitDOOM(int choice);

int use_doublebuffer = 1; // Included not to break m_misc, but not relevant to libctru
int use_fullscreen;
int desired_fullscreen;

////////////////////////////////////////////////////////////////////////////
// Input code
int             leds_always_off = 0; // Expected by m_misc, not relevant

// Mouse handling
extern int     usemouse;        // config file var
extern int     usejoystick;
static boolean mouse_enabled; // usemouse, but can be overriden by -nomouse
static boolean mouse_currently_grabbed;

/////////////////////////////////////////////////////////////////////////////////
// Keyboard handling

/////////////////////////////////////////////////////////////////////////////////
// Main input code

//
//  Translates the key currently in key
//

static int I_TranslateKey(uint32_t key)
{
  int rc = 0;

  switch (key) {
  case KEY_L: rc = 'l'; break; // 'L' key
  case KEY_R: rc = 'r'; break; // 'R' key
  case KEY_A: rc = 'a'; break; // 'A' key
  case KEY_B: rc = 'b'; break; // 'B' key
  case KEY_X: rc = 'x'; break; // 'X' key
  case KEY_Y: rc = 'y'; break; // 'Y' key
  case KEY_SELECT: rc = KEYD_SELECT; break;
  case KEY_START: rc = KEYD_START; break;
  case KEY_DRIGHT: rc = KEYD_DRIGHT; break;
  case KEY_DLEFT: rc = KEYD_DLEFT; break;
  case KEY_DUP: rc = KEYD_DUP; break;
  case KEY_DDOWN: rc = KEYD_DDOWN; break;
  case KEY_ZL: rc = KEYD_ZL; break;
  case KEY_ZR: rc = KEYD_ZR; break;
  case KEY_TOUCH: rc = KEYD_TOUCH; break;
  case KEY_CSTICK_RIGHT: rc = KEYD_CSTICK_RIGHT; break;
  case KEY_CSTICK_LEFT: rc = KEYD_CSTICK_LEFT; break;
  case KEY_CSTICK_UP: rc = KEYD_CSTICK_UP; break;
  case KEY_CSTICK_DOWN: rc = KEYD_CSTICK_DOWN; break;
  case KEY_CPAD_RIGHT: rc = KEYD_CPAD_RIGHT; break;
  case KEY_CPAD_LEFT: rc = KEYD_CPAD_LEFT; break;
  case KEY_CPAD_UP: rc = KEYD_CPAD_UP; break;
  case KEY_CPAD_DOWN: rc = KEYD_CPAD_DOWN; break;
  }

  return rc;

}

static void I_GetInput() {
	event_t event;
	event.data2 = event.data3 = 0;
	
	uint32_t down = hidKeysDown();
	uint32_t held = hidKeysHeld();
	uint32_t up = hidKeysUp();

	// iterate over other possible key values
	int i;
	for (i = 0; i < (usejoystick ? 28 : 32); i++) { // upper 4 bits (c-pad) may be treated as a joystick
		uint32_t key = 1<<i;
		
		if (down & key) {
			event.data1 = I_TranslateKey(key);
			event.type = ev_keydown;
			D_PostEvent(&event);
		} else if (up & key) {
			event.data1 = I_TranslateKey(key);
			event.type = ev_keyup;
			D_PostEvent(&event);
		}
	}

	// handle touch pad movement
	if (usemouse) {
		static int px = 0;
		static int py = 0;
		touchPosition touch;
		hidTouchRead(&touch);
		if (down & KEY_TOUCH) {
			px = touch.px;
			py = touch.py;
			
		} else if (held & KEY_TOUCH && (touch.px != px || touch.py != py)) {
			event.type = ev_mouse;
			event.data1 = 0;
			event.data2 = (touch.px - px) << 5;
			event.data3 = -(touch.py - py) << 5;
			D_PostEvent(&event);
			
			px = touch.px;
			py = touch.py;
		}
	}
}

//
// I_StartTic
//

void I_StartTic (void)
{
  hidScanInput();
  I_GetInput();
  I_PollJoystick();
}

//
// I_StartFrame
//
void I_StartFrame (void)
{
  // TODO: keep the automap enabled
  // automapmode |= am_active;
}

//
// I_InitInputs
//

static void I_InitInputs(void)
{
  mouse_enabled = 1;

  I_InitJoystick();
}
/////////////////////////////////////////////////////////////////////////////

// I_SkipFrame
//
// Returns true if it thinks we can afford to skip this frame

inline static boolean I_SkipFrame(void)
{
  static int frameno;

  frameno++;
  switch (gamestate) {
  case GS_LEVEL:
    if (!paused)
      return false;
  default:
    // Skip odd frames
    return (frameno & 1) ? true : false;
  }
}

///////////////////////////////////////////////////////////
// Palette stuff.
//

// Array of color structs used for setting the 256-colour palette
typedef struct { char r, g, b; } colour_t;
static colour_t *colours = NULL;
static colour_t *current_pal;

static void I_UploadNewPalette(int pal)
{
  // This is used to replace the current 256 colour cmap with a new one
  // Used by 256 colour PseudoColor modes
  static int cachedgamma;
  static size_t num_pals;

  if (V_GetMode() == VID_MODEGL)
    return;

  if ((colours == NULL) || (cachedgamma != usegamma)) {
    int pplump = W_GetNumForName("PLAYPAL");
    int gtlump = (W_CheckNumForName)("GAMMATBL",ns_prboom);
    register const byte * palette = W_CacheLumpNum(pplump);
    register const byte * const gtable = (const byte *)W_CacheLumpNum(gtlump) + 256*(cachedgamma = usegamma);
    register int i;

    num_pals = W_LumpLength(pplump) / (3*256);
    num_pals *= 256;

    if (!colours) {
      // First call - allocate and prepare colour array
      colours = malloc(sizeof(*colours)*num_pals);
    }

    // set the colormap entries
    for (i=0 ; (size_t)i<num_pals ; i++) {
      colours[i].r = gtable[palette[0]];
      colours[i].g = gtable[palette[1]];
      colours[i].b = gtable[palette[2]];
      palette += 3;
    }

    W_UnlockLumpNum(pplump);
    W_UnlockLumpNum(gtlump);
    num_pals/=256;
  }

#ifdef RANGECHECK
  if ((size_t)pal >= num_pals)
    I_Error("I_UploadNewPalette: Palette number out of range (%d>=%d)",
      pal, num_pals);
#endif

  current_pal = colours + pal*256;
}

//////////////////////////////////////////////////////////////////////////////
// Graphics API

void I_ShutdownGraphics(void)
{
	free(colours);
}

//
// I_UpdateNoBlit
//
void I_UpdateNoBlit (void)
{
}

//
// I_TranslateFrameBuffer
//
// TODO: allow copying partial screens (for top screen borders)
void I_TranslateFrameBuffer(unsigned scrn, gfxScreen_t scrndest, gfx3dSide_t side, int xoff) {
	
	if (scrn >= NUM_SCREENS)
		I_Error("I_TranslateFrameBuffer: invalid screen number %u", scrn);
	
	// this is really hacky and obviously doesn't account for the possibility of
	// different color depths, etc.
	uint16_t width, height;
	char *fb = gfxGetFramebuffer(scrndest, side, &width, &height);
	
	// remember that the 3DS framebuffer is rotated 90'
	if (width != screens[scrn].height)
		I_Error("I_TranslateFrameBuffer: screen height mismatch (%u != %u)", width, screens[scrn].height);
	
	// center screen horizontally
	if (xoff < 0) {
		xoff = (height - screens[scrn].width) / 2;
	}
	
	if (xoff > height - screens[scrn].width)
		I_Error("I_TranslateFrameBuffer: bad x-offset (%d > %u)", xoff, height - screens[scrn].width);
		
	uint16_t x, y;
	// do palette lookups and rotate image 90' into the framebuffer here
	char *src = screens[scrn].data;
	
	for (x = 0; x < width; x++) {
		for (y = xoff; y < screens[scrn].width + xoff; y++) {
			char *dest = fb + ((y * width + (width - x - 1)) * 3);
			char px = *src++;
			
			*dest++ = current_pal[px].b;
			*dest++ = current_pal[px].g;
			*dest++ = current_pal[px].r;
		}
	}
}

//
// I_ClearFrameBuffer
//
void I_ClearFrameBuffer(gfxScreen_t scrndest, gfx3dSide_t side) {
	uint16_t width, height;
	char *fb = gfxGetFramebuffer(scrndest, side, &width, &height);
	// assume 3 bytes/pixel
	memset(fb, 0, width * height * 3);
}

//
// I_FinishUpdate
//
static int newpal = 0;
#define NO_PALETTE_CHANGE 1000

void I_FinishUpdate (void)
{
  if (I_SkipFrame()) return;

	// TODO: use enums for screen numbers and apply them where appropriate
	// main game on top screen (TODO: both sides)
	I_TranslateFrameBuffer(0, GFX_TOP, GFX_LEFT, -1);
	// TODO: automap on bottom screen
	// I_TranslateFrameBuffer(0, GFX_BOTTOM, 0, -1);
	
	/* Update the display buffer (flipping video pages if supported)
	 * If we need to change palette, that implicitely does a flip */
	if (newpal != NO_PALETTE_CHANGE) {
		I_UploadNewPalette(newpal);
		newpal = NO_PALETTE_CHANGE;
	}
	
	gfxFlushBuffers();
	gfxSwapBuffers();
	gspWaitForVBlank();
}

//
// I_ScreenShot - moved to i_sshot.c
//

//
// I_SetPalette
//
void I_SetPalette (int pal)
{
  newpal = pal;
}

// I_PreInitGraphics

void I_PreInitGraphics(void)
{
  // currently done in main
  // gfxInitDefault();
  
  /* TODO for 3DS */
}

// CPhipps -
// I_CalculateRes
// Calculates the screen resolution, possibly using the supplied guide
void I_CalculateRes(unsigned int width, unsigned int height)
{
  SCREENWIDTH = (width+15) & ~15;
  SCREENHEIGHT = height;
  if (!(SCREENWIDTH % 1024)) {
    SCREENPITCH = SCREENWIDTH*V_GetPixelDepth()+32;
  } else {
    SCREENPITCH = SCREENWIDTH*V_GetPixelDepth();
  }
}

// CPhipps -
// I_SetRes
// Sets the screen resolution
void I_SetRes(void)
{
  int i;

  I_CalculateRes(SCREENWIDTH, SCREENHEIGHT);

  // set first three to standard values
  for (i=0; i<3; i++) {
    screens[i].width = SCREENWIDTH;
    screens[i].height = SCREENHEIGHT;
    screens[i].byte_pitch = SCREENPITCH;
    screens[i].short_pitch = SCREENPITCH / V_GetModePixelDepth(VID_MODE16);
    screens[i].int_pitch = SCREENPITCH / V_GetModePixelDepth(VID_MODE32);
  }

  // statusbar
  screens[4].width = SCREENWIDTH;
  screens[4].height = (ST_SCALED_HEIGHT+1);
  screens[4].byte_pitch = SCREENPITCH;
  screens[4].short_pitch = SCREENPITCH / V_GetModePixelDepth(VID_MODE16);
  screens[4].int_pitch = SCREENPITCH / V_GetModePixelDepth(VID_MODE32);

  lprintf(LO_INFO,"I_SetRes: Using resolution %dx%d\n", SCREENWIDTH, SCREENHEIGHT);
}

void I_InitGraphics(void)
{
  char titlebuffer[2048];
  static int    firsttime=1;

  if (firsttime)
  {
    firsttime = 0;

	/* TODO for 3DS: clean this up */
	SCREENWIDTH = 320;
	SCREENHEIGHT = 240;

    atexit(I_ShutdownGraphics);
    lprintf(LO_INFO, "I_InitGraphics: %dx%d\n", SCREENWIDTH, SCREENHEIGHT);

    /* Set the video mode */
    I_UpdateVideoMode();

	/* Initialize palette */
	I_UploadNewPalette(0);

    /* Initialize the input system */
    I_InitInputs();
  }
}

void I_UpdateVideoMode(void)
{
  int init_flags;
  int i;
  video_mode_t mode;

  lprintf(LO_INFO, "I_UpdateVideoMode: %dx%d (%s)\n", SCREENWIDTH, SCREENHEIGHT, desired_fullscreen ? "fullscreen" : "nofullscreen");

  // For now, use 8-bit rendering but default 24-bit framebuffer
  mode = VID_MODE8;
  
  // reset video modes
  gfxExit();
  // disable console
  // Done_ConsoleWin();
  // gfxInit(GSP_BGR8_OES, GSP_BGR8_OES, false);
  I_ClearFrameBuffer(GFX_TOP, GFX_LEFT);
  I_ClearFrameBuffer(GFX_TOP, GFX_RIGHT);
  // I_ClearFrameBuffer(GFX_BOTTOM, 0);
  
  V_InitMode(mode);
  V_DestroyUnusedTrueColorPalettes();
  V_FreeScreens();

  I_SetRes();
  
  V_AllocScreens();

  R_InitBuffer(SCREENWIDTH, SCREENHEIGHT);
}
