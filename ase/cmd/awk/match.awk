BEGIN {
	print match ("hhhheeeo", /e+/);
	print RSTART, RLENGTH;
	print match ("heeeo", /e/);
	print RSTART, RLENGTH;
	print match ("heeeo", /t/);
	print RSTART, RLENGTH;

	print "--------------------------";
	print match ("hhhheeeo", "e+");
	print RSTART, RLENGTH;
	print match ("heeeo", "e");
	print RSTART, RLENGTH;
	print match ("heeeo", "t");
	print RSTART, RLENGTH;
	print "--------------------------";
}


