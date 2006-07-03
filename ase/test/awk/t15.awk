BEGIN
{
	local a, b;

	a = 12;
	b = "<" a ">";

	print b;

	print ("\x5C");
	print ("\x5C6_ABCDEGH");
	print ("\xZZ5C6_ABCDEGH");
	print ("\xZZ5C6_AB\u7658&&");
	print "\uC720\uB2C8\uCF54\uB4DC \uD14C\uC2A4\uD2B8";
	print "\UC720\UB2C8\UCF54\UB4DC \UD14C\UC2A4\UD2B8";
}

