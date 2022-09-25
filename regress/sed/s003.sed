#
# author: APRIL1024 @ kldp
#
/^[^ ][^ ]*/{
	N
	/--*/!{
		P
		D
	}
	N
	s/\n.*\n/ /
}

