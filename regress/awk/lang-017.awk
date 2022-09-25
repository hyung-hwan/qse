
function error() { exit (200); }

function getx()
{
	if (x == 2) error();
	return x++;
}
	
function gety() { return y++; }

function main() {
	x = 0;
	y = 0;

	print getx() + gety();
	print getx() + gety();
	print getx() + gety();
	print getx() + gety();

	return 999;
}

BEGIN {
	main ();
}

END {
	print "END OF PROGRAM";
	return 10;
}

END {
	print "END OF PROGRAM 2";
	exit (100);
}

END {
	print "END OF PROGRAM 3";
	exit (900);
}
