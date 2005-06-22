"isNil" "test if self is nil"
perform: method with: x with: y with: z with: a with: b with: c

	| a b c d e f g |

	a := 'this is ''good'.
	b := -30 xxx nil this.

"
	$a. 
	$b.
	^true.
	a := b.
	' this is bad
"
