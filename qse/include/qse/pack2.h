/*
 * $Id: pack2.h 225 2008-06-26 06:48:38Z baconevi $
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

#if defined(__GNUC__)
	#pragma pack(2)
#elif defined(__HP_aCC) || defined(__HP_cc)
	#pragma PACK 2
#elif defined(_MSC_VER) || defined(__BORLANDC__)
	#pragma pack(push,2)
#elif defined(__DECC)
	#pragma pack(push,2)
#else
	#pragma pack(2)
#endif
