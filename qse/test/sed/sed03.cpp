/*
 * $Id$
 *
   Copyright 2006-2009 Chung, Hyung-Hwan.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
 */

#include <qse/sed/StdSed.hpp>
#include <qse/utl/main.h>
#include <qse/utl/stdio.h>

class MySed: protected QSE::StdSed
{
public:
	MySed ()
	{
		if (open() == -1)
		{
			throw 1;
		}
	}	

	~MySed ()
	{
		close ();
	}
	
	int exec (const char_t* cmds, const char_t* in, string& out)
	{
		if (compile (cmds) == -1) return -1;
		if (execute () == -1) return -1;
		return 0;
	}

protected:
	int openConsole (Console& io)
	{
		return 1;
	}

	int closeConsole (Console& io)
	{
		return 0;
	}

	ssize_t readConsole (Console& io, char_t* buf, size_t len)
	{
		return 0;
	}

	ssize_t writeConsole (Console& io, const char_t* buf, size_t len)
	{
		return 0;
	}
};

int sed_main (int argc, qse_char_t* argv[])
{
	MySed sed;

	if (sed.exec (QSE_T("s/abc/XXXXXXXXXXX/g"), in, out) == 0)
	{
		return -1;
	}

	return 0;
}

int qse_main (int argc, char* argv[])
{
	return qse_runmain (argc, argv, sed_main);
}
