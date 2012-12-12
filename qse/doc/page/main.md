QSE                                       {#mainpage}
===================
@image html qse-logo.png 

@section qse_intro INTRODUCTION

The QSE library implements AWK, SED, and Unix commands in an embeddable form 
and defines data types, functions, and classes that you can use when you embed 
them into an application. It also provides more fundamental data types and 
funtions needed when you deal with memory, streams, data structures.
The interface has been designed to be flexible enough to access various 
aspects of embedding application and an embedded object from each other. 

The library is licensed under the GNU Lesser General Public License version 3:
http://www.gnu.org/licenses/

The project webpage: http://code.abiyo.net/@qse

For further information, contact:
Chung, Hyung-Hwan <hyunghwan.chung@gmail.com>

@section components MODULES

See the subpages for various modules available in this library.

- @subpage mem "Memory Management"
- @subpage cenc "Character Encoding"
- @subpage io  "I/O Handling"
- @subpage awk "AWK Interpreter" 
- @subpage sed "SED Stream Editor" 
- @subpage awk-lang "QSEAWK Language" 

@section installation INSTALLATION

@subsection build_from_source BUILINDG FROM A SOURCE PACKAGE

The package uses the standard autoconf build systems. Briefly, you can run 
@b configure and @b make to compile and install it. Here is the simple 
instruction.

Unpack the latest source package downloaded from:
- http://code.google.com/p/qse/downloads/list

Alternatively, you can check out the lastest source code from the subversion 
repository by executing the following command:
- svn checkout http://qse.googlecode.com/svn/trunk/qse/ 

Run @b configure and @b make to compile and install it:

@code
$ ./configure
$ make	
$ make install
@endcode

For additional command line options to @b configure, run @b configure @b --help.

@subsection crosscompile_win32 CROSS-COMPILING FOR WIN32

While the package does not provide build files for native WIN32/WIN64 compilers,
you can cross-compile it for WIN32/WIN64 with a cross-compiler. Get a 
cross-compiler installed first and run @b configure with a host and a target.

With MINGW-W64, you may run @b configure as shown below for WIN32:

@code
$ ./configure --host=i686-w64-mingw32 --target=i686-w64-mingw32
$ make
$ make install
@endcode

With MINGW-W64, you may run @b configure as shown below for WIN64:

@code
$ ./configure --host=x86_64-w64-mingw32 --target=x86_64-w64-mingw32
$ make
$ make install
@endcode

The actual host and target names may vary depending on the cross-compiler 
installed.

@subsection mchar_mode MULTI-BYTE CHARACTER MODE

By default, the package is compiled for wide character mode. However, 
you can compile it for multi-byte character mode by running @b configure 
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
