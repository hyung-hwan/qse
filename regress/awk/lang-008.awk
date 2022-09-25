#
# a local variable can shade a global variable
#

@global x;

BEGIN {
	x = 1;
	{
		@local x;
		x = 2;
		{
			@local x;
			x = 3;
			print x;
		}
		print x;
	}
	print x;
}

