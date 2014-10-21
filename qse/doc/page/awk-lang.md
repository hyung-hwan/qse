QSEAWK Language                                                      {#awk-lang}
================================================================================

#
=======

 Overview
-------------

QSEAWK implements the language described in the 
[The AWK Programming Language][awkbook] with extensions.

QSEAWK reads an AWK program, recognizes various tokens contained while skipping 
comments and whitespaces that don't constinute a token, analyses syntax, and
tranforms them to an internal form for execution.



 Program Structure
----------------------

A QSEAWK program can be composed of the following elements at the top level.

 - pattern-action block pair
 - action block without a pattern
 - pattern without an action block
 - user-defined function
 - \@global variable definition
 - \@include statement 

However, none of the above is mandatory. QSEAWK accepts an empty program.

A typical pattern-action pair is composed of a pattern and an action block 
as shown below:

~~~~~{.awk}
 pattern {
   statement
   statement
   ...
 }
~~~~~

A pattern can be one of the followings when specified:

 - expression
 - first-expression, last-expression
 - *BEGIN*
 - *END*

An action block is a series of statements enclosed in a curly bracket pair.
The *BEGIN* and *END* patterns require an action block while normal patterns
don't. When no action block is specified for a normal pattern, it is treated
as if `{ print $0; }` is specified.

QSEAWK executes the action block for the *BEGIN* pattern when it starts 
executing a program; No start-up action is taken if no *BEGIN* pattern-action 
pair is specified. If a normal pattern-action pair and/or the *END* 
pattern-action is specified, it reads the standard input stream. For each input
line it reads, it checks if a normal pattern expression evaluates to true. For
each pattern that evaluates to true, it executes the action block specified for
the pattern. When it reaches the end of the input stream, it executes the 
action block for the *END* pattern.

QSEAWK allows zero or more *BEGIN* patterns. When multiple *BEGIN* patterns
are specified, it executes their action blocks in the order they appear in the
program. The same applies to the *END* patterns and their action blocks. It 
doesn't read the standard input stream for programs composed of BEGIN
blocks only whereas it reads the stream as long as there is an action block
for an END pattern or a normal pattern. It evaluates an empty pattern to true;
As a result, the action block for an empty pattern is executed for all input
lines read.

You can compose a pattern range by putting 2 patterns separated by a comma.
The pattern range evaluates to true once the first expression evaluates to
true until the last expression evaluates to true.

The following code snippet is a valid QSEAWK program that prints the string
*hello, world* to the console and exits.

~~~~~{.awk}
 BEGIN {
   print "hello, world";
 }
~~~~~

This program prints "hello, world" followed by "hello, all" to the console.

~~~~~{.awk}
 BEGIN {
   print "hello, world";
 }
 BEGIN {
   print "hello, all";
 }
~~~~~

For the following text input,
~~~~~{.txt}
 abcdefgahijklmn
 1234567890
 opqrstuvwxyzabc
 9876543210
~~~~~

this program
~~~~~{.awk}
 BEGIN { mr=0; my_nr=0; }
 /abc/ { print "[" $0 "]"; mr++; }
 { my_nr++; }
 END { 
   print "total records: " NR; 
   print "total records selfcounted: " my_nr; 
   print "matching records: " mr; 
 }
~~~~~

produces the output text like this:
~~~~~{.txt}
 [abcdefgahijklmn]
 [opqrstuvwxyzabc]
 total records: 4
 total records selfcounted: 4
 matching records: 2
~~~~~

See the table for the order of execution indicated by the number and the result 
of pattern evaluation enclosed in parenthesis. The action block is executed if
the evaluation result is true.

 |                                   | START-UP | abcdefgahijklmn | 1234567890 | opqrstuvwxyzabc | 9876543210 | SHUTDOWN |
 |-----------------------------------|----------|-----------------|------------|-----------------|------------|----------|
 | BEGIN { mr = 0; my_nr=0; }        | 1(true)  |                 |            |                 |            |          |
 | /abc/ { print "[" $0 "]"; mr++; } |          | 2(true)         | 4(false)   | 6(true)         | 8(false)   |          |
 | { my_nr++; }                      |          | 3(true)         | 5(true)    | 7(true)         | 9(true)    |          |
 | END { print ... }                 |          |                 |            |                 |            | 10(true) | 


For the same input, this program shows how to use a ranged pattern.
~~~~~{.awk}
 /abc/,/stu/ { print "[" $0 "]"; }
~~~~~

It produces the output text like this:
~~~~~{.txt}
 [abcdefgahijklmn]
 [1234567890]
 [opqrstuvwxyzabc]
~~~~~

The regular expression /abc/ matches the first input line and /stu/ matches the
third input line. So the range is true between the first input line and the
third input line inclusive.


> ### Note ###
> The QSEAWK engine provides a way to use a user-defined function as an entry
> point instead of executing pattern-action block pairs. When you choose
> to use a user-defined function as an entry point, it doesn't execute such 
> pairs. 
> Read \ref awk-embed and see qse_awk_rtx_call() and qse_awk_rtx_callfun() for 
> how to change the entry point programatically.


 Comments
--------------

A single-line comment is introduced by a hash character #, and is terminated at 
the end of the same line. Additionally, it supports a C-style multi-line comment
enclosed in /* and */. The multi-line comment can't nest and can't appear within
string literals and regular expressions.

~~~~~{.awk}
 x = y; # assign y to x.
 /*
 this line is ignored.
 this line is ignored too.
 */
~~~~~




 Tokens
------------

When QSEAWK parses a program, it classifies a series of input characters 
into meaningful tokens. It can extract the smallest meaningful unit through
this tokenization process. 

### Reserved Words ###

The following words are reserved and cannot be used as a variable name,
a parameter name, or a function name.

 - BEGIN
 - END
 - function
 - \@local
 - \@global
 - \@include
 - if
 - else
 - while
 - for
 - do
 - break
 - continue
 - return
 - exit
 - \@abort
 - delete
 - \@reset
 - next
 - nextfile
 - nextofile
 - print
 - printf
 - getline

However, these words can be used as normal names in the context of a
module call. For example, mymod::break.

In practice, the predefined names used for built-in commands, functions,
and variables are treated as if they are reserved since you can't create
another denifition with the same name.

### Numbers ###

An integer begins with a numeric digit between 0 and 9 inclusive and can be 
followed by more numeric digits. If an integer is immediately followed by a 
floating point, and optionally a series of numeric digits without whitespaces,
it becomes a floting-point number. An integer or a simple floating-point number
can be followed by e or E, and optionally a series of numeric digits with a
optional single sign letter. A floating-point number may begin with a floting
point with a preceeding number. 

    369   # integer
    3.69  # floating-pointe number
    13.   # 13.0
    .369  # 0.369
    34e-2 # 34 * (10 ** -2)
    34e+2 # 34 * (10 ** 2)
    34.56e # 34.56 
    34.56E3 

An integer can be prefixed with 0x, 0, 0b for a hexa-decimal number, an octal 
number, and a binary number respectively. For a hexa-decimal number, letters 
from A to F can form a number case-insenstively in addition to numeric digits.

    0xA1   # 161
    0xB0b0 # 45232
    020    # 16
    0b101  # 5

If the prefix is not followed by any numeric digits, it is still a valid token 
and represents the value of 0.

    0x # 0x0 but not desirable.
    0b # 0b0 but not desirable.

### Strings ###

A string is enclosed in a pair of double quotes or single quotes.

A character in a string enclosed in the double-quotes, when preceded with 
a back-slash, changes the meaning. 

 - \\a - alert
 - \\b - backspace
 - \\f - formfeed
 - \\n - newline
 - \\r - carriage return
 - \\t - horizontal tab
 - \\v - vertical tab
 - \\\\ - backslash
 - \\" - double quote

You can specify a character with an octal number or a hexadecimal number.
The actual value can range between 0 and 255 inclusive.

 - \\OOO - O is an octal digit.  
 - \\xXX - X is a hexadecimal digit. 

In the octal sequence, you can specify up to 3 octal digits after \\; In the 
hexadecimal sequence, you can specify as many hexadecimal digits as possible 
after \\x.  

~~~~~{.awk}
 BEGIN { 
   print "\xC720\xB2C8\xCF54\xB4DC \x7D71\x4E00\x78BC"; 
 }
~~~~~

This program should print \em 유니코드 \em 統一碼 if the character type can 
represent the numbers in the sequence. If the number doesn't fit in the range 
that the current character type can represent, the character generated from 
the sequence is undefined.

The \\u and \\U sequences, unlike ths \\x sequence, limits the maximum number of
hexadecimal digits. It is available if the [Character Type](@ref installation)
chosen for building is the wide character type. 

 - \\uXXXX - X is a hexadecimal digit. up to 4 digits
 - \\UXXXXXXXX - X is a hexadecimal digit. up to 8 digits

The program above can be rewritten like this.

~~~~~{.awk}
 BEGIN { 
   print "\uC720\uB2C8\uCF54\uB4DC \U00007D71\U00004E00\U000078BC"; 
 }
~~~~~

If \\x, \\u, \\U are not followed by a hexadecimal digit, *x*, *u*, *U* are
produced respectively.

There are no special sequences supported for a string enclosed in the single
quotes. For that reason, you can't specify the single quote itself within
a single-quoted string. The following program prints *awk* in double quotes.

~~~~~{.awk}
 BEGIN { 
   print '"awk"';
 }
~~~~~

If QSEAWK is built with the wide character support disabled, it doesn support
the \\u and \\U specifiers.


### Regular Expressions ###

A regular expression is enclosed in a pair of forward slashes. The special
sequences for a double-quoted string are all supported in a regular expression.

TBD.

Octal character notation is not supported in a regular expression literal
since it conflicts with the backreference notation.

> ### Note ###
> 
> QSEAWK forms a token with the lognest valid sequences. Tokenization can be
> confusing, especially for the implicit concatention. Let's take **0xT** as
> an example. Since **0x** not followed by a digit is a valid token, and **T**
> is an identifier, it is the same expression as **0x** concatenated with **T**
> (0x \@\@ T).



 Data Types
----------------

QSEAWK supports scalar data types and composite data types. The scalar types
include nil, numeric, string and  the composite type includes a hashed map.

The nil type is an implicit type and it's used to initialize all unassigned
variables and values. It's converted to 0 in the numeric context and an empty
string in the string context. It evaluates to false in the boolean context.

The numeric types include integers and floating-point numbers. The value can
be used interchangeablly in most contexts. A number is converted to a string
in the string context. Automatic conversion occurs whenever necessary depending
on the context. 

A string contains characters. A string is automatically converted to a number
in the numeric context.  Conversion from a string to a number takes all valid 
characters that can form a number token including the integer token and a 
floating-number token. The remainging characters starting from the first 
invalid character are ignored. If no valid characters are found, it's converted
to 0. For example, `"0x20X" / 3` produces 10.6667. The string "0x20X" is 
converted to an integer 32 and the division is performed producing a 
floating-pointer number.

You can create a hashed map by assigning a value to a variable indexed with
a subscript placed within square brackets. You can't convert a hashed map to 
a scalar type implicitly and explcitly.

A regular expression may or may not be viewed as a data type. You can't assign
a compiled regular expression into a variable. A regular expression not placed
on the right-handle side of the **~** operator and the **!~** operator is 
matched against $0 and evalutes to true or false acccording to the matching
result. For an expression **a = /abc/**, a holds true or false, not the regular
expression itself. However, some built-in functions accept compiled regular 
expressions as arguments if only the functions are designed to do so.

In the boolean context, a numeric value of 0 regardless of its numeric type, 
an empty string, and a nil value evaluate to false. All other values evaluate
to true.

> ### Note ###
> See #qse_awk_fnc_spec_t and #qse_awk_fnc_arg_t to define a function that 
> can accept a regular expression.




 User-defined Variables
----------------------------

QSEAWK supports three types of user-defined variables: global, local, named. 

A named variable is a variable created without declaration. It's created
when it's first referenced and it becomes available globally whereas a global 
variable and a local variable is created through declaration.

You can declare a global variable using the \@global keyword at the top-level.

~~~~~{.awk}
 @global var1, var2, ..., varn;
~~~~~

A global variable can be referenced after it's defined. See this example:

~~~~~{.awk}
 BEGIN { a = 10; }
 function print_a () { print a; }
 @global a;
 BEGIN { a = 20; }
 BEGIN { print a; print_a(); }
~~~~~

The variable a is assigned with 10 on the first line. The global variable
declaration appears 2 lines down the code. So the variable a on the first 
and the second line is a named variable. The variable a on the fourth line
appears after the declaration on the third line. It refers to the global
variable a. As a result, it prints 20 and 10 when the fifth line is executed.

You can declare a local variable using the \@local keyword inside a function 
body and an action block. The declaration can be placed at the beginning of
a block statement.

~~~~~{.awk}
 @local var1, var2, ... varn;
~~~~~

A local variable can shade a global variable. See the sample below:
~~~~~{.awk}
 @global g1, g2; #declares two global variables g1 and g2
 BEGIN {
     @local a1, a2, a3; # declares three local variables 
     g1 = 300; a1 = 200;
     {
          @local a1; # a1 here hides the a1 at the outer scope
          @local g1; # g1 here hides the global g1
          a1 = 10; g1 = 5;
          print a1, g1; # it prints 10 and 5
     }
     print a1, g1; # it prints 200 and 300
 }
~~~~~

> ### Note ###
> The QSEAWK engine allows the mixture of named variables and declared variables. 
> However, the mixture may lead to confusion. Use #QSE_AWK_IMPLICIT and 
> #QSE_AWK_EXPLICIT to control what to allow and disallow when configuring 
> the engine.


 Built-in Variables
--------------------------

QSEAWK defines the following built-in variables:

- FS
- NR

TBD.

### FS ###

If the value for FS begins with a question mark followed by 4 
additional letters, QSEAWK can split a record with quoted fields 
delimited by a single-letter separator.

The 4 additional letters are composed of a field separator,
an escaper, a opening quote, and a closing quote.

See this sample code.
~~~~~{.awk}
BEGIN { FS="?:\\[]"; }
{
     for (i = 1; i <= NF; i++)
          print "$" i ": " $i;
     print "---------------";
}
~~~~~

The value of FS in the program above means the following.
- : is a field separator.
- a backslash is an escaper.
- a left bracket is an opening quote.
- a right bracket is a closing quote.

For the input:
~~~~~{.txt}
[fx1]:[fx2]:[f\[x\]3]
abc:def:[a b c]
~~~~~

the sample code produces this output text:
~~~~~{.txt}
$1: fx1
$2: fx2
$3: f[x]3
---------------
$1: abc
$2: def
$3: a b c
---------------
~~~~~


 Hashed Map
---------------

You can store key-values pairs in a hashed map. 

A scalar value is immutable while a hashed map value is mutable.

A variable is tied to a value when it is assigned with a value.
If the variable is tied to a map value, it can't be assigned again.
You can use \@reset to untie the variable from the value, and thus
restore the variable to the 'nil' state.

TBD.

> ### Note ###
> Use #QSE_AWK_FLEXMAP to adjust the switching flexibility of a variable
> between a scalar value and a hashed map.



 User-defined Functions
--------------------------

You can define a function using the following syntax:

~~~~~{.awk}
function name (arg1, arg2, ..., argn) {
	statement
	statement
	...
}
~~~~~

The caller must invoke a function with the same or less number of arguments 
than the definition. When a function is called with the less number of 
arguments, the redundant arguments are initialized to a nil value.

You can't define multiple functions with the same name. The function name
can't confict with named variables and globals variables.

 Built-in Functions
-------------------------


 Function Calls
-----------------------

You can call a function by referencing a function name with arguments
enclosed in parenthesis. The basic syntax looks liket this:

~~~~~{.awk}
 name(arg1, arg2, ..., argn)
~~~~~

TBD.

> ### Note ####
> If there is a space between a name and the left parenthesis,
> the interpretation of the name can be more confusing.
> Let's consider this program.
> ~~~~~{.awk}
>  BEGIN { name (1); }
>  function name(a) { print a; }
> ~~~~~
> When #QSE_AWK_IMPLICIT is on, named variables are allowed. As the function 
> of the name is not defined before the call, the word **name** on the first 
> line is treated a named variable. The function defintion on the second
> line ends up to be a syntax error of redefining a variable as the function
> definition can't shadow a variable name.


### EXTENDED FUNCTIONS ###
index() and match() can accept the third parameter indicating the position 
where the search begins. A negative value indicates a position from the back.

~~~~~{.awk}
 BEGIN {
    xstr = "abcdefabcdefabcdef";
    xsub = "abc";
    xlen = length(xsub);

    i = 1;
    while ((i = index(xstr, xsub, i)) > 0)
    {
       print i, substr(xstr, i, xlen);
       i += xlen;
    }
 }
~~~~~


 Opeartors
---------------

### TEQ operator ###


The === operator compares two values and evaluates to a non-zero value 
if both have the same internal type and the actual values are the same.
so 1 is not equal to 1.0 for the === operator.

A map comparison for the === operator is a bit special. The contents of
the map is never inspected. Comparing two maps always result in inequality.

However, if two variables points to the same map value, it can evaluate
to a non-zero value. This is possible if you allow assigning a map to 
another non-map variable with #QSE_AWK_MAPTOVAR. In this case, a map
is not deep-copied but the reference to it is copied.

~~~~~{.awk}
 BEGIN { 
   a[10]=20; 
   b=a; 
   b[20]=40;
   for (i in a) print i, a[i];  
   print a===b;
 }
~~~~~

The === operator may be also useful when you want to indicate an error
with an uninitialized variable. The following code check if the function
returned a map. Since the variable 'nil' has never been assigned, its 
internal type is 'NIL' and 

~~~~~{.awk}
 function a ()
 {
   x[10] = 2;
   return x;
 }

 BEGIN {
   t = a();
   if (t === nil)
     print "nil";
   else
     print "ok";
 }
~~~~~

The !== operator is a negated form of the === operator.

 Grouped Expression
-----------------------------

When #QSE_AWK_TOLERANT is on, you can use a grouped expression without
the 'in' operator. A grouped expression is a parentheses-enclosed list
of expressions separated with a comma. Each expression in the group is
evaluated in the appearing order. The evaluation result of the last 
expression in the group is returned as that of the group.

~~~~~{.awk}
 BEGIN {
   c = (1, 2, 9);
   a=((1*c, 3*c), (3 - c), ((k = 6+(c+1, c+2)), (-7 * c)));
   print c; # 9;
   print a; # -63	
   print k; # 17
 }
~~~~~






 Commands
--------------------

AWK has the following command statements:
- if
- while
- for
- do .. while
- break
- continue
- return
- exit
- \@abort
- next
- nextfile
- nextofile
- delete
- \@reset
- print
- printf
- expression


### return ###
The return statement is valid in pattern-action blocks as well as in functions.
The execution of a calling block is aborted once the return statement is executed.

~~~~~
 $ qseawk 'BEGIN { return 20; }' ; echo $?
 20
~~~~~

If #QSE_AWK_FLEXMAP is on, you can return an arrayed value from a function.

~~~~~{.awk}
 function getarray() {
   @local a;
   a["one"] = 1;
   a["two"] = 2;
   a["three"] = 3;
   return a;
 }

 BEGIN {
   @local x;
   x = getarray();
   for (i in x) print i, x[i];
 }
~~~~~


### \@reset ###
The \@reset statement resets a variable back to the unassigned state.

~~~~~{.awk}
 BEGIN {
   a[1] = 20;
   @reset a;
   print a+0; # print 0
 }
~~~~~

> ### Note ###
> If the QSEAWK engine is configured with #QSE_AWK_FLEXMAP off, you can use
> the statement over a hashed map variable and assign a scalar value to the
> variable.

### \@abort ###
The \@abort statment is similar to the exit statement except that
it skips executing the END block. You must have #QSE_AWK_ABORT on to be
able to use this statement.

~~~~~{.awk}
 BEGIN {
    print "--- BEGIN ---";
    @abort 10;
 }
 END {
    print "--- END ---"; # this must not be printed
 }
~~~~~


### \@include ###

The \@include directive inserts the contents of the object specified in the
following string, typically a file name, as if they appeared in the source
stream being processed. The directive can only be used at the outmost scope 
where global variable declarations, *BEGIN*, *END*, and/or pattern-action 
blocks appear. 

~~~~~{.awk}
 @include "abc.awk"
 BEGIN { func_in_abc (); }
~~~~~

A semicolon is optional after the included file name. The following is the 
same as the sample above.

~~~~~{.awk}
 @include "abc.awk";
 BEGIN { func_in_abc(); }
~~~~~

> ### Note ###
> If #QSE_AWK_NEWLINE is off, the semicolon is required.



 Built-in I/O
-------------------

QSEAWK comes with built-in I/O commands and functions in addition to the 
implicit input streams for pattern-action blocks. The built-in I/O facility 
is available only if QSEAWK is set with #QSE_AWK_RIO.

### getline ###

The *getline* command has multiple forms of usage. It can be used with or 
without a variable name and can also be associated with a pipe or a file 
redirection. The default association is the console when no pipe and file 
redirection is specified. In principle, it reads a record from the associated
input stream and updates $0 or a variable with the record. If it managed to
perform this successfully, it return 1; it if detected EOF, it returns 0; it
return -1 on failure.

*getline* without a following variable reads a record from an associated
input stream, updates $0 with the value and increments *FNR*, *NR*. Updating
$0 also causes changes in *NF* and fields from $1 to $NF.

The sample below reads records from the console and prints them. 

~~~~~{.awk}
 BEGIN {
   while (getline > 0) print $0;
 }
~~~~~

It is equivalent to 

~~~~~{.awk}
 { print $0 } 
~~~~~

but performs the task in the *BEGIN* block.

*getline* with a variable reads a record from an associated input stream
and updates the variable with the value. It updates *FNR* and *NR*, too.

~~~~~{.awk}
 BEGIN {
   while (getline line > 0) print line;
 }
~~~~~

You can change the stream association to a pipe or a file. If *getline* or
*getline variable* is followed by a input redirection operator(<) and 
an expression, the evaluation result of the expression becomes the name of
the file to read records from. The file is opened at the first occurrence
and can be closed with the *close* function.

~~~~~{.awk}
 BEGIN {
    filename = "/etc/passwd";
    while ((getline line < filename) > 0) print line;
    close (filename);
 }
~~~~~

When *getline* or *getline variable* is preceded with an expression and a pipe
operator(|), the evaluation result of the expression becomes the name of 
the external command to execute. The command is executed at the first occurrence
and can be terminated with the *close* function. The example below reads
the output of the *ls -laF* command and prints it to the console.

~~~~~{.awk}
 BEGIN {
    procname = "ls -laF";
    while ((procname | getline line) > 0) print line;
    close (procname);
 }
~~~~~

The two-way pipe operator(||) can also be used to read records from an 
external command. There is no visible chanages to the end-user in case
of the example above if you switch the operator.

~~~~~{.awk}
 BEGIN {
    procname = "ls -laF";
    while ((procname || getline line) > 0) print line;
    close (procname);
 }
~~~~~

The *getline* command acts like a function in that it returns a value.
But you can't place an empty parentheses when no variable name is specified 
nor can you parenthesize the optional variable name. For example, *getline(a)*
is different from *getline a* and means the concatenation of the return value 
of *getline* and the variable *a*. Besides, it is not clear if 

~~~~~{.awk}
 getline a < b  
~~~~~

is

~~~~~{.awk}
 (getline a) < b 
~~~~~

or 

~~~~~{.awk}
 (getline) (a < b)
~~~~~

For this reason, you are advised to parenthesize *getline* and its related 
components to avoid confusion whenever necessary. The example reading into 
the variable *line* can be made clearer with parenthesization.

~~~~~{.awk}
 BEGIN {
   while ((getline line) > 0) print line;
 }
~~~~~

### print ###

TBD.

### printf ###

When #QSE_AWK_TOLERANT is on, print and printf are treated as if
they are function calls.  In this mode, they return a negative number
on failure and a zero on success and any I/O failure doesn't abort
a running program. 

~~~~~{.awk}
 BEGIN {
   a = print "hello, world" > "/dev/null";
   print a;
   a = print ("hello, world") > "/dev/null";
   print a;
 }
~~~~~

Since print and printf are like function calls, you can use them
in any context where a normal expression is allowed. For example,
printf is used as a conditional expression in an 'if' statement 
in the sample code below.

~~~~~{.awk}
 BEGIN {
   if ((printf "hello, world\n" || "tcp://127.0.0.1:9999") <= -1)
     print "FAILURE";
   else
     print "SUCCESS";
 }
~~~~~

### close (io-name, what) ###

The *close* function closes a stream indicated by the name *io-name*. 
It takes an optional parameter *what* indicating whether input or output 
should be closed. 

If *io-name* is a file, it closes the file handle associated;
If *io-name* is a command, it may kill the running process from the command,
reclaims other sytstem resources, and closes the pipe handles;
If *io-name* is a network stream, it tears down connections to the network
peer and closes the socket handles.

The optional paramenter *what* must be one of *r* or *w* when used is useful
when *io-name* is a command invoked for the two-way pipe operator. The value 
of *r* causes the function to close the read-end of the pipe and the value of
*w* causes the function to close the write-end of the pipe.

The function returns 0 on success and -1 on failure.

Though not so useful, it is possible to create more than 1 streams of different
kinds under the same name. The following program generates a shell script 
/tmp/x containing a command *ls -laF* and executes it without closing the
script file being generated. It reads the execution output via a pipe and
prints it to the console. It is undefined which stream the last *close* 
should close assuming the first *close* is commented out and the program works.

~~~~~{.awk}
 BEGIN {
    print "ls -laF" > "/tmp/x";      # file stream
    system ("chmod ugo+x /tmp/x");   
    #close ("/tmp/x"); 
    while(("/tmp/x" | getline y) > 0) print y;  # pipe stream
    close ("/tmp/x"); # which stream to close?
 }
~~~~~

Note that the execution of generated script fails if the script file is
open on some platforms. That's what the first *close* commented out is 
actually for.

### fflush (io-name) ###

The *fflush* function flushes the output stream indicated by *io-name*. 
If *io-name* is not specified, it flushes the open console output stream.
If *io-name* is an empty stream, it flushes all open output streams.
It returns 0 on success and -1 on failure.

QSEAWK doesn't open the console output stream before it executes any output
commands like *print* or *printf*. so fflush() returns -1 in the following
program.

~~~~~{.awk}
 BEGIN { 
   fflush(); 
 }
~~~~~

The *print* command is executed before fflush() in the following program.
When fflush() is executed, the output stream is open. so fflush() returns 0.

~~~~~{.awk}
 BEGIN { 
   print 1; 
   fflush(); 
 }
~~~~~

Though not so useful, it is possible to create more than 1 output streams
of different kinds under the same name. *fflush* in the following program
flushes both the file stream and the pipe stream.

~~~~~{.awk}
 BEGIN { 
   print 1 | "/tmp/x"; # file stream
   print 1 > "/tmp/x"; # pipe stream
   fflush ("/tmp/x");
 }
~~~~~

### setioattr (io-name, attr-name, attr-value) ###

The *setioattr* function changes the I/O attribute of the name *attr-name* to 
the value *attr-value* for a stream identified by *io-name*. It returns 0 on 
success and -1 on failure.

 - *io-name* is a source or target name used in *getline*, *print*, *printf* 
   combined with |, ||, >, <, >>.
 - *attr-name* is one of *codepage*, *ctimeout*, *atimeout*, *rtimeout*, 
   *wtimeout*.
 - *attr-value* varies depending on *attr-name*.
   + codepage: *cp949*, *cp950*, *utf8*, *slmb*, *mb8*
   + ctimeout, atimeout, rtimeout, wtimeout: the number of seconds. effective 
    on socket based streams only. you may use a floating-point number for 
    lower resolution than a second. a negative value turns off timeout. 

See this sample that prints the contents of a document encoded in cp949.

~~~~~{.awk}
 BEGIN { 
   setioattr ("README.TXT", "codepage", "cp949"); 
   while ((getline x < "README.TXT") > 0) print x; 
  }
~~~~~

### getioattr (io-name, attr-name, attr-value) ###

The getioattr() function retrieves the current attribute value of the attribute
named *attr-name* for the stream identified by *io-name*. The value retrieved 
is set to the variable referenced by *attr-value*. See *setioattr* for 
description on *io-name* and *attr-name*. It returns 0 on success and -1 on 
failure.

~~~~~{.awk}
 BEGIN { 
   setioattr ("README.TXT", "codepage", "cp949"); 
   if (getioattr ("README.TXT", "codepage", codepage) <= -1)
     print "codepage unknown";
   else print "codepage: " codepage;
 }
~~~~~

### Two-way Pipe ###

The two-way pipe is indicated by the two-way pipe operator(||) and QSEAWK
must be set with #QSE_AWK_RWPIPE to be able to use the two-way pipe.

The example redirects the output of *print* to the external *sort* command
and reads back the output. 

~~~~~{.awk}
 BEGIN {
   print "15" || "sort";
   print "14" || "sort";
   print "13" || "sort";
   print "12" || "sort";
   print "11" || "sort";
   # close the input side of the pipe as 'sort' starts emitting result 
   # once the input is closed.
   close ("sort", "r");
   while (("sort" || getline x) > 0) print x; 
 }
~~~~~

This two-way pipe can create a TCP or UDP connection if the pipe command
string is prefixed with one of the followings:

 - tcp:// - establishes a TCP connection to a specified IP address/port.
 - udp:// - establishes a TCP connection to a specified IP address/port.
 - tcpd:// - binds a TCP socket to a specified IP address/port and waits for the first connection.
 - udpd:// - binds a TCP socket to a specified IP address/port and waits for the first sender.

See this example.

~~~~~{.awk}
 BEGIN { 
   # it binds a TCP socket to the IPv6 address :: and the port number 
   # 9999 and waits for the first coming connection. It repeats writing
   # "hello world" to the first connected peer and reading a line from
   # it until the session is torn down.
   do { 
      print "hello world" || "tcpd://[::]:9999"; 
      if (("tcpd://[::]:9999" || getline x) <= 0) break; 
      print x; 
   } 
   while(1);  
 }
~~~~~

You can manipulate TCP or UDP timeouts for connection, accepting, reading, and 
writing with the *setioattr* function and the *getioattr* function.

See the example below.

~~~~~{.awk}
 BEGIN { 
   setioattr ("tcp://127.0.0.1:9999", "ctimeout", 3);
   setioattr ("tcp://127.0.0.1:9999", "rtimeout", 5.5);
   print "hello world" || "tcp://127.0.0.1:9999"; 
   "tcp://127.0.0.1:9999" || getline x; 
   print x;
 }
~~~~~

Here is an interesting example adopting Michael Sanders' AWK web server, 
modified for QSEAWK.

~~~~~{.awk}
 # 
 # Michael Sanders' AWK web server for QSEAWK.
 # Orginal code in http://awk.info/?tools/server
 #
 # qseawk --tolerant=on --rwpipe=on webserver.awk
 #
 BEGIN {
   x        = 1                         # script exits if x < 1 
   port     = 8080                      # port number 
   host     = "tcpd://0.0.0.0:" port    # host string 
   url      = "http://localhost:" port  # server url 
   status   = 200                       # 200 == OK 
   reason   = "OK"                      # server response 
   RS = ORS = "\r\n"                    # header line terminators 
   doc      = Setup()                   # html document 
   len      = length(doc) + length(ORS) # length of document 
   while (x) {
      if ($1 == "GET") RunApp(substr($2, 2))
      if (! x) break
      print "HTTP/1.0", status, reason || host
      print "Connection: Close"        || host
      print "Pragma: no-cache"         || host
      print "Content-length:", len     || host
      print ORS doc                    || host
      close(host)     # close client connection 
      host || getline # wait for new client request 
   }
   # server terminated... 
   doc = Bye()
   len = length(doc) + length(ORS)
   print "HTTP/1.0", status, reason || host
   print "Connection: Close"        || host
   print "Pragma: no-cache"         || host
   print "Content-length:", len     || host
   print ORS doc                    || host
   close(host)
 }
 
 function Setup() {
   tmp = "<html>\
   <head><title>Simple gawk server</title></head>\
   <body>\
   <p><a href=" url "/xterm>xterm</a>\
   <p><a href=" url "/xcalc>xcalc</a>\
   <p><a href=" url "/xload>xload</a>\
   <p><a href=" url "/exit>terminate script</a>\
   </body>\
   </html>"
   return tmp
 }
 
 function Bye() {
   tmp = "<html>\
   <head><title>Simple gawk server</title></head>\
   <body><p>Script Terminated...</body>\
   </html>"
   return tmp
 }
 
 function RunApp(app) {
   if (app == "xterm")  {system("xterm&"); return}
   if (app == "xcalc" ) {system("xcalc&"); return}
   if (app == "xload" ) {system("xload&"); return}
   if (app == "exit")   {x = 0}
 }
~~~~~

### I/O Character Encoding ###

You can change the character encoding encoding of a stream. See qse_findcmgr()
for a list of supported encoding names.

Let's say you run this simple echoing script on a WIN32 platform that has
the active code page of 949 and is reachable at the IP address 192.168.2.8.

~~~~~{.awk}
    C:\> chcp
    Active code page: 949
    C:\> type s.awk
    BEGIN {
        sock = "tcpd://0.0.0.0:9999";
        setioattr (sock, "codepage", "cp949"); 
        do {
            if ((sock || getline x) <= 0) break;
            print "PEER: " x;
            print x || sock;
        }
        while(1);
    }
    C:\> qseawk -f r.awk
    PEER: 안녕
    PEER: ?好!
~~~~~

Now you run the following script on a UTF-8 console of a Linux box.

~~~~~{.awk}
    $ echo $LANG
    en_US.UTF-8
    $ cat  c.awk
    BEGIN {
        peer = "tcp://192.168.2.8:9999";
        setioattr (peer, "codepage", "cp949");
        do
        {
            printf "> ";
            if ((getline x) <= 0) break;
            print x || peer;
            if ((peer || getline line) <= -1) break;
            print "PEER: " line;
        }
        while (1);
    }
    $ qseawk --rwpipe=on -f c.awk
    > 안녕
    PEER: 안녕
    > 你好!
    PEER: ?好!
~~~~~

Note that 你 has been converted to a question mark since the letter is
not supported by cp949.





 Modules
----------------

QSEAWK supports various external modules.

### String ###

The *str* module provides an extensive set of string manipulation functions.

- str::index
- str::isalnum
- str::isalpha
- str::isblank
- str::iscntrl
- str::isdigit
- str::isgraph
- str::islower
- str::isprint
- str::ispunct
- str::isspace
- str::isupper
- str::isxdigit
- str::ltrim
- str::normspace
- str::rindex
- str::rtrim
- str::trim

### Directory ###

The *dir* module provides an interface to read file names in a specified directory.

- dir::open
- dir::close
- dir::read
- dir::reset
- dir::errno
- dir::errstr

~~~~~{.awk}
 BEGIN { 
     x = dir::open (".");  
     while ((dir::read(x, file)) > 0) print file;
     dir::close(x); 
 }
~~~~~

### SED ###

The *sed* module provides built-in sed capabilities.

- sed::file_to_file (script-string, input-file, output-file [, option-string])
- sed::str_to_str (script-string, input-string, output-variable, [, option-string])
- sed::errstr (error-number)

~~~~~{.awk}
 BEGIN { 
    x = sed::file_to_file ("s/[a-z]/#/g", "in.txt", "out.txt");
    if (x <= -1) printf ("ERROR: %s\n"), sed::errstr(x));
 }
~~~~~


[awkbook]: http://cm.bell-labs.com/cm/cs/awkbook/ 

