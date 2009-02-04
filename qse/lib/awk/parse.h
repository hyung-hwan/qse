/*
 * $Id: parse.h 363 2008-09-04 10:58:08Z baconevi $
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

#ifndef _QSE_LIB_AWK_PARSE_H_
#define _QSE_LIB_AWK_PARSE_H_

/* these enums should match kwtab in parse.c */
enum kw_t
{
	KW_BEGIN,
	KW_END,
	KW_BREAK,
	KW_CONTINUE,
	KW_DELETE,
	KW_DO,
	KW_ELSE,
	KW_EXIT,
	KW_FOR,
	KW_FUNCTION,
	KW_GETLINE,
	KW_GLOBAL,
	KW_IF,
	KW_IN,
	KW_LOCAL,
	KW_NEXT,
	KW_NEXTFILE,
	KW_NEXTOFILE,
	KW_PRINT,
	KW_PRINTF,
	KW_RESET,
	KW_RETURN,
	KW_WHILE
};

#ifdef __cplusplus
extern "C" {
#endif

int qse_awk_putsrcstr (qse_awk_t* awk, const qse_char_t* str);
int qse_awk_putsrcstrx (
	qse_awk_t* awk, const qse_char_t* str, qse_size_t len);

const qse_char_t* qse_awk_getgblname (
	qse_awk_t* awk, qse_size_t idx, qse_size_t* len);
qse_cstr_t* qse_awk_getkw (qse_awk_t* awk, int id, qse_cstr_t* s);

int qse_awk_initgbls (qse_awk_t* awk);

#ifdef __cplusplus
}
#endif

#endif
