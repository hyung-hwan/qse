dnl AC_PREREQ([2.67])

dnl Make sure you change the version information
dnl in include/qse/conf_*.h whenever you change the version 
dnl here. Those files don't depend on autoconf, thus requiring
dnl manual change.
AC_INIT([qse],[0.8.2],[Chung, Hyung-Hwan (hyunghwan.chung@gmail.com)],[],[http://code.miflux.com/@qse])

AC_CONFIG_HEADER([include/qse/config.h])
AC_CONFIG_AUX_DIR([ac])
AC_CONFIG_MACRO_DIR([m4])

dnl use CC and CPP for compilation tests with .c test programs.
AC_LANG(C)

dnl initialize automake
AM_INIT_AUTOMAKE([-Wall -Werror foreign])
AM_PROG_AR

dnl enable silent rules if automake supports them.
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([no])])
dnl m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

dnl determine a C compiler to use
AC_PROG_CC

dnl determine a C++ compiler to use
AC_PROG_CXX

dnl check if the C++ compiler exists in PATH
AC_CHECK_PROG(HAVE_CXX, $CXX, yes, no)

dnl check if the C++ compiler supports namespace
AX_CXX_NAMESPACE_STD

dnl determine some key programs
AC_PROG_INSTALL
AC_PROG_EGREP
AC_PROG_LN_S

dnl check other tools
AC_CHECK_TOOL(RM, rm)
AC_CHECK_TOOL(RMDIR, rmdir)
AC_CHECK_TOOL(TRUE, true)
AC_CHECK_TOOL(ECHO, echo)

dnl avoid "'/bin/rm: cannot remove `libtoolT': No such file or directory"
RM="$RM -f"

dnl initialize libtool
LT_INIT
AC_SUBST(LIBTOOL_DEPS)

dnl overrides the default CFLAGS setting
if test "$ac_test_CFLAGS" != "set"
then
	if test "$GCC" = "yes"
	then
		CFLAGS="$CFLAGS -Wall"
	fi
fi

if test "$ac_test_CXXFLAGS" != "set"
then
	if test "$GCC" = "yes"
	then
		CXXFLAGS="$CXXFLAGS -Wall"
	fi
fi

dnl indicate the existence of config.h
CFLAGS="$CFLAGS -DQSE_HAVE_CONFIG_H"
CXXFLAGS="$CXXFLAGS -DQSE_HAVE_CONFIG_H"

dnl make visible the 64-bit interface to the file system
CFLAGS="$CFLAGS -D_LARGEFILE64_SOURCE"
CXXFLAGS="$CXXFLAGS -D_LARGEFILE64_SOURCE"

dnl define the WIN32 conditional if necessary
case "$host" in
*-*-mingw*|*-*-cygwin*)
	#CFLAGS="$CFLAGS -D_WIN32_WINNT=0x0400"
	platform_win32=yes
	;;
*)
	platform_win32=no
	;;
esac
AM_CONDITIONAL(WIN32, test "x${platform_win32}" = "xyes" )

dnl check the math library (check if -lm is needed)
LT_LIB_M
AC_SUBST(LIBM, $LIBM)

dnl check header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([stddef.h wchar.h wctype.h errno.h signal.h fcntl.h dirent.h stdatomic.h])
AC_CHECK_HEADERS([time.h sys/time.h utime.h spawn.h execinfo.h ucontext.h])
AC_CHECK_HEADERS([sys/resource.h sys/wait.h sys/syscall.h sys/ioctl.h])
AC_CHECK_HEADERS([sys/sendfile.h sys/epoll.h sys/event.h sys/poll.h poll.h])
AC_CHECK_HEADERS([sys/sysctl.h sys/socket.h sys/sockio.h sys/un.h])
AC_CHECK_HEADERS([ifaddrs.h tiuser.h linux/netfilter_ipv4.h netinet/sctp.h netpacket/packet.h])
AC_CHECK_HEADERS([net/if.h net/if_dl.h], [], [], [
	#include <sys/types.h>
	#include <sys/socket.h>])
AC_CHECK_HEADERS([sys/stropts.h sys/macstat.h linux/ethtool.h linux/sockios.h])
AC_CHECK_HEADERS([quadmath.h crt_externs.h sys/prctl.h uci.h])
AC_CHECK_HEADERS([ltdl.h dlfcn.h])

dnl check data types
dnl AC_CHECK_TYPE([wchar_t], 
dnl 	[ AC_DEFINE([HAVE_WCHAR_T_IN_STDDEF_H],
dnl	           [],[wchar_t is available in stddef.h]) ],
dnl	[],
dnl	[#include <stddef.h>])

dnl check functions
AC_CHECK_FUNCS([uselocale])
AC_CHECK_FUNCS([mbrlen mbrtowc wcrtomb])
AC_CHECK_FUNCS([mbsnrtowcs mbsrtowcs wcsnrtombs wcsrtombs])
AC_CHECK_FUNCS([wctype iswctype wctrans towctrans])
AC_CHECK_FUNCS([isblank iswblank])
AC_CHECK_FUNCS([lseek64 ftruncate64 readdir64 dirfd])
AC_CHECK_FUNCS([stat64 fstat64 lstat64 fstatat64])
AC_CHECK_FUNCS([fstat fstatat])
AC_CHECK_FUNCS([fchmod fchmodat fchown fchownat lchown fsync ftruncate])
AC_CHECK_FUNCS([timegm timelocal localtime_r gmtime_r gettimeofday settimeofday])
AC_CHECK_FUNCS([utime utimes futimes lutimes futimens utimensat])
AC_CHECK_FUNCS([sysconf prctl fdopendir setrlimit getrlimit getpgid getpgrp])
AC_CHECK_FUNCS([backtrace backtrace_symbols])
AC_CHECK_FUNCS([fork vfork posix_spawn gettid nanosleep select])
AC_CHECK_FUNCS([makecontext swapcontext getcontext setcontext])
AC_CHECK_FUNCS([snprintf _vsnprintf _vsnwprintf])
AC_CHECK_FUNCS([accept4 sendmsg recvmsg writev readv])

OLDLIBS="$LIBS"
LIBS="$LIBM $LIBS"
AC_CHECK_FUNCS([powl fmodl sinl cosl tanl sinhl coshl tanhl asinl acosl atanl atan2l logl log2l log10l expl sqrtl ceill floorl roundl])
AC_CHECK_FUNCS([pow fmod sin cos tan sinh cosh tanh asin acos atan atan2 log log2 log10 exp sqrt ceil floor round])
AC_CHECK_FUNCS([powf fmodf sinf cosf tanf sinhf coshf tanhf asinf acosf atanf atan2f logf log2f log10f expf sqrtf ceilf floorf roundf])
LIBS="$OLDLIBS"

save_LIBS="$LIBS"
AC_SEARCH_LIBS([dlopen], [dl dld], [
	DL_LIBS="$ac_cv_search_dlopen"
	if test "x${DL_LIBS}" = "xnone required"
	then
		DL_LIBS=""
	fi
])
LIBS="$save_LIBS"
AC_SUBST(DL_LIBS)

save_LIBS="$LIBS"
AC_SEARCH_LIBS([lt_dlopen], [ltdl], [
	LTDL_LIBS="$ac_cv_search_lt_dlopen"
	if test "x${LTDL_LIBS}" = "xnone required"
	then
		LTDL_LIBS=""
	fi
])
LIBS="$save_LIBS"
AC_SUBST(LTDL_LIBS)

dnl OLDLIBS="$LIBS"
dnl AC_SEARCH_LIBS([connect], [socket])
dnl LIBS="$OLDLIBS"
dnl if test "$ac_cv_search_connect" != "none required" -a "$ac_cv_search_connect" != "no"
dnl then
dnl	SOCKET_LIBS="$SOCKET_LIBS -lsocket"
dnl fi
dnl AC_SUBST(SOCKET_LIBS)
AC_CHECK_FUNCS([connect gethostbyname])
if test "x$ac_cv_func_gethostbyname" = "xno"
then
	AC_CHECK_LIB([nsl], [gethostbyname], [
		SOCKET_LIBS="$SOCKET_LIBS -lnsl"
		AC_DEFINE(HAVE_GETHOSTBYNAME, 1)
	])
fi
if test "x$ac_cv_func_connect" = "xno"
then
	AC_CHECK_LIB([socket], [connect], [
		SOCKET_LIBS="$SOCKET_LIBS -lsocket"
		AC_DEFINE(HAVE_CONNECT, 1)
	]) 

	if test "x$ac_cv_lib_socket_connect" = xno
	then
		AC_CHECK_FUNCS([t_connect])
		if test "x$ac_cv_func_t_connect" = "xno"
		then
			AC_CHECK_LIB([nsl_s], [t_connect], [
				SOCKET_LIBS="$SOCKET_LIBS -lnsl_s"
				AC_DEFINE(HAVE_T_CONNECT, 1)
			])
		fi
	fi
fi
if test "x${platform_win32}" = "xyes"
then
	SOCKET_LIBS="$SOCKET_LIBS -lws2_32"
fi
AC_SUBST(SOCKET_LIBS)

OLDLIBS="$LIBS"
LIBS="$SOCKET_LIBS $LIBS"
AC_CHECK_FUNCS([if_nametoindex if_indextoname getifaddrs])
LIBS="$OLDLIBS"

dnl check if sendfile and its variants exist in the standard c library
dnl it inspects the existence of each missing function in the sendfile library.
AC_CHECK_FUNCS([sendfile sendfile64 sendfilev sendfilev64])
if test "$ac_cv_func_sendfile" = "no"
then
	AC_CHECK_LIB([sendfile], [sendfile], [
		SENDFILE_LIBS="-lsendfile"
		dnl only for consistency with AC_CHECK_FUNCS
		AC_DEFINE(HAVE_SENDFILE, 1)
	])
fi
if test "$ac_cv_func_sendfile64" = "no"
then
	AC_CHECK_LIB([sendfile], [sendfile64], [
		SENDFILE_LIBS="-lsendfile"
		dnl only for consistency with AC_CHECK_FUNCS
		AC_DEFINE(HAVE_SENDFILE64, 1)
	])
fi
if test "$ac_cv_func_sendfilev" = "no"
then
	AC_CHECK_LIB([sendfile], [sendfilev], [
		SENDFILE_LIBS="-lsendfile"
		dnl only for consistency with AC_CHECK_FUNCS
		AC_DEFINE(HAVE_SENDFILEV, 1)
	])
fi
if test "$ac_cv_func_sendfilev64" = "no"
then
	AC_CHECK_LIB([sendfile], [sendfilev64], [
		SENDFILE_LIBS="-lsendfile"
		dnl only for consistency with AC_CHECK_FUNCS
		AC_DEFINE(HAVE_SENDFILEV64, 1)
	])
fi
AC_SUBST(SENDFILE_LIBS)

AC_CHECK_FUNCS([epoll_create epoll_create1])
if test "$ac_cv_func_epoll_create" = "yes"
then
	AC_DEFINE(HAVE_EPOLL, 1, [epoll support])
fi

AC_CHECK_FUNCS([kqueue kqueue1 kevent])


AC_CHECK_LIB([pthread], [pthread_mutex_timedlock],  [
	AC_DEFINE([HAVE_PTHREAD_MUTEX_TIMEDLOCK],1,[pthreads has pthread_mutex_timedlock()])
])
AC_CHECK_LIB([pthread], [pthread_mutex_trylock],  [
	AC_DEFINE([HAVE_PTHREAD_MUTEX_TRYLOCK],1,[pthreads has pthread_mutex_trylock()])
])

dnl check is the import library for unicows.dll exists
dnl this check doesn't look for a particular symbol
dnl but for the symbol 'main' since i want to check 
dnl the existence of the libaray.
AC_CHECK_LIB([unicows], [main], [UNICOWS_LIBS="-lunicows"])
AC_SUBST(UNICOWS_LIBS)

AC_ARG_ENABLE([ssl], [AS_HELP_STRING([--enable-ssl],[build the library in the ssl mode (default. yes)])],
	enable_ssl_is=$enableval,enable_ssl_is=yes)
if test "x$enable_ssl_is" = "xyes"
then
	AC_CHECK_HEADERS([openssl/ssl.h openssl/err.h openssl/engine.h])

	dnl check for an SSL library
	dnl i used SSL_library_init at first.
	dnl starting from  openssl 1.1.0, SSL_library_init was made to a macro 
	dnl aliasing OPENSSL_init_ssl.
	dnl   #define SSL_library_init() OPENSSL_init_ssl(0, NULL)
	dnl instead of checking both, now i use SSL_new.
	AC_CHECK_FUNCS([SSL_new])

	if test "x$ac_cv_func_SSL_new" = "xyes" 
	then
		AC_DEFINE(HAVE_SSL, 1, [ssl support])
	else
		AC_CHECK_LIB([ssl], [SSL_new], [
			SSL_LIBS="-lssl"
			AC_DEFINE(HAVE_SSL, 1, [ssl support])

			dnl additional check
			AC_CHECK_LIB([crypto], [ERR_get_error], [
				SSL_LIBS="${SSL_LIBS} -lcrypto"
			])

			dnl only for consistency with AC_CHECK_FUNCS
			AC_DEFINE(HAVE_SSL_NEW, 1)
		])
	fi

	OLDLIBS="$LIBS"
	LIBS="$SSL_LIBS $LIBS"
	AC_CHECK_FUNCS([ENGINE_cleanup CRYPTO_cleanup_all_ex_data])
	LIBS="$OLDLIBS"
fi
AC_SUBST(SSL_LIBS)

dnl MPI (optional)
LX_FIND_MPI(C)
AM_CONDITIONAL(HAVE_C_MPI, test "x${have_C_mpi}" = "xyes" )
dnl AM_CONDITIONAL(HAVE_CXX_MPI, test "${have_CXX_mpi}" = "yes" )

dnl libuci (optional)
AC_CHECK_LIB([uci], [uci_alloc_context], [UCI_LIBS="-luci"])
AC_SUBST(UCI_LIBS)
AM_CONDITIONAL(HAVE_LIBUCI, test "x${ac_cv_lib_uci_uci_alloc_context}" = "xyes" -a "x${ac_cv_header_uci_h}" = "xyes")

dnl libmysqlclient
AX_LIB_MYSQL()

AC_STRUCT_DIRENT_D_TYPE
AC_CHECK_MEMBERS([DIR.d_fd, DIR.dd_fd],,,[[#include <dirent.h>]])
AC_CHECK_MEMBERS([struct stat.st_birthtime])
AC_CHECK_MEMBERS([struct stat.st_mtim.tv_nsec])
AC_CHECK_MEMBERS([struct stat.st_birthtim.tv_nsec])
AC_CHECK_MEMBERS([struct stat.st_mtimespec.tv_nsec])
AC_CHECK_MEMBERS([struct stat.st_birthtimespec.tv_nsec])
AC_CHECK_MEMBERS([struct tm.tm_gmtoff],,,[[#include <time.h>]])


AC_CHECK_MEMBERS([struct ifreq.ifr_ifindex, struct ifreq.ifr_mtu], [], [],
	[#include <sys/types.h>
	 #include <sys/socket.h>
	 #ifdef HAVE_NET_IF_H
	 #include <net/if.h>
	 #endif])

AC_CHECK_TYPES([struct lifconf, struct lifreq, struct if_laddrreq], [], [],
	[#include <sys/types.h>
	 #include <sys/socket.h>
	 #ifdef HAVE_NET_IF_H
	 #include <net/if.h>
	 #endif])

AC_CHECK_MEMBERS([struct sockaddr_in6.sin6_scope_id], [], [],
	[#include <sys/types.h> 
	 #include <sys/socket.h> 
	 #include <netinet/in.h>])

AC_CHECK_TYPES([struct timespec], [], [],
	[#include <sys/types.h>
	 #ifdef HAVE_TIME_H
	 #include <time.h>
	 #endif
	 #ifdef HAVE_SYS_TIME_H
	 #include <sys/time.h>
	 #endif])

AC_MSG_CHECKING([for va_copy])
AC_TRY_LINK(
	[#include <stdarg.h>],
	[
		va_list x, y;
		va_copy(x,y);
	],
	[
		AC_MSG_RESULT(yes)
		AC_DEFINE([HAVE_VA_COPY], [1], [va_copy is available])
	],
	[AC_MSG_RESULT(no)]
)

AC_MSG_CHECKING([for __va_copy])
AC_TRY_LINK(
	[#include <stdarg.h>],
	[
		va_list x, y;
		__va_copy(x,y); 
	],
	[
		AC_MSG_RESULT(yes)
		AC_DEFINE([HAVE___VA_COPY], [1], [__va_copy is available])
	],
	[AC_MSG_RESULT(no)]
)

dnl check the size of primitive data types
AC_CHECK_SIZEOF(char,,[[]])
AC_CHECK_SIZEOF(short,,[[]])
AC_CHECK_SIZEOF(int,,[[]])
AC_CHECK_SIZEOF(long,,[[]])
AC_CHECK_SIZEOF(long long,,[[]])
AC_CHECK_SIZEOF(__int8,,[[]])
AC_CHECK_SIZEOF(__int16,,[[]])
AC_CHECK_SIZEOF(__int32,,[[]])
AC_CHECK_SIZEOF(__int64,,[[]])
AC_CHECK_SIZEOF(__int128,,[[]])
AC_CHECK_SIZEOF(__int8_t,,[[]])
AC_CHECK_SIZEOF(__int16_t,,[[]])
AC_CHECK_SIZEOF(__int32_t,,[[]])
AC_CHECK_SIZEOF(__int64_t,,[[]])
AC_CHECK_SIZEOF(__int128_t,,[[]])
dnl AC_CHECK_SIZEOF doesn't work without white-space between void and *
AC_CHECK_SIZEOF(void *,,[[]])
AC_CHECK_SIZEOF(float,,[[]])
AC_CHECK_SIZEOF(double,,[[]])
AC_CHECK_SIZEOF(long double,,[[]])
AC_CHECK_SIZEOF(__float128,,[[]])

AC_CHECK_SIZEOF(wchar_t)
AC_CHECK_SIZEOF(off_t)
AC_CHECK_SIZEOF(off64_t)

AC_CHECK_SIZEOF(mode_t)
if test ${ac_cv_sizeof_mode_t} -gt 0
then
	AX_CHECK_SIGN([mode_t],
		[ AC_DEFINE(QSE_MODE_T_IS_SIGNED, 1, [Define if mode_t is signed]) ],
		[ AS_ECHO_N("") ],
		[])
fi


AC_CHECK_SIZEOF(mbstate_t,,[#include <wchar.h>])
AX_CHECK_NUMVALOF(MB_LEN_MAX,[32],[#include <limits.h>])
AX_CHECK_NUMVALOF(PATH_MAX,[0],[#include <limits.h>])
AX_CHECK_NUMVALOF(NSIG,[32],[#include <signal.h>])

AX_CHECK_NUMVALOF(SOCK_STREAM, [-1],   [#include <sys/types.h>
                                        #include <sys/socket.h>
                                        #include <netinet/in.h>])
AX_CHECK_NUMVALOF(SOCK_DGRAM,  [-1],   [#include <sys/types.h>
                                        #include <sys/socket.h>
                                        #include <netinet/in.h>])
AX_CHECK_NUMVALOF(SOCK_RAW,    [-1],   [#include <sys/types.h>
                                        #include <sys/socket.h>
                                        #include <netinet/in.h>])

AX_CHECK_NUMVALOF(AF_INET,     [-1],   [#include <sys/types.h>
                                        #include <sys/socket.h>
                                        #include <netinet/in.h>])
AX_CHECK_NUMVALOF(AF_INET6,    [-1],   [#include <sys/types.h>
                                        #include <sys/socket.h>
                                        #include <netinet/in.h>])
AX_CHECK_NUMVALOF(AF_PACKET,   [-1],   [#include <sys/types.h>
                                        #include <sys/socket.h>
                                        #include <netinet/in.h>])
AX_CHECK_NUMVALOF(AF_UNIX,     [-1],   [#include <sys/types.h>
                                        #include <sys/socket.h>
                                        #include <sys/un.h>])

if test "x${platform_win32}" = "xyes"
then
	AC_CHECK_SIZEOF(struct sockaddr_in,,[
		#include <winsock2.h>
		#include <ws2tcpip.h>])
	AC_CHECK_SIZEOF(struct sockaddr_in6,,[
		#include <winsock2.h>
		#include <ws2tcpip.h>])
	AC_CHECK_SIZEOF(struct sockaddr_un,,[
		#include <winsock2.h>
		#include <ws2tcpip.h>])
	AC_CHECK_SIZEOF(struct sockaddr_ll,,[
		#include <winsock2.h>
		#include <ws2tcpip.h>])
	AC_CHECK_SIZEOF(struct sockaddr_dl,,[
		#include <winsock2.h>
		#include <ws2tcpip.h>])

	AC_CHECK_SIZEOF(socklen_t,, [
		#include <winsock2.h>
		#include <ws2tcpip.h>])

	if test ${ac_cv_sizeof_socklen_t} -gt 0
	then
		AX_CHECK_SIGN([socklen_t],
			[ AC_DEFINE(QSE_SOCKLEN_T_IS_SIGNED, 1, [Define if socklen_t is signed]) ],
			[ AS_ECHO_N("") ],
			[#include <winsock2.h>
			 #include <ws2tcpip.h>])
	fi
	AC_CHECK_SIZEOF(sa_family_t,, [
		#include <winsock2.h>
		#include <ws2tcpip.h>])
	if test ${ac_cv_sizeof_sa_family_t} -gt 0
	then
		AX_CHECK_SIGN([sa_family_t],
			[ AC_DEFINE(QSE_SA_FAMILY_T_IS_SIGNED, 1, [Define if sa_family_t is signed]) ],
			[ AS_ECHO_N("") ],
			[#include <winsock2.h>
			 #include <ws2tcpip.h>])
	fi

else
	AC_CHECK_SIZEOF(struct sockaddr_in,,[
		#include <sys/types.h>
		#include <sys/socket.h>
		#include <netinet/in.h>])
	AC_CHECK_SIZEOF(struct sockaddr_in6,,[
		#include <sys/types.h>
		#include <sys/socket.h>
		#include <netinet/in.h>])
	AC_CHECK_SIZEOF(struct sockaddr_un,,[
		#include <sys/types.h>
		#include <sys/socket.h>
		#include <sys/un.h>])
	AC_CHECK_SIZEOF(struct sockaddr_ll,,[
		#include <sys/types.h>
		#include <sys/socket.h>
		#include <netpacket/packet.h>])
	AC_CHECK_SIZEOF(struct sockaddr_dl,,[
		#include <sys/types.h>
		#include <sys/socket.h>
		#include <net/if_dl.h>])

	AC_CHECK_SIZEOF(socklen_t,, [
		#include <sys/types.h>
		#include <sys/socket.h>
		#include <netinet/in.h>])
	if test ${ac_cv_sizeof_socklen_t} -gt 0
	then
		AX_CHECK_SIGN([socklen_t],
			[ AC_DEFINE(QSE_SOCKLEN_T_IS_SIGNED, 1, [Define if socklen_t is signed]) ],
			[ AS_ECHO_N("") ],
			[#include <sys/socket.h>])
	fi

	AC_CHECK_SIZEOF(sa_family_t,, [
		#include <sys/socket.h>
		#include <netinet/in.h>])
	if test ${ac_cv_sizeof_sa_family_t} -gt 0
	then
		AX_CHECK_SIGN([sa_family_t],
			[ AC_DEFINE(QSE_SA_FAMILY_T_IS_SIGNED, 1, [Define if sa_family_t is signed]) ],
			[ AS_ECHO_N("") ],
			[#include <sys/types.h>
			 #include <sys/socket.h>
			 #include <netinet/in.h>])
	fi
fi

AC_CHECK_SIZEOF(struct iovec,,[#include <sys/uio.h>])
if test ${ac_cv_sizeof_struct_iovec} -gt 0
then
	AC_MSG_CHECKING([offset of iov_base in struct iovec])
	AC_COMPUTE_INT([ac_cv_offsetof_iov_base],
		[[offsetof(struct iovec, iov_base)]],
		[#include <stddef.h>
		 #include <sys/uio.h>], [ac_cv_offsteof_iov_base=0])
	AC_MSG_RESULT([$ac_cv_offsetof_iov_base])
	AC_DEFINE_UNQUOTED(QSE_OFFSETOF_IOV_BASE, ${ac_cv_offsetof_iov_base}, [offsetof(struct iovec, iov_base)])

	AC_MSG_CHECKING([offset of iov_len in struct iovec])
	AC_COMPUTE_INT([ac_cv_offsetof_iov_len],
		[[offsetof(struct iovec, iov_len)]],
		[#include <stddef.h>
		 #include <sys/uio.h>], [ac_cv_offsteof_iov_len=0])
	AC_MSG_RESULT([$ac_cv_offsetof_iov_len])
	AC_DEFINE_UNQUOTED(QSE_OFFSETOF_IOV_LEN, ${ac_cv_offsetof_iov_len}, [offsetof(struct iovec, iov_len)])
fi

AC_CHECK_SIZEOF(pthread_t,, [#include <pthread.h>])
if test ${ac_cv_sizeof_pthread_t} -gt 0
then
	AX_CHECK_SIGN([pthread_t],
		[AC_DEFINE(QSE_PTHREAD_T_IS_SIGNED, 1, [Define if pthread_t is signed])],
		[AS_ECHO_N("")],
		[#include <pthread.h>])
fi

AC_CHECK_SIZEOF(pthread_mutex_t,, [#include <pthread.h>])
if test ${ac_cv_sizeof_pthread_mutex_t} -gt 0
then
	AX_CHECK_SIGN([pthread_mutex_t],
		[ AC_DEFINE(QSE_PTHREAD_MUTEX_T_IS_SIGNED, 1, [Define if pthread_mutex_t is signed]) ],
		[ AS_ECHO_N("") ],
		[#include <pthread.h>])
fi

AC_CHECK_SIZEOF(pthread_cond_t,, [#include <pthread.h>])
if test ${ac_cv_sizeof_pthread_cond_t} -gt 0
then
	AX_CHECK_SIGN([pthread_cond_t],
		[ AC_DEFINE(QSE_PTHREAD_MUTEX_T_IS_SIGNED, 1, [Define if pthread_cond_t is signed]) ],
		[ AS_ECHO_N("") ],
		[#include <pthread.h>])
fi

dnl gcc 3.4.3 on opensolaris x86 gave this warning without -msse or 
dnl something similar.
dnl    SSE vector argument without SSE enabled changes the ABI
dnl it also ended with an internal compiler error while compiling 
dnl some expressions like 'v % base' where v is of the __int128_t type.
dnl so i've added this check to determine if the compiler has
dnl proper support for __int128_t.
dnl 
dnl Changing ac_cv_sizeof___int128_t here doesn't corret SIZEOF___INT128_T
dnl since it's already generated. It only affects QSE_SIZEOF___INT128_T below.
if test ${ac_cv_sizeof___int128_t} -gt 0
then
	AC_MSG_CHECKING([__int128_t with % and va_arg])
	AC_COMPILE_IFELSE(
		[AC_LANG_PROGRAM([#include <stdarg.h>], [
		 	va_list ap;
			volatile __int128_t x, base;
			va_arg (ap, __int128_t);
			x = (__int128_t)&base;
			base = (__int128_t)&x;
			x = x % base;
			return (int)x;
		])],

		[
			AC_MSG_RESULT(yes)
		],
		[
			AC_MSG_RESULT(no)
			ac_cv_sizeof___int128_t=0
		]
	)
fi

dnl Some compilers doesn't seem to have full support for __float128 
dnl even if the type is available.
if test ${ac_cv_sizeof___float128} -gt 0
then
	AC_MSG_CHECKING([__float128 with linking])
	AC_TRY_LINK(
		[#include <stdio.h>],
		[
			__float128 x, y, z;
			z = x * y + x;
			printf ("%p", (char*)(int)z);
		],
		[AC_MSG_RESULT(yes)],
		[
			AC_MSG_RESULT(no)
			ac_cv_sizeof___float128=0
		]
	)
fi

dnl Quadmath functions may not be in the default c library.
dnl Find a library containing them. Disable the float128 type
dnl if they are not available.
if test ${ac_cv_sizeof___float128} -gt 0
then
	AC_CHECK_FUNCS([quadmath_snprintf])
	if test "$ac_cv_func_quadmath_snprintf" = "no"
	then
		OLDLIBS="$LIBS"
		LIBS="$LIBM $LIBS"
		AC_CHECK_FUNCS([quadmath_snprintf])
		LIBS="$OLDLIBS"
	
		if test "$ac_cv_func_quadmath_snprintf" = "no"
		then
			dnl quadmath_snprintf not avalable in the 
			dnl standard math lib.

			AC_CHECK_LIB([quadmath], [quadmath_snprintf], [
				QUADMATH_LIBS="-lquadmath"
				LIBM="$LIBM -lquadmath"
				AC_DEFINE(HAVE_QUADMATH_SNPRINTF, 1)
			])

			if test "$ac_cv_lib_quadmath_quadmath_snprintf" = "no"
			then
				dnl quadmath_snprintf not avalable in the 
				dnl quadmath lib.

				ac_cv_sizeof___float128=0
			fi

		else
			QUADMATH_LIBS="$LIBM"
		fi
	fi

	if test ${ac_cv_sizeof___float128} -gt 0
	then
		OLDLIBS="$LIBS"
		LIBS="$LIBM $LIBS"
		AC_CHECK_FUNCS([powq fmodq sinq cosq tanq sinhq coshq tanhq asinq acosq atanq atan2q logq log2q log10q expq sqrtq ceilq floorq roundq])
		AC_CHECK_FUNCS([strtoflt128])
		LIBS="$OLDLIBS"
	fi
fi
AC_SUBST(QUADMATH_LIBS)


AC_DEFINE_UNQUOTED(QSE_SIZEOF_CHAR, ${ac_cv_sizeof_char}, [sizeof(char)])
AC_DEFINE_UNQUOTED(QSE_SIZEOF_SHORT, ${ac_cv_sizeof_short}, [sizeof(short)])
AC_DEFINE_UNQUOTED(QSE_SIZEOF_INT, ${ac_cv_sizeof_int}, [sizeof(int)])
AC_DEFINE_UNQUOTED(QSE_SIZEOF_LONG, ${ac_cv_sizeof_long}, [sizeof(long)])
AC_DEFINE_UNQUOTED(QSE_SIZEOF_LONG_LONG, ${ac_cv_sizeof_long_long}, [sizeof(long long)])
AC_DEFINE_UNQUOTED(QSE_SIZEOF___INT8, ${ac_cv_sizeof___int8}, [sizeof(__int8)])
AC_DEFINE_UNQUOTED(QSE_SIZEOF___INT16, ${ac_cv_sizeof___int16}, [sizeof(__int16)])
AC_DEFINE_UNQUOTED(QSE_SIZEOF___INT32, ${ac_cv_sizeof___int32}, [sizeof(__int32)])
AC_DEFINE_UNQUOTED(QSE_SIZEOF___INT64, ${ac_cv_sizeof___int64}, [sizeof(__int64)])
AC_DEFINE_UNQUOTED(QSE_SIZEOF___INT128, ${ac_cv_sizeof___int128}, [sizeof(__int128)])
AC_DEFINE_UNQUOTED(QSE_SIZEOF___INT128_T, ${ac_cv_sizeof___int128_t}, [sizeof(__int128_t)])
AC_DEFINE_UNQUOTED(QSE_SIZEOF_VOID_P, ${ac_cv_sizeof_void_p}, [sizeof(void*)])
AC_DEFINE_UNQUOTED(QSE_SIZEOF_FLOAT, ${ac_cv_sizeof_float}, [sizeof(float)])
AC_DEFINE_UNQUOTED(QSE_SIZEOF_DOUBLE, ${ac_cv_sizeof_double}, [sizeof(double)])
AC_DEFINE_UNQUOTED(QSE_SIZEOF_LONG_DOUBLE, ${ac_cv_sizeof_long_double}, [sizeof(long double)])
AC_DEFINE_UNQUOTED(QSE_SIZEOF___FLOAT128, ${ac_cv_sizeof___float128}, [sizeof(__float128)])

AC_DEFINE_UNQUOTED(QSE_SIZEOF_WCHAR_T, ${ac_cv_sizeof_wchar_t}, [sizeof(wchar_t)])
AC_DEFINE_UNQUOTED(QSE_SIZEOF_OFF_T, ${ac_cv_sizeof_off_t}, [sizeof(off_t)])
AC_DEFINE_UNQUOTED(QSE_SIZEOF_OFF64_T, ${ac_cv_sizeof_off64_t}, [sizeof(off64_t)])
AC_DEFINE_UNQUOTED(QSE_SIZEOF_MODE_T, ${ac_cv_sizeof_mode_t}, [sizeof(mode_t)])

AC_DEFINE_UNQUOTED(QSE_SIZEOF_MBSTATE_T, ${ac_cv_sizeof_mbstate_t}, [sizeof(mbstate_t)])
AC_DEFINE_UNQUOTED(QSE_MBLEN_MAX, ${ax_cv_numvalof_MB_LEN_MAX}, [MB_LEN_MAX])
if test ${ax_cv_numvalof_PATH_MAX} -gt 0
then
	AC_DEFINE_UNQUOTED(QSE_PATH_MAX, ${ax_cv_numvalof_PATH_MAX}, [PATH_MAX])
fi

AC_DEFINE_UNQUOTED(QSE_NSIGS, ${ax_cv_numvalof_NSIG}, [NSIG])

if test ${ax_cv_numvalof_SOCK_STREAM} -ge 0 
then
	AC_DEFINE_UNQUOTED(QSE_SOCK_STREAM, ${ax_cv_numvalof_SOCK_STREAM}, [SOCK_STREAM])
fi
if test ${ax_cv_numvalof_SOCK_DGRAM} -ge 0 
then
	AC_DEFINE_UNQUOTED(QSE_SOCK_DGRAM, ${ax_cv_numvalof_SOCK_DGRAM}, [SOCK_DGRAM])
fi
if test ${ax_cv_numvalof_SOCK_RAW} -ge 0 
then
	AC_DEFINE_UNQUOTED(QSE_SOCK_RAW, ${ax_cv_numvalof_SOCK_RAW}, [SOCK_RAW])
fi
if test ${ax_cv_numvalof_AF_INET} -ge 0 
then
	AC_DEFINE_UNQUOTED(QSE_AF_INET, ${ax_cv_numvalof_AF_INET}, [AF_INET])
fi
if test ${ax_cv_numvalof_AF_INET6} -ge 0 
then
	AC_DEFINE_UNQUOTED(QSE_AF_INET6, ${ax_cv_numvalof_AF_INET6}, [AF_INET6])
fi
if test ${ax_cv_numvalof_AF_PACKET} -ge 0 
then
	AC_DEFINE_UNQUOTED(QSE_AF_PACKET, ${ax_cv_numvalof_AF_PACKET}, [AF_PACKET])
fi
if test ${ax_cv_numvalof_AF_UNIX} -ge 0 
then
	AC_DEFINE_UNQUOTED(QSE_AF_UNIX, ${ax_cv_numvalof_AF_UNIX}, [AF_UNIX])
fi

AC_DEFINE_UNQUOTED(QSE_SIZEOF_STRUCT_SOCKADDR_IN, ${ac_cv_sizeof_struct_sockaddr_in}, [sizeof(struct sockaddr_in)])
AC_DEFINE_UNQUOTED(QSE_SIZEOF_STRUCT_SOCKADDR_IN6, ${ac_cv_sizeof_struct_sockaddr_in6}, [sizeof(struct sockaddr_in6)])
AC_DEFINE_UNQUOTED(QSE_SIZEOF_STRUCT_SOCKADDR_UN, ${ac_cv_sizeof_struct_sockaddr_un}, [sizeof(struct sockaddr_un)])
AC_DEFINE_UNQUOTED(QSE_SIZEOF_STRUCT_SOCKADDR_LL, ${ac_cv_sizeof_struct_sockaddr_ll}, [sizeof(struct sockaddr_ll)])
AC_DEFINE_UNQUOTED(QSE_SIZEOF_STRUCT_SOCKADDR_DL, ${ac_cv_sizeof_struct_sockaddr_dl}, [sizeof(struct sockaddr_dl)])


AC_DEFINE_UNQUOTED(QSE_SIZEOF_SOCKLEN_T, ${ac_cv_sizeof_socklen_t}, [sizeof(socklen_t)])
AC_DEFINE_UNQUOTED(QSE_SIZEOF_SA_FAMILY_T, ${ac_cv_sizeof_sa_family_t}, [sizeof(sa_family_t)])

AC_DEFINE_UNQUOTED(QSE_SIZEOF_SOCKLEN_T, ${ac_cv_sizeof_socklen_t}, [sizeof(socklen_t)])
AC_DEFINE_UNQUOTED(QSE_SIZEOF_STRUCT_IOVEC, ${ac_cv_sizeof_struct_iovec}, [sizeof(struct iovec)])
AC_DEFINE_UNQUOTED(QSE_SIZEOF_PTHREAD_T, ${ac_cv_sizeof_pthread_t}, [sizeof(pthread_t)])
AC_DEFINE_UNQUOTED(QSE_SIZEOF_PTHREAD_MUTEX_T, ${ac_cv_sizeof_pthread_mutex_t}, [sizeof(pthread_mutex_t)])
AC_DEFINE_UNQUOTED(QSE_SIZEOF_PTHREAD_COND_T, ${ac_cv_sizeof_pthread_cond_t}, [sizeof(pthread_cond_t)])


qse_package_version_major="`echo ${PACKAGE_VERSION} | cut -d. -f1`"
qse_package_version_minor="`echo ${PACKAGE_VERSION} | cut -d. -f2`"
qse_package_version_patch="`echo ${PACKAGE_VERSION} | cut -d. -f3`"
AC_SUBST(PACKAGE_VERSION_MAJOR, ${qse_package_version_major})
AC_SUBST(PACKAGE_VERSION_MINOR, ${qse_package_version_minor})
AC_SUBST(PACKAGE_VERSION_PATCH, ${qse_package_version_patch})

AC_DEFINE_UNQUOTED(QSE_PACKAGE_VERSION, "${PACKAGE_VERSION}", [Package version])
dnl AC_DEFINE_UNQUOTED(QSE_PACKAGE_VERSION_MAJOR, $(echo ${VERSION} | cut -d. -f1), [Major version number])
dnl AC_DEFINE_UNQUOTED(QSE_PACKAGE_VERSION_MINOR, $(echo ${VERSION} | cut -d. -f2), [Minor version number])
dnl AC_DEFINE_UNQUOTED(QSE_PACKAGE_VERSION_PATCH, $(echo ${VERSION} | cut -d. -f3), [Patch level])
AC_DEFINE_UNQUOTED(QSE_PACKAGE_VERSION_MAJOR, ${qse_package_version_major}, [Major version number])
AC_DEFINE_UNQUOTED(QSE_PACKAGE_VERSION_MINOR, ${qse_package_version_minor}, [Minor version number])
AC_DEFINE_UNQUOTED(QSE_PACKAGE_VERSION_PATCH, ${qse_package_version_patch}, [Patch level])

AC_DEFINE_UNQUOTED(QSE_PACKAGE_AUTHOR,"${PACKAGE_BUGREPORT}", [Author])
AC_DEFINE_UNQUOTED(QSE_PACKAGE_NAME,"${PACKAGE_NAME}", [package name])
AC_DEFINE_UNQUOTED(QSE_PACKAGE_URL, "${PACKAGE_URL}", [Project URL])

AC_C_BIGENDIAN(
	[AC_DEFINE([QSE_ENDIAN_BIG],[],[Big Endian])],
	[AC_DEFINE([QSE_ENDIAN_LITTLE],[],[Little Endian])],
	[AC_DEFINE([QSE_ENDIAN_UNKNOWN],[],[Unknown Endian])])

dnl ===== enable-libltdl =====
AC_ARG_ENABLE([libltdl],
	[AS_HELP_STRING([--enable-libltdl],[use libltdl(default. no)])],
	enable_libltdl_is=$enableval,
	enable_libltdl_is=no
)
if test "x${enable_libltdl_is}" = "xyes"
then
	if test "x${ac_cv_header_ltdl_h}" = "xyes"  -a "${LTDL_LIBS}" != ""
	then
		AC_DEFINE([QSE_ENABLE_LIBLTDL],[],[use libltdl when loading a dynamic module])
	else
		enable_libltdl_is="no"
	fi
fi
AM_CONDITIONAL(ENABLE_LIBLTDL, test "x${enable_libltdl_is}" = "xyes")


dnl define extra options
AC_ARG_ENABLE([wchar], [AS_HELP_STRING([--enable-wchar],[Use the wide character type as the default charater type when enabled (default. yes)])],
	enable_wchar_is=$enableval,enable_wchar_is=yes)
test "${ac_cv_sizeof_wchar_t}" = "0" && enable_wchar_is=no
test "${ac_cv_sizeof_wchar_t}" = "1" && enable_wchar_is=no
if test "${enable_wchar_is}" = "yes"
then
	AC_DEFINE([QSE_CHAR_IS_WCHAR],[],[The default character type is the wide character type])
	AC_SUBST(CHAR_MODE, "QSE_CHAR_IS_WCHAR")
else
	AC_DEFINE([QSE_CHAR_IS_MCHAR],[],[The default character type is the multi-byte character type])
	AC_SUBST(CHAR_MODE, "QSE_CHAR_IS_MCHAR")
fi
AM_CONDITIONAL(WCHAR, test "${enable_wchar_is}" = "yes")


AC_ARG_ENABLE([wchar-prefer-char16], [AS_HELP_STRING([--enable-wchar-prefer-char16],[Use char16_t for qse_wchar_t if possible(default. no)])],
	enable_wchar_prefer_char16=$enableval,enable_wchar_prefer_char16=no)
if test "${enable_wchar_prefer_char16}" = "yes"
then
	AC_DEFINE([QSE_WCHAR_PREFER_CHAR16],[],[specify preference for char16_t as qse_wchar_t])
fi


AC_ARG_ENABLE([bundled-unicode], [AS_HELP_STRING([--enable-bundled-unicode],[use thn bundled unicode routines (default. no)])],
	enable_bundled_unicode_is=$enableval,enable_bundled_unicode_is=yes)
if test "${ac_cv_header_wctype_h}" = "no" -o "${ac_cv_func_wctrans}" = "no" -o "${ac_cv_func_wctype}" = "no"
then
	dnl force bundled-unicode to yes if some wchar functions are not available
	enable_bundled_unicode_is=yes
fi
if test "${enable_bundled_unicode_is}" = "yes"
then
	AC_DEFINE([QSE_ENABLE_BUNDLED_UNICODE],[],[use the bundled unicode routines])
fi
AM_CONDITIONAL(ENABLE_BUNDLED_UNICODE, test "${enable_bundled_unicode_is}" = "yes")

AC_ARG_ENABLE([xcmgrs], [AS_HELP_STRING([--enable-xcmgrs],[include more built-in cmgrs like cp949 and cp950 when enabled (default. yes)])],
	enable_xcmgrs_is=$enableval,enable_xcmgrs_is=yes)
if test "${enable_xcmgrs_is}" = "yes"
then
	AC_DEFINE([QSE_ENABLE_XCMGRS],[],[include more built-in cmgrs like cp949 and cp950])
fi
AM_CONDITIONAL(ENABLE_XCMGRS, test "${enable_xcmgrs_is}" = "yes" )

AC_ARG_ENABLE([syscall], [AS_HELP_STRING([--enable-syscall],[use the syscall() function to call system calls (default. no)])],
	enable_syscall_is=$enableval,enable_syscall_is=no)
if test "${enable_syscall_is}" = "yes"
then
	AC_DEFINE([QSE_USE_SYSCALL],[],[use the syscall() function to invoke a system call])
fi

AC_ARG_ENABLE([debug], [AS_HELP_STRING([--enable-debug],[build the library in the debug mode (default. no)])],
	enable_debug_is=$enableval,enable_debug_is=no)
if test "$enable_debug_is" = "yes"
then
	[CFLAGS="$CFLAGS -g -D_DEBUG -UNDEBUG -DQSE_BUILD_DEBUG -U_FORTIFY_SOURCE"]
	[CXXFLAGS="$CXXFLAGS -g -D_DEBUG -UNDEBUG -DQSE_BUILD_DEBUG -U_FORTIFY_SOURCE"]
	AC_SUBST(BUILD_MODE, "debug")
else
	[CFLAGS="$CFLAGS -DNDEBUG -U_DEBUG -DQSE_BUILD_RELEASE -U_FORTIFY_SOURCE"]
	[CXXFLAGS="$CXXFLAGS -DNDEBUG -U_DEBUG -DQSE_BUILD_RELEASE -U_FORTIFY_SOURCE"]
	AC_SUBST(BUILD_MODE, "release")
fi

AC_ARG_ENABLE([cxx], 
	[AS_HELP_STRING([--enable-cxx],[build the library for C++ if a C++ compiler is available (default. yes)])],
	enable_cxx_is=$enableval,
	enable_cxx_is=yes
)

dnl disable c++ if no c++ compiler was found
test "${HAVE_CXX}" = "yes" || enable_cxx_is="no"
dnl disable c++ if the compiler is too old.
test "${ax_cv_cxx_have_std_namespace}" = "yes" || enable_cxx_is="no"

AM_CONDITIONAL(ENABLE_CXX, test "${enable_cxx_is}" = "yes" )

AX_PTHREAD()
AC_ARG_ENABLE([reentrant], [AS_HELP_STRING([--enable-reentrant],[add thread options (default. no)])],
	enable_reentrant_is=$enableval,enable_reentrant_is=no)
if test "$enable_reentrant_is" = "yes"
then
	[CFLAGS="$CFLAGS $PTHREAD_CFLAGS"]
	[CXXFLAGS="$CXXFLAGS $PTHREAD_CFLAGS"]
fi

AC_ARG_ENABLE([awk-intmax], 
	[AS_HELP_STRING([--enable-awk-intmax],[use qse_intmax_t for integers in AWK (default. no)])],
	enable_awk_intmax_is=$enableval,
	enable_awk_intmax_is=no
)
if test "${enable_awk_intmax_is}" = "yes"
then
	AC_DEFINE([QSE_USE_AWK_INTMAX],[],[use qse_intmax_t for integers in AWK])
fi

AC_ARG_ENABLE([awk-fltmax], 
	[AS_HELP_STRING([--enable-awk-fltmax],[use qse_fltmax_t for floating-point numbers in AWK (default. no)])],
	enable_awk_fltmax_is=$enableval,
	enable_awk_fltmax_is=no
)
if test "${enable_awk_fltmax_is}" = "yes"
then
	AC_DEFINE([QSE_USE_AWK_FLTMAX],[],[use qse_fltmax_t for floating-point numbers in AWK])
fi

AC_ARG_ENABLE([sed-tracer],
	[AS_HELP_STRING([--enable-sed-tracer],[enable qse_sed_setexectracer() (default. yes)])],
	enable_sed_tracer_is=$enableval,
	enable_sed_tracer_is=yes
)
if test "${enable_sed_tracer_is}" = "yes"
then
	AC_DEFINE([QSE_ENABLE_SED_TRACER],[],[enable qse_sed_setexectracer()])
fi

dnl ===== enable-static-module =====
AC_ARG_ENABLE([static-module],
	[AS_HELP_STRING([--enable-static-module],[build modules statically into the main library(default. no)])],
	enable_static_module_is=$enableval,
	enable_static_module_is=no
)
if test "x${enable_shared}" = "xno" -a "x${enable_static}" = "xyes"
then
	enable_static_module_is="yes"
fi

if test "${enable_static_module_is}" = "yes"
then
	AC_DEFINE([MOO_ENABLE_STATIC_MODULE],[1],[link modules statically into the main library])
fi
AM_CONDITIONAL(ENABLE_STATIC_MODULE, test "x${enable_static_module_is}" = "xyes")

dnl ==== enable-awkawk-mpi ====
AC_ARG_ENABLE([awkmod-mpi],
	[AS_HELP_STRING([--enable-awkmod-mpi],[enable the mpi module for awk(default. yes)])],
	enable_awkmod_mpi_is=$enableval,
	enable_awkmod_mpi_is=yes
)
if test "x${have_C_mpi}" != "xyes"
then
	enable_awkmod_mpi_is="no"
fi
AM_CONDITIONAL(ENABLE_AWKMOD_MPI, test "x${enable_awkmod_mpi_is}" = "xyes")

dnl ==== enable-awkawk-sed ====
AC_ARG_ENABLE([awkmod-sed],
	[AS_HELP_STRING([--enable-awkmod-sed],[enable the sed module for awk(default. yes)])],
	enable_awkmod_sed_is=$enableval,
	enable_awkmod_sed_is=yes
)
AM_CONDITIONAL(ENABLE_AWKMOD_SED, test "x${enable_awkmod_sed_is}" = "xyes")

dnl ==== enable-awkawk-uci ====
AC_ARG_ENABLE([awkmod-uci],
	[AS_HELP_STRING([--enable-awkmod-uci],[enable the uci module for awk(default. no)])],
	enable_awkmod_uci_is=$enableval,
	enable_awkmod_uci_is=no
)
if test "x${ac_cv_lib_uci_uci_alloc_context}" != "xyes" -o "x${ac_cv_header_uci_h}" = "xyes"
then
	enable_awkmod_uci_is="no"
fi
AM_CONDITIONAL(ENABLE_AWKMOD_UCI, test "x${enable_awkmod_uci_is}" = "xyes")


AC_SUBST(QSE_SIZEOF_WCHAR_T, $ac_cv_sizeof_wchar_t)
AC_SUBST(QSE_SIZEOF_LONG_LONG, $ac_cv_sizeof_long_long)
AC_SUBST(QSE_SIZEOF_LONG, $ac_cv_sizeof_long)
AC_SUBST(QSE_SIZEOF_INT, $ac_cv_sizeof_int)
AC_SUBST(QSE_SIZEOF_SHORT, $ac_cv_sizeof_short)
AC_SUBST(QSE_SIZEOF_CHAR, $ac_cv_sizeof_char)
AC_SUBST(QSE_SIZEOF_VOID_P, $ac_cv_sizeof_void_p)
AC_SUBST(QSE_SIZEOF_FLOAT, $ac_cv_sizeof_float)
AC_SUBST(QSE_SIZEOF_DOUBLE, $ac_cv_sizeof_double)
AC_SUBST(QSE_SIZEOF_LONG_DOUBLE, $ac_cv_sizeof_long_double)
AC_SUBST(QSE_SIZEOF_OFF_T, $ac_cv_sizeof_off_t)
AC_SUBST(QSE_SIZEOF_OFF64_T, $ac_cv_sizeof_off64_t)

AC_SUBST(QSE_PROJECT_AUTHOR, "${PACKAGE_BUGREPORT}")
AC_SUBST(QSE_PROJECT_URL, "${PACKAGE_URL}")

AC_CONFIG_FILES([
	Makefile 
	include/Makefile
	include/qse/Makefile
	include/qse/cmn/Makefile
	include/qse/cry/Makefile
	include/qse/si/Makefile
	include/qse/awk/Makefile
	include/qse/sed/Makefile
	include/qse/xli/Makefile
	include/qse/http/Makefile
	include/qse/rad/Makefile
	include/qse/dhcp/Makefile
	include/qse/sttp/Makefile
	lib/Makefile 
	lib/cmn/Makefile 
	lib/cry/Makefile
	lib/si/Makefile 
	lib/awk/Makefile 
	lib/awkmod/Makefile
	lib/sed/Makefile 
	lib/xli/Makefile 
	lib/http/Makefile 
	lib/rad/Makefile 
	lib/dhcp/Makefile 
	lib/sttp/Makefile
	lib/qsecmn.pc
	lib/qsecry.pc
	lib/qsesi.pc
	lib/qsesed.pc
	lib/qseawk.pc
	lib/qsexli.pc
	lib/qsehttp.pc
	lib/qserad.pc
	lib/qsedhcp.pc
	cmd/Makefile 
	cmd/awk/Makefile 
	cmd/sed/Makefile 
	cmd/xli/Makefile 
	cmd/http/Makefile 
	samples/Makefile 
	samples/cmn/Makefile 
	samples/cry/Makefile 
	samples/si/Makefile 
	samples/awk/Makefile 
	samples/sed/Makefile 
	samples/xli/Makefile 
	samples/http/Makefile
	samples/rad/Makefile
	samples/dhcp/Makefile
	regress/Makefile
	regress/awk/Makefile
	regress/awk/regress.sh
	regress/sed/Makefile
	regress/sed/regress.sh
	doc/Makefile
	doc/Doxyfile
	tools/Makefile
])
AC_OUTPUT

[
echo 
echo "-[SUMMARY]---------------------------------------------------------------"
echo "Configured for ${host}"
echo "  Build mode : ${BUILD_MODE}"
echo "  Source directory: ${srcdir}"
echo "  Installation directory: ${prefix}"
echo "  C compiler: ${CC} ${CFLAGS}"
echo "  C++ compiler: ${CXX} ${CXXFLAGS}"
echo "  C++ support: ${enable_cxx_is}"
echo "  Wide character: ${enable_wchar_is}" 
echo "  Math library: ${LIBM}"
echo "  Pthread library: ${PTHREAD_LIBS}"
echo "  Socket library: ${SOCKET_LIBS}"
echo "  Sendfile library: ${SENDFILE_LIBS}"
echo "  SSL library: ${SSL_LIBS}"
echo "-------------------------------------------------------------------------"
]
