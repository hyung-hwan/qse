#
# a local variable can shade a function name and other local variables.
#

function a (x) { print x; } 

BEGIN {

	{
		@local a;
		a = 50;
		{
			@local a;
			a = 30;
			print a;
		}
		print a;
	}

	a (100);
}
