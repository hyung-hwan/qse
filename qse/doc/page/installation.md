Installation                                                     {#installation}
================================================================================

Source Package
--------------

You can download the source package from 

    http://code.google.com/p/qse/downloads/list

A source package has this naming format of *qse-<version>.tar.gz*.

Alternatively, you can check out the lastest source files from the subversion
repository by executing the following command:

    svn checkout http://qse.googlecode.com/svn/trunk/qse/

Building on Unix/Linux
----------------------

The project uses the standard autoconf/automake generated script files for 
buildiing. If you work on the systems where these scripts can run, you can 
follow the standard procedures of configuring and making the project.

    $ ./configure
    $ make
    $ make install

You can use this method of building for MinGW or Cygwin on Windows.
 
Cross-compiling for WIN32
-------------------------

While the autoconf/automake scripts may not support your native compilers,
you can cross-compile it for WIN32/WIN64 with a cross-compiler. Get a 
cross-compiler installed first and run the *configure* script with a host 
and a target.

With MINGW-W64, you may run *configure* as shown below:

    $ ./configure --host=i686-w64-mingw32 --target=i686-w64-mingw32
    $ make
    $ make install

With MINGW-W64, you may run *configure* as shown below:

    $ ./configure --host=x86_64-w64-mingw32 --target=x86_64-w64-mingw32
    $ make
    $ make install

The actual host and target names may vary depending on the cross-compiler 
installed.

Native Makefiles 
----------------

The project provides makefiles for some selected compilers and platforms.
The makefiles were generated with bakefile (www.bakefile.org) and can be
found in the *bld* subdirectory.

 - os2-watcom/makefile      (Watcom C/C++ for OS/2)
 - win32-watcom/makefile    (Watcom C/C++ for Windows)
 - win32-borland/makefile   (Borland C/C++ for Windows)
 - win32-msvc/makefile      (Microsoft Visual C/C++ for Windows)

You can execute your native make utility for building in each subdirectory.
For example, to build for OS/2 with Watcom C/C++ in the result mode using
the wide character type, you can execute this:

    cd bld\os2-watcom
    wmake BUILD=release CHAR=wchar

Build Options
-------------

The configure script and the native makefiles provides some options that you
can use to change the build environment. The options presented here can be
specified to the command line of the configure script or the native make 
utilities. 

For the configure script, the options should prefixed with double 
slashes and mutliples options can be specified together. See this example:

    ./configure --enable-debug --disable-wchar


For the native makefiles, the options can be appened to the end of the command 
line. See this example:

     make BUILD=debug CHAR=mchar

### Build Mode ###

You can choose to build the project in the **release** mode or in the **debug**
mode. The resulting libraries and programs in the **debug** mode contain
extra information useful for debugging. The default mode is **release**.

 value   | configure      | native makefile
 --------|----------------|-----------------
 debug   | enable-debug   | BUILD=debug
 release | disable-debug  | BUILD=release

### Character Type ###

You can choose between the wide charcter type and the multi-byte character
type as a basic character type represented in the #qse_char_t type. The default
character type is the wide character type.

 value      | configure      | native makefile
 -----------|----------------|-----------------
 wide       | enable-wchar   | CHAR=wchar
 multi-byte | disable-wchar  | CHAR=mchar

If the wide charater type is chosen: 
 - #QSE_CHAR_IS_WCHAR is defined.
 - #qse_char_t maps to #qse_wchar_t.

If the multi-byte charater type is chosen: 
 - #QSE_CHAR_IS_MCHAR is defined.
 - #qse_char_t maps to #qse_mchar_t.

### Bundled Unicode Routines ###

You can choose to use the bundled character classification routines 
based on unicode. It is disabled by default.

 value      | configure                | native makefile
 -----------|--------------------------|-----------------
 on         | enable-bundled-unicode   | BUNDLED_UNICODE=on
 off        | disable-bundled-unicode  | BUNDLED_UNICODE=off

Enabling this option makes the routines defined in <qse/cmn/uni.h> 
to be included in the resulting library. It also affects somes routines
defined in <qse/cmn/chr.h> to use these bundled unicode routines.

### Character Encoding Conversion ###

You can include extra routines for character encoding conversion into
the resulting library. This option is disabled by default.

 value      | configure       | native makefile
 -----------|-----------------|---------------------
 on         | enable-xcmgrs   | XCMGRS=on
 off        | disable-xcmgrs  | XCMGRS=off

More #qse_cmgr_t instances are made available when this option is enabled.
The UTF-8 conversion and the locale-based conversion are included regardless
of this option.

### TCPV40HDRS ###

The option, when turned on, enables you to use *tcp32dll.dll* and *so32dll.dll*
instead of *tcpip32.dll*. Doing so allows a resulting program to run on OS/2 
systems without the 32-bit TCP/IP stack. This option is off by default and 
available for the native makefile for Watcom C/C++ for OS/2 only.

    wmake TCPV40HDRS=on

### SCO UNIX System V/386 Release 3.2 ###

- If /usr/include/netinet and /usr/include/net are missing,
  check if there are /usr/include/sys/netinet and /usr/include/sys/net.
  f they exists, you can make these symbolic links.

    cd /usr/include
    ln -sf sys/netinet netinet
    ln -sf sys/net net

- Specify GREP if configure fails to find an acceptable grep.
- Build in the source tree. Building outside the source tree is likely to fail
  for dificiency of the bundled make utility.
- Do not include -g in CFLAGS. 

    ./configure GREP=/bin/grep CFLAGS=""

- Change RANLIB from "ranlib" to "true" in libltdl/libtool.

    make

### More options ###

More options are available for the configure script. Execute this for more 
information:

    ./configure --help

