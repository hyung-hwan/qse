/*
 * $Id: hash.c,v 1.19 2005-06-08 16:00:51 bacon Exp $
 */

#include <xp/stx/hash.h>
#include <xp/stx/object.h>
#include <xp/stx/misc.h>

xp_word_t xp_stx_new_pairlink (
	xp_stx_t* stx, xp_word_t key, xp_word_t value)
{
	xp_word_t x;
	xp_stx_pairlink_t* obj;

	x = xp_stx_alloc_word_object (stx, XP_STX_PAIRLINK_SIZE);	
	obj = (xp_stx_pairlink_t*)XP_STX_WORD_OBJECT(stx, x);
	/*
	XP_STX_CLASS(stx,x) = stx->class_pairlink;
	XP_STX_WORDAT(stx,x,XP_STX_PAIRLINK_LINK) = stx->nil;
	XP_STX_WORDAT(stx,x,XP_STX_PAIRLINK_KEY) = key;
	XP_STX_WORDAT(stx,x,XP_STX_PAIRLINK_VALUE) = value;
	*/
	obj->header.class = stx->class_pairlink;
	obj->link = stx->nil;
	obj->key = key;
	obj->value = value;
	
	return x;
}

/* returns the entire link */
xp_word_t xp_stx_hash_lookup (
	xp_stx_t* stx, xp_word_t table,
	xp_word_t hash, xp_word_t key)
{
	xp_word_t link;
	xp_stx_pairlink_t* obj;

	xp_assert (XP_STX_TYPE(stx,table) == XP_STX_WORD_INDEXED);

	hash = hash % XP_STX_SIZE(stx,table);
	link = XP_STX_WORDAT(stx,table,hash);

	while (link != stx->nil) {
		/*
		if (XP_STX_WORDAT(stx,link,XP_STX_PAIRLINK_KEY) == key) return link;
		link = XP_STX_WORDAT(stx,link,XP_STX_PAIRLINK_LINK);
		*/

		obj = (xp_stx_pairlink_t*)XP_STX_WORD_OBJECT(stx,link);
		if (obj->key == key) return link;
		link = obj->link;
	}

	return stx->nil; /* not found */
}

xp_word_t xp_stx_hash_lookup_symbol (
	xp_stx_t* stx, xp_word_t table, const xp_char_t* name)
{
	xp_word_t link, hash;
	xp_stx_pairlink_t* obj;
	xp_stx_char_object_t* tmp;

	xp_assert (XP_STX_TYPE(stx,table) == XP_STX_WORD_INDEXED);

	hash = xp_stx_strhash(name) % XP_STX_SIZE(stx,table);
	link = XP_STX_WORDAT(stx,table,hash);

	while (link != stx->nil) {
		obj = (xp_stx_pairlink_t*)XP_STX_WORD_OBJECT(stx,link);
		tmp = XP_CHAR_OBJECT(stx,obj->key);
		if (tmp->header.class == stx->class_symbol &&
		    xp_stx_strcmp (tmp->data, name) == 0) return link;
		link = obj->link;
	}

	return stx->nil; /* not found */
}

void xp_stx_hash_insert (
	xp_stx_t* stx, xp_word_t table,
	xp_word_t hash, xp_word_t key, xp_word_t value)
{
	xp_word_t link, next;

	xp_assert (XP_STX_TYPE(stx,table) == XP_STX_WORD_INDEXED);

	hash = hash % XP_STX_SIZE(stx,table);
	link = XP_STX_WORDAT(stx,table,hash);

	if (link == stx->nil) {
		XP_STX_WORDAT(stx,table,hash) =
			xp_stx_new_pairlink (stx, key, value);
	}
	else {
		for (;;) {
			if (XP_STX_WORDAT(stx,link,1) == key) {
				XP_STX_WORDAT(stx,link,XP_STX_PAIRLINK_VALUE) = value;
				break;		
			}

			next = XP_STX_WORDAT(stx,link,XP_STX_PAIRLINK_LINK);
			if (next == stx->nil) {
				XP_STX_WORDAT(stx,link,XP_STX_PAIRLINK_LINK) = 
					xp_stx_new_pairlink (stx, key, value);
				break;
			}

			link = next;
		}
	}
}

void xp_stx_hash_traverse (
	xp_stx_t* stx, xp_word_t table, 
	void (*func) (xp_stx_t*,xp_word_t))
{
	xp_word_t link;
	xp_word_t size = XP_STX_SIZE(stx,table);
	
	while (size-- > 0) {
		link = XP_STX_WORDAT(stx,table,size);

		while (link != stx->nil) {
			func (stx,link);
			link = XP_STX_WORDAT(stx,link,XP_STX_PAIRLINK_LINK);
		}
	}
}

