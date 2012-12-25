The directory contains makefiles generated with bakefile(www.bakefile.org)
for non-autoconf environments. 

 - os2-watcom/makefile       Watcom C/C++ for OS/2
 - win32-watcom/makefile     Watcom C/C++ for Windows
 - win32-borland/makefile    Borland C/C++ for Windows
 - win32-msvc/makefile       Microsoft Visual C/C++ for Windows

These makefiles contain some options - BUILD, CHAR, XCMGRS, BUNDLED_UNICODE.

To build a debug version with the default wide-charcter type for OS/2 
using the Watcom C/C++ compiler, you can do this.

 - cd os2-watcom
 - wmake BUILD=debug CHAR=wchar

Use relevant native tools for other supported environments.

You can execute the following commands to regenerate the makefiles.

 - bakefile_gen -d os2.bkgen
 - bakefile_gen -d win32.bkgen

