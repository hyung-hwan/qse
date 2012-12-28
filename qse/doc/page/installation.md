Installation                                    {#installation}
============

## Source Package ##

You can download the source package from 

    http://code.google.com/p/qse/downloads/list

A source package has this naming format of *qse-<version>.tar.gz*.

Alternatively, you can check out the lastest source files from the subversion
repository by executing the following command:

    svn checkout http://qse.googlecode.com/svn/trunk/qse/

## Building on Unix/Linux ##

The project uses the standard autoconf/automake generated script files for 
buildiing. If you work on the systems where these scripts can run, you can 
follow the standard procedures of configuring and making the project.

    $ ./configure
    $ make
    $ make install

You can use this method of building for MinGW or Cygwin on Windows.
 
## Cross-compiling for WIN32 ##

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

## Native Makefiles ##

The project provides makefiles for some selected compilers and platforms.
The makefiles were generated with bakefile (www.bakefile.org) and can be
found in the *bld* subdirectory.

 - os2-watcom/makefile      (Watcom C/C++ for OS/2)
 - win32-watcom/makefile    (Watcom C/C++ for Windows)
 - win32-borland/makefile   (Borland C/C++ for Windows)
 - win32-msvc/makefile      (Microsoft Visual C/C++ for Windows)

You can execute your native make utility for building in each subdirectory.

## Build Options ## 

### MULTI-BYTE CHARACTER MODE ###

By default, the package is compiled for the wide character mode. However, 
you can compile it for the multi-byte character mode by running @b configure 
@b --disable-wchar.

@code
$ ./configure --disable-wchar
$ make
$ make install
@endcode

Under the multi-byte character mode:
- #QSE_CHAR_IS_MCHAR is defined.
- #qse_char_t maps to #qse_mchar_t.

Under the wide character mode:
- #QSE_CHAR_IS_WCHAR is defined.
- #qse_char_t maps to #qse_wchar_t.

#qse_mchar_t maps to @b char and #qse_wchar_t maps to @b wchar_t or equivalent.

### TCPV40HDRS ###

The option, when turned on, enables you to use *tcp32dll.dll* and *so32dll.dll*
instead of *tcpip32.dll*. Doing so allows a resulting program to run on OS/2 
systems without the 32-bit TCP/IP stack. This option is off by default and 
available for the native makefile for Watcom C/C++ for OS/2 only.

    wmake TCPV40HDRS=on


