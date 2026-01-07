# Sampler file manager

This is a file manager for old sampler filesystems. Right now, it only works
with image files (it can't work with device files, yet). Before you attempt to
use it, it would be wise to back any filesystems involved up (including your
computer's). I'm not responsible if this somehow nukes all your HDDs; but, if it
does, do report it so I can try and fix it.


## Supported filesystems

E-Mu (EIII and EIV)  
Roland S5XX (S550 and W30, both CD and HDD)  
Roland S7XX  

S5XX support is read-only. FSCK is only implemented for EMU and S7XX, and it's
pretty basic. MKFS is only implemented for S7XX.


## Building

See building.txt


## License

The main program (src/main.cpp and everything under the GUI directory) is
licensed under the terms of the GPLv3.

The libraries (directories Utils, Tests, min_vfs, Host_FS, E-MU and Roland) are
licensed under the terms of the LGPLv3.

Qt could be licensed either as GPLv3 or LGPLv3. Since main and the GUI are
GPLv3, I guess it makes the most sense if Qt is considered to be licensed under
the GPLv3.

Because library_IDs.hpp would otherwise be left in limbo, it's under the WTFPL.

These licenses can all be found under the "licenses" directory; or  
here https://www.gnu.org/licenses/gpl-3.0.txt,  
here https://www.gnu.org/licenses/lgpl-3.0.txt and  
here https://www.wtfpl.net/about/  
