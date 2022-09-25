#!/bin/awk

BEGIN { 
        RS = "\n\n"; 
        FS = "\n"; 
}

$3 ~ /^S/ { 
	print $1, $3, $NF;        
}
