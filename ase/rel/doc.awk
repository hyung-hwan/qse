BEGIN {
	header = 1;
}

header && /^\.[[:alpha:]]+[[:space:]]/ {
	if ($1 == ".title")
	{
		print "TITLE: " $2;
	}
}

header && !/^\.[[:alpha:]]+[[:space:]]/ { header = 0; }

!header {
	if (/^== [^=]+ ==$/)
	{
		print "H2" $0;
	}
	else if (/^=== [^=]+ ===$/)
	{
		print "H3" $0;
	}
}



