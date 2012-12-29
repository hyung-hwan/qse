QSEAWK Language {#awk-lang}
===============

QSEAWK implements the language described in the 
[The AWK Programming Language][awkbook] with extensions.

QSEAWK reads an AWK program, recognizes various tokens contained while skipping 
comments and whitespaces that don't constinute a token, analyses syntax, and
tranforms them to an internal form for execution.

### Comments ###

A single-line comment is introduced by a hash character #, and is terminated at 
the end of the same line. Additionally, it supports a C-style multi-line comment
enclosed in /* and */. The multi-line comment can't nest and can't appear within
string literals and regular expressions.

    x = y; # assign y to x.
    /*
    this line is ignored.
    this line is ignored too.
    */

## Tokens ##

A token is composed of one or more consecutive characters.

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

An integer can be prefixed with 0x, 0, 0b for a hexa-decimal number, an octal number,
and a binary number respectively. For a hexa-decimal number, letters from A to F
can form a number case-insenstively in addition to numeric digits.

    0xA1   # 161
    0xB0b0 # 45232
    020    # 16
    0b101  # 5

If the prefix is not followed by any numeric digits, it is still a valid token and
represents the value of 0.

    0x # 0x0 but not desirable.
    0b # 0b0 but not desirable.

### Strings ###

A string is enclosed in a pair of double quotes or single quotes.

A character in a string encosed in the double-quotes can be preceeded with 
a back-slash to change the meaning of the character.

\\
\a
\b
\uXXXX
\UXXXXXXXX

There are no escaping sequences supported for a string enclosed in the single
quotes. For that reason, you can't specify the single quote itself within
a single-quoted string.

### Regular Expressions ###

A regular expression is enclosed in a pair of forward slashes.


### Note ###

QSEAWK forms a token with the lognest valid sequences.

Tokenization cab confusing, especially for the implicit concatention.
Let's take this as an example.

    0xT

Since 0x not followed by a digit is a valid token, and T is an identifier,
it is the same expression as 0x concatenated with T (0x @@ T).

An AWK program can be composed of the following elements shown below. 
Each language element requires the option in the second column to be on. 

<table>
<tr><th>Element                    </th><th>Option             </th></tr>
<tr><td>Comment                    </td><td>                   </td></tr>
<tr><td>Global variable declaration</td><td>#QSE_AWK_EXPLICIT  </td></tr>
<tr><td>Pattern-action block       </td><td>#QSE_AWK_PABLOCK   </td></tr>
<tr><td>User-defined function      </td><td>                   </td></tr>
<tr><td>\@include                  </td><td>#QSE_AWK_INCLUDE   </td></tr>
</table>

Single line comments begin with the '#' letter and end at the end of the
same line. The C style multi-line comments are supported as well.
Comments are ignored.

- pattern-action-block := pattern action-block
- pattern := BEGIN | END | expression | expression-range
- expression-range := expression , expression

A pattern in a pattern action block can be omitted.
The action part can be omitted if the pattern is not BEGIN nor END.

A pattern-action block, and a user-defined function can have the following elements.

<table>
<tr><th>Element                    </th><th>Option            </th></tr>
<tr><td>Local variable declaration</td><td>#QSE_AWK_EXPLICIT  </td></tr>
<tr><td>Statement                 </td><td>                   </td></tr>
<tr><td>getline                   </td><td>#QSE_AWK_RIO       </td></tr>
<tr><td>print                     </td><td>#QSE_AWK_RIO       </td></tr>
<tr><td>nextofile                 </td><td>#QSE_AWK_NEXTOFILE </td></tr>
<tr><td>reset                     </td><td>#QSE_AWK_RESET     </td></tr>
<tr><td>abort                     </td><td>#QSE_AWK_ABORT     </td></tr>
</table>


AWK has the following statement constructs.
- if
- while
- for
- do .. while
- break
- continue
- return
- exit
- abort
- next
- nextfile
- nextofile
- delete
- reset
- print
- printf
- expression

@subsection awk_litvar LITERAL AND VARIABLE

Value type
- Scalar
-- String
-- Integer
-- Floating-Pointer number
- Hashed Map
- Regular expression

Scalar values are immutable while a hashed map value is mutable.
A regular expression value is specially treated.

A variable is tied to a value when it is assigned with a value.
If the variable is tied to a map value, it can't be assigned again.
You can use 'reset' to untie the variable from the value, and thus
restore the variable to the 'nil' state.

....

@subsection awk_ext_teq TEQ OPERATOR

The === operator compares two values and evaluates to a non-zero value 
if both have the same internal type and the actual values are the same.
so 1 is not equal to 1.0 for the === operator.

A map comparison for the === operator is a bit special. The contents of
the map is never inspected. Comparing two maps always result in inequality.

However, if two variables points to the same map value, it can evaluate
to a non-zero value. This is possible if you allow assigning a map to 
another non-map variable with #QSE_AWK_MAPTOVAR. In this case, a map
is not deep-copied but the reference to it is copied.

@code
BEGIN { 
	a[10]=20; 
	b=a; 
	b[20]=40;
	for (i in a) print i, a[i];  
	print a===b;
}
@endcode


The === operator may be also useful when you want to indicate an error
with an uninitialized variable. The following code check if the function
returned a map. Since the variable 'nil' has never been assigned, its 
internal type is 'NIL' and 

@code
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
@endcode.

The !== operator is a negated form of the === operator.


@subsection awk_ext_vardecl VARIABLE DECLARATION

#QSE_AWK_EXPLICIT enables variable declaration. Variables declared are accessed
directly bypassing the global named map that stores undeclared variables.
The keyword @b global introduces a global variable and the keyword @b local 
introduces local variable. Local variable declaraion in a block must be 
located before an expression or a statement appears.

@code
global g1, g2; #declares two global variables g1 and g2

BEGIN {
	local a1, a2, a3; # declares three local variables 

	g1 = 300; a1 = 200;

	{
		local a1; # a1 here hides the a1 at the outer scope
		local g1; # g1 here hides the global g1
		a1 = 10; g1 = 5;
		print a1, g1; # it prints 10 and 5
	}

	print a1, g1; # it prints 200 and 300
}

@endcode

However, turning on #QSE_AWK_EXPLICIT does not disable named variables.
To disable named variables, you must turn off #QSE_AWK_IMPLICIT.

@subsection awk_ext_include INCLUDE

The \@include directive inserts the contents of the object specified in the
following string, typically a file name, as if they appeared in the source
stream being processed. The directive can only be used at the outmost scope 
where global variable declarations, @b BEGIN, @b END, and/or pattern-action 
blocks appear. To use \@include, you must turn on #QSE_AWK_INCLUDE.

@code
@include "abc.awk"
BEGIN { func_in_abc (); }
@endcode

A semicolon is optional after the included file name. The following is the 
same as the sample above.
@code
@include "abc.awk";
BEGIN { func_in_abc(); }
@endcode

If #QSE_AWK_NEWLINE is off, the semicolon is required.


@subsection awk_ext_funcall FUNCTIONC CALL


name(1);
if there is no space between 'name' and the left parenthesis, the 
name is treated as a function name.

name (1);
If there is a space, the name is treated as a function name if the 
name has been declared as the function or if #QSE_AWK_IMPLICIT is on,
it may be 'name' concatenated with the expression in the parentheses.

The following is a valid program.
@code
@pragma implicit off
BEGIN { name (1); }
function name(a) { print a; }'
@endcode

However, in this program, the first 'name' becomes a named global variable.
so the function declaration with 'name' triggers the variable redefinition 
error.
@pragma implicit on
BEGIN { name (1); }
function name(a) { print a; }'
@endcode

@subsection awk_ext_print EXTENDED PRINT/PRINTF
When #QSE_AWK_TOLERANT is on, print and printf are treated as if
they are function calls.  In this mode, they return a negative number
on failure and a zero on success and any I/O failure doesn't abort
a running program. 

@code
BEGIN {
	a = print "hello, world" > "/dev/null";
	print a;	
	a = print ("hello, world") > "/dev/null";
	print a;	
}
@endcode

Since print and printf are like function calls, you can use them
in any context where a normal expression is allowed. For example,
printf is used as a conditional expression in an 'if' statement 
in the sample code below.
@code
BEGIN {
	if ((printf "hello, world\n" || "tcp://127.0.0.1:9999") <= -1)
		print "FAILURE";
	else
		print "SUCCESS";
}
@endcode

@subsection awk_ext_exprgroup GROUPED EXPRESSION
When #QSE_AWK_TOLERANT is on, you can use a grouped expression without
the 'in' operator. A grouped expression is a parentheses-enclosed list
of expressions separated with a comma. Each expression in the group is
evaluated in the appearing order. The evaluation result of the last 
expression in the group is returned as that of the group.

@code
BEGIN {
	c = (1, 2, 9);
	a=((1*c, 3*c), (3 - c), ((k = 6+(c+1, c+2)), (-7 * c)));
	print c; # 9;
	print a; # -63	
	print k; # 17
}
@endcode

@subsection awk_ext_rwpipe TWO-WAY PIPE

The two-way pipe indicated by @b || is supproted, in addition to the one-way 
pipe indicated by @b |. Turn on #QSE_AWK_RWPIPE to enable the two-way pipe.

@code
BEGIN {
	print "15" || "sort";
	print "14" || "sort";
	print "13" || "sort";
	print "12" || "sort";
	print "11" || "sort";
	# close the input side of the pipe as 'sort' starts emitting result 
	# once the input is closed.
	close ("sort", "r");
	while (("sort" || getline x) > 0) print "xx:", x; 
}
@endcode

This two-way pipe can create a TCP or UDP connection if the pipe command
string is prefixed with one of the followings:

- tcp:// - establishes a TCP connection to a specified IP address/port.
- udp:// - establishes a TCP connection to a specified IP address/port.
- tcpd:// - binds a TCP socket to a specified IP address/port and waits for the first connection.
- udpd:// - binds a TCP socket to a specified IP address/port and waits for the first sender.

@code
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
@endcode

You can specify TCP or UDP timeouts for connection, accepting, reading, and 
writing with setioattr (pipe-name, timeout-name, timeout-value). timeout-name 
should be one of "ctimeout", "atimeout", "rtimeout", and "wtimeout". 
timeout-value is a number specifying the actual timeout in milliseconds. 
A negative value indicates no timeout. 

You can call getioattr (pipe-name, timeout-name) to get the current 
timeout-value set.

See the example below.

@code
BEGIN { 
	setioattr ("tcp://127.0.0.1:9999", "ctimeout", 3000);
	setioattr ("tcp://127.0.0.1:9999", "rtimeout", 5000);
	print "hello world" || "tcp://127.0.0.1:9999"; 
	"tcp://127.0.0.1:9999" || getline x; 
	print x;
}
@endcode

Here is a more interesting example adopting Michael Sanders'
AWK web server, modified for QSEAWK.

@code
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
@endcode

@subsection awk_ext_return RETURN
The return statement is valid in pattern-action blocks as well as in functions.
The execution of a calling block is aborted once the return statement is executed.

@code
$ qseawk 'BEGIN { return 20; }' ; echo $?
20
@endcode

If #QSE_AWK_MAPTOVAR is on, you can return an arrayed value from a function.
@code
function getarray() {
	local a;
	a["one"] = 1;
	a["two"] = 2;
	a["three"] = 3;
	return a;
}

BEGIN {
	local x;

	x = getarray();
	for (i in x) print i, x[i];
}
@endcode

@subsection awk_ext_reset RESET
The reset statement resets an array variable back to the initial state.
After that, the array variable can also be used as a scalar variable again.
You must have #QSE_AWK_RESET on to be able to be able to use this 
statement.

@code
BEGIN {
	a[1] = 20;
	reset a;
	a = 20; # this is legal
	print a;
}
@endcode

@subsection awk_ext_abort ABORT
The abort statment is similar to the exit statement except that
it skips executing the END block. You must have #QSE_AWK_ABORT on to be
able to use this statement.

@code
BEGIN {
	print "--- BEGIN ---";
	abort 10;
}
END {
	print "--- END ---"; # this must not be printed
}
@endcode

@subsection awk_ext_comment COMMENT
You can use the C-style comment as well as the pound comment.

@subsection awk_ext_fnc EXTENDED FUNCTIONS
index() and match() can accept the third parameter indicating the position 
where the search begins. A negative value indicates a position from the back.

@code
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
@endcode

@subsection awk_ext_fs EXTENDED FS

If the value for FS begins with a question mark followed by 4 
additional letters, QSEAWK can split a record with quoted fields 
delimited by a single-letter separator.

The 4 additional letters are composed of a field separator,
an escaper, a opening quote, and a closing quote.

@code
$ cat x.awk
BEGIN { FS="?:\\[]"; }
{
     for (i = 1; i <= NF; i++)
          print "$" i ": " $i;
     print "---------------";
}
@endcode

The value of FS above means the following.
- : is a field separator.
- a backslash is an escaper.
- a left bracket is an opening quote.
- a right bracket is a closing quote.

See the following output.
@code
$ cat x.dat
[fx1]:[fx2]:[f\[x\]3]
abc:def:[a b c]
$ qseawk -f x.awk x.dat
$1: fx1
$2: fx2
$3: f[x]3
---------------
$1: abc
$2: def
$3: a b c
---------------
@endcode
	


@subsection awk_ext_binnum BINARY NUMBER
Use 0b to begin a binary number sequence.

@code 
$ qseawk 'BEGIN { printf ("%b %o %d %x\n", 0b1101, 0b1101, 0b1101, 0b1101); }'
1101 15 13 d
@endcode



@subsection awk_ext_unicode UNICODE ESCAPE SEQUENCE 

If QSE is compiled for #QSE_CHAR_IS_WCHAR, you can use \\u and \\U in a 
string to specify a character by unicode.

@code
$ qseawk 'BEGIN { print "\uC720\uB2C8\uCF54\uB4DC \U00007D71\U00004E00\U000078BC"; }'
유니코드 統一碼
@endcode


@subsection awk_ext_ioenc I/O ENCODING
You can call setioattr() to set the character encoding of a stream resource 
like a pipe or a file. See qse_findcmgr() for a list of supported encoding names.

Let's say you run this simple echoing script on a WIN32 platform that has
the active code page of 949 and is reachable at the IP address 192.168.2.8.

@code
C:\> chcp
Active code page: 949
C:\> type s.awk
BEGIN {
    sock = "tcpd://0.0.0.0:9999";
    setioattr (sock, "codepage", "cp949"); # this is not needed since the active
                                           # code page is already 949.
    do {
         if ((sock || getline x) <= 0) break;
         print "PEER: " x;
         print x || sock;
    }
    while(1);
}
C:\> qseawk --rwpipe=on -f r.awk
PEER: 안녕
PEER: ?好!
@endcode

Now you run the following script on a UTF-8 console of a Linux box.

@code
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
@endcode

Note that 你 has been converted to a question mark since the letter is
not supported by cp949.

## Built-in I/O ##

QSEAWK comes with built-in I/O commands and functions in addition to the 
implicit input streams for pattern-action blocks. The built-in I/O facility 
is available only if QSEAWK is set with #QSE_AWK_RIO.

### getline ###
	
The *getline* command has multiple forms of usage. It can be used with or 
without a variable name and can also be associated with a pipe or a file 
redirection. Basically, it reads a record from an input stream associated 
and stores it.

*getline* without a following variable reads a record from an associated
input stream and updates $0 with the value. It also updates *NF*, *FNR*, *NR*.
The sample below reads records from the console and prints them. 

    BEGIN {
        while (getline > 0) print $0;
    }

It is equivalent to 

    { print $0 } 

but performs the task in the *BEGIN* block.

*getline* with a variable reads a record from an associated input stream
and updates the variable with the value. It updates *FNR* and *NR*, too.

    BEGIN {
        while (getline line > 0) print line;
    }

*getline* is associated with the console by default. you can change it
to a file or a pipe by using |, ||, <.

The *getline* command acts like a function in that it returns a value: 1 on 
success, 0 on EOF, -1 on error. But you can't place an empty parentheses
when no variable name is specified nor can you parenthesize the optional 
variable name. For example, *getline(a)* is different from *getline a* and 
means the concatenation of the return value of *getline* and the variable *a*.

### print ###

### printf ###

### setioattr (io-name, attr-name, attr-value) ###

The *setioattr* function changes the I/O attribute of the name *attr-name* to 
the value *attr-value* for a stream identified by *io-name*. It returns 0 on 
success and -1 on failure.

 - *io-name* is a source or target name used in *getline*, *print*, *printf* 
   combined with |, ||, >, <, >>.
 - *attr-name* is one of *codepage*, *ctimeout*, *atimeout*, *rtimeout*, 
   *wtimeout*.
 - *attr-value* varies depending on *attr-name*.
   + codepage: *cp949*, *cp950*, *utf8*
   + ctimeout, atimeout, rtimeout, wtimeout: the number of seconds. effective 
    on socket based streams only. you may use a floating-point number for 
    lower resoluation than a second. a negative value turns off timeout. 

See this sample that prints the contents of a document encoded in cp949.

    BEGIN { 
        setioattr ("README.TXT", "codepage", "cp949"); 
        while ((getline x < "README.TXT") > 0) print x; 
    }

### getioattr (io-name, attr-name, attr-value) ###

The getioattr() function retrieves the current attribute value of the attribute
named *attr-name* for the stream identified by *io-name*. The value retrieved 
is set to the variable referenced by *attr-value*. See *setioattr* for 
description on *io-name* and *attr-name*. It returns 0 on success and -1 on 
failure.

    BEGIN { 
        setioattr ("README.TXT", "codepage", "cp949"); 
        if (getioattr ("README.TXT", "codepage", codepage) <= -1)
            print "codepage unknown";
        else print "codepage: " codepage;
    }

[awkbook]: http://cm.bell-labs.com/cm/cs/awkbook/ 
