
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
		return 10;
	}

	END {
		print "END OF PROGRAM 2";
		return 20;
	}
