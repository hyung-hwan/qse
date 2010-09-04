/*
 * $Id$
 *
    Copyright 2006-2009 Chung, Hyung-Hwan.
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

#ifndef _QSE_CMN_MAP_H_
#define _QSE_CMN_MAP_H_

/* 
 * it is a convenience header file to switch easily between a red-black tree 
 * and a hash table. You must define either QSE_MAP_AS_HTB or QSE_MAP_AS_RBT
 * before including this file.
 */

#if defined(QSE_MAP_AS_HTB)
#	include <qse/cmn/htb.h>
#	define qse_map_open(mmgr,ext,capa,factor)       qse_htb_open(mmgr,ext,capa,factor)
#	define qse_map_close(map)                       qse_htb_close(map)
#	define qse_map_init(map,mmgr,capa,factor)       qse_htb_init(map,mmgr,capa,factor)
#	define qse_map_fini(map)                        qse_htb_fini(map)
#	define qse_map_getsize(map)                     qse_htb_getsize(map)
#	define qse_map_getcapa(map)                     qse_htb_getcapa(map)
#	define qse_map_getscale(map,id)                 qse_htb_getscale(map,id)
#	define qse_map_setscale(map,id,scale)           qse_htb_setscale(map,id,scale)
#	define qse_map_getcopier(map,id)                qse_htb_getcopier(map,id)
#	define qse_map_setcopier(map,id,cb)             qse_htb_setcopier(map,id,cb)
#	define qse_map_getfreeer(map,id)                qse_htb_getfreeer(map,id)
#	define qse_map_setfreeer(map,id,cb)             qse_htb_setfreeer(map,id,cb)
#	define qse_map_getcomper(map,id)                qse_htb_getcomper(map,id)
#	define qse_map_setcomper(map,id,cb)             qse_htb_setcomper(map,id,cb)
#	define qse_map_getkeeper(map,id)                qse_htb_getkeeper(map,id)
#	define qse_map_setkeeper(map,id,cb)             qse_htb_setkeeper(map,id,cb)
#	define qse_map_gethasher(map)                   qse_htb_gethasher(map)
#	define qse_map_sethasher(map,cb)                qse_htb_sethasher(map,cb) 
#	define qse_map_getsizer(map)                    qse_htb_getsizer(map)
#	define qse_map_setsizer(map,cb)                 qse_htb_setsizer(map,cb) 
#	define qse_map_search(map,kptr,klen)            qse_htb_search(map,kptr,klen)
#	define qse_map_upsert(map,kptr,klen,vptr,vlen)  qse_htb_upsert(map,kptr,klen,vptr,vlen)
#	define qse_map_ensert(map,kptr,klen,vptr,vlen)  qse_htb_ensert(map,kptr,klen,vptr,vlen)
#	define qse_map_insert(map,kptr,klen,vptr,vlen)  qse_htb_insert(map,kptr,klen,vptr,vlen)
#	define qse_map_update(map,kptr,klen,vptr,vlen)  qse_htb_update(map,kptr,klen,vptr,vlen)
#	define qse_map_delete(map,kptr,klen)            qse_htb_delete(map,kptr,klen)
#	define qse_map_clear(map)                       qse_htb_clear(map)
#	define qse_map_walk(map,walker,ctx)             qse_htb_walk(map,walker,ctx)
#	define QSE_MAP_WALK_STOP                        QSE_HTB_WALK_STOP
#	define QSE_MAP_WALK_FORWARD                    QSE_HTB_WALK_FORWARD
#	define qse_map_walk_t                           qse_htb_walk_t
#	define QSE_MAP_KEY                              QSE_HTB_KEY
#	define QSE_MAP_VAL                              QSE_HTB_VAL
#	define qse_map_id_t                             qse_htb_id_t
#	define qse_map_t                                qse_htb_t
#	define qse_map_pair_t                           qse_htb_pair_t
#	define QSE_MAP_COPIER_SIMPLE                    QSE_HTB_COPIER_SIMPLE
#	define QSE_MAP_COPIER_INLINE                    QSE_HTB_COPIER_INLINE
#	define QSE_MAP_SIZE(map)                        QSE_HTB_SIZE(map)
#	define QSE_MAP_KCOPIER(map)                     QSE_HTB_KCOPIER(map)
#	define QSE_MAP_VCOPIER(map)                     QSE_HTB_VCOPIER(map)
#	define QSE_MAP_KFREEER(map)                     QSE_HTB_KFREEER(map)
#	define QSE_MAP_VFREEER(map)                     QSE_HTB_VFREEER(map)
#	define QSE_MAP_COMPER(map)                      QSE_HTB_COMPER(map) 
#	define QSE_MAP_KEEPER(map)                      QSE_HTB_KEEPER(map)
#	define QSE_MAP_KSCALE(map)                      QSE_HTB_KSCALE(map) 
#	define QSE_MAP_VSCALE(map)                      QSE_HTB_VSCALE(map) 
#	define QSE_MAP_KPTR(p)                          QSE_HTB_KPTR(p)
#	define QSE_MAP_KLEN(p)                          QSE_HTB_KLEN(p)
#	define QSE_MAP_VPTR(p)                          QSE_HTB_VPTR(p)
#	define QSE_MAP_VLEN(p)                          QSE_HTB_VLEN(p)
#elif defined(QSE_MAP_AS_RBT)
#	include <qse/cmn/rbt.h>
#	define qse_map_open(mmgr,ext,capa,factor)       qse_rbt_open(mmgr,ext)
#	define qse_map_close(map)                       qse_rbt_close(map)
#	define qse_map_init(map,mmgr,capa,factor)       qse_rbt_init(map,mmgr)
#	define qse_map_fini(map)                        qse_rbt_fini(map)
#	define qse_map_getsize(map)                     qse_rbt_getsize(map)
#	define qse_map_getcapa(map)                     qse_rbt_getcapa(map)
#	define qse_map_getscale(map,id)                 qse_rbt_getscale(map,id)
#	define qse_map_setscale(map,id,scale)           qse_rbt_setscale(map,id,scale)
#	define qse_map_getcopier(map,id)                qse_rbt_getcopier(map,id)
#	define qse_map_setcopier(map,id,cb)             qse_rbt_setcopier(map,id,cb)
#	define qse_map_getfreeer(map,id)                qse_rbt_getfreeer(map,id)
#	define qse_map_setfreeer(map,id,cb)             qse_rbt_setfreeer(map,id,cb)
#	define qse_map_getcomper(map,id)                qse_rbt_getcomper(map,id)
#	define qse_map_setcomper(map,id,cb)             qse_rbt_setcomper(map,id,cb)
#	define qse_map_getkeeper(map,id)                qse_rbt_getkeeper(map,id)
#	define qse_map_setkeeper(map,id,cb)             qse_rbt_setkeeper(map,id,cb)
#	define qse_map_gethasher(map,id)          
#	define qse_map_sethasher(map,id,cb)       
#	define qse_map_getsizer(map,id)          
#	define qse_map_setsizer(map,id,cb)       
#	define qse_map_search(map,kptr,klen)            qse_rbt_search(map,kptr,klen)
#	define qse_map_upsert(map,kptr,klen,vptr,vlen)  qse_rbt_upsert(map,kptr,klen,vptr,vlen)
#	define qse_map_ensert(map,kptr,klen,vptr,vlen)  qse_rbt_ensert(map,kptr,klen,vptr,vlen)
#	define qse_map_insert(map,kptr,klen,vptr,vlen)  qse_rbt_insert(map,kptr,klen,vptr,vlen)
#	define qse_map_update(map,kptr,klen,vptr,vlen)  qse_rbt_update(map,kptr,klen,vptr,vlen)
#	define qse_map_delete(map,kptr,klen)            qse_rbt_delete(map,kptr,klen)
#	define qse_map_clear(map)                       qse_rbt_clear(map)
#	define qse_map_walk(map,walker,ctx)             qse_rbt_walk(map,walker,ctx)
#	define QSE_MAP_WALK_STOP                        QSE_RBT_WALK_STOP
#	define QSE_MAP_WALK_FORWARD                    QSE_RBT_WALK_FORWARD
#	define qse_map_walk_t                           qse_rbt_walk_t
#	define QSE_MAP_KEY                              QSE_RBT_KEY
#	define QSE_MAP_VAL                              QSE_RBT_VAL
#	define qse_map_id_t                             qse_rbt_id_t
#	define qse_map_t                                qse_rbt_t
#	define qse_map_pair_t                           qse_rbt_pair_t
#	define QSE_MAP_COPIER_SIMPLE                    QSE_RBT_COPIER_SIMPLE
#	define QSE_MAP_COPIER_INLINE                    QSE_RBT_COPIER_INLINE
#	define QSE_MAP_SIZE(map)                        QSE_RBT_SIZE(map)
#	define QSE_MAP_KCOPIER(map)                     QSE_RBT_KCOPIER(map)
#	define QSE_MAP_VCOPIER(map)                     QSE_RBT_VCOPIER(map)
#	define QSE_MAP_KFREEER(map)                     QSE_RBT_KFREEER(map)
#	define QSE_MAP_VFREEER(map)                     QSE_RBT_VFREEER(map)
#	define QSE_MAP_COMPER(map)                      QSE_RBT_COMPER(map) 
#	define QSE_MAP_KEEPER(map)                      QSE_RBT_KEEPER(map)
#	define QSE_MAP_KSCALE(map)                      QSE_RBT_KSCALE(map) 
#	define QSE_MAP_VSCALE(map)                      QSE_RBT_VSCALE(map) 
#	define QSE_MAP_KPTR(p)                          QSE_RBT_KPTR(p)
#	define QSE_MAP_KLEN(p)                          QSE_RBT_KLEN(p)
#	define QSE_MAP_VPTR(p)                          QSE_RBT_VPTR(p)
#	define QSE_MAP_VLEN(p)                          QSE_RBT_VLEN(p)
#else
#	error define QSE_MAP_AS_HTB or QSE_MAP_AS_RBT before including this file
#endif

#endif
