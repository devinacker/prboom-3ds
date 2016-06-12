# prboom-3ds

## NOTE: This is a formerly in-development 3DS Doom port which has no relation to any other similar ports to the same platform. It was being developed primarily for my own entertainment, and has since been supplanted by other ports made by active members of the 3DS homebrew community.

## This is not an actively developed or maintained project, and is unlikely to become one.

This is a preliminary (but playable) port of the Doom engine to the Nintendo 3DS. It is based on PrBoom v2.5.0.

## How to run

- Follow the guide to running 3DS homebrew: [http://smealum.github.io/3ds/](http://smealum.github.io/3ds/)
- Place `prboom-3ds.3dsx` and `prboom-3ds.smdh` on your SD card under the directory **/3ds/prboom-3ds**
- Place `prboom.wad` (in the data directory) and an IWAD (such as `doom.wad` or `doom2.wad`) in the same directory
- Launch prboom-3ds from the Homebrew Launcher

The default controls are: (subject to change pending testing)

- Circle pad or touch screen to move
- L/R to strafe
- Y to run, B to fire, A to use/open
- D-pad up/down to cycle weapons
- Select to show the automap
- Start to show the main menu

PWADs and DeHackEd patches can be loaded through the frontend, which also allows you to customize some other pre-game settings (such as starting episode/map, skill level, game mode, etc.).

`prboom.cfg` will be generated at run time; you can edit this manually to change some miscellaneous settings that aren't accessible from the frontend or within the game.

This is also subject to break in weird ways that I don't know about yet. If you see something, say something.

## How to build

- Follow the guide to setting up a 3DS development environment: [http://3dbrew.org/wiki/Setting_up_Development_Environment](http://3dbrew.org/wiki/Setting_up_Development_Environment)
- Run `make`. The .3dsx and .smdh files will be placed in the project root directory.

## To do

- Add fancy stereoscopic 3D on the top screen
- Minimal touch screen interface for things like selecting weapons or manipulating the automap
- Multiplayer?????

