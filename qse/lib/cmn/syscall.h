/*
 * $Id: syscall.h 441 2011-04-22 14:28:43Z hyunghwan.chung $
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

#ifndef _QSE_LIB_CMN_SYSCALL_H_
#define _QSE_LIB_CMN_SYSCALL_H_

/* This file defines unix/linux system calls */

#ifdef HAVE_SYS_TYPES_H
#	include <sys/types.h>
#endif
#ifdef HAVE_UNISTD_H
#	include <unistd.h>
#endif
#ifdef HAVE_SYS_WAIT_H
#	include <sys/wait.h>
#endif
#ifdef HAVE_SIGNAL_H
#	include <signal.h>
#endif
#ifdef HAVE_ERRNO_H
#	include <errno.h>
#endif
#ifdef HAVE_TIME_H
#	include <time.h>
#endif
#ifdef HAVE_SYS_TIME_H
#	include <sys/time.h>
#endif
#ifdef HAVE_UTIME_H
#	include <utime.h>
#endif
#ifdef HAVE_SYS_RESOURCE_H
#	include <sys/resource.h>
#endif
#ifdef HAVE_SYS_STAT_H
#	include <sys/stat.h>
#endif

#if defined(QSE_USE_SYSCALL) && defined(HAVE_SYS_SYSCALL_H)
#	include <sys/syscall.h>
#endif

#ifdef SYS_open
#	define QSE_OPEN(path,flags,mode) syscall(SYS_open,path,flags,mode)
#else
#	define QSE_OPEN(path,flags,mode) open(path,flags,mode)
#endif

#ifdef SYS_close
#	define QSE_CLOSE(handle) syscall(SYS_close,handle)
#else
#	define QSE_CLOSE(handle) close(handle)
#endif

#ifdef SYS_read
#	define QSE_READ(handle,buf,size) syscall(SYS_read,handle,buf,size)
#else
#	define QSE_READ(handle,buf,size) read(handle,buf,size)
#endif

#ifdef SYS_write
#	define QSE_WRITE(handle,buf,size) syscall(SYS_write,handle,buf,size)
#else
#	define QSE_WRITE(handle,buf,size) write(handle,buf,size)
#endif

#if !defined(_LP64) && defined(SYS__llseek)
#	define QSE_LLSEEK(handle,hoffset,loffset,out,whence) syscall(SYS__llseek,handle,hoffset,loffset,out,whence)
#elif !defined(_LP64) && defined(HAVE__LLSEEK)
#	define QSE_LLSEEK(handle,hoffset,loffset,out,whence) _llseek(handle,hoffset,loffset,out,whence)
#endif

#if defined(SYS_lseek64)
#	define QSE_LSEEK64(handle,offset,whence) syscall(SYS_lseek64,handle,offset,whence)
#elif defined(HAVE_lseek64)
#	define QSE_LSEEK64(handle,offset,whence) lseek64(handle,offset,whence)
#endif

#if defined(SYS_lseek)
#	define QSE_LSEEK(handle,offset,whence) syscall(SYS_lseek,handle,offset,whence)
#else
#	define QSE_LSEEK(handle,offset,whence) lseek(handle,offset,whence)
#endif

#if defined(SYS_lstat64)
#	define QSE_LSTAT64(path,stbuf) syscall(SYS_lstat64,path,stbuf)
#elif defined(HAVE_lstat64)
#	define QSE_LSTAT64(path,stbuf) lstat64(path,stbuf)
#endif

#if defined(SYS_lstat)
#	define QSE_LSTAT(path,stbuf) syscall(SYS_lstat,path,stbuf)
#else
#	define QSE_LSTAT(path,stbuf) lstat(path,stbuf)
#endif

#if !defined(_LP64) && defined(SYS_ftruncate64)
#	define QSE_FTRUNCATE(handle,size) syscall(SYS_ftruncate64,handle,size)
#elif defined(SYS_ftruncate)
#	define QSE_FTRUNCATE(handle,size) syscall(SYS_ftruncate,handle,size)
#elif !defined(_LP64) && defined(HAVE_FTRUNCATE64)
#	define QSE_FTRUNCATE(handle,size) ftruncate64(handle,size)
#else
#	define QSE_FTRUNCATE(handle,size) ftruncate(handle,size)
#endif

#if defined(SYS_fchmod)
#	define QSE_FCHMOD(handle,mode) syscall(SYS_fchmod,handle,mode)
#else
#	define QSE_FCHMOD(handle,mode) fchmod(handle,mode)
#endif

#if defined(SYS_fsync)
#	define QSE_FSYNC(handle) syscall(SYS_fsync,handle)
#else
#	define QSE_FSYNC(handle) fsync(handle)
#endif

#if defined(SYS_fcntl)
#	define QSE_FCNTL(handle,cmd,arg) syscall(SYS_fcntl,handle,cmd,arg)
#else
#	define QSE_FCNTL(handle,cmd,arg) fcntl(handle,cmd,arg)
#endif

#ifdef SYS_dup2
#	define QSE_DUP2(ofd,nfd) syscall(SYS_dup2,ofd,nfd)
#else
#	define QSE_DUP2(ofd,nfd) dup2(ofd,nfd)
#endif

#ifdef SYS_pipe
#	define QSE_PIPE(pfds) syscall(SYS_pipe,pfds)
#else
#	define QSE_PIPE(pfds) pipe(pfds)
#endif

#ifdef SYS_exit
#	define QSE_EXIT(code) syscall(SYS_exit,code)
#else
#	define QSE_EXIT(code) _exit(code)
#endif

#ifdef SYS_fork
#	define QSE_FORK() syscall(SYS_fork)
#else
#	define QSE_FORK() fork()
#endif

#ifdef SYS_execve
#	define QSE_EXECVE(path,argv,envp) syscall(SYS_execve,path,argv,envp)
#else
#	define QSE_EXECVE(path,argv,envp) execve(path,argv,envp)
#endif

#ifdef SYS_waitpid
#	define QSE_WAITPID(pid,status,options) syscall(SYS_waitpid,pid,status,options)
#else
#	define QSE_WAITPID(pid,status,options) waitpid(pid,status,options)
#endif

#ifdef SYS_kill
#	define QSE_KILL(pid,sig) syscall(SYS_kill,pid,sig)
#else
#	define QSE_KILL(pid,sig) kill(pid,sig)
#endif

#ifdef SYS_getpid
#	define QSE_GETPID() syscall(SYS_getpid)
#else
#	define QSE_GETPID() getpid()
#endif

#ifdef SYS_getuid
#	define QSE_GETUID() syscall(SYS_getuid)
#else
#	define QSE_GETUID() getuid()
#endif

#ifdef SYS_geteuid
#	define QSE_GETEUID() syscall(SYS_geteuid)
#else
#	define QSE_GETEUID() geteuid()
#endif

#ifdef SYS_getgid
#	define QSE_GETGID() syscall(SYS_getgid)
#else
#	define QSE_GETGID() getgid()
#endif

#ifdef SYS_getegid
#	define QSE_GETEGID() syscall(SYS_getegid)
#else
#	define QSE_GETEGID() getegid()
#endif

#ifdef SYS_chroot
#	define QSE_CHROOT(path) syscall(SYS_chroot,path)
#else
#	define QSE_cHROOT(path) chroot(path)
#endif

#ifdef SYS_gettimeofday
#	define QSE_GETTIMEOFDAY(tv,tz) syscall(SYS_gettimeofday,tv,tz)
#else
#	define QSE_GETTIMEOFDAY(tv,tz) gettimeofday(tv,tz)
#endif

#ifdef SYS_settimeofday
#	define QSE_SETTIMEOFDAY(tv,tz) syscall(SYS_settimeofday,tv,tz)
#else
#	define QSE_SETTIMEOFDAY(tv,tz) settimeofday(tv,tz)
#endif

#ifdef SYS_utime
#	define QSE_UTIME(file,t) syscall(SYS_utime,file,t)
#else
#	define QSE_UTIME(file,t) utime(file,t)
#endif

#ifdef SYS_utimes
#	define QSE_UTIMES(file,t) syscall(SYS_utimes,file,t)
#else
#	define QSE_UTIMES(file,t) utimes(file,t)
#endif

#ifdef SYS_getrlimit
#	define QSE_GETRLIMIT(res,lim) syscall(SYS_getrlimit,res,lim)
#else
#	define QSE_GETRLIMIT(res,lim) getrlimit(res,lim)
#endif

#ifdef SYS_setrlimit
#	define QSE_SETRLIMIT(res,lim) syscall(SYS_setrlimit,res,lim)
#else
#	define QSE_SETRLIMIT(res,lim) setrlimit(res,lim)
#endif

#endif
