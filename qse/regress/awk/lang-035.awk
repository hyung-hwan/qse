BEGIN {
	# the maximum numbers of different voice 
	# numbers for a single circuit ID
	max_cid_vars = 100; 

	first = 1;
	while ((getline x < datafile) > 0)
	{
		# check if it is the first line.
		if (first) 
		{
			# skip it if so
			first = 0;
			continue;
		}

		# split a line into fields
		n = split (x, f, ",");
		if (n < 3) continue;

		# filter out data with an empty number.
		if (f[3] == "") continue;
	
		# store unique voice numbers to a table
		for (suffix = 0; suffix < max_cid_vars; suffix++)
		{
			oldval = tab[f[2],suffix];
			if (oldval == "") 
			{
				# store a cid/val pair into a table
				tab[f[2],suffix] = f[3];
				break;
			}
		}
	}
}

/^lease[[:space:]]+.+[[:space:]]*{[[:space:]]*$/ { 
	# reset the counter for each "lease x.x.x.x {" line
	voice_no=0; 
} 

{ 
	if ($1 == "option" && $2 == "agent.circuit-id")
	{
		# extract the circuit ID
		pos = index ($0, "agent.circuit-id ")
		len = length($0);

		last = substr ($0, len, 1);
		adj = 0; if (last != ";") adj++;
		cid = substr ($0, pos + 17, length($0) - (pos + 17 + adj));

		# insert all instances of voice numbers for a circuit ID
		for (suffix = 0; suffix < max_cid_vars; suffix++)
		{
			val = tab[cid,suffix];
			if (val == "") break;

			print "  info awk.voice-no-" voice_no " " val ";";
			voice_no++;
		}
	}

	print $0;

	if ($1 == "hardware" && $2 == "ethernet")
	{
		# append group information 
		print "  info awk.groupname \"" groupname "\";";
	}
}
