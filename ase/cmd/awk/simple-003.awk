
        function error() { exit (200); }
	function getx()
	{
		if (x == 2) error();
		return x++;
	}
	
	function gety() { return y++; }

        BEGIN {
		x = 0;
		y = 0;
		print getx() + gety();
		print getx() + gety();
		print getx() + gety();
		print getx() + gety();
        }


	END {
		print "END OF PROGRAM";
		exit (20);
	}
