Embedding Guid                                                {#embedding-guide}
================================================================================

Overview
---------

The design of the library is divided into two layers: core and standard.
The core layer is a skeleton implmenetation that requires various callbacks
to be useful. The standard layer provides these callbacks in a general respect.
For example, qse_awk_open() in the core layer requires a set of primitive 
functions to be able to create an awk object while qse_awk_openstd() provides 
qse_awk_open() with a standard set of primitive functions.

Embedding QSEAWK involves the following steps in the simplest form:

 - open a new awk object
 - parse in a source script
 - open a new runtime context
 - execute pattern-action blocks or call a function
 - decrement the reference count of the return value
 - close the runtime context
 - close the awk object

\includelineno awk01.c

You can create multiple runtime contexts over a single awk object. It is useful
if you want to execute the same AWK script over different data streams.

