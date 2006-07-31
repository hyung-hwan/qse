"abc" != "def"  {
	print $0 ~ /abc/;
	print $0 !~ /abc/;
	print /abc/;
}
