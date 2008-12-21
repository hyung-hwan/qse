print: a1 and: a2
	| t1 t2 |
	<primitive: 3>
	t1 := #abcdefg.
	"a1 := 2341 arguments are not assignable"
	t2 := a2.
	t1 prim2: t2.
	super prim2: 999999.
	self prim2: 999999.
	^67891.
