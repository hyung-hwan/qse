function a (a) { 
	print a; 
}

BEGIN {
	local a;
	a = 20; 
}

END { a (1000); }

