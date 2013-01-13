QSESED Embedding Guide                                              {#sed-embed}
================================================================================

Overview
--------

The QSESED library is divided into the core layer and the standard layer.
The core layer is a skeleton implmenetation that requires various callbacks
to be useful. The standard layer provides these callbacks in a general respect.

You can find core layer routines in <qse/sed/sed.h> while you can find standard
layer routines in <qse/sed/std.h>.

Embedding QSESED involves the following steps in the simplest form:

 - create a new sed object
 - compile commands
 - execute commands
 - destroy the sed object

The sample here shows a simple stream editor than can accepts a command string,
and optionally an input file name and an output file name.

 \includelineno sed01.c

You can call qse_sed_compstdfile() instead of qse_sed_compstdstr() to compile
sed commands stored in a file. You can use qse_sed_compstd() or qse_sed_comp()
for more flexibility. 

Locale
------

While QSESED can use a wide character type as the default character type,
the hosting program still has to initialize the locale whenever necessary.
All the samples shown in this page calls a common function 
init_sed_sample_locale(), use the qse_main() macro as the main function,
and call qse_runmain() for cross-platform and cross-character-set support.

Here is the function prototype.

 \includelineno sed00.h

Here goes the actual function.

 \includelineno sed00.c

Note that these two files do not constitute QSEAWK and are used for samples
here only.

Customizing Streams
-------------------

You can use qse_sed_execstd() in customzing the input and output streams.
The sample below uses I/O resources of the #QSE_SED_IOSTD_STR type to use
an argument as input data and let the output to be dynamically allocated.

 \includelineno sed02.c

You can use the core layer function qse_sed_exec() and implement the 
::qse_sed_io_impl_t interface for more flexibility. No samples will
be provided here because the standard layer functions qse_sed_execstd() 
and qse_sed_execstdfile() are the good samples.

Accessing Pattern and Hold Space
--------------------------------

The qse_sed_getspace() allows to you get the pointer and the length
of the pattern space and the hold space. It may not be so useful you 
access them after execution is completed. The qse_sed_setopt() 
function called with #QSE_SED_TRACER lets you set up a hook function 
that can inspect various things during execution time.

The following sample prints the contents of the pattern space and
hold space at each phase of execution.

 \includelineno sed03.c

Embedding In C++
----------------

The QSE::Sed and QSE::StdSed classes are provided for C++. The sample here shows
how to embed QSE::StdSed for stream editing.

 \includelineno sed21.cpp




\skipline ---------------------------------------------------------------------
\skipline the sample files are listed here for example list generation purpose.
\skipline ---------------------------------------------------------------------
\example sed01.c
\example sed02.c
\example sed03.c
\example sed21.cpp
