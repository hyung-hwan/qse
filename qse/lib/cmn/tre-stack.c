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
  tre-stack.c - Simple stack implementation

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

#include "tre.h"
#include "tre-stack.h"

union tre_stack_item
{
	void *voidptr_value;
	int int_value;
};

struct tre_stack_rec
{
	qse_mmgr_t* mmgr;
	int size;
	int max_size;
	int increment;
	int ptr;
	union tre_stack_item *stack;
};


tre_stack_t* tre_stack_new(qse_mmgr_t* mmgr, int size, int max_size, int increment)
{
	tre_stack_t *s;

	s = xmalloc(mmgr, sizeof(*s));
	if (s != NULL)
	{
		s->stack = xmalloc(mmgr, sizeof(*s->stack) * size);
		if (s->stack == NULL)
		{
			xfree(mmgr, s);
			return NULL;
		}
		s->size = size;
		s->max_size = max_size;
		s->increment = increment;
		s->ptr = 0;
		s->mmgr = mmgr;
	}
	return s;
}

void
tre_stack_destroy(tre_stack_t *s)
{
	xfree(s->mmgr,s->stack);
	xfree(s->mmgr,s);
}

int
tre_stack_num_objects(tre_stack_t *s)
{
	return s->ptr;
}

static reg_errcode_t
tre_stack_push(tre_stack_t *s, union tre_stack_item value)
{
	if (s->ptr < s->size)
	{
		s->stack[s->ptr] = value;
		s->ptr++;
	}
	else
	{
		if (s->size >= s->max_size)
		{
			DPRINT(("tre_stack_push: stack full\n"));
			return REG_ESPACE;
		}
		else
		{
			union tre_stack_item *new_buffer;
			int new_size;
			DPRINT(("tre_stack_push: trying to realloc more space\n"));
			new_size = s->size + s->increment;
			if (new_size > s->max_size)
				new_size = s->max_size;
			new_buffer = xrealloc(s->mmgr, s->stack, sizeof(*new_buffer) * new_size);
			if (new_buffer == NULL)
			{
				DPRINT(("tre_stack_push: realloc failed.\n"));
				return REG_ESPACE;
			}
			DPRINT(("tre_stack_push: realloc succeeded.\n"));
			assert(new_size > s->size);
			s->size = new_size;
			s->stack = new_buffer;
			tre_stack_push(s, value);
		}
	}
	return REG_OK;
}

#define define_pushf(typetag, type)  \
  declare_pushf(typetag, type) {     \
    union tre_stack_item item;	     \
    item.typetag ## _value = value;  \
    return tre_stack_push(s, item);  \
}

define_pushf(int, int)
define_pushf(voidptr, void *)

#define define_popf(typetag, type)		    \
  declare_popf(typetag, type) {			    \
    return s->stack[--s->ptr].typetag ## _value;    \
  }

define_popf(int, int)
define_popf(voidptr, void *)

/* EOF */
