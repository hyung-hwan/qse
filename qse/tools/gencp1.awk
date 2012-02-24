#global mb_min, mb_max, wc_min, wc_max, mb, wc;

BEGIN {
	if (ARGC != 2)
	{
		ERROR_CODE=1
		exit 1; 
	}

	mb_min = 0xFFFFFFFF;
	mb_max = 0;
	wc_min = 0xFFFFFFFF;
	wc_max = 0;

	if (MAX_GAP <= 0) MAX_GAP=64
}

!/^[[:space:]]*#/ {
	if (!($1 ~ /^0x/ && $2 ~ /^0x/)) next;

	mb = int($1);
	wc = int($2);

	if (mb < 128) 
	{
		if (mb != wc)
		{
			ERROR_CODE = 2;
			exit 1;
		}
		next;
	}

	if (mb < mb_min) mb_min = mb;
	if (mb > mb_max) mb_max = mb;
	if (wc < wc_min) wc_min = wc;
	if (wc > wc_max) wc_max = wc;

	if (mb in mb_arr)
		printf ("WARNING: 0x%04X already in mb_arr. old value = 0x%04X, this value = 0x%04x\n", mb, mb_arr[mb], wc) >  "/dev/stderr";
	else
		mb_arr[mb] = wc;
		
	if (wc in wc_arr)
		printf ("WARNING: 0x%04X already in wc_arr. old value = 0x%04X, this value = 0x%04x\n", wc, wc_arr[wc], mb) > "/dev/stderr";
	else
		wc_arr[wc] = mb;
}

function emit_simple (name, min, max, arr) {
	printf ("static qse_uint16_t %s_tab[] =\n", name);
	printf ("{\n");
	for (i = min; i <= max; i++)
	{
		wc = (i in arr)? arr[i]: 0xffff;

		printf ("\t0x%04xu", wc);
		if (i < max) printf (",\n");
		else printf ("\n");
	}
	printf ("};\n");

	printf ("static qse_uint16_t %s (qse_uint16_t c)\n{\n", name);
	#printf ("\tif (c >= 0 && c <= 127) return c;\n");
	printf ("\tif (c >= 0x%04xu && c <= 0x%04xu) return %s_tab[c - 0x%04xu];\n", min, max, name, min);
	printf ("\treturn 0xffffu;\n");
	printf ("};\n");
}

function emit_bsearch (name, min, max, arr) {
	prev_in_arr = 0;
	prev_no_in_arr = 0;
	seg_no = 0;

	for (i = min; i <= max; i++)
	{
		if (i in arr)
		{
			if (prev_in_arr <= 0)
			{
				if (prev_not_in_arr > 0 && prev_not_in_arr <= MAX_GAP)
				{
					# if the segment whole is not large enough
					# combine two segments together
					for (j = 0; j < prev_not_in_arr; j++) 
						printf (",\n\t0xffffu");
					seg_last[seg_no] = i;
					printf (",\n");
				}
				else 
				{
					if (prev_not_in_arr > 0)
					{
						printf ("\n}; /* range 0x%x - 0x%x, total %d chars */\n", 
							seg_first[seg_no], seg_last[seg_no], 
							seg_last[seg_no] - seg_first[seg_no] + 1);
						seg_no++;
					}

					printf ("static qse_uint16_t %s_seg_%d[] =\n{\n", name, seg_no);
					seg_first[seg_no] = i;
					seg_last[seg_no] = i;
				}
			}
			else
			{
				seg_last[seg_no] = i;
				printf (",\n");
			}

			printf ("\t0x%04xu /* 0x%04x */", arr[i], i);
			prev_in_arr++;
			prev_not_in_arr = 0;
		}
		else
		{
		#	if (prev_in_arr > 0) 
		#	{
		#		printf ("\n}; /* range 0x%x - 0x%x, total %d chars */\n", 
		#			seg_first[seg_no], seg_last[seg_no], 
		#			seg_last[seg_no] - seg_first[seg_no] + 1);
		#		seg_no++;
		#	}

			prev_in_arr = 0;
			prev_not_in_arr++;
		}
	}

	if (prev_in_arr > 0) 
	{
		printf ("\n}; /* range 0x%x - 0x%x, total %d chars */\n", 
			seg_first[seg_no], seg_last[seg_no], 
			seg_last[seg_no] - seg_first[seg_no] + 1);
	}

	printf ("static struct %s_range_t\n{\n\tqse_uint16_t first, last;\n\tqse_uint16_t* seg;\n} %s_range[] =\n{\n", name, name); 
	printf ("\t{ 0x%04xu, 0x%04xu, %s_seg_0 }", seg_first[0], seg_last[0], name); 
	for (i = 1; i <= seg_no; i++) printf (",\n\t{ 0x%04xu, 0x%04xu, %s_seg_%d }", seg_first[i], seg_last[i], name, i); 
	printf ("\n};\n"); 

	printf ("static qse_uint16_t %s (qse_uint16_t c)\n{\n", name);

	#printf ("\tif (c >= 0 && c <= 127) return c;\n");
	printf ("\tif (c >= %s_range[0].first &&\n\t    c <= %s_range[QSE_COUNTOF(%s_range)-1].last)\n\t{\n", name, name, name);

	printf ("\t\tint left = 0, right = QSE_COUNTOF(%s_range) - 1, mid;
		while (left <= right)
		{
			mid = (left + right) / 2;
			if (c >= %s_range[mid].first && c <= %s_range[mid].last) 
				return %s_range[mid].seg[c - %s_range[mid].first];
			else if (c > %s_range[mid].last) 
				left = mid + 1; 
			else
				right = mid - 1;
		}\n", name, name, name, name, name, name);

	printf ("\t}\n\treturn 0xffffu;\n");
	printf ("}\n");
}

END {

	if (ERROR_CODE == 1)
	{
		print "USAGE: gencp.awk codepage-file" > "/dev/stderr";
		exit 1
	}
	else if (ERROR_CODE == 2)
	{
		print "ERROR: mb != wc where mb < 128. i can't handle this encoding map" > "/dev/stderr";
		exit 1;
	}
	else
	{
		"date" | getline date;
		printf ("/* This is a privite file automatically generated\n");
		printf (" * from %s on %s.\n", ARGV[1], date);
		printf (" * Never include this file directly into your source code.\n");
		printf (" *   mode=%s \n", (SIMPLE_MODE? "simple": "bsearch"));
		printf (" *   mb_min=0x%04x \n", mb_min);
		printf (" *   mb_max=0x%04x \n", mb_max);
		printf (" *   wc_min=0x%04x \n", wc_min);
		printf (" *   wc_max=0x%04x \n", wc_max);
		printf (" */\n\n");

		if (SIMPLE_MODE)
		{
			emit_simple ("mbtowc", mb_min, mb_max, mb_arr);
			printf ("\n/* ----------------------------------------- */\n\n");
			emit_simple ("wctomb", wc_min, wc_max, wc_arr);
		}
		else
		{
			emit_bsearch ("mbtowc", mb_min, mb_max, mb_arr);
			printf ("\n/* ----------------------------------------- */\n\n");
			emit_bsearch ("wctomb", wc_min, wc_max, wc_arr);
		}
	}
}
