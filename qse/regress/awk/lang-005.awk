function a (a) { print a; } 

BEGIN {
	local a;
	a = 20; 
	a (1000); 
}
