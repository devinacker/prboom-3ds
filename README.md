# prboom-3ds

This is a preliminary (but playable) port of the Doom engine to the Nintendo 3DS. It is based on PrBoom v2.5.0.

It is unrelated to [prboom3ds](https://github.com/elhobbs/prboom3ds) by elhobbs and will probably be given a more original name later.

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

Currently there is no convenient way to select from multiple IWADs or to load additional PWADs and DeHackEd patches. In the future prboom-3ds will have a minimal frontend which will allow you to load additional files and configure other options.

`prboom.cfg` will be generated at run time; you can edit this manually to change some settings that aren't accessible from within the game, like screen resolution (supported ones are 320x200 and 320x240).

This is also subject to break in weird ways that I don't know about yet. If you see something, say something.

A few known bugs:

- Saving games is currently not possible due to lack of hardware keyboard
- Some config options can't be changed ingame for the same reason
- Picking up weapons in Doom 2 (and Final Doom?) may hang the game
- The number "2" for the pistol on the status bar sometimes disappears when picking up other weapons
- IWAD demos usually (or always) desync, other demos probably do too

## How to build

- Follow the guide to setting up a 3DS development environment: [http://3dbrew.org/wiki/Setting_up_Development_Environment](http://3dbrew.org/wiki/Setting_up_Development_Environment)
- Run `make`. The .3dsx and .smdh files will be placed in the project root directory.

## To do

- Re-add 24-bit color support
- Sound support
- Add fancy stereoscopic 3D on the top screen
- Persistent automap on the bottom screen (probably)
- Optional widescreen (with increased FOV) on the top screen
- Minimal touch screen interface for things like selecting weapons or manipulating the automap
- Multiplayer?????

