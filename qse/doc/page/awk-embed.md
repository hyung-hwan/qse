QSEAWK Embedding Guide                                              {#awk-embed}
================================================================================

Overview
---------

The QSEAWK library is divided into two layers: core and standard.
The core layer is a skeleton implmenetation that requires various callbacks
to be useful. The standard layer provides these callbacks in a general respect.
For example, qse_awk_open() in the core layer requires a set of primitive 
functions to be able to create an awk object while qse_awk_openstd() provides 
qse_awk_open() with a standard set of primitive functions. 

The core layer is defined in <qse/awk/awk.h> while the standard layer is 
defined in <qse/awk/std.h>. Naming-wise, a standard layer name contains *std*
over its corresponding core layer name. 

Embedding QSEAWK involves the following steps in the simplest form:

 - create a new awk object
 - parse in a source script
 - create a new runtime context
 - execute pattern-action blocks or call a function
 - decrement the reference count of the return value
 - destroy the runtime context
 - destroy the awk object

The sample below follows these steps using as many standard layer functions as
possible for convenience sake. It simply prints *hello, world* to the console.

 \includelineno awk01.c

Separation of the awk object and the runtime context was devised to deal with
such cases as you want to reuse the same script over different data streams.
More complex samples concerning this will be shown later.

Locale
------

While QSEAWK can use a wide character type as the default character type,
the hosting program still has to initialize the locale whenever necessary.
All the samples to be shown from here down will call a common function 
init_awk_sample_locale(), use the qse_main() macro as the main function,
and call qse_runmain() for cross-platform and cross-character-set support.

Here is the function prototype.

 \includelineno awk00.h

Here goes the actual function.

 \includelineno awk00.c

Note that these two files do not constitute QSEAWK and are used for samples
here only.

Customizing Console I/O
-----------------------

The qse_awk_rtx_openstd() function implements I/O related callback functions
for files, pipes, and the console. While you are unlikely to change the 
definition of files and pipes, the console is the most frequently customized 
I/O object. Most likely, you may want to feed the console with a string or 
something and capture the console output into a buffer. Though you can define
your own callback functions for files, pipes, and the console, it is possible
to override the callback functions implemented by qse_awk_rtx_openstd() 
partially. This sample redefines the console handler while keeping the file 
and pipe handler by qse_awk_rtx_openstd().

 \includelineno awk02.c

Extention Area
--------------

When creating an awk object or a runtime context object, you can ask
a private extension area to be allocated with the main object. You can 
use this extension area to store data associated with the object.
You can specify the size of the extension area when calling qse_awk_open(),
qse_awk_rtx_open(), qse_awk_openstd(), and qse_awk_rtx_openstd(). 
These functions iniitlize the area to zeros. You can get the pointer
to the beginning of the area with qse_awk_getxtn(), qse_awk_rtx_getxtn(),
qse_awk_getxtnstd(), qse_awk_rtx_getxtnstd() respectively.

In the sample above, the string and the buffer used for I/O customization
are declared globally. When you have multiple runtime contexts and independent
console strings and buffers, you may want to associate a runtime context
with an independent console string and buffer. The extension area that can 
be allocated on demand when you create a runtime context comes in handy. 
The sample below shows how to associate them through the extension area 
but does not create multiple runtime contexts for simplicity.

 \includelineno awk03.c

Entry Point
-----------

A typical AWK program executes BEGIN, patten-action, END blocks. QSEAWK provides
a way to drive a AWK program in a different style. That is, you can execute
a particular user-defined function on demand. It can be useful if you want
to drive an AWK program in an event-driven mannger though you can free to
change the entry point for your preference. The qse_awk_rtx_call() function
used is limited to user-defined functions. It is not able to call built-in
functions like *gsub* or *index*.

 \includelineno awk04.c  

If you want to pass arguments to the function, you must create values with 
value creation functions, updates their reference count, and pass them to 
qse_awk_rtx_call(). The sample below creates 2 integer values with 
qse_awk_rtx_makeintval() and pass them to the *pow* function.  

 \includelineno awk05.c

While qse_awk_rtx_call() looks up a function in the function table by name, 
you can find the function in advance and use the information found when 
calling it. qse_awk_rtx_findfun() and qse_awk_rtx_callfun() come to play a role
in this situation. qse_awk_rtx_call() in the sample above can be translated 
into 2 separate calls to qse_awk_rtx_findfun() and qse_awk_rtx_callfun(). 
You can reduce look-up overhead via these 2 functions if you are to execute
the same function multiple times.

 \includelineno awk06.c

Similarly, you can pass a more complex value than a plain number or string. 
You can compose a map value with qse_awk_rtx_makemapval() or 
qse_awk_rtx_makemapvalwithdata(). The following sample demonstrates how to
use qse_awk_rtx_makemapvalwithdata(), pass a created map value to 
qse_awk_rtx_call(), and traverse a map value returned with 
qse_awk_rtx_getfirstmapvalitr() and qse_awk_rtx_getnextmapvalitr().

 \includelineno awk07.c

Built-in Global Variables
--------------------------

QSEAWK predefines global variables such as *SUBSEP* and *ARGC*.  You can add 
your own built-in variables in the global scope with qse_awk_addgbl(). You 
must add new variables before qse_awk_parse() or qse_awk_parsestd(). Later,
you can get the values of the global variables using qse_awk_rtx_getgbl()
with an ID returned by qse_awk_addgbl(). The IDs of the predefined global 
variables are available as the ::qse_awk_gbl_id_t type values

 \includelineno awk08.c

Built-in Functions
------------------

QSEAWK predefines built-in functions like *match* and *gsub*. You can add your
own built-in function with qse_awk_addfnc(). The following sample shows how to 
add a function named *basename* that get the base file name part of a path name.

 \includelineno awk09.c

In the sample above, the *basename* function returns a resulting string. In 
case of any implemenation errors, it would cause the runtime context to abort
with an error since it returned -1. To avoid the situation, you may change
the way basename() works by defining it to return the resulting string via
the second parameter and return 0 or -1 as a return value. For the arguements
to pass by reference, you can specify the letter *r* into the *arg.spec* field
at the argument position. That is, speciying *r* at the second position in
the *arg.spec* string means that you want to pass the second argument by
reference.

 \includelineno awk10.c

Customizing Other Behaviors
---------------------------

QSEAWK comes with more more trait options that you can use to change the
behavior. For instance, you have seen how to disable the standard BEGIN, 
END, pattern-action blocks by turning off the #QSE_AWK_PABLOCK trait option
in several sample program above. 

The ::qse_awk_trait_t type defines various trait options that you can turn
on or off using qse_awk_setopt() with #QSE_AWK_TRAIT. The following code 
snippet shows how to disable all built-in I/O statements like *getline*, 
*print*, *printf*, *close*, *fflush*, piping, and file redirection. 
Additionally, it disables the BEGIN, END, pattern-action blocks.

~~~~~{.c}
 qse_awk_getopt (awk, QSE_AWK_TRAIT, &opt);
 opt &= ~QSE_AWK_PABLOCK;
 opt &= ~QSE_AWK_RIO;
 qse_awk_setopt (awk, QSE_AWK_TRAIT, &opt);
~~~~~

This way, you can change the QSEAWK language behave differently for your
own needs.

Multiple Instances
------------------

The awk object and the runtime context object reside in its own memory blocks
allocated and maintain related information in their own object space. Multiple
instances created are independent of each other.

You can run a script over multiple data streams by creating multiple runtime
context objects from a single awk object.

TBD.

Memory Pool
-----------

You can confine the information used for an awk object include the related
runtime context objects in a single memory pool.

TBD.

Writing Modules
---------------

Modular built-in functions and variables reside in a shared object.

TBD.

Embedding in C++
-----------------

The QSE::Awk class and QSE::StdAwk classe wrap the underlying C library routines
for better object-orientation. These two classes are defined in <qse/awk/Awk.hpp>
and <qse/awk/StdAwk.hpp> respectively. The embedding task can be simplified despite
slight performance overhead. The hello-world sample in C can be rewritten with 
less numbers of lines in C++.

 \includelineno awk21.cpp

Customizing the console I/O is not much different in C++. When using the
QSE::StdAwk class, you can inherit the class and implement these five methods:

 - int openConsole (Console& io);
 - int closeConsole (Console& io);
 - int flushConsole (Console& io);
 - int nextConsole (Console& io);
 - ssize_t readConsole (Console& io, char_t* data, size_t size);
 - ssize_t writeConsole (Console& io, const char_t* data, size_t size);

The sample below shows how to do it to use a string as the console input
and store the console output to a string buffer.

 \includelineno awk22.cpp

Alternatively, you can choose to implement QSE::Awk::Console::Handler
and call QSE::Awk::setConsoleHandler() with the implemented handler.
This way, you do not need to inherit QSE::Awk or QSE::StdAwk.
The sample here shows how to customize the console I/O by implementing
QSE::Awk::Console::Handler. It also shows how to run the same script
over two different data streams in a row.

 \includelineno awk23.cpp


Changes in 0.6.0
----------------

### qse_awk_parsestd() ###

The second parameter of qse_awk_parsestd() specifies the input script.

In 0.5.6, it accepted a single script for input.

~~~~~{.c}
 qse_awk_parsestd_t psin;
 psin.type = QSE_AWK_PARSESTD_STR;
 psin.u.str.ptr = src;
 psin.u.str.len = qse_strlen(src);
 qse_awk_parsestd (awk, &psin, QSE_NULL);
~~~~~

In 0.6.X, it accepts an array of scripts for input. To specify a single script,
use an array of 2 elements whose last element is of the #QSE_AWK_PARSESTD_NULL
type.

~~~~~{.c}
 qse_awk_parsestd_t psin[2];
 psin[0].type = QSE_AWK_PARSESTD_STR;
 psin[0].u.str.ptr = src;
 psin[0].u.str.len = qse_strlen(src);
 psin[1].type = QSE_AWK_PARSESTD_NULL;
 qse_awk_parsestd (awk, psin, QSE_NULL);
~~~~~

### 0 upon Opening ###
I/O handlers can return 0 for success upon opening.



\skipline ---------------------------------------------------------------------
\skipline the sample files are listed here for example list generation purpose.
\skipline ---------------------------------------------------------------------
\example awk01.c
\example awk02.c
\example awk03.c
\example awk04.c
\example awk05.c
\example awk06.c
\example awk07.c
\example awk08.c
\example awk09.c
\example awk10.c
\example awk21.cpp
\example awk22.cpp
\example awk23.cpp

