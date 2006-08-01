BEGIN { exit 10; }

{
	print $0;
}

END { print "== END OF PROGRAM =="; }
