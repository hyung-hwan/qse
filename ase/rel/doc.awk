/*
 * $Id: doc.awk,v 1.14 2007-03-18 15:20:00 bacon Exp $
 *
 * {License}
 */

global header, mode;
global empty_line_count;
global para_started;
global list_count;

func print_text (full)
{
	local fra1, fra2, link, idx, t1, t2;

	gsub ("<", "\\&lt;", full);
	gsub (">", "\\&gt;", full);

	while (match (full, /\{[^{},]+,[^{},]+\}/) > 0)
	{
		fra1 = substr (full, 1, RSTART-1);
		link = substr (full, RSTART, RLENGTH);
		fra2 = substr (full, RSTART+RLENGTH, length(full)-RLENGTH);

		idx = index(link, ",");
		t1 = substr (link, 2, idx-2);
		t2 = substr (link, idx+1, length(link)-idx-1);

		full = sprintf ("%s<a href='%s'>%s</a>%s", fra1, t2, t1, fra2);
	}

	while (match (full, /\[\[[^\[\][:space:]]+\]\]/) > 0)
	{
		fra1 = substr (full, 1, RSTART-1);
		link = substr (full, RSTART+2, RLENGTH-4);
		fra2 = substr (full, RSTART+RLENGTH, length(full)-RLENGTH);

		full = sprintf ("%s<i>%s</i>%s", fra1, link, fra2);
	}

	while (match (full, /##[^#[:space:]]+##/) > 0)
	{
		fra1 = substr (full, 1, RSTART-1);
		link = substr (full, RSTART+2, RLENGTH-4);
		fra2 = substr (full, RSTART+RLENGTH, length(full)-RLENGTH);

		full = sprintf ("%s<b>%s</b>%s", fra1, link, fra2);
	}

	print full;
}

BEGIN {
	header = 1;
	mode = 0;
	empty_line_count = 0;
	para_started = 0;

	#output=ARGV[0];
	#gsub (/\.man/, ".html", output);
	#print "OUTPUT TO: " output;

	print "</html>";
	print "</head>";
	print "<meta http-equiv='content-type' content='text/html; charset=UTF-8'>";
	print "<link href='doc.css' rel='stylesheet' type='text/css' />";
}

header && /^\.[[:alpha:]]+[[:space:]]/ {
	if ($1 == ".title")
	{
		local i;

		printf "<title>";
		for (i = 2; i <= NF; i++) printf "%s ", $i;
		print "</title>";
	}
}

header && !/^\.[[:alpha:]]+[[:space:]]/ { 

	header = 0; 
	print "</head>";
	print "<body>";
}

!header {
	local text;

	if (mode == 0)
	{
		if (/^$/)
		{
			# empty line
			if (para_started) 
			{
				para_started = 0;
				print "</p>";
			}
			empty_line_count++;
		}
		else
		{
			if (/^= [^=]+ =$/)
			{
				if (para_started)
				{
					print "</p>";
					para_started = 0;
				}
				text=substr($0, 2, length($0)-2);
				print "<h1>" text "</h1>";
			}
			else if (/^== [^=]+ ==$/)
			{
				if (para_started)
				{
					print "</p>";
					para_started = 0;
				}
				text=substr($0, 3, length($0)-4);
				print "<h2>" text "</h2>";
			}
			else if (/^=== [^=]+ ===$/)
			{
				if (para_started)
				{
					print "</p>";
					para_started = 0;
				}
				text=substr($0, 4, length($0)-6);
				print "<h3>" text "</h3>";
			}
			else if (/^==== [^=]+ ====$/)
			{
				if (para_started)
				{
					print "</p>";
					para_started = 0;
				}
				text=substr($0, 5, length($0)-8);
				print "<h4>" text "</h4>";
			}
			else if (/^\{\{\{$/)
			{
				# {{{
				if (para_started)
				{
					print "</p>";
					para_started = 0;
				}
				print "<pre class='code'>";
				mode = 1;
			}
			else if (/\[\[\[/)
			{
				if (para_started)
				{
					print "</p>";
					para_started = 0;
				}
				print "<ul>";
				mode = 2;
				list_count = 0;
			}
			else if (/\(\(\(/)
			{
				if (para_started)
				{
					print "</p>";
					para_started = 0;
				}
				print "<ol>";
				mode = 3;
				list_count = 0;
			}
			else
			{
				if (!para_started > 0) 
				{
					print "<p>";
					para_started = 1;
				}
					
				/*
				gsub ("<", "\\&lt;");
				gsub (">", "\\&gt;");
				print $0;
				*/

				print_text ($0);
				print "<br>";
			}

			empty_line_count = 0;
		}
	}
	else if (mode == 1)
	{
		if (/^}}}$/) 
		{
			# }}}
			print "</pre>";
			mode = 0;
		}
		else
		{
			gsub ("<", "\\&lt;");
			gsub (">", "\\&gt;");
			print $0;
		}
	}
	else if (mode == 2)
	{
		if (/^]]]$/)
		{
			# ]]]
			print "</li>";
			print "</ul>";
			mode = 0;
		}
		else if (/^\* [^[:space:]]+/)
		{
			gsub ("<", "\\&lt;");
			gsub (">", "\\&gt;");
			if (list_count > 0) print "</li>";
			print "<li>";

			print_text (substr ($0, 3, length($0)-2));
			list_count++;
		}
		else
		{
			print_text ($0);
		}
	}
	else if (mode == 3)
	{
		if (/^\)\)\)$/)
		{
			# )))
			print "</li>";
			print "</ol>";
			mode = 0;
		}
		else if (/^\* [^[:space:]]+/)
		{
			gsub ("<", "\\&lt;");
			gsub (">", "\\&gt;");
			if (list_count > 0) print "</li>";
			print "<li>";

			print_text (substr ($0, 3, length($0)-2));
			list_count++;
		}
		else
		{
			print_text ($0);
		}
	}
}

END {
	print "</body>";
	print "</html>";
}
