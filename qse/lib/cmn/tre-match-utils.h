/*
 * $Id$
 *
    Copyright 2006-2011 Chung, Hyung-Hwan.
    This file is part of QSE.

    QSE is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as 
    published by the Free Software Foundation, either version 3 of 
    the License, or (at your option) any later version.

    QSE is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public 
    License along with QSE. If not, see <http://www.gnu.org/licenses/>.
 */

/*
  tre-match-utils.h - TRE matcher helper definitions

This is the license, copyright notice, and disclaimer for TRE, a regex
matching package (library and tools) with support for approximate
matching.

Copyright (c) 2001-2009 Ville Laurikari <vl@iki.fi>
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:

  1. Redistributions of source code must retain the above copyright
     notice, this list of conditions and the following disclaimer.

  2. Redistributions in binary form must reproduce the above copyright
     notice, this list of conditions and the following disclaimer in the
     documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT
HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#define str_source ((const tre_str_source*)string)

#ifdef TRE_WCHAR

#ifdef TRE_MULTIBYTE

/* Wide character and multibyte support. */

#define GET_NEXT_WCHAR()						      \
  do {									      \
    prev_c = next_c;							      \
    if (type == STR_BYTE)						      \
      {									      \
	pos++;								      \
	if (len >= 0 && pos >= len)					      \
	  next_c = '\0';						      \
	else								      \
	  next_c = (unsigned char)(*str_byte++);			      \
      }									      \
    else if (type == STR_WIDE)						      \
      {									      \
	pos++;								      \
	if (len >= 0 && pos >= len)					      \
	  next_c = QSE_T('\0');						      \
	else								      \
	  next_c = *str_wide++;						      \
      }									      \
    else if (type == STR_MBS)						      \
      {									      \
        pos += pos_add_next;					      	      \
	if (str_byte == NULL)						      \
	  next_c = QSE_T('\0');						      \
	else								      \
	  {								      \
	    size_t w;							      \
	    int max;							      \
	    if (len >= 0)						      \
	      max = len - pos;						      \
	    else							      \
	      max = 32;							      \
	    if (max <= 0)						      \
	      {								      \
		next_c = QSE_T('\0');						      \
		pos_add_next = 1;					      \
	      }								      \
	    else							      \
	      {								      \
		w = qse_mbrtowc(str_byte, (size_t)max, &next_c, &mbstate);    \
		if (w <= 0 || w > max)			      \
		  return REG_NOMATCH;					      \
		if (next_c == QSE_T('\0') && len >= 0)					      \
		  {							      \
		    pos_add_next = 1;					      \
		    next_c = 0;						      \
		    str_byte++;						      \
		  }							      \
		else							      \
		  {							      \
		    pos_add_next = w;					      \
		    str_byte += w;					      \
		  }							      \
	      }								      \
	  }								      \
      }									      \
    else if (type == STR_USER)						      \
      {									      \
        pos += pos_add_next;					      	      \
	str_user_end = str_source->get_next_char(&next_c, &pos_add_next,      \
                                                 str_source->context);	      \
      }									      \
  } while(/*CONSTCOND*/0)

#else /* !TRE_MULTIBYTE */

/* Wide character support, no multibyte support. */

#define GET_NEXT_WCHAR()						      \
  do {									      \
    prev_c = next_c;							      \
    if (type == STR_BYTE)						      \
      {									      \
	pos++;								      \
	if (len >= 0 && pos >= len)					      \
	  next_c = '\0';						      \
	else								      \
	  next_c = (unsigned char)(*str_byte++);			      \
      }									      \
    else if (type == STR_WIDE)						      \
      {									      \
	pos++;								      \
	if (len >= 0 && pos >= len)					      \
	  next_c = QSE_T('\0');						      \
	else								      \
	  next_c = *str_wide++;						      \
      }									      \
    else if (type == STR_USER)						      \
      {									      \
        pos += pos_add_next;					      	      \
	str_user_end = str_source->get_next_char(&next_c, &pos_add_next,      \
                                                 str_source->context);	      \
      }									      \
  } while(/*CONSTCOND*/0)

#endif /* !TRE_MULTIBYTE */

#else /* !TRE_WCHAR */

/* No wide character or multibyte support. */

#define GET_NEXT_WCHAR()						      \
  do {									      \
    prev_c = next_c;							      \
    if (type == STR_BYTE)						      \
      {									      \
	pos++;								      \
	if (len >= 0 && pos >= len)					      \
	  next_c = '\0';						      \
	else								      \
	  next_c = (unsigned char)(*str_byte++);			      \
      }									      \
    else if (type == STR_USER)						      \
      {									      \
	pos += pos_add_next;						      \
	str_user_end = str_source->get_next_char(&next_c, &pos_add_next,      \
						 str_source->context);	      \
      }									      \
  } while(/*CONSTCOND*/0)

#endif /* !TRE_WCHAR */



#define IS_WORD_CHAR(c)	 ((c) == QSE_T('_') || tre_isalnum(c))

#define CHECK_ASSERTIONS(assertions)					      \
  (((assertions & ASSERT_AT_BOL)					      \
    && (pos > 0 || reg_notbol)						      \
    && (prev_c != QSE_T('\n') || !reg_newline))				      \
   || ((assertions & ASSERT_AT_EOL)					      \
       && (next_c != QSE_T('\0') || reg_noteol)				      \
       && (next_c != QSE_T('\n') || !reg_newline))				      \
   || ((assertions & ASSERT_AT_BOW)					      \
       && (IS_WORD_CHAR(prev_c) || !IS_WORD_CHAR(next_c)))	              \
   || ((assertions & ASSERT_AT_EOW)					      \
       && (!IS_WORD_CHAR(prev_c) || IS_WORD_CHAR(next_c)))		      \
   || ((assertions & ASSERT_AT_WB)					      \
       && (pos != 0 && next_c != QSE_T('\0')					      \
	   && IS_WORD_CHAR(prev_c) == IS_WORD_CHAR(next_c)))		      \
   || ((assertions & ASSERT_AT_WB_NEG)					      \
       && (pos == 0 || next_c == QSE_T('\0')					      \
	   || IS_WORD_CHAR(prev_c) != IS_WORD_CHAR(next_c))))

#define CHECK_CHAR_CLASSES(trans_i, tnfa, eflags)                             \
  (((trans_i->assertions & ASSERT_CHAR_CLASS)                                 \
       && !(tnfa->cflags & REG_ICASE)                                         \
       && !tre_isctype((tre_cint_t)prev_c, trans_i->u.class))                 \
    || ((trans_i->assertions & ASSERT_CHAR_CLASS)                             \
        && (tnfa->cflags & REG_ICASE)                                         \
        && !tre_isctype(tre_tolower((tre_cint_t)prev_c),trans_i->u.class)     \
	&& !tre_isctype(tre_toupper((tre_cint_t)prev_c),trans_i->u.class))    \
    || ((trans_i->assertions & ASSERT_CHAR_CLASS_NEG)                         \
        && tre_neg_char_classes_match(trans_i->neg_classes,(tre_cint_t)prev_c,\
                                      tnfa->cflags & REG_ICASE)))




/* Returns 1 if `t1' wins `t2', 0 otherwise. */
QSE_INLINE static int
tre_tag_order(int num_tags, tre_tag_direction_t *tag_directions,
              int *t1, int *t2)
{
	int i;
	for (i = 0; i < num_tags; i++)
	{
		if (tag_directions[i] == TRE_TAG_MINIMIZE)
		{
			if (t1[i] < t2[i])
				return 1;
			if (t1[i] > t2[i])
				return 0;
		}
		else
		{
			if (t1[i] > t2[i])
				return 1;
			if (t1[i] < t2[i])
				return 0;
		}
	}
	/*  assert(0);*/
	return 0;
}

QSE_INLINE static int
tre_neg_char_classes_match(tre_ctype_t *classes, tre_cint_t wc, int icase)
{
	DPRINT(("neg_char_classes_test: %p, %d, %d\n", classes, wc, icase));
	while (*classes != (tre_ctype_t)0)
		if ((!icase && tre_isctype(wc, *classes))
		        || (icase && (tre_isctype(tre_toupper(wc), *classes)
		                      || tre_isctype(tre_tolower(wc), *classes))))
			return 1; /* Match. */
		else
			classes++;
	return 0; /* No match. */
}
