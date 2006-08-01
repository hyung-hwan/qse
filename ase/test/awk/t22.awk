//BEGIN { exit 10; }

//{ while (1) {if (x == 20) continue;  if (a) break; while (10) break; }}
//END { while (1) {if (x == 20) continue;  if (a) break; while (10) break; }}

{
	//return 20;

	print getline abc < "";
	print "[[" abc "]]";
	print close("");
	//exit 20;
}

END { print "end"; }
