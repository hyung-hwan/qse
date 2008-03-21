#!/bin/awk

BEGIN { 
        RS = "\n\n"; 
        FS = "\n"; 
}
{ 
	print $1, $NF;         
}
