/*
 * $Id: lic.awk,v 1.1 2007/03/28 14:05:25 bacon Exp $
 *
 * {License}
 */

NR == 1 { 
	new_file = ARGV[0];
	printf "" > new_file; /* clear the file */
}

/^ \* \{License\}/ { 
	print " * Copyright (c) 2007, Hyung-Hwan Chung." >> new_file;
	print " * All rights reserved." >> new_file;
	print " * Licensed under the BSD license: " >> new_file;
	print " *     http://www.abiyo.net/ase/license.html" >> new_file;
}

!/^ \* \{License\}/ { 
	print $0 >> new_file;
}
