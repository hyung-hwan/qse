/*
 * $Id: lic.awk,v 1.1 2007-02-03 11:54:02 bacon Exp $
 *
 * {License}
 */

NR == 1 { 
	new_file = ARGV[0];
	printf "" > new_file; /* clear the file */
}

/^ \* \{License\}/ { 
	print " * Copyright (c) 2007, Hyung-Hwan Chung (brian@abiyo.net)." >> new_file;
	print " * All rights reserved." >> new_file;
	print " * Licensed under the BSD license: " >> new_file;
	print " *     http://www.abiyo.net/ase/license.txt" >> new_file;
}

!/^ \* \{License\}/ { 
	print $0 >> new_file;
}


