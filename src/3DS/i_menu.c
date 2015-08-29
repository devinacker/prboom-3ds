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
 *      Main menu / frontend for 3DS
 *
 *-----------------------------------------------------------------------------
 */

#include <3ds.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "m_argv.h"

char **newargv;

// Enum for menu item types
typedef enum {
	item_void,
	item_choice,
	item_int,
	item_bool,
	item_file,
	item_menu,
} itemtype_t;

// Struct for single menu items
typedef struct {
	short y; // position on screen
	const char *text; // item text
	itemtype_t type;  // type
	void *data; // pointer to variable
	short width; // for text positioning
	const char **values; // pointer to item choices
	int min, max; // (for item_int) min and max integer values
} menuitem_t;

const char *screenWidthVals[] = {"320", "400", 0};
const char *screenHeightVals[] = {"200", "240", 0};
const char *screenDepthVals[] = {"8", "32", 0};

int screenWidth = 0;
int screenHeight = 0;
int screenDepth = 0;

// TODO: fill this by seeing which ones are actually available
const char *gameWadVals[] = {"doom.wad", "doom2.wad", "tnt.wad", "plutonia.wad", 0};

int gameWad = 0;

static menuitem_t menuMain[] = {
	{2, "Start game", item_menu, 0, 0, 0},
	
	{4, "Screen width", item_choice, &screenWidth, 3, screenWidthVals},
	{5, "Screen height", item_choice, &screenHeight, 3, screenHeightVals},
	{6, "Screen depth (bits)", item_choice, &screenDepth, 3, screenDepthVals},
	
	{8, "Game IWAD", item_choice, &gameWad, 12, gameWadVals},
	
	{-1}
};

void I_Cleanup() {
	do {
		free((void*)newargv[--myargc]);
	} while (myargc);
	
	free((void*)newargv);
	myargv = 0;
}

void I_AddArg(const char *value) {
	newargv = realloc(newargv, sizeof(char*) * (myargc + 1));
	newargv[myargc] = calloc(1 + strlen(value), 1);
	strcpy(newargv[myargc], value);
	myargc++;
	myargv = (const char* const *)newargv;
}

void I_MainMenu() {
	gfxInitDefault();
	consoleInit(GFX_BOTTOM, 0);
	myargc = 0; newargv = 0;
	
	menuitem_t *current = menuMain;
	int menupos = 0;
	
	atexit(I_Cleanup);
	
	// TODO: get some defaults from prboom.cfg
	
	while (1) {
		consoleClear();
		
		menuitem_t *selected = &current[menupos];
		
		// draw cursor
		printf("\x1b[%u;1H--> ", selected->y);
		
		// draw all available menu items
		menuitem_t *i;
		for (i = current; i->y >= 0; i++) {
			printf("\x1b[%u;5H%s", i->y, i->text);
			
			// print value
			if (!i->data) continue;
			
			int *iData = (int*)i->data;
			bool *bData = (bool*)i->data;
			
			char hasPrev, hasNext;
			
			switch (i->type) {
			case item_choice:
				hasPrev = *iData ? '<' : ' ';
				hasNext = i->values[*iData + 1] ? '>' : ' ';
				
				printf("\x1b[%u;%uH%c %*s %c", i->y, 31 - i->width, hasPrev, i->width, i->values[*iData], hasNext);
				break;
				
			case item_int:
				hasPrev = (*iData > i->min) ? '<' : ' ';
				hasNext = (*iData < i->max) ? '>' : ' ';
			
				printf("\x1b[%u;%uH%-*u", i->y, 31 - i->width, i->width, *iData);
				break;
				
			case item_bool:
				printf("\x1b[%u;30H[%c]", i->y, *bData ? 'x' : ' ');
				break;
			}
		}
		
		// handle input for current menu item
		hidScanInput();
		u32 down = hidKeysDown();
		
		int *iData = (int*)selected->data;
		bool *bData = (bool*)selected->data;
		int selectedOption = (iData) ? *iData : 0;
		
		// TODO: handle skipping blank menu items
		
		if (down & KEY_UP) {
			if (menupos > 0) menupos--;
			
		} else if (down & KEY_DOWN) {
			if (current[menupos+1].y >= 0) menupos++;
			
		} else if (down & KEY_LEFT && selected->data) {
			switch (selected->type) {
			case item_choice:
				if (selectedOption > 0 && selected->values[selectedOption - 1]) *iData = (*iData)-1;
				break;
				
			case item_int:
				if (*iData > selected->min) *iData = (*iData)-1;
				break;
				
			case item_bool:
				*bData = false;
			}
			
		} else if (down & KEY_RIGHT && selected->data) {
			switch (selected->type) {
			case item_choice:
				if (selected->values[selectedOption + 1]) *iData = (*iData)+1;
				break;
				
			case item_int:
				if (*iData < selected->max) *iData = (*iData)+1;
				break;
				
			case item_bool:
				*bData = true;
			}
		
		} else if (down & KEY_A) {
			if (selected->type == item_bool && selected->data) {
				*bData ^= true;
			} else if (selected->type == item_menu) {
				current = (menuitem_t*)selected->data;
				menupos = 0;
				if (!current) break;
			}
		
		} else if (down & KEY_B) {
			if (current != menuMain) {
				current = menuMain;
				menupos = 0;
			}
		}
		
		gfxFlushBuffers();
		gfxSwapBuffers();
		gspWaitForVBlank();
	}

	// TODO: handle variadic arguments based on other stuff (for warping)
	I_AddArg(PACKAGE);
	
	I_AddArg("-width");
	I_AddArg(screenWidthVals[screenWidth]);
	
	I_AddArg("-height");
	I_AddArg(screenHeightVals[screenHeight]);
	
	I_AddArg("-vidmode");
	I_AddArg(screenDepthVals[screenDepth]);
	
	I_AddArg("-iwad");
	I_AddArg(gameWadVals[gameWad]);
	
	gfxExit();
}
