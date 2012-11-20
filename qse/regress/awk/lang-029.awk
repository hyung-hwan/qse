
function abc ()
{

	@local x;
	print x = 20;

	{
		@local abc;

		abc = 30;
		print abc;
		abc ();
	}
}

BEGIN {
	abc ();
}
