BEGIN {
	print 1 + 0;
	print 0B11111111 + 0;
	print 10 + 0;
	print 0x10 + 0;
	print 0b00000010 + 0;
	print 0b + 0;
	print 0x + 0;

	print "-----------------------";
	print +1 + 0;
	print +0B11111111 + 0;
	print +10 + 0;
	print +0x10 + 0;
	print +0b00000010 + 0;
	print +0b + 0;
	print +0x + 0;

	print "-----------------------";
	print -1 + 0;
	print -0B11111111 + 0;
	print -10 + 0;
	print -0x10 + 0;
	print -0b00000010 + 0;
	print -0b + 0;
	print -0x + 0;
}
