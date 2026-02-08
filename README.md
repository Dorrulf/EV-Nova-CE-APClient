EV Nova Community Edition - Archipelago Client
==================

Configures EV Nova to work with Archipelago randomizer servers!

How to install
--------------
1. Download the latest release.
(CE Setup Steps)
2. Extract into your EV Nova folder, replacing any existing files. *I recommend creating a new EV Nova install to work from, or backing up your original.*
3. Install the Geneva and Charcoal font files (double-click to open, then click "Install").
4. Open the ddraw.ini file and adjust any settings as desired.
(AP Setup Steps)
5. Download [EV New](https://drive.google.com/file/d/1-3DyYFvp46FunDwX76Y2YYRo3oNFdVZ1/view) and extract it into your EV Nova folder.
6. Now you're ready to join an AP run.

Joining Archipelago
-------------------
1. Create a yaml for EV Nova. Your Archipelago client can generate a default file for you to use and modify.
2. Once the room has been generated, download you patch file.
3. Drag this downloaded zip to the DROP_AP_PATCH_HERE.bat file.
4. Adjust the ap_config.ini file with your settings.
5. Launch the game!

When you are done with that AP run, delete the associated plugin in the Plugins folder, as well as the unzipped folder in EV Nova's base directory.

Authors
-------
 - Dorrulf
 
Special Thanks
--------------
So many resources help make bigger projects easier to do these days. So while I have a lot of thanks, I'd like to highlight a few:

 - The CE team, check out their [GitHub](https://github.com/andrews05/EV-Nova-CE)
 - The APCpp team, check out their [GitHub](https://github.com/N00byKing/APCpp)

!WARNINGS!
----------
This mod is still very early in development. This means several important things:
 - You may run into errors or crashes. If so, please log a ticket on the issues page, that would be helpful.
 - There are many more features planned.
 - There are going to be logic errors (looking at you branching story paths). Do *NOT* cross into another story other than the one you selected in your yaml!
 - Progression logic may not be all there yet either.
 
Planned Features
----------------
 - Hotkeys for connecting / disconnecting (possibly with ingame prompts)
 - Inclusion of additional flags (some examples include: purchasing items, first system explores, etc.)
 - Inclusion of additional items (outfits, ammo)
 - Enable system shuffling via yaml flags. (Be warned: due to how "changed" systems/spobs are handled, this may not happen.)
 - Enable mission locking via yaml (requires checks to unlock the missions you need to progress.)
 - Deathlink

Building on *nix
-----------------
Building is the same - follow the original instructions:
 - Install [petool](https://github.com/FunkyFr3sh/petool) from github
 - Get mingw-w64 from your package manager
 - Get NASM
 - make or gmake

Building on Windows
-------------------
Building is the same - follow the original instructions:
 - Download [w64devkit-mini-for-patching.zip](https://github.com/FunkyFr3sh/petool/releases/latest/download/w64devkit-mini-for-patching.zip) ([.7z](https://github.com/FunkyFr3sh/petool/releases/latest/download/w64devkit-mini-for-patching.7z))
 - Extract `w64devkit-mini-for-patching.zip` to `C:\`
 - run `build.cmd`

Notes on using APCpp
--------------------
To build it, I had a local w64 kit installed. I had to launch that, cd to the dev directory, then run my commands.
I ran into issues with incorrect data types in two files. Luckily, they were just for debug statements, so I removed them and was able to build.
When you build, the result will be a .dll and .dll.a file. I didn't know what to expect, so it took me a second to find those.
I ended up creating a copy of the .h file and decorated it with __decspec(dllexport/dllimport) statements because of how I wanted to incorporate the linked dlls / calls in my project.

To finally include it in my project, I copied over the .h file and the .dll, .dll.a to my dev project.

These aren't super detailed hints, but I hope they help get you going on APCpp faster than it took me to get started.