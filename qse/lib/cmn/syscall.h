#ifndef _QSE_LIB_CMN_SYSCALL_H_
#define _QSE_LIB_CMN_SYSCALL_H_

#if defined(HAVE_UNISTD_H)
#include <unistd.h>
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

#if !defined(_LP64) && defined(SYS_ftruncate64)
	#define QSE_FTRUNCATE(handle,size) syscall(SYS_ftruncate64,handle,size)
#elif defined(SYS_ftruncate)
	#define QSE_FTRUNCATE(handle,size) syscall(SYS_ftruncate,handle,size)
#elif !defined(_LP64) && defined(HAVE_FTRUNCATE64)
	#define QSE_FTRUNCATE(handle,size) ftruncate64(handle,size)
#else
	#define QSE_FTRUNCATE(handle,size) ftruncate(handle,size)
#endif

#ifdef SYS_fork
	#define QSE_FORK() syscall(SYS_fork)
#else
	#define QSE_FORK() fork()
#endif

#endif
