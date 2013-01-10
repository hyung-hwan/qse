QSESED Commands                                                       {#sed-cmd}
================================================================================

Overview
--------
A stream editor is a non-interactive text editing tool commonly used
on Unix environment. It reads text from an input stream, stores it to
pattern space, manipulates the pattern space by applying a set of editing
commands, and writes the pattern space to an output stream. Typically, the
input and output streams are a console or a file. 

Commands
--------

A sed command is composed of:

 - line selector (optional)
 - ! (optional)
 - command code
 - command arguments (optional, dependent on command code)

A line selector selects input lines to apply a command to and has the following
forms:
 - address - specify a single address
 - address,address - specify an address range 
 - start~step - specify a starting line and a step. 
                #QSE_SED_EXTENDEDADR enables this form.

An *address* is a line number, a regular expression, or a dollar sign ($) 
while a *start* and a *step* is a line number. 

A regular expression for an address has the following form:
 - /rex/ - a regular expression *rex* is enclosed in slashes
 - \\CrexC - a regular expression *rex* is enclosed in \\C and *C*
            where *C* can be any character.

It treats the \\n sequence specially to match a newline character.

Here are examples of line selectors:
 - 10 - match the 10th line
 - 10,20 - match lines from the 10th to the 20th.
 - /^[[:space:]]*$/ - match an empty line 
 - /^abc$/,/^def$/ - match all lines between *abc* and *def* inclusive
 - 10,$ - match the 10th line down to the last line.
 - 3~4 - match every 4th line from the 3rd line.

Note that an address range always selects the line matching the first address
regardless of the second address; For example, 8,6 selects the 8th line.

The exclamation mark(!), when used after the line selector and before
the command code, negates the line selection; For example, 1! selects all
lines except the first line.

A command without a line selector is applied to all input lines; 
A command with a single address is applied to an input line that matches 
the address; A command with an address range is applied to all input 
lines within the range, inclusive; A command with a start and a step is
applied to every <b>step</b>'th line starting from the line start.

Here is the summary of the commands.

### # comment ###
The text beginning from # to the line end is ignored; # in a line following
<b>a \\</b>, <b>i \\</b>, and <b>c \\</b> is treated literally and does not
introduce a comment.

### : label ###
A label can be composed of letters, digits, periods, hyphens, and underlines.
It remembers a target label for *b* and *t* commands and prohibits a line
selector.

### { ###
The left curly bracket forms a command group where you can nest other 
commands. It should be paired with an ending }.

### q ###
Terminates the exection of commands. Upon termination, it prints the pattern
space if #QSE_SED_QUIET is not set.

### Q ###
Terminates the exection of commands quietly.

### a \\ \n text ###
Stores *text* into the append buffer which is printed after the pattern 
space for each input line. If #QSE_SED_STRICT is on, an address range is not
allowed for a line selector. If #QSE_SED_SAMELINE is on, the backslash and the 
text can be located on the same line without a line break.

### i \\ \n text ###
Inserts *text* into an insert buffer which is printed before the pattern
space for each input line. If #QSE_SED_STRICT is on, an address range is not
allowed for a line selector. If #QSE_SED_SAMELINE is on, the backslash and the
text can be located on the same line without a line break.

### c \\ \n text ###
If a single line is selected for the command (i.e. no line selector, a single
address line selector, or a start~step line selector is specified), it changes
the pattern space to *text* and branches to the end of commands for the line.
If an address range is specified, it deletes the pattern space and branches 
to the end of commands for all input lines but the last, and changes pattern
space to *text* and branches to the end of commands. If #QSE_SED_SAMELINE is
on, the backlash and the text can be located on the same line without a line
break.

### d ###
Deletes the pattern space and branches to the end of commands.

### D ###
Deletes the first line of the pattern space. If the pattern space is emptied,
it branches to the end of script. Otherwise, the commands from the first are 
reapplied to the current pattern space.

### = ###
Prints the current line number. If #QSE_SED_STRICT is on, an address range is 
not allowed as a line selector.

### p ###
Prints the pattern space.

### P ###
Prints the first line of the pattern space.

### l ###
Prints the pattern space in a visually unambiguous form.

### h ###
Copies the pattern space to the hold space

### H ###
Appends the pattern space to the hold space

### g ###
Copies the hold space to the pattern space

### G ###
Appends the hold space to the pattern space

### x ###
Exchanges the pattern space and the hold space

### n ###
Prints the pattern space and read the next line from the input stream to fill
the pattern space.

### N ###
Prints the pattern space and read the next line from the input stream 
to append it to the pattern space with a newline inserted.

### b ###
Branches to the end of commands.

### b label ###
Branches to *label*

### t ###
Branches to the end of commands if substitution(s//) has been made 
successfully since the last reading of an input line or the last *t* command.

### t label ###
Branches to *label* if substitution(s//) has been made successfully 
since the last reading of an input line or the last *t* command.

### r file ###
Reads text from *file* and prints it after printing the pattern space but 
before printing the append buffer. Failure to read *file* does not cause an
error.

### R file ###
Reads a line of text from *file* and prints it after printing pattern space 
but before printing the append buffer. Failure to read *file* does not cause
an error.

### w file ###
Writes the pattern space to *file*

### W file ####
Writes the first line of the pattern space to *file*

### s/rex/repl/opts ###
Finds a matching substring with *rex* in pattern space and replaces it 
with *repl*. An ampersand(&) in *repl* refers to the matching substring. 
*opts* may be empty; You can combine the following options into *opts*:

 - *g* replaces all occurrences of a matching substring with *rex*
 - *number* replaces the <b>number</b>'th occurrence of a matching substring 
   with *rex*
 - *p* prints pattern space if a successful replacement was made
 - *w* file writes pattern space to *file* if a successful replacement 
      was made. It, if specified, should be the last option.

### y/src/dst/ ###
Replaces all occurrences of characters in *src* with characters in *dst*.
*src* and *dst* must contain equal number of characters.

### C/selector/opts ###
Selects characters or fields from the pattern space as specified by the
*selector* and update the pattern space with the selected text. A selector
is a comma-separated list of specifiers. A specifier is one of the followings:

 + *d* specifies the input field delimiter with the next character. e.g) d:
 + *D* sepcifies the output field delimiter with the next character. e.g) D;
 + *c* specifies a position or a range of characters to select. e.g) c23-25
 + *f* specifies a position or a range of fields to select. e.g) f1,f4-3

*opts* may be empty; You can combine the following options into *opts*:

 + *f* folds consecutive delimiters into one.
 + *w* uses white spaces for a field delimiter regardless of the input
       delimiter specified in the selector.
 + *d* deletes the pattern space if the line is not delimited by 
       the input field delimiter

In principle, this can replace the *cut* utility with the *C* command.

Examples
--------

Here are some samples.

### G;G;G ###
Triple spaces input lines. If #QSE_SED_QUIET is on, <b>G;G;G;p</b>. 
It works because the hold space is empty unless something is copied to it.

### $!d ###
Prints the last line. If #QSE_SED_QUIET is on, try <b>$p</b>.

### 1!G;h;$!d ###
Prints input lines in the reverse order. That is, it prints the last line 
first and the first line last.

    $ echo -e "a\nb\nc" | qsesed '1!G;h;$!d'
    c
    b
    a

### s/[[:space:]]{2,}/ /g ###
Compacts whitespaces if #QSE_SED_EXTENDEDREX is on.

### C/d:,f3,1/ ###
Prints the third field and the first field from a colon separated text.

    $ head -5 /etc/passwd
    root:x:0:0:root:/root:/bin/bash
    daemon:x:1:1:daemon:/usr/sbin:/bin/sh
    bin:x:2:2:bin:/bin:/bin/sh
    sys:x:3:3:sys:/dev:/bin/sh
    sync:x:4:65534:sync:/bin:/bin/sync
    $ qsesed '1,3C/d:,f3,1/;4,$d' /etc/passwd 
    0 root
    1 daemon
    2 bin
