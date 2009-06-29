function x (y) { print y; }
function a (x) { print x; } 

BEGIN {
	local a;
	a = 20; 
	a (1000);
	{
		local a;
		print a;
	}
}
