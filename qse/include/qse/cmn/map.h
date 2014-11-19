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

#ifndef _QSE_CMN_MAP_H_
#define _QSE_CMN_MAP_H_

/* 
 * it is a convenience header file to switch easily between a red-black tree 
 * and a hash table. You must define either QSE_MAP_AS_HTB or QSE_MAP_AS_RBT
 * before including this file.
 */

#if defined(QSE_MAP_AS_HTB)
#	include <qse/cmn/htb.h>
#	define QSE_MAP_STYLE_DEFAULT                   QSE_HTB_STYLE_DEFAULT
#	define QSE_MAP_STYLE_INLINE_COPIERS            QSE_HTB_STYLE_INLINE_COPIERS
#	define QSE_MAP_STYLE_INLINE_KEY_COPIER         QSE_HTB_STYLE_INLINE_KEY_COPIER
#	define QSE_MAP_STYLE_INLINE_VALUE_COPIER       QSE_HTB_STYLE_INLINE_VALUE_COPIER
#	define qse_getmapstyle(kind)                   qse_gethtbstyle(kind)
#	define qse_map_open(mmgr,ext,capa,factor,ks,vs) qse_htb_open(mmgr,ext,capa,factor,ks,vs)
#	define qse_map_close(map)                       qse_htb_close(map)
#	define qse_map_init(map,mmgr,capa,factor,ks,vs) qse_htb_init(map,mmgr,capa,factor,ks,vs)
#	define qse_map_fini(map)                        qse_htb_fini(map)
#	define qse_map_getsize(map)                     qse_htb_getsize(map)
#	define qse_map_getcapa(map)                     qse_htb_getcapa(map)
#	define qse_map_getstyle(map)                   qse_htb_getstyle(map)
#	define qse_map_setstyle(map,cbs)               qse_htb_setstyle(map,cbs)
#	define qse_map_search(map,kptr,klen)            qse_htb_search(map,kptr,klen)
#	define qse_map_upsert(map,kptr,klen,vptr,vlen)  qse_htb_upsert(map,kptr,klen,vptr,vlen)
#	define qse_map_ensert(map,kptr,klen,vptr,vlen)  qse_htb_ensert(map,kptr,klen,vptr,vlen)
#	define qse_map_insert(map,kptr,klen,vptr,vlen)  qse_htb_insert(map,kptr,klen,vptr,vlen)
#	define qse_map_update(map,kptr,klen,vptr,vlen)  qse_htb_update(map,kptr,klen,vptr,vlen)
#	define qse_map_cbsert(map,kptr,klen,cb,ctx)     qse_htb_cbsert(map,kptr,klen,cb,ctx)
#	define qse_map_delete(map,kptr,klen)            qse_htb_delete(map,kptr,klen)
#	define qse_map_clear(map)                       qse_htb_clear(map)
#	define qse_map_walk(map,walker,ctx)             qse_htb_walk(map,walker,ctx)
#	define QSE_MAP_WALK_STOP                        QSE_HTB_WALK_STOP
#	define QSE_MAP_WALK_FORWARD                     QSE_HTB_WALK_FORWARD
#	define qse_map_walk_t                           qse_htb_walk_t
#	define QSE_MAP_KEY                              QSE_HTB_KEY
#	define QSE_MAP_VAL                              QSE_HTB_VAL
#	define qse_map_id_t                             qse_htb_id_t
#	define qse_map_t                                qse_htb_t
#	define qse_map_pair_t                           qse_htb_pair_t
#	define qse_map_style_t                         qse_htb_style_t
#	define qse_map_cbserter_t                       qse_htb_cbserter_t
#	define qse_map_walker_t                         qse_htb_walker_t
#	define QSE_MAP_COPIER_SIMPLE                    QSE_HTB_COPIER_SIMPLE
#	define QSE_MAP_COPIER_INLINE                    QSE_HTB_COPIER_INLINE
#	define QSE_MAP_COPIER_DEFAULT                   QSE_HTB_COPIER_DEFAULT
#	define QSE_MAP_FREEER_DEFAULT                   QSE_HTB_FREEER_DEFAULT
#	define QSE_MAP_COMPER_DEFAULT                   QSE_HTB_COMPER_DEFAULT
#	define QSE_MAP_KEEPER_DEFAULT                   QSE_HTB_KEEPER_DEFAULT
#	define QSE_MAP_SIZER_DEFAULT                    QSE_HTB_SIZER_DEFAULT
#	define QSE_MAP_HASHER_DEFAULT                   QSE_HTB_HASHER_DEFAULT
#	define QSE_MAP_SIZE(map)                        QSE_HTB_SIZE(map)
#	define QSE_MAP_KCOPIER(map)                     QSE_HTB_KCOPIER(map)
#	define QSE_MAP_VCOPIER(map)                     QSE_HTB_VCOPIER(map)
#	define QSE_MAP_KFREEER(map)                     QSE_HTB_KFREEER(map)
#	define QSE_MAP_VFREEER(map)                     QSE_HTB_VFREEER(map)
#	define QSE_MAP_COMPER(map)                      QSE_HTB_COMPER(map) 
#	define QSE_MAP_KEEPER(map)                      QSE_HTB_KEEPER(map)
#	define QSE_MAP_KSCALE(map)                      QSE_HTB_KSCALE(map) 
#	define QSE_MAP_VSCALE(map)                      QSE_HTB_VSCALE(map) 
#	define QSE_MAP_KPTL(p)                          QSE_HTB_KPTL(p)
#	define QSE_MAP_VPTL(p)                          QSE_HTB_VPTL(p)
#	define QSE_MAP_KPTR(p)                          QSE_HTB_KPTR(p)
#	define QSE_MAP_KLEN(p)                          QSE_HTB_KLEN(p)
#	define QSE_MAP_VPTR(p)                          QSE_HTB_VPTR(p)
#	define QSE_MAP_VLEN(p)                          QSE_HTB_VLEN(p)
#elif defined(QSE_MAP_AS_RBT)
#	include <qse/cmn/rbt.h>
#	define QSE_MAP_STYLE_DEFAULT                   QSE_RBT_STYLE_DEFAULT
#	define QSE_MAP_STYLE_INLINE_COPIERS            QSE_RBT_STYLE_INLINE_COPIERS
#	define QSE_MAP_STYLE_INLINE_KEY_COPIER         QSE_RBT_STYLE_INLINE_KEY_COPIER
#	define QSE_MAP_STYLE_INLINE_VALUE_COPIER       QSE_RBT_STYLE_INLINE_VALUE_COPIER
#	define qse_getmapstyle(kind)                   qse_getrbtstyle(kind)
#	define qse_map_open(mmgr,ext,capa,factor,ks,vs) qse_rbt_open(mmgr,ext,ks,vs)
#	define qse_map_close(map)                       qse_rbt_close(map)
#	define qse_map_init(map,mmgr,capa,factor,ks,vs) qse_rbt_init(map,mmgr,ks,vs)
#	define qse_map_fini(map)                        qse_rbt_fini(map)
#	define qse_map_getsize(map)                     qse_rbt_getsize(map)
#	define qse_map_getcapa(map)                     qse_rbt_getsize(map)
#	define qse_map_getstyle(map)                   qse_rbt_getstyle(map)
#	define qse_map_setstyle(map,cbs)               qse_rbt_setstyle(map,cbs)
#	define qse_map_search(map,kptr,klen)            qse_rbt_search(map,kptr,klen)
#	define qse_map_upsert(map,kptr,klen,vptr,vlen)  qse_rbt_upsert(map,kptr,klen,vptr,vlen)
#	define qse_map_ensert(map,kptr,klen,vptr,vlen)  qse_rbt_ensert(map,kptr,klen,vptr,vlen)
#	define qse_map_insert(map,kptr,klen,vptr,vlen)  qse_rbt_insert(map,kptr,klen,vptr,vlen)
#	define qse_map_update(map,kptr,klen,vptr,vlen)  qse_rbt_update(map,kptr,klen,vptr,vlen)
#	define qse_map_cbsert(map,kptr,klen,cb,ctx)     qse_rbt_cbsert(map,kptr,klen,cb,ctx)
#	define qse_map_delete(map,kptr,klen)            qse_rbt_delete(map,kptr,klen)
#	define qse_map_clear(map)                       qse_rbt_clear(map)
#	define qse_map_walk(map,walker,ctx)             qse_rbt_walk(map,walker,ctx)
#	define QSE_MAP_WALK_STOP                        QSE_RBT_WALK_STOP
#	define QSE_MAP_WALK_FORWARD                     QSE_RBT_WALK_FORWARD
#	define qse_map_walk_t                           qse_rbt_walk_t
#	define QSE_MAP_KEY                              QSE_RBT_KEY
#	define QSE_MAP_VAL                              QSE_RBT_VAL
#	define qse_map_id_t                             qse_rbt_id_t
#	define qse_map_t                                qse_rbt_t
#	define qse_map_pair_t                           qse_rbt_pair_t
#	define qse_map_style_t                         qse_rbt_style_t
#	define qse_map_cbserter_t                       qse_rbt_cbserter_t
#	define qse_map_walker_t                         qse_rbt_walker_t
#	define QSE_MAP_COPIER_SIMPLE                    QSE_RBT_COPIER_SIMPLE
#	define QSE_MAP_COPIER_INLINE                    QSE_RBT_COPIER_INLINE
#	define QSE_MAP_COPIER_DEFAULT                   QSE_RBT_COPIER_DEFAULT
#	define QSE_MAP_FREEER_DEFAULT                   QSE_RBT_FREEER_DEFAULT
#	define QSE_MAP_COMPER_DEFAULT                   QSE_RBT_COMPER_DEFAULT
#	define QSE_MAP_KEEPER_DEFAULT                   QSE_RBT_KEEPER_DEFAULT
/*#	define QSE_MAP_SIZER_DEFAULT 
#	define QSE_MAP_HASHER_DEFAULT*/
#	define QSE_MAP_SIZE(map)                        QSE_RBT_SIZE(map)
#	define QSE_MAP_KCOPIER(map)                     QSE_RBT_KCOPIER(map)
#	define QSE_MAP_VCOPIER(map)                     QSE_RBT_VCOPIER(map)
#	define QSE_MAP_KFREEER(map)                     QSE_RBT_KFREEER(map)
#	define QSE_MAP_VFREEER(map)                     QSE_RBT_VFREEER(map)
#	define QSE_MAP_COMPER(map)                      QSE_RBT_COMPER(map) 
#	define QSE_MAP_KEEPER(map)                      QSE_RBT_KEEPER(map)
#	define QSE_MAP_KSCALE(map)                      QSE_RBT_KSCALE(map) 
#	define QSE_MAP_VSCALE(map)                      QSE_RBT_VSCALE(map) 
#	define QSE_MAP_KPTL(p)                          QSE_RBT_KPTL(p)
#	define QSE_MAP_VPTL(p)                          QSE_RBT_VPTL(p)
#	define QSE_MAP_KPTR(p)                          QSE_RBT_KPTR(p)
#	define QSE_MAP_KLEN(p)                          QSE_RBT_KLEN(p)
#	define QSE_MAP_VPTR(p)                          QSE_RBT_VPTR(p)
#	define QSE_MAP_VLEN(p)                          QSE_RBT_VLEN(p)
#else
#	error define QSE_MAP_AS_HTB or QSE_MAP_AS_RBT before including this file
#endif

#endif
