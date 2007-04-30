/*
 * $Id: object.c,v 1.1.1.1 2007/03/28 14:05:28 bacon Exp $
 */

#include <ase/stx/object.h>
#include <ase/stx/memory.h>
#include <ase/stx/symbol.h>
#include <ase/stx/class.h>
#include <ase/stx/misc.h>

/* n: number of instance variables */
ase_word_t ase_stx_alloc_word_object (
	ase_stx_t* stx, const ase_word_t* data, ase_word_t nfields, 
	const ase_word_t* variable_data, ase_word_t variable_nfields)
{
	ase_word_t idx, n;
	ase_stx_word_object_t* obj;

	ase_assert (stx->nil == ASE_STX_NIL);

	/* bytes to allocated =
	 *     (number of instance variables + 
	 *      number of variable instance variables) * word_size 
	 */
	n = nfields + variable_nfields;
	idx = ase_stx_memory_alloc (&stx->memory,
		n * ase_sizeof(ase_word_t) + ase_sizeof(ase_stx_object_t));
	if (idx >= stx->memory.capacity) return idx; /* failed TODO: return a difference value OINDEX_INVALID */

	idx = ASE_STX_TO_OINDEX(idx);
	obj = ASE_STX_WORD_OBJECT(stx,idx);
	obj->header.class = stx->nil;
	obj->header.access = (n << 2) | ASE_STX_WORD_INDEXED;

	if (variable_data == ASE_NULL) {
		while (n > nfields) obj->data[--n] = stx->nil;
	}
	else {
		while (n > nfields) {
			n--; obj->data[n] = variable_data[n - nfields];
		}
	}

	if (data == ASE_NULL) { 
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
ase_word_t ase_stx_alloc_byte_object (
	ase_stx_t* stx, const ase_byte_t* data, ase_word_t n)
{
	ase_word_t idx;
	ase_stx_byte_object_t* obj;

	ase_assert (stx->nil == ASE_STX_NIL);

	idx = ase_stx_memory_alloc (
		&stx->memory, n + ase_sizeof(ase_stx_object_t));
	if (idx >= stx->memory.capacity) return idx; /* failed */

	idx = ASE_STX_TO_OINDEX(idx);
	obj = ASE_STX_BYTE_OBJECT(stx,idx);
	obj->header.class = stx->nil;
	obj->header.access = (n << 2) | ASE_STX_BYTE_INDEXED;

	if (data == ASE_NULL) {
		while (n-- > 0) obj->data[n] = 0;
	}
	else {
		while (n-- > 0) obj->data[n] = data[n];
	}

	return idx;
}

ase_word_t ase_stx_alloc_char_object (
	ase_stx_t* stx, const ase_char_t* str)
{
	return (str == ASE_NULL)?
		ase_stx_alloc_char_objectx (stx, ASE_NULL, 0):
		ase_stx_alloc_char_objectx (stx, str, ase_strlen(str));
}

/* n: number of characters */
ase_word_t ase_stx_alloc_char_objectx (
	ase_stx_t* stx, const ase_char_t* str, ase_word_t n)
{
	ase_word_t idx;
	ase_stx_char_object_t* obj;

	ase_assert (stx->nil == ASE_STX_NIL);

	idx = ase_stx_memory_alloc (&stx->memory, 
		(n + 1) * ase_sizeof(ase_char_t) + ase_sizeof(ase_stx_object_t));
	if (idx >= stx->memory.capacity) return idx; /* failed */

	idx = ASE_STX_TO_OINDEX(idx);
	obj = ASE_STX_CHAR_OBJECT(stx,idx);
	obj->header.class = stx->nil;
	obj->header.access = (n << 2) | ASE_STX_CHAR_INDEXED;
	obj->data[n] = ASE_T('\0');

	if (str == ASE_NULL) {
		while (n-- > 0) obj->data[n] = ASE_T('\0');
	}
	else {
		while (n-- > 0) obj->data[n] = str[n];
	}

	return idx;
}

ase_word_t ase_stx_allocn_char_object (ase_stx_t* stx, ...)
{
	ase_word_t idx, n = 0;
	const ase_char_t* p;
	ase_va_list ap;
	ase_stx_char_object_t* obj;

	ase_assert (stx->nil == ASE_STX_NIL);

	ase_va_start (ap, stx);
	while ((p = ase_va_arg(ap, const ase_char_t*)) != ASE_NULL) {
		n += ase_strlen(p);
	}
	ase_va_end (ap);

	idx = ase_stx_memory_alloc (&stx->memory, 
		(n + 1) * ase_sizeof(ase_char_t) + ase_sizeof(ase_stx_object_t));
	if (idx >= stx->memory.capacity) return idx; /* failed */

	idx = ASE_STX_TO_OINDEX(idx);
	obj = ASE_STX_CHAR_OBJECT(stx,idx);
	obj->header.class = stx->nil;
	obj->header.access = (n << 2) | ASE_STX_CHAR_INDEXED;
	obj->data[n] = ASE_T('\0');

	ase_va_start (ap, stx);
	n = 0;
	while ((p = ase_va_arg(ap, const ase_char_t*)) != ASE_NULL) {
		while (*p != ASE_T('\0')) {
			/*ASE_STX_CHAR_AT(stx,idx,n++) = *p++;*/
			obj->data[n++] = *p++;
		}
	}
	ase_va_end (ap);

	return idx;
}

ase_word_t ase_stx_hash_object (ase_stx_t* stx, ase_word_t object)
{
	ase_word_t hv;

	if (ASE_STX_IS_SMALLINT(object)) {
		ase_word_t tmp = ASE_STX_FROM_SMALLINT(object);
		hv = ase_stx_hash(&tmp, ase_sizeof(tmp));
	}
	else if (ASE_STX_IS_CHAR_OBJECT(stx,object)) {
		/* the additional null is not taken into account */
		hv = ase_stx_hash (ASE_STX_DATA(stx,object),
			ASE_STX_SIZE(stx,object) * ase_sizeof(ase_char_t));
	}
	else if (ASE_STX_IS_BYTE_OBJECT(stx,object)) {
		hv = ase_stx_hash (
			ASE_STX_DATA(stx,object), ASE_STX_SIZE(stx,object));
	}
	else {
		ase_assert (ASE_STX_IS_WORD_OBJECT(stx,object));
		hv = ase_stx_hash (ASE_STX_DATA(stx,object),
			ASE_STX_SIZE(stx,object) * ase_sizeof(ase_word_t));
	}

	return hv;
}

ase_word_t ase_stx_instantiate (
	ase_stx_t* stx, ase_word_t class, const void* data, 
	const void* variable_data, ase_word_t variable_nfields)
{
	ase_stx_class_t* class_obj;
	ase_word_t spec, nfields, new;
	int indexable;

	ase_assert (class != stx->class_smallinteger);
	class_obj = (ase_stx_class_t*)ASE_STX_OBJECT(stx, class);

	/* don't instantiate a metaclass whose instance must be 
	   created in a different way */
	/* TODO: maybe delete the following line */
	ase_assert (class_obj->header.class != stx->class_metaclass);
	ase_assert (ASE_STX_IS_SMALLINT(class_obj->spec));

	spec = ASE_STX_FROM_SMALLINT(class_obj->spec);
	nfields = (spec >> ASE_STX_SPEC_INDEXABLE_BITS);
	indexable = spec & ASE_STX_SPEC_INDEXABLE_MASK;

	if (indexable == ASE_STX_SPEC_BYTE_INDEXABLE) {
		ase_assert (nfields == 0 && data == ASE_NULL);
		new = ase_stx_alloc_byte_object(
			stx, variable_data, variable_nfields);
	}
	else if (indexable == ASE_STX_SPEC_CHAR_INDEXABLE) {
		ase_assert (nfields == 0 && data == ASE_NULL);
		new = ase_stx_alloc_char_objectx(
			stx, variable_data, variable_nfields);
	}
	else if (indexable == ASE_STX_SPEC_WORD_INDEXABLE) {
		new = ase_stx_alloc_word_object (
			stx, data, nfields, variable_data, variable_nfields);
	}
	else {
		ase_assert (indexable == ASE_STX_SPEC_NOT_INDEXABLE);
		ase_assert (variable_nfields == 0 && variable_data == ASE_NULL);
		new = ase_stx_alloc_word_object (
			stx, data, nfields, ASE_NULL, 0);
	}

	ASE_STX_CLASS(stx, new) = class;
	return new;
}

ase_word_t ase_stx_class (ase_stx_t* stx, ase_word_t obj)
{
	return ASE_STX_IS_SMALLINT(obj)? 
		stx->class_smallinteger: ASE_STX_CLASS(stx,obj);
}

ase_word_t ase_stx_classof (ase_stx_t* stx, ase_word_t obj)
{
	return ASE_STX_IS_SMALLINT(obj)? 
		stx->class_smallinteger: ASE_STX_CLASS(stx,obj);
}

ase_word_t ase_stx_sizeof (ase_stx_t* stx, ase_word_t obj)
{
	return ASE_STX_IS_SMALLINT(obj)? 1: ASE_STX_SIZE(stx,obj);
}
