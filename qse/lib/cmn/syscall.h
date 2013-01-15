/*
 * $Id$
 *
    Copyright 2006-2012 Chung, Hyung-Hwan.
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

#if defined(_WIN32) || defined(__OS2__) || defined(__DOS__)
#	error Do not include this file
#endif

/* This file defines unix/linux system calls */

#if defined(HAVE_SYS_TYPES_H)
#	include <sys/types.h>
#endif
#if defined(HAVE_UNISTD_H)
#	include <unistd.h>
#endif
#if defined(HAVE_SYS_WAIT_H)
#	include <sys/wait.h>
#endif
#if defined(HAVE_SIGNAL_H)
#	include <signal.h>
#endif
#if defined(HAVE_ERRNO_H)
#	include <errno.h>
#endif
#if defined(HAVE_FCNTL_H)
#	include <fcntl.h>
#endif
#if defined(HAVE_TIME_H)
#	include <time.h>
#endif
#if defined(HAVE_SYS_TIME_H)
#	include <sys/time.h>
#endif
#if defined(HAVE_UTIME_H)
#	include <utime.h>
#endif
#if defined(HAVE_SYS_RESOURCE_H)
#	include <sys/resource.h>
#endif
#if defined(HAVE_SYS_STAT_H)
#	include <sys/stat.h>
#endif
#if defined(HAVE_DIRENT_H)
#	include <dirent.h>
#endif

#if defined(QSE_USE_SYSCALL) && defined(HAVE_SYS_SYSCALL_H)
#	include <sys/syscall.h>
#endif

#if defined(SYS_open) && defined(QSE_USE_SYSCALL)
#	define QSE_OPEN(path,flags,mode) syscall(SYS_open,path,flags,mode)
#else
#	define QSE_OPEN(path,flags,mode) open(path,flags,mode)
#endif

#if defined(SYS_close) && defined(QSE_USE_SYSCALL)
#	define QSE_CLOSE(handle) syscall(SYS_close,handle)
#else
#	define QSE_CLOSE(handle) close(handle)
#endif

#if defined(SYS_read) && defined(QSE_USE_SYSCALL)
#	define QSE_READ(handle,buf,size) syscall(SYS_read,handle,buf,size)
#else
#	define QSE_READ(handle,buf,size) read(handle,buf,size)
#endif

#if defined(SYS_write) && defined(QSE_USE_SYSCALL)
#	define QSE_WRITE(handle,buf,size) syscall(SYS_write,handle,buf,size)
#else
#	define QSE_WRITE(handle,buf,size) write(handle,buf,size)
#endif

#if !defined(_LP64) && (QSE_SIZEOF_VOID_P<8) && defined(SYS__llseek) && defined(QSE_USE_SYSCALL)
#	define QSE_LLSEEK(handle,hoffset,loffset,out,whence) syscall(SYS__llseek,handle,hoffset,loffset,out,whence)
#elif !defined(_LP64) && (QSE_SIZEOF_VOID_P<8) && defined(HAVE__LLSEEK) && defined(QSE_USE_SYSCALL)
#	define QSE_LLSEEK(handle,hoffset,loffset,out,whence) _llseek(handle,hoffset,loffset,out,whence)
#endif

#if !defined(_LP64) && (QSE_SIZEOF_VOID_P<8) && defined(SYS_lseek64) && defined(QSE_USE_SYSCALL)
#	define QSE_LSEEK(handle,offset,whence) syscall(SYS_lseek64,handle,offset,whence)
#elif defined(SYS_lseek) && defined(QSE_USE_SYSCALL)
#	define QSE_LSEEK(handle,offset,whence) syscall(SYS_lseek,handle,offset,whence)
#elif !defined(_LP64) && (QSE_SIZEOF_VOID_P<8) && defined(HAVE_LSEEK64)
#	define QSE_LSEEK(handle,offset,whence) lseek64(handle,offset,whence)
#else
#	define QSE_LSEEK(handle,offset,whence) lseek(handle,offset,whence)
#endif

#if !defined(_LP64) && (QSE_SIZEOF_VOID_P<8) && defined(SYS_fstat64) && defined(QSE_USE_SYSCALL)
#	define QSE_FSTAT(handle,stbuf) syscall(SYS_fstat64,handle,stbuf)
	typedef struct stat64 qse_fstat_t;
#elif defined(SYS_fstat) && defined(QSE_USE_SYSCALL)
#	define QSE_FSTAT(handle,stbuf) syscall(SYS_fstat,handle,stbuf)
	typedef struct stat qse_fstat_t;
#elif !defined(_LP64) && (QSE_SIZEOF_VOID_P<8) && defined(HAVE_FSTAT64)
#	define QSE_FSTAT(handle,stbuf) fstat64(handle,stbuf)
	typedef struct stat64 qse_fstat_t;
#else
#	define QSE_FSTAT(handle,stbuf) fstat(handle,stbuf)
	typedef struct stat qse_fstat_t;
#endif

#if !defined(_LP64) && (QSE_SIZEOF_VOID_P<8) && defined(SYS_ftruncate64) && defined(QSE_USE_SYSCALL)
#	define QSE_FTRUNCATE(handle,size) syscall(SYS_ftruncate64,handle,size)
#elif defined(SYS_ftruncate) && defined(QSE_USE_SYSCALL)
#	define QSE_FTRUNCATE(handle,size) syscall(SYS_ftruncate,handle,size)
#elif !defined(_LP64) && (QSE_SIZEOF_VOID_P<8) && defined(HAVE_FTRUNCATE64)
#	define QSE_FTRUNCATE(handle,size) ftruncate64(handle,size)
#else
#	define QSE_FTRUNCATE(handle,size) ftruncate(handle,size)
#endif

#if defined(SYS_fchmod) && defined(QSE_USE_SYSCALL)
#	define QSE_FCHMOD(handle,mode) syscall(SYS_fchmod,handle,mode)
#else
#	define QSE_FCHMOD(handle,mode) fchmod(handle,mode)
#endif

#if defined(SYS_fchown) && defined(QSE_USE_SYSCALL)
#	define QSE_FCHOWN(handle,owner,group) syscall(SYS_fchown,handle,owner,group)
#else
#	define QSE_FCHOWN(handle,owner,group) fchown(handle,owner,group)
#endif

#if defined(SYS_fsync) && defined(QSE_USE_SYSCALL)
#	define QSE_FSYNC(handle) syscall(SYS_fsync,handle)
#else
#	define QSE_FSYNC(handle) fsync(handle)
#endif

#if defined(SYS_fcntl) && defined(QSE_USE_SYSCALL)
#	define QSE_FCNTL(handle,cmd,arg) syscall(SYS_fcntl,handle,cmd,arg)
#else
#	define QSE_FCNTL(handle,cmd,arg) fcntl(handle,cmd,arg)
#endif

#if defined(SYS_dup2) && defined(QSE_USE_SYSCALL)
#	define QSE_DUP2(ofd,nfd) syscall(SYS_dup2,ofd,nfd)
#else
#	define QSE_DUP2(ofd,nfd) dup2(ofd,nfd)
#endif

#if defined(SYS_pipe) && defined(QSE_USE_SYSCALL)
#	define QSE_PIPE(pfds) syscall(SYS_pipe,pfds)
#else
#	define QSE_PIPE(pfds) pipe(pfds)
#endif

#if defined(SYS_exit) && defined(QSE_USE_SYSCALL)
#	define QSE_EXIT(code) syscall(SYS_exit,code)
#else
#	define QSE_EXIT(code) _exit(code)
#endif

#if defined(SYS_fork) && defined(QSE_USE_SYSCALL)
#	define QSE_FORK() syscall(SYS_fork)
#else
#	define QSE_FORK() fork()
#endif

#if defined(SYS_vfork) && defined(QSE_USE_SYSCALL)
#	define QSE_VFORK() syscall(SYS_vfork)
#else
#	define QSE_VFORK() vfork()
#endif

#if defined(SYS_execve) && defined(QSE_USE_SYSCALL)
#	define QSE_EXECVE(path,argv,envp) syscall(SYS_execve,path,argv,envp)
#else
#	define QSE_EXECVE(path,argv,envp) execve(path,argv,envp)
#endif

#if defined(SYS_waitpid) && defined(QSE_USE_SYSCALL)
#	define QSE_WAITPID(pid,status,options) syscall(SYS_waitpid,pid,status,options)
#else
#	define QSE_WAITPID(pid,status,options) waitpid(pid,status,options)
#endif

#if defined(SYS_kill) && defined(QSE_USE_SYSCALL)
#	define QSE_KILL(pid,sig) syscall(SYS_kill,pid,sig)
#else
#	define QSE_KILL(pid,sig) kill(pid,sig)
#endif

#if defined(SYS_getpid) && defined(QSE_USE_SYSCALL)
#	define QSE_GETPID() syscall(SYS_getpid)
#else
#	define QSE_GETPID() getpid()
#endif

#if defined(SYS_getuid) && defined(QSE_USE_SYSCALL)
#	define QSE_GETUID() syscall(SYS_getuid)
#else
#	define QSE_GETUID() getuid()
#endif

#if defined(SYS_geteuid) && defined(QSE_USE_SYSCALL)
#	define QSE_GETEUID() syscall(SYS_geteuid)
#else
#	define QSE_GETEUID() geteuid()
#endif

#if defined(SYS_getgid) && defined(QSE_USE_SYSCALL)
#	define QSE_GETGID() syscall(SYS_getgid)
#else
#	define QSE_GETGID() getgid()
#endif

#if defined(SYS_getegid) && defined(QSE_USE_SYSCALL)
#	define QSE_GETEGID() syscall(SYS_getegid)
#else
#	define QSE_GETEGID() getegid()
#endif

#if defined(SYS_gettimeofday) && defined(QSE_USE_SYSCALL)
#	define QSE_GETTIMEOFDAY(tv,tz) syscall(SYS_gettimeofday,tv,tz)
#else
#	define QSE_GETTIMEOFDAY(tv,tz) gettimeofday(tv,tz)
#endif

#if defined(SYS_settimeofday) && defined(QSE_USE_SYSCALL)
#	define QSE_SETTIMEOFDAY(tv,tz) syscall(SYS_settimeofday,tv,tz)
#else
#	define QSE_SETTIMEOFDAY(tv,tz) settimeofday(tv,tz)
#endif

#if defined(SYS_getrlimit) && defined(QSE_USE_SYSCALL)
#	define QSE_GETRLIMIT(res,lim) syscall(SYS_getrlimit,res,lim)
#else
#	define QSE_GETRLIMIT(res,lim) getrlimit(res,lim)
#endif

#if defined(SYS_setrlimit) && defined(QSE_USE_SYSCALL)
#	define QSE_SETRLIMIT(res,lim) syscall(SYS_setrlimit,res,lim)
#else
#	define QSE_SETRLIMIT(res,lim) setrlimit(res,lim)
#endif


/* ===== FILE SYSTEM CALLS ===== */

#if defined(SYS_chmod) && defined(QSE_USE_SYSCALL)
#	define QSE_CHMOD(path,mode) syscall(SYS_chmod,path,mode)
#else
#	define QSE_CHMOD(path,mode) chmod(path,mode)
#endif

#if defined(SYS_chown) && defined(QSE_USE_SYSCALL)
#	define QSE_CHOWN(path,owner,group) syscall(SYS_chown,path,owner,group)
#else
#	define QSE_CHOWN(path,owner,group) chown(path,owner,group)
#endif

#if defined(SYS_chroot) && defined(QSE_USE_SYSCALL)
#	define QSE_CHROOT(path) syscall(SYS_chroot,path)
#else
#	define QSE_CHROOT(path) chroot(path)
#endif

#if defined(SYS_lchown) && defined(QSE_USE_SYSCALL)
#	define QSE_LCHOWN(path,owner,group) syscall(SYS_lchown,path,owner,group)
#else
#	define QSE_LCHOWN(path,owner,group) lchown(path,owner,group)
#endif

#if defined(SYS_link) && defined(QSE_USE_SYSCALL)
#	define QSE_LINK(oldpath,newpath) syscall(SYS_link,oldpath,newpath)
#else
#	define QSE_LINK(oldpath,newpath) link(oldpath,newpath)
#endif

#if !defined(_LP64) && (QSE_SIZEOF_VOID_P<8) && defined(SYS_lstat64) && defined(QSE_USE_SYSCALL)
#	define QSE_LSTAT(path,stbuf) syscall(SYS_lstat,path,stbuf)
	typedef struct stat64 qse_lstat_t;
#elif defined(SYS_lstat) && defined(QSE_USE_SYSCALL)
#	define QSE_LSTAT(path,stbuf) syscall(SYS_lstat,path,stbuf)
	typedef struct stat qse_lstat_t;
#elif !defined(_LP64) && (QSE_SIZEOF_VOID_P<8) && defined(HAVE_LSTAT64)
#	define QSE_LSTAT(path,stbuf) lstat64(path,stbuf)
	typedef struct stat64 qse_lstat_t;
#else
#	define QSE_LSTAT(path,stbuf) lstat(path,stbuf)
	typedef struct stat qse_lstat_t;
#endif

#if defined(SYS_access) && defined(QSE_USE_SYSCALL)
#	define QSE_ACCESS(path,mode) syscall(SYS_access,path,mode)
#else
#	define QSE_ACCESS(path,mode) access(path,mode)
#endif

#if defined(SYS_rename) && defined(QSE_USE_SYSCALL)
#	define QSE_RENAME(oldpath,newpath) syscall(SYS_rename,oldpath,newpath)
#else
	int rename(const char *oldpath, const char *newpath); /* not to include stdio.h */
#	define QSE_RENAME(oldpath,newpath) rename(oldpath,newpath)
#endif

#if defined(SYS_rmdir) && defined(QSE_USE_SYSCALL)
#	define QSE_RMDIR(path) syscall(SYS_rmdir,path)
#else
#	define QSE_RMDIR(path) rmdir(path)
#endif

#if !defined(_LP64) && (QSE_SIZEOF_VOID_P<8) && defined(SYS_stat64) && defined(QSE_USE_SYSCALL)
#	define QSE_STAT(path,stbuf) syscall(SYS_stat64,path,stbuf)
	typedef struct stat64 qse_stat_t;
#elif defined(SYS_stat) && defined(QSE_USE_SYSCALL)
#	define QSE_STAT(path,stbuf) syscall(SYS_stat,path,stbuf)
	typedef struct stat qse_stat_t;
#elif !defined(_LP64) && (QSE_SIZEOF_VOID_P<8) && defined(HAVE_STAT64)
#	define QSE_STAT(path,stbuf) stat64(path,stbuf)
	typedef struct stat64 qse_stat_t;
#else
#	define QSE_STAT(path,stbuf) stat(path,stbuf)
	typedef struct stat qse_stat_t;
#endif

#if defined(SYS_symlink) && defined(QSE_USE_SYSCALL)
#	define QSE_SYMLINK(oldpath,newpath) syscall(SYS_symlink,oldpath,newpath)
#else
#	define QSE_SYMLINK(oldpath,newpath) symlink(oldpath,newpath)
#endif

#if defined(SYS_unlink) && defined(QSE_USE_SYSCALL)
#	define QSE_UNLINK(path) syscall(SYS_unlink,path)
#else
#	define QSE_UNLINK(path) unlink(path)
#endif

#if defined(SYS_utime) && defined(QSE_USE_SYSCALL)
#	define QSE_UTIME(path,t) syscall(SYS_utime,path,t)
#else
#	define QSE_UTIME(path,t) utime(path,t)
#endif

#if defined(SYS_utimes) && defined(QSE_USE_SYSCALL)
#	define QSE_UTIMES(path,t) syscall(SYS_utimes,path,t)
#else
#	define QSE_UTIMES(path,t) utimes(path,t)
#endif

/* ===== DIRECTORY - not really system calls ===== */
#define QSE_OPENDIR(name) opendir(name)
#define QSE_CLOSEDIR(name) closedir(name)

#if defined(HAVE_READDIR64)
	typedef struct dirent64 qse_dirent_t;
#	define QSE_READDIR(x) readdir64(x)
#else
	typedef struct dirent qse_dirent_t;
#	define QSE_READDIR(x) readdir(x)
#endif

/* ------------------------------------------------------------------------ */

#if defined(__linux) && defined(__GNUC__) && defined(__x86_64) 

#include <sys/syscall.h>

/*

http://www.x86-64.org/documentation/abi.pdf

A.2 AMD64 Linux Kernel Conventions
The section is informative only.

A.2.1 Calling Conventions
The Linux AMD64 kernel uses internally the same calling conventions as user-
level applications (see section 3.2.3 for details). User-level applications 
that like to call system calls should use the functions from the C library. 
The interface between the C library and the Linux kernel is the same as for
the user-level applications with the following differences:

1. User-level applications use as integer registers for passing the sequence
   %rdi, %rsi, %rdx, %rcx, %r8 and %r9. The kernel interface uses %rdi,
   %rsi, %rdx, %r10, %r8 and %r9.
2. A system-call is done via the syscall instruction. The kernel destroys
   registers %rcx and %r11.
3. The number of the syscall has to be passed in register %rax.
4. System-calls are limited to six arguments, no argument is passed directly 
   on the stack.
5. Returning from the syscall, register %rax contains the result of the
   system-call. A value in the range between -4095 and -1 indicates an error,
   it is -errno.
6. Only values of class INTEGER or class MEMORY are passed to the kernel.

*/

/*
#define QSE_SYSCALL0(ret,num) \
	__asm__ volatile ( \
		"movq %1, %%rax\n\t" \
		"syscall\n"  \
		: "=&a"(ret)  \
		: "g"((qse_uint64_t)num) \
		: "%rcx", "%r11")

#define QSE_SYSCALL1(ret,num,arg1) \
	__asm__ volatile ( \
		"movq %1, %%rax\n\t" \
		"movq %2, %%rdi\n\t" \
		"syscall\n"  \
		: "=&a"(ret)  \
		: "g"((qse_uint64_t)num), "g"((qse_uint64_t)arg1) \
		: "%rdi", "%rcx", "%r11")

#define QSE_SYSCALL2(ret,num,arg1,arg2) \
	__asm__ volatile ( \
		"movq %1, %%rax\n\t" \
		"movq %2, %%rdi\n\t" \
		"movq %3, %%rsi\n\t" \
		"syscall\n"  \
		: "=&a"(ret) \
		: "g"((qse_uint64_t)num), "g"((qse_uint64_t)arg1), "g"((qse_uint64_t)arg2) \
		: "%rdi", "%rsi", "%rcx", "%r11")

#define QSE_SYSCALL3(ret,num,arg1,arg2,arg3) \
	__asm__ volatile ( \
		"movq %1, %%rax\n\t" \
		"movq %2, %%rdi\n\t" \
		"movq %3, %%rsi\n\t" \
		"movq %4, %%rdx\n\t" \
		"syscall\n"  \
		: "=&a"(ret)  \
		: "g"((qse_uint64_t)num), "g"((qse_uint64_t)arg1), "g"((qse_uint64_t)arg2), "g"((qse_uint64_t)arg3) \
		: "%rdi", "%rsi", "%rdx", "%rcx", "%r11")
*/

#define QSE_SYSCALL0(ret,num) \
	__asm__ volatile ( \
		"syscall\n" \
		: "=a"(ret) \
		: "a"((qse_uint64_t)num) \
		: "memory", "cc", "%rcx", "%r11" \
	)
 

#define QSE_SYSCALL1(ret,num,arg1) \
	__asm__ volatile ( \
		"syscall\n" \
		: "=a"(ret) \
		: "a"((qse_uint64_t)num), "D"((qse_uint64_t)arg1) \
		: "memory", "cc", "%rcx", "%r11" \
	)

#define QSE_SYSCALL2(ret,num,arg1,arg2) \
	__asm__ volatile ( \
		"syscall\n"  \
		: "=a"(ret) \
		: "a"((qse_uint64_t)num), "D"((qse_uint64_t)arg1), "S"((qse_uint64_t)arg2) \
		: "memory", "cc", "%rcx", "%r11" \
	)

#define QSE_SYSCALL3(ret,num,arg1,arg2,arg3) \
	__asm__ volatile ( \
		"syscall\n" \
		: "=a"(ret) \
		: "a"((qse_uint64_t)num), "D"((qse_uint64_t)arg1), "S"((qse_uint64_t)arg2), "d"((qse_uint64_t)arg3) \
		: "memory", "cc", "%rcx", "%r11" \
	)

#define QSE_SYSCALL4(ret,num,arg1,arg2,arg3,arg4) \
	__asm__ volatile ( \
		"syscall\n" \
		: "=a"(ret) \
		: "a"((qse_uint64_t)num), "D"((qse_uint64_t)arg1), "S"((qse_uint64_t)arg2), "d"((qse_uint64_t)arg3),  "c"((qse_uint64_t)arg4) \
		: "memory", "cc", "%rcx", "%r11" \
	)

#elif defined(__linux) && defined(__GNUC__) && defined(__i386) 

#include <sys/syscall.h>

#define QSE_SYSCALL0(ret,num) \
	__asm__ volatile ( \
		"int $0x80\n" \
		: "=a"(ret) \
		: "a"((qse_uint32_t)num) \
		: "memory" \
	)

/*
#define QSE_SYSCALL1(ret,num,arg1) \
	__asm__ volatile ( \
		"int $0x80\n"  \
		: "=a"(ret)  \
		: "a"((qse_uint32_t)num), "b"((qse_uint32_t)arg1) \
		: "memory" \
	)

GCC in x86 PIC mode uses ebx to store the GOT table. so the macro shouldn't
clobber the ebx register. this modified version stores ebx before interrupt
and restores it after interrupt.
*/
#define QSE_SYSCALL1(ret,num,arg1) \
	__asm__ volatile ( \
		"push %%ebx\n\t" \
		"movl %2, %%ebx\n\t" \
		"int $0x80\n\t" \
		"pop %%ebx\n" \
		: "=a"(ret) \
		: "a"((qse_uint32_t)num), "r"((qse_uint32_t)arg1) \
		: "memory" \
	)

/*
#define QSE_SYSCALL2(ret,num,arg1,arg2) \
	__asm__ volatile ( \
		"int $0x80\n"  \
		: "=a"(ret) \
		: "a"((qse_uint32_t)num), "b"((qse_uint32_t)arg1), "c"((qse_uint32_t)arg2) \
		: "memory" \
	)
*/
#define QSE_SYSCALL2(ret,num,arg1,arg2) \
	__asm__ volatile ( \
		"push %%ebx\n\t" \
		"movl %2, %%ebx\n\t" \
		"int $0x80\n\t"  \
		"pop %%ebx\n" \
		: "=a"(ret) \
		: "a"((qse_uint32_t)num), "r"((qse_uint32_t)arg1), "c"((qse_uint32_t)arg2) \
		: "memory" \
	)

/*
#define QSE_SYSCALL3(ret,num,arg1,arg2,arg3) \
	__asm__ volatile ( \
		"int $0x80\n"  \
		: "=a"(ret)  \
		: "a"((qse_uint32_t)num), "b"((qse_uint32_t)arg1), "c"((qse_uint32_t)arg2), "d"((qse_uint32_t)arg3) \
		: "memory" \
	)
*/
#define QSE_SYSCALL3(ret,num,arg1,arg2,arg3) \
	__asm__ volatile ( \
		"push %%ebx\n\t" \
		"movl %2, %%ebx\n\t" \
		"int $0x80\n\t" \
		"pop %%ebx\n" \
		: "=a"(ret) \
		: "a"((qse_uint32_t)num), "r"((qse_uint32_t)arg1), "c"((qse_uint32_t)arg2), "d"((qse_uint32_t)arg3) \
		: "memory" \
	)

#define QSE_SYSCALL4(ret,num,arg1,arg2,arg3,arg4) \
	__asm__ volatile ( \
		"push %%ebx\n\t" \
		"movl %2, %%ebx\n\t" \
		"int $0x80\n\t" \
		"pop %%ebx\n" \
		: "=a"(ret) \
		: "a"((qse_uint32_t)num), "r"((qse_uint32_t)arg1), "c"((qse_uint32_t)arg2), "d"((qse_uint32_t)arg3), "S"((qse_uint32_t)arg4) \
		: "memory" \
	)

#endif

/* ------------------------------------------------------------------------ */

#endif
