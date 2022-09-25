BEGIN { FS = "\t"; }
      { pop[$4] += $3; }
END   { 
	# specifying a postion with + notation seems obsolete for sort
	# on most platforms. 
	# sort -t'\t' +1rn => sort -t'\t' -k2 -rn

	for (c in pop) 
		printf ("%15s\t%6d\n", c, pop[c]) | "sort -t'\t' -k2 -rn";

	# the following two statements make the program behave
	# consistently across different platforms.
	# on some platforms, the sort command output has
	# been delayed until the program exits. 
	close ("sort -t'\t' -k2 -rn");
	sleep (1);
}
