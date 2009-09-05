BEGIN {
	RS="\n-+\n";
	first = 1;
}
{
	if (!first) printf " ";
	printf "%s", $0;
	first = 0;
}
