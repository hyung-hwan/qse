Embedding Guide                                               {#embedding-guide}
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


Changes
-------

### qse_awk_parsestd() ###

In 0.5.6, it accepted a single script.

    qse_awk_parsestd_t psin;
    psin.type = QSE_AWK_PARSESTD_STR;
    psin.u.str.ptr = src;
    psin.u.str.len = qse_strlen(src);
    qse_awk_parsestd (awk, &psin, QSE_NULL);

In 0.6.0 or later, it accepts an array of scripts.

    qse_awk_parsestd_t psin[2];
    psin[0].type = QSE_AWK_PARSESTD_STR;
    psin[0].u.str.ptr = src;
    psin[0].u.str.len = qse_strlen(src);
    psin[1].type = QSE_AWK_PARSESTD_STR;
    qse_awk_parsestd (awk, psin, QSE_NULL)

### qse_awk_parsestd_t ###
the cmgr field moved from the union member file to the outer structure.


