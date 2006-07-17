global x, y;

{
	print "NF = " NF;
	for (i = 0; i < 10; i++)
	{
		print $i;
		if (i == 3) a = $i;
	}

	$1 = 100;

	//$1 = $2;
	//$3 = $2;
	//$2 = $2;

	OFS["1234"]=":";
	$20 = 10;
	print $0;
	print "--------------------";
	print "NF ===>>> " NF;
	print "====================";
}

END { system ("dir /w/p"); print sin(270); }
