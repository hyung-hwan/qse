/*
 * $Id$
 *
    Copyright (c) 2006-2014 Chung, Hyung-Hwan. All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:
    1. Redistributions of source code must retain the above copyright
       notice, this list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright
       notice, this list of conditions and the following disclaimer in the
       documentation and/or other materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
    IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
    OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
    IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
    INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
    NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
    THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <qse/cmn/gdl.h>

void qse_gdl_chain (qse_gdl_t* gdl, qse_gdl_link_t* prev, qse_gdl_link_t* x, qse_gdl_link_t* next)
{
	x->prev = prev;
	x->next = next; 
	next->prev = x; 
	prev->next = x;
}

void qse_gdl_unchain (qse_gdl_t* gdl, qse_gdl_link_t* x)
{
	qse_gdl_link_t* p = x->prev;
	qse_gdl_link_t* n = x->next;
	n->prev = p; p->next = n;
}

void qse_gdl_replace (qse_gdl_t* gdl, qse_gdl_link_t* old_link, qse_gdl_link_t* new_link)
{
	new_link->next = old_link->next;
	new_link->next->prev = new_link;
	new_link->prev = old_link->prev;
	new_link->prev->next = new_link;
}
