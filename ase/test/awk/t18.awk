BEGIN { 
	print "0. FILENAME=", FILENAME, "FNR=", FNR; 
}

/hello/ { 
	print "1. FILENAME=", FILENAME, "FNR=", FNR; 
	print "[" $0 "]";
	#nextfile;
	print "----------------";
}

/hello/ { 
	print "2. FILENAME=", FILENAME, "FNR=", FNR; 
	print "[" $0 "]";
	nextfile;
	print "----------------";
}

END {
	print "== END OF PROGRAM ==";
}
