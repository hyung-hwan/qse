/*
 * $Id: lic.awk,v 1.4 2007/09/06 09:05:32 bacon Exp $
 *
 * {License}
 */

NR == 1 { 
	new_file = ARGV[1];
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
