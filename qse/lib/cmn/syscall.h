#ifndef _QSE_LIB_CMN_SYSCALL_H_
#define _QSE_LIB_CMN_SYSCALL_H_

/* This file defines unix/linux system calls */

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif

#if defined(QSE_USE_SYSCALL) && defined(HAVE_SYS_SYSCALL_H)
#include <sys/syscall.h>
#endif

#ifdef SYS_open
	#define QSE_OPEN(path,flags,mode) syscall(SYS_open,path,flags,mode)
#else
	#define QSE_OPEN(path,flags,mode) open(path,flags,mode)
#endif

#ifdef SYS_close
	#define QSE_CLOSE(handle) syscall(SYS_close,handle)
#else
	#define QSE_CLOSE(handle) close(handle)
#endif

#ifdef SYS_read
	#define QSE_READ(handle,buf,size) syscall(SYS_read,handle,buf,size)
#else
	#define QSE_READ(handle,buf,size) read(handle,buf,size)
#endif

#ifdef SYS_write
	#define QSE_WRITE(handle,buf,size) syscall(SYS_write,handle,buf,size)
#else
	#define QSE_WRITE(handle,buf,size) write(handle,buf,size)
#endif

#if !defined(_LP64) && defined(SYS__llseek)
	#define QSE_LLSEEK(handle,hoffset,loffset,out,whence)  \
		syscall(SYS__llseek,handle,hoffset,loffset,out,whence)
#elif !defined(_LP64) && defined(HAVE__LLSEEK)
	#define QSE_LLSEEK(handle,hoffset,loffset,out,whence)  \
		_llseek(handle,hoffset,loffset,out,whence)
#endif

#if defined(SYS_lseek65)
	#define QSE_LSEEK64(handle,offset,whence) \
		syscall(SYS_lseek64,handle,offset,whence)
#elif defined(HAVE_lseek64)
	#define QSE_LSEEK64(handle,offset,whence) \
		lseek64(handle,offset,whence)
#endif

#if defined(SYS_lseek)
	#define QSE_LSEEK(handle,offset,whence) \
		syscall(SYS_lseek,handle,offset,whence)
#else
	#define QSE_LSEEK(handle,offset,whence) \
		lseek(handle,offset,whence)
#endif

#if !defined(_LP64) && defined(SYS_ftruncate64)
	#define QSE_FTRUNCATE(handle,size) syscall(SYS_ftruncate64,handle,size)
#elif defined(SYS_ftruncate)
	#define QSE_FTRUNCATE(handle,size) syscall(SYS_ftruncate,handle,size)
#elif !defined(_LP64) && defined(HAVE_FTRUNCATE64)
	#define QSE_FTRUNCATE(handle,size) ftruncate64(handle,size)
#else
	#define QSE_FTRUNCATE(handle,size) ftruncate(handle,size)
#endif

#if defined(SYS_fchmod)
	#define QSE_FCHMOD(handle,mode) syscall(SYS_fchmod,handle,mode)
#else
	#define QSE_FCHMOD(handle,mode) fchmod(handle,mode)
#endif

#ifdef SYS_dup2
	#define QSE_DUP2(ofd,nfd) syscall(SYS_dup2,ofd,nfd)
#else
	#define QSE_DUP2(ofd,nfd) dup2(ofd,nfd)
#endif

#ifdef SYS_pipe
	#define QSE_PIPE(pfds) syscall(SYS_pipe,pfds)
#else
	#define QSE_PIPE(pfds) pipe(pfds)
#endif

#ifdef SYS_exit
	#define QSE_EXIT(code) syscall(SYS_exit,code)
#else
	#define QSE_EXIT(code) _exit(code)
#endif

#ifdef SYS_fork
	#define QSE_FORK() syscall(SYS_fork)
#else
	#define QSE_FORK() fork()
#endif

#ifdef SYS_execve
	#define QSE_EXECVE(path,argv,envp) syscall(SYS_execve,path,argv,envp)
#else
	#define QSE_EXECVE(path,argv,envp) execve(path,argv,envp)
#endif

#ifdef SYS_waitpid
	#define QSE_WAITPID(pid,status,options) syscall(SYS_waitpid,pid,status,options)
#else
	#define QSE_WAITPID(pid,status,options) waitpid(pid,status,options)
#endif

#ifdef SYS_getpid
	#define QSE_GETPID() syscall(SYS_getpid)
#else
	#define QSE_GETPID() getpid()
#endif

#ifdef SYS_getuid
	#define QSE_GETUID() syscall(SYS_getuid)
#else
	#define QSE_GETUID() getuid()
#endif

#ifdef SYS_geteuid
	#define QSE_GETEUID() syscall(SYS_geteuid)
#else
	#define QSE_GETEUID() geteuid()
#endif

#ifdef SYS_getgid
	#define QSE_GETGID() syscall(SYS_getgid)
#else
	#define QSE_GETGID() getgid()
#endif

#ifdef SYS_getegid
	#define QSE_GETEGID() syscall(SYS_getegid)
#else
	#define QSE_GETEGID() getegid()
#endif

#endif
