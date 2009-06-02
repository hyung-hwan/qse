/*
 * $Id: misc.h 135 2009-05-15 13:31:43Z baconevi $
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

#ifndef _QSE_LIB_AWK_ERR_H_
#define _QSE_LIB_AWK_ERR_H_

#ifdef __cplusplus
extern "C" {
#endif

const qse_char_t* qse_awk_dflerrstr (qse_awk_t* awk, qse_awk_errnum_t errnum);

#ifdef __cplusplus
}
#endif

#endif

