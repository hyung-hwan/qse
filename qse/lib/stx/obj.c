/*
 * $Id$
 */

#include "stx.h"

qse_word_t qse_stx_allocwordobj (
	qse_stx_t* stx, const qse_word_t* data, qse_word_t nflds, 
	const qse_word_t* variable_data, qse_word_t variable_nflds)
{
	qse_word_t total_nflds;
	qse_size_t total_bytes;
	qse_stx_objidx_t idx;
	qse_word_t ref;
	qse_stx_wordobj_t* ptr;

	total_nflds = nflds + variable_nflds;
	total_bytes = 
		(total_nflds * QSE_SIZEOF(qse_word_t)) +
		QSE_SIZEOF(qse_stx_objhdr_t);
		
/* TODO: check if n is larger then header.access nfield bits can represent...
 *       then.... reject .... */

	idx = qse_stx_allocmem (stx, total_bytes);
	if (idx == QSE_STX_OBJIDX_INVALID) return stx->ref.nil;

	ref = IDXTOREF(stx,idx);
	ptr = (qse_stx_wordobj_t*)PTRBYIDX(stx,idx);

	ptr->h._type    = QSE_STX_WORDOBJ;
	ptr->h._mark    = 0;
	ptr->h._refcnt  = 0;
	ptr->h._size    = total_nflds;
	ptr->h._class   = stx->ref.nil;
	ptr->h._backref = ref;

	if (variable_data)
	{
		while (total_nflds > nflds) 
		{
			total_nflds--; 
			ptr->slot[total_nflds] = variable_data[total_nflds - nflds];
		}
	}
	else
	{
		while (total_nflds > nflds) 
			ptr->slot[--total_nflds] = stx->ref.nil;
	}

	if (data)
	{
		while (total_nflds > 0) 
		{
			total_nflds--; 
			ptr->slot[total_nflds] = data[total_nflds];
		}
	}
	else
	{
		while (total_nflds > 0) 
			ptr->slot[--total_nflds] = stx->ref.nil;
	}

	return ref;
}

qse_word_t qse_stx_allocbyteobj (
	qse_stx_t* stx, const qse_byte_t* variable_data, qse_word_t variable_nflds)
{
	qse_stx_objidx_t idx;
	qse_word_t ref;
	qse_stx_byteobj_t* ptr;

	idx = qse_stx_allocmem (
		stx, variable_nflds + QSE_SIZEOF(qse_stx_objhdr_t));
	if (idx == QSE_STX_OBJIDX_INVALID) return stx->ref.nil;

	ref = QSE_STX_IDXTOREF(stx,idx);
	ptr = (qse_stx_byteobj_t*)PTRBYIDX(stx,idx);

	ptr->h._type    = QSE_STX_BYTEOBJ;
	ptr->h._mark    = 0;
	ptr->h._refcnt  = 0;
	ptr->h._size    = variable_nflds;
	ptr->h._class   = stx->ref.nil;
	ptr->h._backref = ref;

	if (variable_data)
	{
		while (variable_nflds > 0) 
		{
			variable_nflds--;
			ptr->slot[variable_nflds] = variable_data[variable_nflds];
		}
	}

	return ref;
}

#if 0
qse_word_t qse_stx_alloc_char_object (
	qse_stx_t* stx, const qse_char_t* str)
{
	return (str == QSE_NULL)?
		qse_stx_alloc_char_objectx (stx, QSE_NULL, 0):
		qse_stx_alloc_char_objectx (stx, str, qse_strlen(str));
}
#endif

qse_word_t qse_stx_alloccharobj (
	qse_stx_t* stx, const qse_char_t* variable_data, qse_word_t variable_nflds)
{
	qse_stx_objidx_t idx;
	qse_word_t ref;
	qse_stx_charobj_t* ptr;
	qse_size_t total_bytes;

	total_bytes =
		(variable_nflds + 1) * QSE_SIZEOF(qse_char_t) + 
		QSE_SIZEOF(qse_stx_objhdr_t);

	idx = qse_stx_allocmem (stx, total_bytes);
	if (idx == QSE_STX_OBJIDX_INVALID) return stx->ref.nil;

	ref = QSE_STX_IDXTOREF(stx,idx);
	ptr = (qse_stx_charobj_t*)PTRBYIDX(stx,idx);

	ptr->h._type     = QSE_STX_CHAROBJ;
	ptr->h._mark     = 0;
	ptr->h._refcnt   = 0;
	/* the size for the character object does not include
	 * the extra 1 byte allocated for an implicit terminating 
	 * '\0' character */
	ptr->h._size     = variable_nflds; 
	ptr->h._class    = stx->ref.nil;
	ptr->h._backref  = ref;

	if (variable_data)
	{
		while (variable_nflds > 0) 
		{
			variable_nflds--;
			ptr->slot[variable_nflds] = variable_data[variable_nflds];
		}
	}

	QSE_ASSERT (ptr->slot[ptr->h._size] == QSE_T('\0'));
	return ref;
}

#if 0
qse_word_t qse_stx_allocn_char_object (qse_stx_t* stx, ...)
{
	qse_word_t idx, n = 0;
	const qse_char_t* p;
	qse_va_list ap;
	qse_stx_char_object_t* obj;

	QSE_ASSERT (stx->ref.nil == QSE_STX_NIL);

	qse_va_start (ap, stx);
	while ((p = qse_va_arg(ap, const qse_char_t*)) != QSE_NULL) {
		n += qse_strlen(p);
	}
	qse_va_end (ap);

	idx = qse_stx_memory_alloc (&stx->memory, 
		(n + 1) * QSE_SIZEOF(qse_char_t) + QSE_SIZEOF(qse_stx_object_t));
	if (idx >= stx->memory.capacity) return idx; /* failed */

	idx = QSE_STX_TOOBJIDX(idx);
	obj = QSE_STX_CHAR_OBJECT(stx,idx);
	obj->header.class = stx->ref.nil;
	obj->header.access = (n << 2) | QSE_STX_CHAR_INDEXED;
	obj->data[n] = QSE_T('\0');

	qse_va_start (ap, stx);
	n = 0;
	while ((p = qse_va_arg(ap, const qse_char_t*)) != QSE_NULL) 
	{
		while (*p != QSE_T('\0')) 
		{
			/*QSE_STX_CHAR_AT(stx,idx,n++) = *p++;*/
			obj->data[n++] = *p++;
		}
	}
	qse_va_end (ap);

	return idx;
}
#endif

#if 0
qse_word_t qse_stx_class (qse_stx_t* stx, qse_stx_objref_t obj)
{
	return QSE_STX_ISSMALLINT(obj)? 
		stx->class_smallinteger: QSE_STX_CLASS(stx,obj);
}

qse_word_t qse_stx_classof (qse_stx_t* stx, qse_stx_objref_t obj)
{
	return QSE_STX_ISSMALLINT(obj)? 
		stx->class_smallinteger: QSE_STX_CLASS(stx,obj);
}

qse_word_t qse_stx_sizeof (qse_stx_t* stx, qse_stx_objref_t obj)
{
	return QSE_STX_ISSMALLINT(obj)? 1: QSE_STX_SIZE(stx,obj);
}
#endif
