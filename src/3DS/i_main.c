/* Emacs style mode select   -*- C++ -*-
 *-----------------------------------------------------------------------------
 *
 *
 *  PrBoom: a Doom port merged with LxDoom and LSDLDoom
 *  based on BOOM, a modified and improved DOOM engine
 *  Copyright (C) 1999 by
 *  id Software, Chi Hoang, Lee Killough, Jim Flynn, Rand Phares, Ty Halderman
 *  Copyright (C) 1999-2000 by
 *  Jess Haas, Nicolas Kalkhof, Colin Phipps, Florian Schulze
 *  Copyright 2005, 2006 by
 *  Florian Schulze, Colin Phipps, Neil Stevens, Andrey Budko
 *  Copyright 2015 by
 *  Devin Acker
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
 *      Startup and quit functions. Handles signals, inits the
 *      memory management, then calls D_DoomMain. Also contains
 *      I_Init which does other system-related startup stuff.
 *
 *-----------------------------------------------------------------------------
 */

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <3ds.h>

#include "doomdef.h"
#include "m_argv.h"
#include "d_main.h"
#include "m_fixed.h"
#include "i_system.h"
#include "i_video.h"
#include "z_zone.h"
#include "lprintf.h"
#include "m_random.h"
#include "doomstat.h"
#include "g_game.h"
#include "m_misc.h"
#include "i_sound.h"
#include "i_main.h"
#include "r_fps.h"
#include "lprintf.h"
#include "z_zone.h"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

extern void I_MainMenu();

/* Most of the following has been rewritten by Lee Killough
 *
 * I_GetTime
 * killough 4/13/98: Make clock rate adjustable by scale factor
 * cphipps - much made static
 */

int realtic_clock_rate = 100;
static int_64_t I_GetTime_Scale = 1<<24;

static int I_GetTime_Scaled(void)
{
  return (int)( (int_64_t) I_GetTime_RealTime() * I_GetTime_Scale >> 24);
}



static int  I_GetTime_FastDemo(void)
{
  static int fasttic;
  return fasttic++;
}



static int I_GetTime_Error(void)
{
  I_Error("I_GetTime_Error: GetTime() used before initialization");
  return 0;
}



int (*I_GetTime)(void) = I_GetTime_Error;

void I_Init(void)
{
  /* killough 4/14/98: Adjustable speedup based on realtic_clock_rate */
  if (fastdemo)
    I_GetTime = I_GetTime_FastDemo;
  else
    if (realtic_clock_rate != 100)
      {
        I_GetTime_Scale = ((int_64_t) realtic_clock_rate << 24) / 100;
        I_GetTime = I_GetTime_Scaled;
      }
    else
      I_GetTime = I_GetTime_RealTime;

  {
    /* killough 2/21/98: avoid sound initialization if no sound & no music */
    if (!(nomusicparm && nosfxparm))
      I_InitSound();
  }

  R_InitInterpolation();
}

/* cleanup handling -- killough:
 */
static void I_SignalHandler(int s)
{
  char buf[2048];

  signal(s,SIG_IGN);  /* Ignore future instances of this signal.*/

  strcpy(buf,"Exiting on signal: ");
  I_SigString(buf+strlen(buf),2000-strlen(buf),s);

  /* If corrupted memory could cause crash, dump memory
   * allocation history, which points out probable causes
   */
  if (s==SIGSEGV || s==SIGILL || s==SIGFPE)
    Z_DumpHistory(buf);

  I_Error("I_SignalHandler: %s", buf);
}



/* convert BIOS color codes to RGB
 */

inline static int convert(int color, int *bold)
{
  if (bold && color > 7) {
    *bold = 1;
  }
  switch (color) {
  case 0: // black
    return 0x000000;
  case 1: // blue
    return 0x0000AA;
  case 2: // green
    return 0x00AA00;
  case 3: // cyan
    return 0x00AAAA;
  case 4: // red
    return 0xAA0000;
  case 5: // magenta
    return 0xAA00AA;
  case 6: // brown
    return 0xAA5500;
  case 7: // light grey
    return 0xAAAAAA;
  case 8: // grey
    return 0x555555;
  case 9: // light blue
    return 0x5555ff;
  case 10: // light green
    return 0x55ff55;
  case 11: // light cyan
    return 0x55ffff;
  case 12: // light red
    return 0xff5555;
  case 13: // light magenta
    return 0xff55ff;
  case 14: // yellow
    return 0xffff55;
  case 15: // white
    return 0xffffff;
  }
  return 0;
}

/* CPhipps - flags controlling ENDOOM behaviour */
enum {
  endoom_colours = 1,
  endoom_nonasciichars = 2,
  endoom_droplastline = 4
};

unsigned int endoom_mode;

static void PrintVer(void)
{
  char vbuf[200];
  lprintf(LO_INFO,"%s\n",I_GetVersionString(vbuf,200));
}

/* I_EndDoom
 * Prints out ENDOOM.
 * cphipps - moved to l_main.c, made static
 */
extern const char endoom_font[];
 
static void I_EndDoom(void)
{
	consoleClear();
	
	int lump = W_CheckNumForName("ENDOOM");
	if (lump == -1) return;

	PrintVer();
	lprintf(LO_INFO, "press any button to quit\n");

	const char (*endoom)[2] = (const void*)W_CacheLumpNum(lump);
	int l = W_LumpLength(lump) / 2;

	/* drop the last line, so everything fits on one screen */
	l -= 80;

	char *screen = malloc(400 * 240 * 3);
	
	int i, x, y;
	for (i = 0; i < l; i++) {
		int r = i / 80;
		int c = i % 80;
		
		char chr = endoom[i][0];
		
		int fg = convert(endoom[i][1] & 0xf, 0);
		int bg = convert((endoom[i][1] >> 4) & 0x7, 0);
		
		// TODO
		int blink = endoom[i][1] & 0x80;
		
		// source font character
		const char *fc = endoom_font + (chr * 10);
		
		// draw font character
		for (y = 0; y < 10; y++) {
			for (x = 0; x < 5; x++) {
				// destination pixel
				int px = (c * 5) + x;
				int py = (r * 10) + y;
				
				char *dest = screen + ((px * 240 + (240 - py - 1)) * 3);
			
				int color = (fc[y] >> (7-x)) & 1 ? fg : bg;
				dest[0] = color;
				dest[1] = color >> 8;
				dest[2] = color >> 16;
			}
		}
	}

	uint16_t width, height;

	while (hidScanInput(), !hidKeysDown()) {
		char *fb = gfxGetFramebuffer(GFX_TOP, GFX_LEFT, &width, &height);
		memcpy(fb, screen, width * height * 3);
		gfxSwapBuffers();
		gspWaitForVBlank();
	}

	free(screen);
	W_UnlockLumpNum(lump);
}

static int has_exited;

/* I_SafeExit
 * This function is called instead of exit() by functions that might be called
 * during the exit process (i.e. after exit() has already been called)
 * Prevent infinitely recursive exits -- killough
 */

void I_SafeExit(int rc)
{
  if (!has_exited)    /* If it hasn't exited yet, exit now -- killough */
    {
      has_exited=rc ? 2 : 1;
	  
      exit(rc);
    }
}

static void I_Quit (void)
{
  if (!has_exited)
    has_exited=1;   /* Prevent infinitely recursive exits -- killough */

  if (has_exited == 1) {
	Init_ConsoleWin();
	I_EndDoom();
    if (demorecording)
      G_CheckDemoStatus();
    M_SaveDefaults ();
  }
  
  gfxExit();
  hidExit();
  aptExit();
  srvExit();
}

//int main(int argc, const char * const * argv)
int main(int argc, char **argv)
{
  srvInit();
  aptInit();
  hidInit(0);
  
  gfxInitDefault();

  /* initialize the console window */
  Init_ConsoleWin();

  /* Version info */
  lprintf(LO_INFO,"\n");
  PrintVer();

  /* cph - Z_Close must be done after I_Quit, so we register it first. */
  atexit(Z_Close);
  /*
     killough 1/98:

     This fixes some problems with exit handling
     during abnormal situations.

     The old code called I_Quit() to end program,
     while now I_Quit() is installed as an exit
     handler and exit() is called to exit, either
     normally or abnormally. 
  */

  Z_Init();                  /* 1/18/98 killough: start up memory stuff first */
  atexit(I_Quit);

  I_SetAffinityMask();

  /* run 3DS frontend */
  I_MainMenu();
  consoleClear();

  /* cphipps - call to video specific startup code */
  I_PreInitGraphics();

  D_DoomMain ();
  return 0;
}
