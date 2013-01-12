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

Customize Console
-----------------

Accessing Pattern Space
-----------------------

Accessing Hold Space
--------------------

Embedding In C++
----------------

The QSE::Sed and QSE::StdSed classes are provided for C++. The sample here shows
how to embed QSE::StdSed for stream editing.

 \includelineno sed02.cpp

