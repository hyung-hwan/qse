/*
 * $Id: ctype.c,v 1.1 2007-02-20 14:47:33 bacon Exp $
 */

#include <ase/utl/ctype.h>

#if defined(ASE_CHAR_IS_MCHAR) && defined(isupper)

	int ase_isupper (int c) { return isupper (c); }
	int ase_islower (int c) { return islower (c); }
	int ase_isalpha (int c) { return isalpha (c); }
	int ase_isdigit (int c) { return isdigit (c); }
	int ase_isxdigit (int c) { return isxdigit (c); }
	int ase_isalnum (int c) { return isalnum (c); }
	int ase_isspace (int c) { return isspace (c); }
	int ase_isprint (int c) { return isprint (c); }
	int ase_isgraph (int c) { return isgraph (c); }
	int ase_iscntrl (int c) { return iscntrl (c); }
	int ase_ispunct (int c) { return ispunct (c); }

#endif

#if defined(ASE_CHAR_IS_MCHAR) && defined(toupper)
	int ase_toupper (int c) { return toupper (c); }
	int ase_tolower (int c) { return tolower (c); }
#endif

