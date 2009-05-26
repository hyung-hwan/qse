/*
 * $Id: Sed.hpp 127 2009-05-07 13:15:04Z baconevi $
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

#ifndef _QSE_SED_SED_HPP_
#define _QSE_SED_SED_HPP_

#include <qse/Mmgr.hpp>
#include <qse/sed/sed.h>

/////////////////////////////////
QSE_BEGIN_NAMESPACE(QSE)
/////////////////////////////////

/** 
 * The Sed class implements a stream editor.
 */
class Sed: public Mmgr
{
public:
	typedef qse_sed_t sed_t;
	typedef qse_sed_io_cmd_t sed_io_cmd_t;
	typedef qse_sed_io_arg_t sed_io_arg_t;

	Sed () throw (): sed (QSE_NULL) {}

	int open () throw ();
	void close () throw ();
	int compile (const char_t* sptr, size_t slen) throw ();
	int execute () throw ();

protected:
	sed_t* sed;

	virtual int openIn (const char_t* path) = 0;
	virtual int closeIn () = 0;
	virtual ssize_t readIn (char_t* buf, size_t len) = 0;

	virtual int openOut (const char_t* path) = 0;
	virtual int closeOut () = 0;
	virtual ssize_t writeOut (const char_t* buf, size_t len) = 0;

private:
	static int xin (sed_t* s, sed_io_cmd_t cmd, sed_io_arg_t* arg);
	static int xout (sed_t* s, sed_io_cmd_t cmd, sed_io_arg_t* arg);
};

/////////////////////////////////
QSE_END_NAMESPACE(QSE)
/////////////////////////////////

#endif
