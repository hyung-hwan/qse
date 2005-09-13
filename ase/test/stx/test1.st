print: a1 and: a2
	| t1 |
	<primitive: 3>
	t1 := #abcdefg.
	"a1 := 2341 arguments are not assignable"
	t1 prim2: a2.
	^nil.
