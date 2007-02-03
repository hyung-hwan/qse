global header, mode;

BEGIN {
	header = 1;
	mode = 0;
}

header && /^\.[[:alpha:]]+[[:space:]]/ {
	if ($1 == ".title")
	{
		print "TITLE: " $2;
	}
}

header && !/^\.[[:alpha:]]+[[:space:]]/ { header = 0; }

!header {
	local text;

	if (mode == 0)
	{
		if (/^== [^=]+ ==$/)
		{
			text=substr($0, 3, length($0)-4);
			print "<h2>" text "</h2>";
		}
		else if (/^=== [^=]+ ===$/)
		{
			text=substr($0, 4, length($0)-6);
			print "<h3>" text "</h3>";
		}
		else if (/^\{\{\{$/)
		{
			print "<pre>";
			mode = 1;
		}
		else if (/^$/)
		{
			print "<br>";
		}
	}
	else if (mode == 1)
	{
		if (/^}}}$/) 
		{
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
}
