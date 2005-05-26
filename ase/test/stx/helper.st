| x |
"print the reverse hierarchy of a class"
x := Class.
[x isNil] whileFalse: [
	Transcript show: x printString; cr.
	x := x superclass].

| x |
"print the reverse hierarchy of a metaclass"
x := Class class.
[x isNil] whileFalse: [
	Transcript show: x printString; cr.
	x := x superclass].
