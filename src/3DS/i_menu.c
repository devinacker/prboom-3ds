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
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>

#include "config.h"
#include "doomtype.h"
#include "m_argv.h"
#include "i_system.h"
#include "d_main.h"
#include "z_zone.h" // ensure malloc and free work correctly

#define MAX_PWADS 20

char **newargv = 0;

char **filelist = 0;
int filecount = 0;

char *addfiles[MAX_PWADS] = {0};
int addfilescount = 0;

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
	int min, max, step; // (for item_int) min and max integer values
} menuitem_t;

const char *screenWidthVals[] = {"320", "400", 0};
const char *screenHeightVals[] = {"200", "240", 0};
const char *screenDepthVals[] = {"8", /* "32", */ 0};

int screenWidth = 0;
int screenHeight = 0;
int screenDepth = 0;

// will be filled by seeing which ones are actually available
const char *gameWadVals[16] = {0};

int gameWad = 0;

static const menuitem_t menuExtra[];

static const menuitem_t menuMain[] = {
	{2, "Start game", item_menu, 0},
	{3, "Extra options...", item_menu, menuExtra},
	
	{5, "Game IWAD", item_choice, &gameWad, 13, gameWadVals},
	
	{7, "Screen width", item_choice, &screenWidth, 3, screenWidthVals},
	{8, "Screen height", item_choice, &screenHeight, 3, screenHeightVals},
	/*
	{9, "Screen depth (bits)", item_choice, &screenDepth, 3, screenDepthVals},
	*/
	{10, "Additional files...", item_file},
	
	{-1}
};

int compLevel = 0;

const char *compLevelVals[] = { 
	"Default", 
	"Doom v1.2 (0)", 
	"Doom v1.666 (1)", 
	"Doom / Doom 2 v1.9 (2)", 
	"Ultimate Doom (3)", 
	"Final Doom (4)",
	"DOSDoom compatibility (5)", 
	"TASDoom compatibility (6)", 
	"\"Boom compatibility\" (7)", 
	"Boom v2.01 (8)", 
	"Boom v2.02 (9)", 
	"LxDoom v1.3.2+ (10)",
	"MBF (11)", 
	"PrBoom 2.03beta (12)", 
	"PrBoom v2.1.0-2.1.1 (13)", 
	"PrBoom v2.1.2-v2.2.6 (14)",
	"PrBoom v2.3.x (15)", 
	"PrBoom 2.4.0 (16)", 
	"Current PrBoom (17)", 
	0 };

boolean warpEnable = false;
int warpEpisode = 0; 
int warpMap = 1;
int warpSkill = 2;

const char *warpSkillVals[] = {"ITYTD (1)", "HNTR (2)", "HMP (3)", "UV (4)", "Nightmare (5)", 0};

int gameMode = 0;

const char *gameModeVals[] = {"Single player", "Solo coop", "Deathmatch", "Altdeath", 0};

int turbo = 100;

boolean noMonsters = false;
boolean fastMonsters = false;

static const menuitem_t menuExtra[] = {
	{2, "Back...", item_menu, menuMain},
	{3, "Start game", item_menu, 0},
	
	{5, "Compatibility level", item_choice, &compLevel, 25, compLevelVals},
	
	{8, "Warp", item_bool, &warpEnable},
	{9, "| Episode (Doom 1 only)", item_int, &warpEpisode, 2, NULL, 0, 4, 1},
	{10, "| Map", item_int, &warpMap, 2, NULL, 1, 32, 1},
	{11, "| Skill", item_choice, &warpSkill, 14, warpSkillVals},
	{12, "|", item_void},
	{13, "| Game mode", item_choice, &gameMode, 14, gameModeVals},
	{14, "| No monsters", item_bool, &noMonsters},
	{15, "| Fast monsters", item_bool, &fastMonsters},
	
	{17, "Turbo (percent)", item_int, &turbo, 3, NULL, 10, 400, 10},
	
	{-1}
};

void I_Cleanup() {
	while (myargc) {
		free((void*)newargv[--myargc]);
	} 
	free((void*)newargv);
	myargv = 0;
	
	while (filecount) {
		free((void*)filelist[--filecount]);
	}
	free((void*)filelist);
	filelist = 0;
}

void I_AddArg(const char *value) {
	newargv = realloc(newargv, sizeof(char*) * (myargc + 1));
	newargv[myargc++] = strdup(value);
	myargv = (const char* const *)newargv;
}

void I_AddArgNum(int value) {
	static char str[16];
	sprintf(str, "%d", value);
	I_AddArg(str);
}

void I_AddFile(const char *name) {
	filelist = realloc(filelist, sizeof(char*) * (filecount + 1));
	filelist[filecount++] = strdup(name);
}

void I_AddFileDir(const char *dirname) {

	DIR *dir = opendir(dirname);
	if (dir) {
		struct dirent *entry;
		
		while (entry = readdir(dir)) {
			const char *ext = strrchr(entry->d_name, '.');
			if (ext && ((tolower(ext[1]) == 'w' && tolower(ext[2]) == 'a' && tolower(ext[3]) == 'd')
			            || (tolower(ext[1]) == 'd' && tolower(ext[2]) == 'e' && tolower(ext[3]) == 'h')))
				I_AddFile(entry->d_name);
		}
		closedir (dir);
	}
}

void I_BrowseFiles(int num) {
	int menupos = 0;
	int from = 0;
	const int pagesize = 25;

	while (1) {
		consoleClear();
		
		// draw cursor
		printf("\x1b[%u;1H> ", menupos+1);
		
		// draw all available menu items
		int i;
		for (i = 0; i < pagesize && from + i < filecount; i++) {
			printf("\x1b[%u;3H%s", i+1, filelist[from+i]);
		}
		
		printf("\n\n %d-%d of %d (L/R to page)", from+1, MIN(filecount, from+pagesize), filecount);
		
		gfxFlushBuffers();
		gfxSwapBuffers();

		// handle input for current menu item
		u32 down = 0;
		while (!down) {
			hidScanInput();
			down = hidKeysDown();
			gspWaitForVBlank();
		}
		
		if (down & KEY_UP) {
			if (from && menupos == 0) {
				from -= pagesize;
				menupos = pagesize - 1;
			} else if (menupos > 0) {
				menupos--;
			}
			
		} else if (down & KEY_DOWN) {
			if (from + menupos + 1 < filecount && ++menupos >= pagesize) {
				from += pagesize;
				menupos = 0;
			}
			
		} else if (down & KEY_A) {
			addfiles[num] = filelist[from + menupos];
			if (num == addfilescount) addfilescount++;
			break;
		
		} else if (down & KEY_L) {
			// page up
			if (from > 0) {
				from -= pagesize;
				if (from < 0) 
					from = 0;
			}
				
		} else if (down & KEY_R) {
			// page down
			if (from + pagesize < filecount) {
				from += pagesize;
				if (from + menupos >= filecount) 
					menupos = filecount - from - 1;
			}
			
		} else if (down & KEY_B) {
			break;
		}
	}
}

void I_ListFiles() {
	int menupos = 0;

	while (1) {
		consoleClear();
		
		// draw cursor
		printf("\x1b[%u;1H> ", menupos+1);
		
		// draw all available menu items
		int i;
		for (i = 0; i < MAX_PWADS; i++) {
			printf("\x1b[%u;3H%2u: ", i+1, i+1);
			
			// print value
			if (i == addfilescount)
				printf("(add new...)");
			else if (i < addfilescount)
				printf("%s", addfiles[i]);
		}
		
		printf("\n\n A: select Y: remove L/R: move B: exit");
		
		gfxFlushBuffers();
		gfxSwapBuffers();

		// handle input for current menu item
		u32 down = 0;
		while (!down) {
			hidScanInput();
			down = hidKeysDown();
			gspWaitForVBlank();
		}
		
		if (down & KEY_UP) {
			if (menupos > 0) menupos--;
			
		} else if (down & KEY_DOWN) {
			if (menupos + 1 < MIN(1+addfilescount, MAX_PWADS)) menupos++;
			
		} else if (down & KEY_A) {
			I_BrowseFiles(menupos);
		
		} else if (down & KEY_L) {
			// move file up
			if (menupos > 0 && menupos < addfilescount) {
				char *temp = addfiles[menupos - 1];
				addfiles[menupos - 1] = addfiles[menupos];
				addfiles[menupos] = temp;
				menupos--;
			}
				
		} else if (down & KEY_R) {
			// move file down
			if (menupos + 1 < addfilescount) {
				char *temp = addfiles[menupos + 1];
				addfiles[menupos + 1] = addfiles[menupos];
				addfiles[menupos] = temp;
				menupos++;
			}
			
		} else if (down & KEY_Y) {
			// remove file
			for (i = menupos; i < addfilescount && i < MAX_PWADS; i++)
				addfiles[i] = addfiles[i+1];
			
			if (addfilescount) {
				addfiles[addfilescount--] = 0;
			}
			
		} else if (down & KEY_B) {
			break;
		}
	}
}

void I_MainMenu() {
	consoleInit(GFX_BOTTOM, 0);
	myargc = 0; newargv = 0;
	atexit(I_Cleanup);
	
	menuitem_t const *current = menuMain;
	int menupos = 0;
	
	// enumerate available IWADs here
	int foundIwads = 0;
	int i;
	for (i = 0; i < nstandard_iwads; i++) {
		char *iwad = I_FindFile(standard_iwads[i], "");
	
		if (iwad) {
			gameWadVals[foundIwads++] = standard_iwads[i];
			free(iwad);
		}
	}
	
	if (!foundIwads) {
		// be lazy and just let prboom handle the "no iwads" issue if we couldn't find any
		return;
	}
	
	// enumerate available PWADs
	I_AddFileDir(".");
	I_AddFileDir(I_DoomExeDir());
	I_AddFileDir(DOOMWADDIR);
	
	// TODO: get some defaults from prboom.cfg
	
	while (1) {
		consoleClear();
		
		menuitem_t const *selected = &current[menupos];
		
		// draw cursor
		printf("\x1b[%u;1H> ", selected->y);
		
		// draw all available menu items
		menuitem_t *i;
		for (i = current; i->y >= 0; i++) {
			printf("\x1b[%u;3H%s", i->y, i->text);
			
			// print value
			int *iData = (int*)i->data;
			boolean *bData = (boolean*)i->data;
			
			char hasPrev, hasNext;
			// alternate y-position for string choice items only
			short y = i->y;
			if (i->width + strlen(i->text) > 30)
				y++;
			
			switch (i->type) {
			case item_choice:
				hasPrev = *iData ? '<' : ' ';
				hasNext = i->values[*iData + 1] ? '>' : ' ';
				
				printf("\x1b[%u;%uH%c %*s %c", y, 33 - i->width, hasPrev, i->width, i->values[*iData], hasNext);
				break;
				
			case item_int:
				hasPrev = (*iData > i->min) ? '<' : ' ';
				hasNext = (*iData < i->max) ? '>' : ' ';
			
				printf("\x1b[%u;%uH%*u", i->y, 35 - i->width, i->width, *iData);
				break;
				
			case item_bool:
				printf("\x1b[%u;32H%3s", i->y, *bData ? "yes" : "no");
				break;
		
			case item_file:
				for (y = 0; y < MIN(10, addfilescount); y++)
					printf("\x1b[%u;3H| %s", i->y + y + 1, addfiles[y]);
				
				if (addfilescount >= 10)
					printf("\x1b[%u;3H| ... (%d more files)", i->y + 10 + 1, addfilescount - 10);
				else if (!addfilescount)
					printf("\x1b[%u;3H| none selected", i->y + 1);
					
				break;
			}
		}
		
		gfxFlushBuffers();
		gfxSwapBuffers();

		int *iData = (int*)selected->data;
		boolean *bData = (boolean*)selected->data;
		int selectedOption = (iData) ? *iData : 0;
		
		// handle input for current menu item
		// TODO: handle skipping blank menu items
		u32 down = 0;
		while (!down) {
			hidScanInput();
			down = hidKeysDown();
			gspWaitForVBlank();
		}
		
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
				if (*iData > selected->min) *iData = (*iData) - selected->step;
				break;
				
			case item_bool:
				*bData = false;
				break;
			}
			
		} else if (down & KEY_RIGHT && selected->data) {
			switch (selected->type) {
			case item_choice:
				if (selected->values[selectedOption + 1]) *iData = (*iData)+1;
				break;
				
			case item_int:
				if (*iData < selected->max) *iData = (*iData) + selected->step;
				break;
				
			case item_bool:
				*bData = true;
				break;
			}
		
		} else if (down & KEY_A) {
			if (selected->type == item_bool && selected->data) {
				*bData ^= true;
			} else if (selected->type == item_menu) {
				current = (menuitem_t*)selected->data;
				menupos = 0;
				if (!current) break;
			} else if (selected->type == item_file) {
				I_ListFiles();
			}
		
		} else if (down & KEY_B) {
			if (current != menuMain) {
				current = menuMain;
				menupos = 0;
			}
		}
	}

	I_AddArg(PACKAGE);
	
	I_AddArg("-width");
	I_AddArg(screenWidthVals[screenWidth]);
	
	I_AddArg("-height");
	I_AddArg(screenHeightVals[screenHeight]);
	
	I_AddArg("-vidmode");
	I_AddArg(screenDepthVals[screenDepth]);
	
	I_AddArg("-iwad");
	I_AddArg(gameWadVals[gameWad]);
	
	boolean addWad = false;
	boolean addDeh = false;
	for (i = 0; i < addfilescount; i++) {
		char *filename = addfiles[i];
		
		const char *ext = strrchr(filename, '.');
		if (ext && tolower(ext[1]) == 'w' && tolower(ext[2]) == 'a' && tolower(ext[3]) == 'd') {
			if (!addWad) {
				addWad = true;
				I_AddArg("-file");
			}
			
			char *path = I_FindFile(filename, "");
			if (path) {
				I_AddArg(path);
				free(path);
			} else {
				I_AddArg(filename);
			}
		}
	}
	for (i = 0; i < addfilescount; i++) {
		char *filename = addfiles[i];
		const char *ext = strrchr(filename, '.');
		if (ext && tolower(ext[1]) == 'd' && tolower(ext[2]) == 'e' && tolower(ext[3]) == 'h') {
			if (!addDeh) {
				addDeh = true;
				I_AddArg("-deh");
			}
			
			char *path = I_FindFile(filename, "");
			if (path) {
				I_AddArg(path);
				free(path);
			} else {
				I_AddArg(filename);
			}
		}
	}
	
	if (compLevel) {
		I_AddArg("-complevel");
		I_AddArgNum(compLevel - 1);
	}
	
	if (warpEnable) {
		I_AddArg("-warp");
		
		if (warpEpisode)
			I_AddArgNum(warpEpisode);
		I_AddArgNum(warpMap);
		
		I_AddArg("-skill");
		I_AddArgNum(warpSkill + 1);
		
		if (gameMode == 1)
			I_AddArg("-solo-net");
		else if (gameMode == 2)
			I_AddArg("-deathmatch");
		else if (gameMode == 3)
			I_AddArg("-altdeath");
			
		if (noMonsters)
			I_AddArg("-nomonsters");
		else if (fastMonsters)
			I_AddArg("-fast");
	}
	
	if (turbo != 100) {
		I_AddArg("-turbo");
		I_AddArgNum(turbo);
	}
}
