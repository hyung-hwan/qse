#global mb_min, mb_max, wc_min, wc_max, mb, wc;

BEGIN {
	mb_min = 0xFFFFFFFF;
	mb_max = 0;
	wc_min = 0xFFFFFFFF;
	wc_max = 0;
}

!/^[[:space:]]*#/ {
	if (!($1 ~ /^0x/ && $2 ~ /^0x/)) next;

	mb = int($1);
	wc = int($2);

	if (mb < 128) 
	{
		if (mb != wc)
		{
			print "ERROR: mb != wc where mb < 128. i can't handle this encoding map";
			exit 1;
		}
		next;
	}

	if (mb < mb_min) mb_min = mb;
	if (mb > mb_max) mb_max = mb;
	if (wc < wc_min) wc_min = wc;
	if (wc > mb_max) wc_max = wc;

#	print mb, wc;
	#mb_arr[mb] = wc;
	#wc_arr[wc] = mb;
	if (mb in mb_arr)
		printf ("WARNING: 0x%04X already in mb_arr. old value = 0x%04X, this value = 0x%04x\n", mb, mb_arr[mb], wc) >  "/dev/stderr";
	else
		mb_arr[mb] = wc;

	if (wc in wc_arr)
		printf ("WARNING: 0x%04X already in mb_arr. old value = 0x%04X, this value = 0x%04x\n", wc, wc_arr[wc], mb) > "/dev/stderr";
	else
		wc_arr[wc] = mb;
}

END {
	#for (i = mb_min; i <= mb_max; i++)
	for (mb = 0; mb < 0xffff; mb++)
	{
		#wc = (i in mb_arr)? mb_arr[i]: 0xffff;
		if (mb <= 127) wc = mb;
		else wc = (mb in mb_arr)? mb_arr[mb]: 0xffff;
		printf ("0x%04x 0x%04x\n", mb, wc);
	}
}
