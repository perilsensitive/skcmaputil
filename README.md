skcmaputil
==========
skcmaputil (for 'Shovel Knight Controller Map Utility') is a utility
for viewing and modifying the SDL GameController mapping lists stored
in Shovel Knight's global.pak file.

Supported Platforms
-------------------
* Linux
* Windows
* Mac OS X (untested, but really all *nix platforms should work)

Features
--------
* List the GameController mappings in the specified PAK file for all
  supported platforms. The mappings are output in a format that can be
  easily stored in a text file for editing and later use. It is also
  usable by any other game that can load mappings from a file in the
  format supported by SDL's SDL_GameControllerAddMappingsFrom*()
  functions.
* Remove all mappings from the specified PAK file.  Useful for users
  of the Steam version of Shovel Knight who want to force the game to
  use the mapping supplied by Steam (configured in Big Picture mode)
  through the SDL_HINT_GAMECONTROLLERCONFIG environment variable.
* Import new mappings from a text file (in the same format as that
  output by the list feature) into the specified PAK file.  These new
  mappings may be appended to those in the PAK file or replace them
  entirely.

Compiling
---------
Run 'make'.  That's it. :-)

Installing
----------
Copy the skcmaputil binary someplace useful.  Your Shovel Knight
installation directory/folder is a good choice.  If you're installing
from the zipfile, just extract it where you want the skcmaputil binary
to be installed.

Note to Steam Users
-------------------
If, when trying to map your controller's d-pad to the d-pad directions
in Big Picture mode, you press a direction on your d-pad and the
mapping appears as 'Axis 0' or 'Axis 1' your controller's d-pad is
acting like an analog stick.  In order to make it work with Shovel
Knight (or any game supporting SDL2's GameControlelr API), you will
need to map your controller's d-pad to 'Left X' and 'Left Y' instead
of the d-pad directions.  Just press left or right on the d-pad when
mapping 'Left X', and press up or down on the d-pad when mapping 'Left
Y', and leave the four d-pad directions unmapped.

Make sure your gamepad is mapped in Big Picture mode as described
above, and Shovel Knight *should* be able to use it correctly.  If
not, read the section "Running (for Steam Users)" below.

Running (for Steam Users)
-------------------------
If you've configured your controller correctly in Big Picture mode and
it still doesn't work correctly in Shovel Knight, the mapping database
built in to the game may be overriding the mapping you configured in
Steam.  To force Shovel Knight to always use Steam's supplied mapping,
copy skcmaputil's files to your Shovel Knight data directory and run
one of the following commands (depending on your platform):

Windows: force_steam_controller_mapping.bat
Linux: ./force_steam_controller_mapping.sh

You can just double-click the appropriate file in Explorer or whatever
file manager you're using as well.  You'll probably see a cmd.exe window (or
terminal window on Linux) pop up for a second and then disappear.  At this
point, run Shovel Knight and see if it recognizes your gamepad properly.

Note that this removes *all* mappings from Shovel Knight's built-in
controller database (forcing it to always use the mapping Steam
provides at run-time), so you will need to configure your controller
in Big Picture mode for it to be recognized at all.  (Controllers
supported by the mapping database built in to SDL2 will always be
recognized even if Shovel Knight's database is cleared).

If you want to do more with skcmaputil, read the "Running (Advanced)"
section below.

Running (for non-Steam users)
-----------------------------
If all you want to do is import controller mappings from skcmaputil's
database into your global.pak file, copy skcmaputil's files to your
Shovel Knight data directory and run one of the following commands
(depending on your platform):

Windows: update_controller_mappings.bat
Linux: ./update_controller_mappings.sh

You can just double-click the appropriate file in Explorer or whatever
file manager you're using as well.  You'll probably see a cmd.exe window (or
terminal window on Linux) pop up for a second and then disappear.  At this
point, run Shovel Knight and see if it recognizes your gamepad.

If you want to do more with skcmaputil, read the "Running (Advanced)"
section below.

Running (Advanced)
--------------------------
First, you'll want to create a backup of your global.pak file.  This
can be found in the directory or folder called 'data' where Shovel
Knight is installed.  You can call this backup whatever you'd like
(e.g, 'global.pak.bak').  This backup is there so that you can restore
the original file in case something goes wrong.  If something does go
wrong, just copy your backup file on top of global.pak to make Shovel
Knight work again.

Here are some example commands.  These assume you've put skcmaputil
somewhere in your path, or possibly the Shovel Knight data directory
if you're a Windows user.

To view the mappings stored in the global.pak file:

    skcmaputil dump global.pak

To remove all mappings from the global.pak file:

    skcmaputil clear global.pak tmp.pak

To add new mappings stored in a file called 'gamecontrollerdb.txt':

    skcmaputil append global.pak tmp.pak gamecontrollerdb.txt

To replace the mappings with those from a file called 'gamecontrollerdb.txt':

    skcmaputil replace global.pak tmp.pak gamecontrollerdb.txt

When skcmaputil is done, it will have created a new PAK file ('tmp.pak' in the
examples above).  Copy the new PAK file on top of 'global.pak' to put the
modified version in place.

Note on Updates
---------------
Official updates to Shovel Knight may replace global.pak with a new
version.  If that happens, your custom mappings will no longer be
installed and your controller will not work (unless the update happens
to include compatible mappings, of course).  If this happens, just use
skcmaputil to install your mappings into the new version of
global.pak.  Keeping a copy of your custom mappings in a text file is
a good idea so that you don't completely lose them due to an upgrade.

Creating or Finding Mappings
----------------------------
If you need to create or find a mapping for your controller, you can
try the gamecontrollerdb.txt file included in the source code (or
zipfile of the binary).  This is derived from the database found here:

https://raw.githubusercontent.com/gabomdq/SDL_GameControllerDB/master/gamecontrollerdb.txt

You can create your own mapping using AntiMicro, which can be
downloaded from here:

https://github.com/Ryochan7/antimicro/releases

Once you create a mapping, consider contributing it to the
SDL_GameControllerDB project:

https://github.com/gabomdq/SDL_GameControllerDB

Bugs
----
Bug reports can be filed on the GitHub issues page:
https://github.com/perilsensitive/skcmaputil/issues

Contact
-------
You can contact me at perilsensitive@gmail.com.

Legal Stuff
-----------
Shovel Knight is the property of Yacht Club Games.

Steam is the property of Valve Corporation.

I am not affiliated with either Yacht Club Games or Valve.


