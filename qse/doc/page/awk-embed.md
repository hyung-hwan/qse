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

 - open a new awk object
 - parse in a source script
 - open a new runtime context
 - execute pattern-action blocks or call a function
 - decrement the reference count of the return value
 - close the runtime context
 - close the awk object

The sample below follows these steps using as many standard layer functions as
possible for convenience sake. It simply prints *hello, world* to the console.

 \includelineno awk01.c

Separation of the awk object and the runtime context was devised to deal with
such cases as you want to reuse the same script over different data streams.
More complex samples concerning this will be shown later.

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

Entry Point
-----------

A typical AWK program executes BEGIN, patten-action, END blocks. QSEAWK provides
a way to drive a AWK program in a different style. That is, you can execute
a particular user-defined function on demand. It can be useful if you want
to drive an AWK program in an event-driven mannger though you can free to
change the entry point for your preference. The qse_awk_rtx_call() function
used is limited to user-defined functions. It is not able to call built-in
functions like *gsub* or *index*.

 \includelineno awk03.c  

If you want to pass arguments to the function, you must create values with 
value creation functions, updates their reference count, and pass them to 
qse_awk_rtx_call(). The sample below creates 2 integer values with 
qse_awk_rtx_makeintval() and pass them to the *pow* function.  

 \includelineno awk04.c

While qse_awk_rtx_call() looks up a function in the function table by name, 
you can find the function in advance and use the information found when 
calling it. qse_awk_rtx_findfun() and qse_awk_rtx_callfun() come to play a role
in this situation. qse_awk_rtx_call() in the sample above can be translated 
into 2 separate calls to qse_awk_rtx_findfun() and qse_awk_rtx_callfun(). 
You can reduce look-up overhead via these 2 functions if you are to execute
the same function multiple times.

 \includelineno awk05.c

Similarly, you can pass a more complex value than a number. You can compose
a map value with qse_awk_rtx_makemapval() or qse_awk_rtx_makemapvalwithdata().
The sample below demonstrates how to use qse_awk_rtx_makemapvalwithdata(),
pass a created map value to qse_awk_rtx_call(), and traverse a map value
returned with qse_awk_rtx_getfirstmapvalitr() and qse_awk_rtx_getnextmapvalitr().

 \includelineno awk06.c

Global Variables
----------------

You can add built-in global variables with qse_awk_addgbl().
Use qse_awk_getgbl() to get information. 

Built-in Functions
------------------

You can add built-in functions with qse_awk_addfnc().
On the other hand, modular built-in functions reside in a shared object.

Single Script over Multiple Data Streams
----------------------------------------

Customizing Language Features
-----------------------------

Creating multiple awk objects
-----------------------------

Memory Pool
-----------

Embedding in C++
-----------------

The QSE::Awk class and QSE::StdAwk classe wrap the underlying C library routines
for better object-orientation. These two classes are defined in <qse/awk/Awk.hpp>
and <qse/awk/StdAwk.hpp> respectively. The embedding task can be simplified despite
slight performance overhead. The hello-world sample in C can be rewritten with 
less numbers of lines in C++.

 \includelineno awk21.cpp

Changes in 0.6.0
----------------

### qse_awk_parsestd() ###

In 0.5.6, it accepted a single script.

    qse_awk_parsestd_t psin;
    psin.type = QSE_AWK_PARSESTD_STR;
    psin.u.str.ptr = src;
    psin.u.str.len = qse_strlen(src);
    qse_awk_parsestd (awk, &psin, QSE_NULL);

In 0.6.X, it accepts an array of scripts.

    qse_awk_parsestd_t psin[2];
    psin[0].type = QSE_AWK_PARSESTD_STR;
    psin[0].u.str.ptr = src;
    psin[0].u.str.len = qse_strlen(src);
    psin[1].type = QSE_AWK_PARSESTD_STR;
    qse_awk_parsestd (awk, psin, QSE_NULL)

### qse_awk_parsestd_t ###

the cmgr field moved from the union member file to the outer structure.

### 0 upon Opening ###
I/O handlers can return 0 for success upon opening.
