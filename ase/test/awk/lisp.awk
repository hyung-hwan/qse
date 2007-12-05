#
# $Id$
#
# This program is a modified version of awklisp originally written 
# by Darius Bacon. The only modification is to append a semicolon
# onto the end of each statement to cater for the semicolon requirement
# of ASEAWK.
#

# <<README>>
#
# See the Manual file for documentation.
# 
# This release also has a Perl version, perlisp, contributed by the Perl
# Avenger, who writes:
# 
#   It has new primitives: a reentrant "load", a "trace" command, and more
#   error reporting.  Perlisp will attempt to load a program called
#   "testme" before anything else, when it runs.  After that, it will load
#   $HOME/.perlisprc if that file exists, before reverting to the
#   interactive read/eval/print loop.
# 
# The awk code is still essentially the code posted to alt.sources (May
# 31, 1994), but with a garbage collector added.
# 
# 
# Copyright (c) 1994, 2001 by Darius Bacon.
# 
# Permission is granted to anyone to use this software for any
# purpose on any computer system, and to redistribute it freely,
# subject to the following restrictions:
# 
# 1. The author is not responsible for the consequences of use of
#    this software, no matter how awful, even if they arise from
#    defects in it.
# 
# 2. The origin of this software must not be misrepresented, either
#    by explicit claim or by omission.
# 
# 3. Altered versions must be plainly marked as such, and must not
#    be misrepresented as being the original software.

# <<Manual>>
#
# 		  awklisp: a Lisp interpreter in awk
# 			     version 1.2
# 
# 			     Darius Bacon
# 			 darius@accesscom.com
# 		  http://www.accesscom.com/~darius/
# 
# 
# 1. Usage
# 
#     mawk [-v profiling=1] -f awklisp <optional-Lisp-source-files>
# 
# The  -v profiling=1  option turns call-count profiling on.
# 
# If you want to use it interactively, be sure to include '-' (for the standard 
# input) among the source files.  For example:
# 
#     mawk -f awklisp startup -
# 
# It should work with nawk and gawk, too, but even less quickly.
#     
# 
# 2. Overview
# 
# This program arose out of one-upmanship.  At my previous job I had to
# use MapBasic, an interpreter so astoundingly slow (around 100 times
# slower than GWBASIC) that one must wonder if it itself is implemented
# in an interpreted language.  I still wonder, but it clearly could be:
# a bare-bones Lisp in awk, hacked up in a few hours, ran substantially
# faster.  Since then I've added features and polish, in the hope of
# taking over the burgeoning market for stately language
# implementations.
# 
# This version tries to deal with as many of the essential issues in
# interpreter implementation as is reasonable in awk (though most would
# call this program utterly unreasonable from start to finish, perhaps...).
# Awk's impoverished control structures put error recovery and tail-call
# optimization out of reach, in that I can't see a non-painful way to code
# them.  The scope of variables is dynamic because that was easier to 
# implement efficiently.  Subject to all those constraints, the language
# is as Schemely as I could make it: it has a single namespace with 
# uniform evaluation of expressions in the function and argument positions,
# and the Scheme names for primitives and special forms.
# 
# The rest of this file is a reference manual.  My favorite tutorial would be
# _The Little LISPer_ (see section 5, References); don't let the cute name
# and the cartoons turn you off, because it's a really excellent book with 
# some mind-stretching material towards the end.  All of its code will work
# with awklisp, except for the last two chapters.  (You'd be better off
# learning with a serious Lisp implementation, of course.)
# 
# The file Impl-notes in this distribution gives an overview of the 
# implementation.
# 
# 
# 3. Expressions and their evaluation
# 
# Lisp evaluates expressions, which can be simple (atoms) or compound (lists).
# 
# An atom is a string of characters, which can be letters, digits, and most
# punctuation; the characters may -not- include spaces, quotes, parentheses,
# brackets, '.', '#', or ';' (the comment character).  In this Lisp, case is
# significant ( X  is different from  x ).
# 
# Atoms:	atom 42 1/137 + ok? hey:names-with-dashes-are-easy-to-read
# Not atoms: 	don't-include-quotes 	(or spaces or parentheses)
# 
# A list is a '(', followed by zero or more objects (each of which is an atom
# or a list), followed by a ')'.
# 
# Lists:	()   (a list of atoms)	((a list) of atoms (and lists))
# Not lists:	)    ((())		(two) (lists)
# 
# The special object  nil  is both an atom and the empty list.  That is,
# nil = ().  A non-nil list is called a -pair-, because it is represented by a
# pair of pointers, one to the first element of the list (its -car-), and one to
# the rest of the list (its -cdr-).  For example, the car of ((a list) of stuff)
# is (a list), and the cdr is (of stuff).  It's also possible to have a pair
# whose cdr is not a list; the pair with car A and cdr B is printed as (A . B).
# 
# That's the syntax of programs and data.  Now let's consider their meaning.  You
# can use Lisp like a calculator: type in an expression, and Lisp prints its 
# value.  If you type 25, it prints 25.  If you type (+ 2 2), it prints 4.  In 
# general, Lisp evaluates a particular expression in a particular environment
# (set of variable bindings) by following this algorithm:
# 
# If the expression is a number, return that number.
# 
# If the expression is a non-numeric atom (a -symbol-), return the value of that
# symbol in the current environment.  If the symbol is currently unbound, that's
# an error.
# 
# Otherwise the expression is a list.  If its car is one of the symbols: quote, 
# lambda, if, begin, while, set!, or define, then the expression is a -special-
# -form-, handled by special rules.  Otherwise it's just a procedure call,
# handled like this: evaluate each element of the list in the current environment,
# and then apply the operator (the value of the car) to the operands (the values
# of the rest of the list's elements).  For example, to evaluate (+ 2 3), we
# first evaluate each of its subexpressions: the value of + is (at least in the
# initial environment) the primitive procedure that adds, the value of 2 is 2,
# and the value of 3 is 3.  Then we call the addition procedure with 2 and 3 as
# arguments, yielding 5.  For another example, take (- (+ 2 3) 1).  Evaluating
# each subexpression gives the subtraction procedure, 5, and 1.  Applying the
# procedure to the arguments gives 4.
# 
# We'll see all the primitive procedures in the next section.  A user-defined 
# procedure is represented as a list of the form (lambda <parameters> <body>),
# such as (lambda (x) (+ x 1)).  To apply such a procedure, evaluate its body
# in the environment obtained by extending the current environment so that the
# parameters are bound to the corresponding arguments.  Thus, to apply the above
# procedure to the argument 41, evaluate (+ x 1) in the same environment as the
# current one except that x is bound to 41.
# 
# If the procedure's body has more than one expression -- e.g., 
# (lambda () (write 'Hello) (write 'world!)) -- evaluate them each in turn, and
# return the value of the last one.
# 
# We still need the rules for special forms.  They are:
# 
# The value of (quote <x>) is <x>.  There's a shorthand for this form: '<x>.
# E.g., the value of '(+ 2 2) is (+ 2 2), -not- 4.
# 
# (lambda <parameters> <body>) returns itself: e.g., the value of (lambda (x) x)
# is (lambda (x) x).
# 
# To evaluate (if <test-expr> <then-exp> <else-exp>), first evaluate <test-expr>.
# If the value is true (non-nil), then return the value of <then-exp>, otherwise
# return the value of <else-exp>.  (<else-exp> is optional; if it's left out, 
# pretend there's a  nil  there.)  Example: (if nil 'yes 'no) returns no.
# 
# To evaluate (begin <expr-1> <expr-2>...), evaluate each of the subexpressions
# in order, returning the value of the last one.
# 
# To evaluate (while <test> <expr-1> <expr-2>...), first evaluate <test>.  If 
# it's nil, return nil.  Otherwise, evaluate <expr-1>, <expr-2>,... in order,
# and then repeat.
# 
# To evaluate (set! <variable> <expr>), evaluate <expr>, and then set the value
# of <variable> in the current environment to the result.  If the variable is
# currently unbound, that's an error.  The value of the whole set! expression
# is the value of <expr>.
# 
# (define <variable> <expr>) is like set!, except it's used to introduce new
# bindings, and the value returned is <variable>.
# 
# It's possible to define new special forms using the macro facility provided in
# the startup file.  The macros defined there are: 
# 
# (let ((<var> <expr>)...) 
#   <body>...)
#   
#   Bind each <var> to its corresponding <expr> (evaluated in the current
#   environment), and evaluate <body> in the resulting environment.
# 
# (cond (<test-expr> <result-expr>...)... (else <result-expr>...))
#   
#   where the final  else  clause is optional.  Evaluate each <test-expr> in
#   turn, and for the first non-nil result, evaluate its <result-expr>.  If
#   none are non-nil, and there's no  else  clause, return nil.
# 
# (and <expr>...)
# 
#   Evaluate each <expr> in order, until one returns nil; then return nil.
#   If none are nil, return the value of the last <expr>.
# 
# (or <expr>...)
# 
#   Evaluate each <expr> in order, until one returns non-nil; return that value.
#   If all are nil, return nil.
# 
# 
# 4. Built-in procedures
# 
# List operations:
# (null? <x>) returns true (non-nil) when <x> is nil.
# (atom? <x>) returns true when <x> is an atom.
# (pair? <x>) returns true when <x> is a pair.
# (car <pair>) returns the car of <pair>.
# (cdr <pair>) returns the cdr of <pair>.
# (cadr <pair>) returns the car of the cdr of <pair>. (i.e., the second element.)
# (cddr <pair>) returns the cdr of the cdr of <pair>.
# (cons <x> <y>) returns a new pair whose car is <x> and whose cdr is <y>.
# (list <x>...) returns a list of its arguments.
# (set-car! <pair> <x>) changes the car of <pair> to <x>.
# (set-cdr! <pair> <x>) changes the cdr of <pair> to <x>.
# (reverse! <list>) reverses <list> in place, returning the result.
# 
# Numbers:
# (number? <x>) returns true when <x> is a number.
# (+ <n> <n>) returns the sum of its arguments.
# (- <n> <n>) returns the difference of its arguments.
# (* <n> <n>) returns the product of its arguments.
# (quotient <n> <n>) returns the quotient.  Rounding is towards zero.
# (remainder <n> <n>) returns the remainder.
# (< <n1> <n2>) returns true when <n1> is less than <n2>.
# 
# I/O:
# (write <x>) writes <x> followed by a space.
# (newline) writes the newline character.
# (read) reads the next expression from standard input and returns it.
# 
# Meta-operations:
# (eval <x>) evaluates <x> in the current environment, returning the result.
# (apply <proc> <list>) calls <proc> with arguments <list>, returning the result.
# 
# Miscellany:
# (eq? <x> <y>) returns true when <x> and <y> are the same object.  Be careful
#     using eq? with lists, because (eq? (cons <x> <y>) (cons <x> <y>)) is false. 
# (put <x> <y> <z>)
# (get <x> <y>) returns the last value <z> that was put for <x> and <y>, or nil
#     if there is no such value.
# (symbol? <x>) returns true when <x> is a symbol.
# (gensym) returns a new symbol distinct from all symbols that can be read.
# (random <n>) returns a random integer between 0 and <n>-1 (if <n> is positive).
# (error <x>...) writes its arguments and aborts with error code 1.
# 
# 
# 5. References
# 
# Harold Abelson and Gerald J. Sussman, with Julie Sussman.
#   Structure and Interpretation of Computer Programs.  MIT Press, 1985.
# 
# John Allen.  Anatomy of Lisp.  McGraw-Hill, 1978.
# 
# Daniel P. Friedman and Matthias Felleisen.  The Little LISPer.  Macmillan, 1989.
# 
# Roger Rohrbach wrote a Lisp interpreter, in old awk (which has no
# procedures!), called walk .  It can't do as much as this Lisp, but it
# certainly has greater hack value.  Cooler name, too.  It's available at
# http://www-2.cs.cmu.edu/afs/cs/project/ai-repository/ai/lang/lisp/impl/awk/0.html
# 
# 
# 6. Bugs
# 
# Eval doesn't check the syntax of expressions.  This is a probably-misguided
# attempt to bump up the speed a bit, that also simplifies some of the code.
# The macroexpander in the startup file would be the best place to add syntax-
# checking.



# --- Representation of Lisp data

BEGIN {
    a_number = 0;
    pair_ptr = a_pair = 1;
    symbol_ptr = a_symbol = 2;
    
    type_name[a_number] = "number";
    type_name[a_pair] = "pair";
    type_name[a_symbol] = "symbol";
}

function is(type, expr)	
{ 
    if (expr % 4 != type) 
        error("Expected a " type_name[type] ", not a " type_name[expr % 4]) ;
    return expr;
}

function is_number(expr)	{ return expr % 4 == 0; }
function is_pair(expr)		{ return expr % 4 == 1; }
function is_symbol(expr)	{ return expr % 4 == 2; }
function is_atom(expr)		{ return expr % 4 != 1; }

function make_number(n)		{ return n * 4; }

function numeric_value(expr)
{ 
    if (expr % 4 != 0) error("Not a number");
    return expr / 4;
}

# Return the symbol :string names.
function string_to_symbol(string)
{
    if (string in intern)
        return intern[string];
    symbol_ptr += 4;
    intern[string] = symbol_ptr;
    printname[symbol_ptr] = string;
    return symbol_ptr;
}

# Define a primitive procedure, with :nparams parameters,
# bound to the symbol named :name.
function def_prim(name, nparams,	sym)
{
    sym = string_to_symbol(name);
    value[sym] = string_to_symbol(sprintf("#<Primitive %s>", name));
    if (nparams != "")
        num_params[value[sym]] = nparams;
    return value[sym];
}

# --- Garbage collection

# Make a new pair.
function cons(the_car, the_cdr)
{
    while (pair_ptr in marks) {
	delete marks[pair_ptr];
	pair_ptr += 4;
    }
    if (pair_ptr == pair_limit)
        gc(the_car, the_cdr);
    car[pair_ptr] = the_car;
    cdr[pair_ptr] = the_cdr;
    pair_ptr += 4;
    return pair_ptr - 4;
}

function protect(object)	{ protected[++protected_ptr] = object; }
function unprotect()		{ --protected_ptr; }

function mark(object)
{
    while (is_pair(object) && !(object in marks)) {		#** speed
        marks[object] = 1;
        mark(car[object]);
        object = cdr[object];
    }
}

function gc(the_car, the_cdr,	p, i)
{
    if (loud_gc) 
        printf("\nGC...") >"/dev/stderr";
    mark(the_car); mark(the_cdr);
    for (p in protected)
        mark(protected[p]);
    for (p in stack)
        mark(stack[p]);
    for (p in value)
        mark(value[p]);
    for (p in property) {
        i = index(SUBSEP, p);
        mark(substr(p, 1, i-1));
        mark(substr(p, i+1));
        mark(property[p]);
    }
    pair_ptr = a_pair;
    while (pair_ptr in marks) {
	delete marks[pair_ptr];
	pair_ptr += 4;
    }
    if (pair_ptr == pair_limit) {
	if (loud_gc);
	    printf("Expanding heap...") >"/dev/stderr";
	pair_limit += 4 * heap_increment;
    }
}

# --- Set up

BEGIN {	
    srand();
    
    frame_ptr = stack_ptr = 0;

    if (heap_increment == "") heap_increment = 1500;
    pair_limit = a_pair + 4 * heap_increment;

    NIL 	= string_to_symbol("nil");
    T 		= string_to_symbol("t");
    value[NIL] = NIL;
    value[T] = T;
    car[NIL] = cdr[NIL] = NIL; # this is convenient in a couple places...

    THE_EOF_OBJECT = string_to_symbol("#eof");
    value[string_to_symbol("the-eof-object")] = THE_EOF_OBJECT;
    eof = "(eof)";

    QUOTE 	= string_to_symbol("quote");	is_special[QUOTE] = 1;
    LAMBDA 	= string_to_symbol("lambda");	is_special[LAMBDA] = 1;
    IF 		= string_to_symbol("if");	is_special[IF] = 1;
    SETQ 	= string_to_symbol("set!");	is_special[SETQ] = 1;
    DEFINE 	= string_to_symbol("define");	is_special[DEFINE] = 1;
    PROGN 	= string_to_symbol("begin");	is_special[PROGN] = 1;
    WHILE 	= string_to_symbol("while");	is_special[WHILE] = 1;

    EQ		= def_prim("eq?", 2);
    NULL 	= def_prim("null?", 1);
    CAR 	= def_prim("car", 1);
    CDR 	= def_prim("cdr", 1);
    CADR 	= def_prim("cadr", 1);
    CDDR 	= def_prim("cddr", 1);
    CONS 	= def_prim("cons", 2);
    LIST 	= def_prim("list");
    EVAL 	= def_prim("eval", 1);
    APPLY 	= def_prim("apply", 2);
    READ 	= def_prim("read", 0);
    WRITE 	= def_prim("write", 1);
    NEWLINE 	= def_prim("newline", 0);
    ADD		= def_prim("+", 2);
    SUB 	= def_prim("-", 2);
    MUL 	= def_prim("*", 2);
    DIV 	= def_prim("quotient", 2);
    MOD 	= def_prim("remainder", 2);
    LT 		= def_prim("<", 2);
    GET 	= def_prim("get", 2);
    PUT 	= def_prim("put", 3);
    ATOMP 	= def_prim("atom?", 1);
    PAIRP 	= def_prim("pair?", 1);
    SYMBOLP 	= def_prim("symbol?", 1);
    NUMBERP 	= def_prim("number?", 1);
    SETCAR 	= def_prim("set-car!", 2);
    SETCDR 	= def_prim("set-cdr!", 2);
    NREV 	= def_prim("reverse!", 1);
    GENSYM 	= def_prim("gensym", 0);
    RANDOM	= def_prim("random", 1);
    ERROR	= def_prim("error");

    DRIVER 	= string_to_symbol("top-level-driver");
}

# --- The interpreter

BEGIN {	
    for (;;) {
        if (DRIVER in value && value[DRIVER] != NIL)
            apply(value[DRIVER]);
        else {
            expr = read();
            if (expr == THE_EOF_OBJECT)
                break;
            protect(expr);
            print_expr(eval(expr));
            unprotect();
        }
    }
    
    if (profiling)
        for (proc in call_count) {
            printf("%5d ", call_count[proc]);
            print_expr(proc);
        }
}

# All the interpretation routines have the precondition that their
# arguments are protected from garbage collection.

function eval(expr,	old_frame_ptr)
{
    if (is_atom(expr))			#** speed
        if (is_symbol(expr)) {
            if (!(expr in value)) error("Unbound variable: " printname[expr]);
            return value[expr];
        } else
            return expr;

    op = car[expr];	# op is global to save awk stack space

    if (!(op in is_special)) {
        old_frame_ptr = frame_ptr;
        frame_ptr = stack_ptr;

        eval_rands(cdr[expr]);
        protect(proc = eval(car[expr]));
        result = apply(proc);
        unprotect();

        stack_ptr = frame_ptr;
        frame_ptr = old_frame_ptr;
        return result;
    }

    if (op == QUOTE)	return car[cdr[expr]];
    if (op == LAMBDA)	return expr;
    if (op == IF)	return eval(car[cdr[expr]]) != NIL 
                                ? eval(car[cdr[cdr[expr]]])	
                                : eval(car[cdr[cdr[cdr[expr]]]]);
    if (op == PROGN)	return progn(cdr[expr]);
    if (op == SETQ)	{
        if (!(car[cdr[expr]] in value))
            error("Unbound variable: " printname[car[cdr[expr]]]);
        return value[car[cdr[expr]]] = eval(car[cdr[cdr[expr]]]);
    }
    if (op == WHILE) {
        while (eval(car[cdr[expr]]) != NIL)
            progn(cdr[cdr[expr]]);
        return NIL;
    }
    if (op == DEFINE) {
        value[car[cdr[expr]]] = eval(car[cdr[cdr[expr]]]);
        return car[cdr[expr]];
    }
    
    error("BUG: Unknown special form");
}

# Evaluate a sequence of expressions, returning the last value.
function progn(exprs)
{
    for (; cdr[exprs] != NIL; exprs = cdr[exprs])
        eval(car[exprs]);
    return eval(car[exprs]);
}

# Evaluate the operands of a procedure, pushing the results on the stack.
function eval_rands(rands)
{
    for (; rands != NIL; rands = cdr[rands])
        stack[stack_ptr++] = eval(car[rands]);
}

# Call the procedure :proc, with args stack[frame_ptr]..stack[stack_ptr-1]
# (in that order).
function apply(proc)
{
    if (profiling) 
        ++call_count[proc];
    if (car[proc] == LAMBDA) {
        extend_env(car[cdr[proc]]);
        result = progn(cdr[cdr[proc]]); # result is global to save stack space
        unwind_env(car[cdr[proc]]);
        return result;
    }
    if (proc in num_params && num_params[proc] != stack_ptr - frame_ptr)
        error("Wrong number of arguments to " printname[cdr[proc]]);

    if (proc == CAR)	return car[is(a_pair, stack[frame_ptr])];
    if (proc == CDR)	return cdr[is(a_pair, stack[frame_ptr])];
    if (proc == CONS)	return cons(stack[frame_ptr], stack[frame_ptr+1]);
    if (proc == NULL)	return stack[frame_ptr] == NIL ? T : NIL;
    if (proc == EQ)	return stack[frame_ptr] == stack[frame_ptr+1] ? T : NIL;
    if (proc == ATOMP)	return is_atom(stack[frame_ptr]) ? T : NIL;
    if (proc == ADD)	return is(a_number, stack[frame_ptr]) + is(a_number, stack[frame_ptr+1]);
    if (proc == SUB)	return is(a_number, stack[frame_ptr]) - is(a_number, stack[frame_ptr+1]);
    if (proc == MUL)	return make_number(numeric_value(stack[frame_ptr]) * numeric_value(stack[frame_ptr+1]));
    if (proc == DIV)	return make_number(int(numeric_value(stack[frame_ptr]) / numeric_value(stack[frame_ptr+1])));
    if (proc == MOD)	return make_number(numeric_value(stack[frame_ptr]) % numeric_value(stack[frame_ptr+1]));
    if (proc == LT)	return (stack[frame_ptr] + 0 < stack[frame_ptr+1] + 0) ? T : NIL;
    if (proc == GET)	return (stack[frame_ptr], stack[frame_ptr+1]) in property ? property[stack[frame_ptr], stack[frame_ptr+1]] : NIL;
    if (proc == PUT) 	return property[stack[frame_ptr], stack[frame_ptr+1]] = stack[frame_ptr+2];
    if (proc == CADR)	return car[is(a_pair, cdr[is(a_pair, stack[frame_ptr])])];
    if (proc == CDDR)	return cdr[is(a_pair, cdr[is(a_pair, stack[frame_ptr])])];
    if (proc == LIST)	return listify_args();
    if (proc == SYMBOLP)return is_symbol(stack[frame_ptr]) ? T : NIL;
    if (proc == PAIRP)	return is_pair(stack[frame_ptr]) ? T : NIL;
    if (proc == NUMBERP)return is_number(stack[frame_ptr]) ? T : NIL;
    if (proc == SETCAR)	return car[is(a_pair, stack[frame_ptr])] = stack[frame_ptr+1];
    if (proc == SETCDR)	return cdr[is(a_pair, stack[frame_ptr])] = stack[frame_ptr+1];
    if (proc == APPLY)	return do_apply(stack[frame_ptr], stack[frame_ptr+1]);
    if (proc == EVAL)	return eval(stack[frame_ptr]);
    if (proc == NREV)	return nreverse(stack[frame_ptr], NIL);
    if (proc == WRITE)	{ write_expr(stack[frame_ptr]); printf(" "); return NIL; }
    if (proc == NEWLINE){ printf("\n"); return NIL;}
    if (proc == READ)	return read();
    if (proc == RANDOM)	return make_number(int(rand() * numeric_value(stack[frame_ptr])));
    if (proc == GENSYM)	return string_to_symbol("#G" ++gensym_counter);
    if (proc == ERROR)	{ printf("Error!\n"); print_expr(listify_args()); exit(1); }
    error("Unknown procedure type");
}

function do_apply(proc, args,		old_frame_ptr)
{
    old_frame_ptr = frame_ptr;
    frame_ptr = stack_ptr;

    for (; is_pair(args); args = cdr[args])
        stack[stack_ptr++] = car[args];
    if (args != NIL)
        error("Bad argument to APPLY: not a proper list");
    result = apply(proc);

    stack_ptr = frame_ptr;
    frame_ptr = old_frame_ptr;
    return result;
}

function listify_args(		p, result)
{
    result = NIL;
    for (p = stack_ptr - 1; frame_ptr <= p; --p)
        result = cons(stack[p], result);
    return result;
}

# --- The environment
# The current environment is represented by the set of values
# value[sym] where sym is a symbol.  extend_env(vars) adds a new
# set of bindings for vars, saving the old values; unwind_env(vars)
# restores those old values.  The new value for the nth member of
# vars is frame_ptr[n]; coincidentally, that's also where we 
# store away the old value, since that stack frame's not needed 
# for anything else after the extend_env() call.

function extend_env(vars,	p, temp)
{
    for (p = frame_ptr; vars != NIL; vars = cdr[vars]) {
        if (p == stack_ptr) 
	    error("Too many arguments to procedure");
        temp = value[car[vars]];
        value[car[vars]] = stack[p];
        stack[p] = temp;
        ++p;
    }
    if (p != stack_ptr) 
	error("Not enough arguments to procedure");
}

function unwind_env(vars,	p)
{
    for (p = frame_ptr; vars != NIL; vars = cdr[vars]) {
        if (stack[p] == "")
	    delete value[car[vars]];
	else
	    value[car[vars]] = stack[p];
	++p;
    }
}

# --- Output

function print_expr(expr)
{
    write_expr(expr);
    print "";
}

function write_expr(expr)
{
    if (is_atom(expr)) {
        if (!is_symbol(expr))
            printf("%d", numeric_value(expr));
        else {
            if (!(expr in printname))
                error("BUG: " expr " has no printname");
            printf("%s", printname[expr]);
        }
    } else {
        printf("(");
        write_expr(car[expr]);
        for (expr = cdr[expr]; is_pair(expr); expr = cdr[expr]) {
            printf(" ");
            write_expr(car[expr]);
        }
        if (expr != NIL) {
            printf(" . ");
            write_expr(expr);
        }
        printf(")");
    }
}

# --- Input

function read(		committed,	result)
{
    skip_blanks();
    if (token == eof)
        if (committed)
            error("Unexpected EOF");
        else
            return THE_EOF_OBJECT;
    if (token == "(") {			# read a list
        advance();
        result = NIL;
        for (;;) {
            skip_blanks();
            if (token == ".") {
                advance();
                after_dot = read(1);
                skip_blanks();
                if (token != ")")
                    error("')' expected");
                advance();
                return nreverse(result, after_dot);
            } else if (token == ")") {
                advance();
                return nreverse(result, NIL);
            } else {
                protect(result);
                result = cons(read(1), result);
                unprotect();
            }
        }
    } else if (token == "'") {		# a quoted expression
        advance();
        return cons(QUOTE, cons(read(1), NIL));
    } else if (token ~ /^-?[0-9]+$/) {	# a number
        result = make_number(token);
        advance();
        return result;
    } else {				# a symbol
        result = string_to_symbol(token);
        advance();
        return result;
    }
}

function skip_blanks()
{
    while (token ~ /^[ \t]*$/)
        advance();
}

function advance()
{
    if (token == eof) return eof;
    if (token == "") {
        if (getline line <= 0) {
            token = eof;
            return;
        }
    }
    if (match(line, "^[()'.]") ||
        match(line, "^[_A-Za-z0-9=!@$%&*<>?+\\-*/:]+") ||
        match(line, "^[ \\t]+")) {
        token = substr(line, RSTART, RLENGTH);
        line = substr(line, RLENGTH+1);
    } else if (line == "" || substr(line, 1, 1) == ";")
        token = ""; # this kludge permits interactive use
    else
        error("Lexical error starting at " line);
}

# --- Miscellany

# Destructively reverse :list and append :reversed_head.
function nreverse(list, reversed_head,		tail)
{
    while (is_pair(list)) {		#** speed?
        tail = cdr[list];
        cdr[list] = reversed_head;
        reversed_head = list;
        list = tail;
    }
    if (list != NIL)
 	error("Not a proper list - reverse!");
    return reversed_head;
}

function error(reason)
{
    print "ERROR: " reason >"/dev/stderr";
    exit(1);
}

