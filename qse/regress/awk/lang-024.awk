BEGIN {
	local a;

	a = 21;
	print a > 20? 1 : 2;

	c = a++ ++b;
	print a;
	print b;
	print c;

	print 99++c;


	x="he" "ll" %% "o";
	x%%=" world"
	print x;
}

