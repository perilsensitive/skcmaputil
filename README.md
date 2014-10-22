skcmaputil
==========
skcmaputil (for 'Shovel Knight Controller Map Utility') is exactly that:
a utility for viewing and modifying the SDL GameController mapping lists
stored in Shovel Knight's global.pak file.

Supported Platforms
-------------------
* Linux
* Windows
* Mac OS X (untested, but really all *nix platforms should work)

Features
--------
* List the GameController mappings in the specified PAK file for all
  supported platforms. The mappings are output in a format that can be easily
  stored in a text file for editing and later use. It is also usable by
  any other game that can load mappings from a file in the format supported by
  SDL's SDL_GameControllerAddMappingsFrom*() functions.
* Remove all mappings from the specified PAK file.  Useful for users of the Steam
  version of Shovel Knight who want to force the game to use the mapping supplied
  by Steam (configured in Big Picture mode) through the
  SDL_HINT_GAMECONTROLLERCONFIG environment variable.
* Import new mappings from a text file (in the same format as that output by the
  list feature) into the specified PAK file.  These new mappings may be appended
  to those in the PAK file or replace them entirely.

Compiling
---------
Run 'make'.  That's it. :-)

Installing
----------
Copy the skcmaputil binary someplace useful.  Your Shovel Knight installation
directory/folder is a good choice.  If you're installing from the zipfile, just
extract it where you want the skcmaputil binary to be installed.

Running
-------
First, you'll want to create a backup of your global.pak file.  This can be found
in the directory or folder called 'data' where Shovel Knight is installed.  You
can call this backup whatever you'd like (e.g, 'global.pak.bak').  This backup is
there so that you can restore the original file in case something goes wrong.  If
something does go wrong, just copy your backup file on top of global.pak to make
Shovel Kight work again.

Here are some example commands.  These assume you've put skcmaputil somewhere in
your path, or possibly the Shovel Knight data directory if you're a
Windows user.

To view the mappings stored in the global.pak file:

skcmaputil dump global.pak

To remove all mappings from the global.pak file:

skcmaputil clear global.pak tmp.pak

To add new mappings stored in a file called 'gamecontrollerdb.txt':

skcmaputil append global.pak tmp.pak gamecontrollerdb.txt

To replace the mappings those from a file called 'gamecontrollerdb.txt':

skcmaputil replace global.pak tmp.pak gamecontrollerdb.txt

If you created new pack file (like 'tmp.pak' in the examples above),
just copy 'tmp.pak' on top of 'global.pak' to put the modified version
in place.

Bugs
----
Bug reports can be filed on the GitHub issues page:
https://github.com/perilsensitive/skcmaputil/issues

Contact
-------
You can contact me at perilsensitive@gmail.com.

Legal Stuff
-----------
Shovel Knight is copyright Yacht Club Games
Steam is copyright Valve

I am not affiliated with either Yacht Club Games or Valve.


