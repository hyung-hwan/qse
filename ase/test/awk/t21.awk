BEGIN { exit 10; }

{
	print $0;
	//print close ("");
}

END { print "== END OF PROGRAM =="; }
