/*
 * $Id: name.c 118 2008-03-03 11:21:33Z baconevi $
 */

#include <qse/stx/name.h>
#include <qse/stx/misc.h>

qse_stx_name_t* qse_stx_name_open (
	qse_stx_name_t* name, qse_word_t capacity)
{
	if (capacity == 0) 
		capacity = qse_countof(name->static_buffer) - 1;

	if (name == QSE_NULL) {
		name = (qse_stx_name_t*)
			qse_malloc (qse_sizeof(qse_stx_name_t));
		if (name == QSE_NULL) return QSE_NULL;
		name->__dynamic = qse_true;
	}
	else name->__dynamic = qse_false;
	
	if (capacity < qse_countof(name->static_buffer)) {
		name->buffer = name->static_buffer;
	}
	else {
		name->buffer = (qse_char_t*)
			qse_malloc ((capacity + 1) * qse_sizeof(qse_char_t));
		if (name->buffer == QSE_NULL) {
			if (name->__dynamic) qse_free (name);
			return QSE_NULL;
		}
	}

	name->size      = 0;
	name->capacity  = capacity;
	name->buffer[0] = QSE_T('\0');

	return name;
}

void qse_stx_name_close (qse_stx_name_t* name)
{
	if (name->capacity >= qse_countof(name->static_buffer)) {
		qse_assert (name->buffer != name->static_buffer);
		qse_free (name->buffer);
	}
	if (name->__dynamic) qse_free (name);
}

int qse_stx_name_addc (qse_stx_name_t* name, qse_cint_t c)
{
	if (name->size >= name->capacity) {
		/* double the capacity. */
		qse_size_t new_capacity = name->capacity * 2;

		if (new_capacity >= qse_countof(name->static_buffer)) {
			qse_char_t* space;

			if (name->capacity < qse_countof(name->static_buffer)) {
				space = (qse_char_t*)qse_malloc (
					(new_capacity + 1) * qse_sizeof(qse_char_t));
				if (space == QSE_NULL) return -1;

				/* don't need to copy up to the terminating null */
				qse_memcpy (space, name->buffer, 
					name->capacity * qse_sizeof(qse_char_t));
			}
			else {
				space = (qse_char_t*)qse_realloc (name->buffer, 
					(new_capacity + 1) * qse_sizeof(qse_char_t));
				if (space == QSE_NULL) return -1;
			}

			name->buffer   = space;
		}

		name->capacity = new_capacity;
	}

	name->buffer[name->size++] = c;
	name->buffer[name->size]   = QSE_T('\0');
	return 0;
}

int qse_stx_name_adds (qse_stx_name_t* name, const qse_char_t* s)
{
	while (*s != QSE_T('\0')) {
		if (qse_stx_name_addc(name, *s) == -1) return -1;
		s++;
	}

	return 0;
}

void qse_stx_name_clear (qse_stx_name_t* name)
{
	name->size      = 0;
	name->buffer[0] = QSE_T('\0');
}

qse_char_t* qse_stx_name_yield (qse_stx_name_t* name, qse_word_t capacity)
{
	qse_char_t* old_buffer, * new_buffer;

	if (capacity == 0) 
		capacity = qse_countof(name->static_buffer) - 1;
   
	if (name->capacity < qse_countof(name->static_buffer)) {
		old_buffer = (qse_char_t*)
			qse_malloc((name->capacity + 1) * qse_sizeof(qse_char_t));
		if (old_buffer == QSE_NULL) return QSE_NULL;
		qse_memcpy (old_buffer, name->buffer, 
			(name->capacity + 1) * qse_sizeof(qse_char_t));
	}
	else old_buffer = name->buffer;

	if (capacity < qse_countof(name->static_buffer)) {
		new_buffer = name->static_buffer;
	}
	else {
		new_buffer = (qse_char_t*)
			qse_malloc((capacity + 1) * qse_sizeof(qse_char_t));
		if (new_buffer == QSE_NULL) return QSE_NULL;
	}

	name->buffer    = new_buffer;
	name->size      = 0;
	name->capacity  = capacity;
	name->buffer[0] = QSE_T('\0');

	return old_buffer;
}

int qse_stx_name_compare (qse_stx_name_t* name, const qse_char_t* str)
{
	qse_char_t* p = name->buffer;
	qse_word_t index = 0;

	while (index < name->size) {
		if (*p > *str) return 1;
		if (*p < *str) return -1;
		index++; p++; str++;
	}

	return (*str == QSE_T('\0'))? 0: -1;
}
