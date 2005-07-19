/*
 * $Id: object.c,v 1.38 2005-07-19 12:08:04 bacon Exp $
 */

#include <xp/stx/object.h>
#include <xp/stx/memory.h>
#include <xp/stx/symbol.h>
#include <xp/stx/class.h>
#include <xp/stx/hash.h>
#include <xp/stx/misc.h>

/* n: number of instance variables */
xp_word_t xp_stx_alloc_word_object (
	xp_stx_t* stx, const xp_word_t* data, xp_word_t nfields, 
	const xp_word_t* variable_data, xp_word_t variable_nfields)
{
	xp_word_t idx, n;
	xp_stx_word_object_t* obj;

	xp_assert (stx->nil == XP_STX_NIL);

	/* bytes to allocated =
	 *     (number of instance variables + 
	 *      number of variable instance variables) * word_size 
	 */
	n = nfields + variable_nfields;
xp_printf (XP_TEXT(">> %d\n"), n);
	idx = xp_stx_memory_alloc (&stx->memory,
		n * xp_sizeof(xp_word_t) + xp_sizeof(xp_stx_object_t));
	if (idx >= stx->memory.capacity) return idx; /* failed TODO: return a difference value OINDEX_INVALID */

	idx = XP_STX_TO_OINDEX(idx);
	obj = XP_STX_WORD_OBJECT(stx,idx);
	obj->header.class = stx->nil;
	obj->header.access = (n << 2) | XP_STX_WORD_INDEXED;

	if (variable_data == XP_NULL) {
		while (n > nfields) obj->data[--n] = stx->nil;
	}
	else {
		while (n > nfields) {
			n--; obj->data[n] = variable_data[n - nfields];
		}
	}

	if (data == XP_NULL) { 
		while (n > 0) obj->data[--n] = stx->nil;
	}
	else {
		while (n > 0) {
			n--; obj->data[n] = data[n];
		}
	}

	return idx;
}

/* n: number of bytes */
xp_word_t xp_stx_alloc_byte_object (
	xp_stx_t* stx, const xp_byte_t* data, xp_word_t n)
{
	xp_word_t idx;
	xp_stx_byte_object_t* obj;

	xp_assert (stx->nil == XP_STX_NIL);

	idx = xp_stx_memory_alloc (
		&stx->memory, n + xp_sizeof(xp_stx_object_t));
	if (idx >= stx->memory.capacity) return idx; /* failed */

	idx = XP_STX_TO_OINDEX(idx);
	obj = XP_STX_BYTE_OBJECT(stx,idx);
	obj->header.class = stx->nil;
	obj->header.access = (n << 2) | XP_STX_BYTE_INDEXED;

	if (data == XP_NULL) {
		while (n-- > 0) obj->data[n] = 0;
	}
	else {
		while (n-- > 0) obj->data[n] = data[n];
	}

	return idx;
}

xp_word_t xp_stx_alloc_char_object (
	xp_stx_t* stx, const xp_char_t* str)
{
	return (str == XP_NULL)?
		xp_stx_alloc_char_objectx (stx, XP_NULL, 0):
		xp_stx_alloc_char_objectx (stx, str, xp_strlen(str));
}

/* n: number of characters */
xp_word_t xp_stx_alloc_char_objectx (
	xp_stx_t* stx, const xp_char_t* str, xp_word_t n)
{
	xp_word_t idx;
	xp_stx_char_object_t* obj;

	xp_assert (stx->nil == XP_STX_NIL);

	idx = xp_stx_memory_alloc (&stx->memory, 
		(n + 1) * xp_sizeof(xp_char_t) + xp_sizeof(xp_stx_object_t));
	if (idx >= stx->memory.capacity) return idx; /* failed */

	idx = XP_STX_TO_OINDEX(idx);
	obj = XP_STX_CHAR_OBJECT(stx,idx);
	obj->header.class = stx->nil;
	obj->header.access = (n << 2) | XP_STX_CHAR_INDEXED;
	obj->data[n] = XP_CHAR('\0');

	if (str == XP_NULL) {
		while (n-- > 0) obj->data[n] = XP_CHAR('\0');
	}
	else {
		while (n-- > 0) obj->data[n] = str[n];
	}

	return idx;
}

xp_word_t xp_stx_allocn_char_object (xp_stx_t* stx, ...)
{
	xp_word_t idx, n = 0;
	const xp_char_t* p;
	xp_va_list ap;
	xp_stx_char_object_t* obj;

	xp_assert (stx->nil == XP_STX_NIL);

	xp_va_start (ap, stx);
	while ((p = xp_va_arg(ap, const xp_char_t*)) != XP_NULL) {
		n += xp_strlen(p);
	}
	xp_va_end (ap);

	idx = xp_stx_memory_alloc (&stx->memory, 
		(n + 1) * xp_sizeof(xp_char_t) + xp_sizeof(xp_stx_object_t));
	if (idx >= stx->memory.capacity) return idx; /* failed */

	idx = XP_STX_TO_OINDEX(idx);
	obj = XP_STX_CHAR_OBJECT(stx,idx);
	obj->header.class = stx->nil;
	obj->header.access = (n << 2) | XP_STX_CHAR_INDEXED;
	obj->data[n] = XP_CHAR('\0');

	xp_va_start (ap, stx);
	n = 0;
	while ((p = xp_va_arg(ap, const xp_char_t*)) != XP_NULL) {
		while (*p != XP_CHAR('\0')) {
			/*XP_STX_CHAR_AT(stx,idx,n++) = *p++;*/
			obj->data[n++] = *p++;
		}
	}
	xp_va_end (ap);

	return idx;
}

xp_word_t xp_stx_hash_object (xp_stx_t* stx, xp_word_t object)
{
	xp_word_t hv;

	if (XP_STX_IS_SMALLINT(object)) {
		xp_word_t tmp = XP_STX_FROM_SMALLINT(object);
		hv = xp_stx_hash(&tmp, xp_sizeof(tmp));
	}
	else if (XP_STX_IS_CHAR_OBJECT(stx,object)) {
		/* the additional null is not taken into account */
		hv = xp_stx_hash (XP_STX_DATA(stx,object),
			XP_STX_SIZE(stx,object) * xp_sizeof(xp_char_t));
	}
	else if (XP_STX_IS_BYTE_OBJECT(stx,object)) {
		hv = xp_stx_hash (
			XP_STX_DATA(stx,object), XP_STX_SIZE(stx,object));
	}
	else {
		xp_assert (XP_STX_IS_WORD_OBJECT(stx,object));
		hv = xp_stx_hash (XP_STX_DATA(stx,object),
			XP_STX_SIZE(stx,object) * xp_sizeof(xp_word_t));
	}

	return hv;
}

xp_bool_t xp_stx_shallow_compare_object (
	xp_stx_t* stx, xp_word_t a, xp_word_t b)
{
	if (XP_STX_TYPE(stx,a) != XP_STX_TYPE(stx,b)) return xp_false;
	if (XP_STX_SIZE(stx,a) != XP_STX_SIZE(stx,b)) return xp_false;
	if (XP_STX_CLASS(stx,a) != XP_STX_CLASS(stx,b)) return xp_false;
	return xp_memcmp (XP_STX_DATA(stx,a), 
		XP_STX_DATA(stx,b), XP_STX_SIZE(stx,a)) == 0;
}

xp_word_t xp_stx_instantiate (
	xp_stx_t* stx, xp_word_t class, const void* data, 
	const void* variable_data, xp_word_t variable_nfields)
{
	xp_stx_class_t* class_obj;
	xp_word_t spec, nfields, new;
	int indexable;

	xp_assert (class != stx->class_smallinteger);
	class_obj = (xp_stx_class_t*)XP_STX_OBJECT(stx, class);

	/* don't instantiate a metaclass whose instance must be 
	   created in a different way */
	/* TODO: maybe delete the following line */
	xp_assert (class_obj->header.class != stx->class_metaclass);
	xp_assert (XP_STX_IS_SMALLINT(class_obj->spec));

	spec = XP_STX_FROM_SMALLINT(class_obj->spec);
	nfields = (spec >> XP_STX_SPEC_INDEXABLE_BITS);
	indexable = spec & XP_STX_SPEC_INDEXABLE_MASK;

	if (indexable == XP_STX_SPEC_BYTE_INDEXABLE) {
		xp_assert (nfields == 0 && data == XP_NULL);
		new = xp_stx_alloc_byte_object(
			stx, variable_data, variable_nfields);
	}
	else if (indexable == XP_STX_SPEC_CHAR_INDEXABLE) {
		xp_assert (nfields == 0 && data == XP_NULL);
		new = xp_stx_alloc_char_objectx(
			stx, variable_data, variable_nfields);
	}
	else if (indexable == XP_STX_SPEC_WORD_INDEXABLE) {
		new = xp_stx_alloc_word_object (
			stx, data, nfields, variable_data, variable_nfields);
	}
	else {
		xp_assert (indexable == XP_STX_SPEC_NOT_INDEXABLE);
		xp_assert (variable_nfields == 0 && variable_data == XP_NULL);
		new = xp_stx_alloc_word_object (
			stx, data, nfields, XP_NULL, 0);
	}

	XP_STX_CLASS(stx, new) = class;
	return new;
}

xp_word_t xp_stx_class (xp_stx_t* stx, xp_word_t obj)
{
	return XP_STX_IS_SMALLINT(obj)? 
		stx->class_smallinteger: XP_STX_CLASS(stx,obj);
}

xp_word_t xp_stx_classof (xp_stx_t* stx, xp_word_t obj)
{
	return XP_STX_IS_SMALLINT(obj)? 
		stx->class_smallinteger: XP_STX_CLASS(stx,obj);
}

xp_word_t xp_stx_sizeof (xp_stx_t* stx, xp_word_t obj)
{
	return XP_STX_IS_SMALLINT(obj)? 1: XP_STX_SIZE(stx,obj);
}
