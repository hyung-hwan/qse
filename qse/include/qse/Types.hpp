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

#ifndef _QSE_TYPES_HPP_
#define _QSE_TYPES_HPP_

/// \file
/// Defines aliases to various QSE types in a class and holds various feature
/// configuration options.

#include <qse/types.h>
#include <qse/macros.h>

// The QSE_CPP_CALL_DESTRUCTOR() macro calls a destructor explicitly.
// The QSE_CPP_CALL_PLACEMENT_DELETE1() macro calls the global operator delete
// with 1 extra argument given.

#if (__cplusplus >= 201103L) // C++11

	/// The QSE_ENABLE_CPP11_MOVE macro enables C++11 move semantics
	/// in various classes.
	#define QSE_ENABLE_CPP11_MOVE 1

	#define QSE_CPP_CALL_DESTRUCTOR(ptr, class_name) ((ptr)->~class_name())
	#define QSE_CPP_CALL_PLACEMENT_DELETE1(ptr, arg1) (::operator delete ((ptr), (arg1)))

#elif (__cplusplus >= 199711L) // C++98

	#define QSE_CPP_CALL_DESTRUCTOR(ptr, class_name) ((ptr)->~class_name())
	#define QSE_CPP_CALL_PLACEMENT_DELETE1(ptr, arg1) (::operator delete ((ptr), (arg1)))

#else

	#if defined(__BORLANDC__)

		// Explicit destructor call requires a class name depending on the
		// C++ standard/compiler.  
		// 
		//   Node* x;
		//   x->~Node (); 
		//
		// While x->~Node() is ok with modern compilers, some old compilers
		// like BCC55 required the class name in the call as shown below.
		//
		//   x->Node::~Node ();

		#define QSE_CPP_CALL_DESTRUCTOR(ptr, class_name) ((ptr)->class_name::~class_name())
		#define QSE_CPP_CALL_PLACEMENT_DELETE1(ptr, arg1) (::operator delete ((ptr), (arg1)))

	#elif defined(__WATCOMC__)
		// WATCOM has a problem with this syntax.
		//    Node* x; x->Node::~Node(). 
		// But it doesn't support operator delete overloading.

		#define QSE_CPP_CALL_DESTRUCTOR(ptr, class_name) ((ptr)->~class_name())
		#define QSE_CPP_CALL_PLACEMENT_DELETE1(ptr, arg1) (::operator_delete ((ptr), (arg1)))

	#else

		#define QSE_CPP_CALL_DESTRUCTOR(ptr, class_name) ((ptr)->~class_name())
		#define QSE_CPP_CALL_PLACEMENT_DELETE1(ptr, arg1) (::operator delete ((ptr), (arg1)))

	#endif

#endif

#if defined(QSE_ENABLE_CPP11_MOVE)

	template<typename T> struct QSE_CPP_RMREF      { typedef T Type; };
	template<typename T> struct QSE_CPP_RMREF<T&>  { typedef T Type; };
	template<typename T> struct QSE_CPP_RMREF<T&&> { typedef T Type; };

	template<typename T> inline
	typename QSE_CPP_RMREF<T>::Type&& QSE_CPP_RVREF(T&& v) 
	{
		return (typename QSE_CPP_RMREF<T>::Type&&)v; 
	}
#else

	/*
	template<typename T> inline
	T& QSE_CPP_RVREF(T& v) { return (T&)v; }

	template<typename T> inline
	const T& QSE_CPP_RVREF(const T& v) { return (const T&)v; }
	*/
	#define QSE_CPP_RVREF(x) x
#endif


/////////////////////////////////
QSE_BEGIN_NAMESPACE(QSE)
/////////////////////////////////

/**
 * The Types class defines handy aliases for various QSE types.
 */
class QSE_EXPORT Types
{
public:
	/** boolean data type */
	typedef qse_bool_t  bool_t;

	/** data type that can hold any character */
	typedef qse_char_t  char_t;

	/** data type that can hold any character or an end-of-file value */
	typedef qse_cint_t  cint_t;

	/** redefines an unsigned integer number of the same size as void* */
	typedef qse_size_t  size_t;

	/** signed version of size_t */
	typedef qse_ssize_t ssize_t;

	/** redefines qse_long_t */
	typedef qse_long_t  long_t;

	/** redefines qse_ulong_t */
	typedef qse_ulong_t  ulong_t;

	/** redefines qse_intptr_t */
	typedef qse_intptr_t intptr_t;

	/** redefines qse_uintptr_t */
	typedef qse_uintptr_t uintptr_t;

	/** redefines qse_intmax_t */
	typedef qse_intmax_t intmax_t;

	/** redefines qse_uintmax_t */
	typedef qse_uintmax_t uintmax_t;

	/** redefines a floating-point number */
	typedef qse_flt_t flt_t;

	/** redefines a structure of a character pointer and length */
	typedef qse_cstr_t cstr_t;

	/** redefines a structure of a character pointer and length */
	typedef qse_wcstr_t wcstr_t;

	/** redefines a structure of a character pointer and length */
	typedef qse_mcstr_t mcstr_t;
};

/////////////////////////////////
QSE_END_NAMESPACE(QSE)
/////////////////////////////////

#endif
