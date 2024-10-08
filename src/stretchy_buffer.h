// stretchy_buffer.h - v1.04 - public domain - nothings.org/stb
// a vector<>-like dynamic array for C
//
// version history:
//      1.04 -  fix warning
//      1.03 -  compile as C++ maybe
//      1.02 -  tweaks to syntax for no good reason
//      1.01 -  added a "common uses" documentation section
//      1.0  -  fixed bug in the version I posted prematurely
//      0.9  -  rewrite to try to avoid strict-aliasing optimization
//              issues, but won't compile as C++
//
// Will probably not work correctly with strict-aliasing optimizations.
//
// The idea:
//
//    This implements an approximation to C++ vector<> for C, in that it
//    provides a generic definition for dynamic arrays which you can
//    still access in a typesafe way using arr[i] or *(arr+i). However,
//    it is simply a convenience wrapper around the common idiom of
//    of keeping a set of variables (in a struct or globals) which store
//        - pointer to array
//        - the length of the "in-use" part of the array
//        - the current size of the allocated array
//
//    I find it to be the single most useful non-built-in-structure when
//    programming in C (hash tables a close second), but to be clear
//    it lacks many of the capabilities of C++ vector<>: there is no
//    range checking, the object address isn't stable (see next section
//    for details), the set of methods available is small (although
//    the file stb.h has another implementation of stretchy buffers
//    called 'stb_arr' which provides more methods, e.g. for insertion
//    and deletion).
//
// How to use:
//
//    Unlike other stb header file libraries, there is no need to
//    define an _IMPLEMENTATION symbol. Every #include creates as
//    much implementation is needed.
//
//    stretchy_buffer.h does not define any types, so you do not
//    need to #include it to before defining data types that are
//    stretchy buffers, only in files that *manipulate* stretchy
//    buffers.
//
//    If you want a stretchy buffer aka dynamic array containing
//    objects of TYPE, declare such an array as:
//
//       TYPE *myarray = NULL;
//
//    (There is no typesafe way to distinguish between stretchy
//    buffers and regular arrays/pointers; this is necessary to
//    make ordinary array indexing work on these objects.)
//
//    Unlike C++ vector<>, the stretchy_buffer has the same
//    semantics as an object that you manually malloc and realloc.
//    The pointer may relocate every time you add a new object
//    to it, so you:
//
//         1. can't take long-term pointers to elements of the array
//         2. have to return the pointer from functions which might expand it
//            (either as a return value or by storing it to a ptr-to-ptr)
//
//    Now you can do the following things with this array:
//
//         sb_free(TYPE *a)           free the array
//         sb_count(TYPE *a)          the number of elements in the array
//         sb_push(TYPE *a, TYPE v)   adds v on the end of the array, a la push_back
//         sb_add(TYPE *a, int n)     adds n uninitialized elements at end of array & returns pointer to first added
//         sb_last(TYPE *a)           returns an lvalue of the last item in the array
//         a[n]                       access the nth (counting from 0) element of the array
//
//     #define STRETCHY_BUFFER_NO_SHORT_NAMES to only export
//     names of the form 'stb_sb_' if you have a name that would
//     otherwise collide.
//
//     Note that these are all macros and many of them evaluate
//     their arguments more than once, so the arguments should
//     be side-effect-free.
//
//     Note that 'TYPE *a' in sb_push and sb_add must be lvalues
//     so that the library can overwrite the existing pointer if
//     the object has to be reallocated.
//
//     In an out-of-memory condition, the code will try to
//     set up a null-pointer or otherwise-invalid-pointer
//     exception to happen later. It's possible optimizing
//     compilers could detect this write-to-null statically
//     and optimize away some of the code, but it should only
//     be along the failure path. Nevertheless, for more security
//     in the face of such compilers, #define STRETCHY_BUFFER_OUT_OF_MEMORY
//     to a statement such as assert(0) or exit(1) or something
//     to force a failure when out-of-memory occurs.
//
// Common use:
//
//    The main application for this is when building a list of
//    things with an unknown quantity, either due to loading from
//    a file or through a process which produces an unpredictable
//    number.
//
//    My most common idiom is something like:
//
//       SomeStruct *arr = NULL;
//       while (something)
//       {
//          SomeStruct new_one;
//          new_one.whatever = whatever;
//          new_one.whatup   = whatup;
//          new_one.foobar   = barfoo;
//          sb_push(arr, new_one);
//       }
//
//    and various closely-related factorings of that. For example,
//    you might have several functions to create/init new SomeStructs,
//    and if you use the above idiom, you might prefer to make them
//    return structs rather than take non-const-pointers-to-structs,
//    so you can do things like:
//
//       SomeStruct *arr = NULL;
//       while (something)
//       {
//          if (case_A) {
//             sb_push(arr, some_func1());
//          } else if (case_B) {
//             sb_push(arr, some_func2());
//          } else {
//             sb_push(arr, some_func3());
//          }
//       }
//
//    Note that the above relies on the fact that sb_push doesn't
//    evaluate its second argument more than once. The macros do
//    evaluate the *array* argument multiple times, and numeric
//    arguments may be evaluated multiple times, but you can rely
//    on the second argument of sb_push being evaluated only once.
//
//    Of course, you don't have to store bare objects in the array;
//    if you need the objects to have stable pointers, store an array
//    of pointers instead:
//
//       SomeStruct **arr = NULL;
//       while (something)
//       {
//          SomeStruct *new_one = malloc(sizeof(*new_one));
//          new_one->whatever = whatever;
//          new_one->whatup   = whatup;
//          new_one->foobar   = barfoo;
//          sb_push(arr, new_one);
//       }
//
// How it works:
//
//    A long-standing tradition in things like malloc implementations
//    is to store extra data before the beginning of the block returned
//    to the user. The stretchy buffer implementation here uses the
//    same trick; the current-count and current-allocation-size are
//    stored before the beginning of the array returned to the user.
//    (This means you can't directly free() the pointer, because the
//    allocated pointer is different from the type-safe pointer provided
//    to the user.)
//
//    The details are trivial and implementation is straightforward;
//    the main trick is in realizing in the first place that it's
//    possible to do this in a generic, type-safe way in C.
//
// Contributors:
//
// Timothy Wright (github:ZenToad)
//
// LICENSE
//
//   See end of file for license information.

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#ifndef STB_STRETCHY_BUFFER_H_INCLUDED
#define STB_STRETCHY_BUFFER_H_INCLUDED

// how many int's are packed in before the beginning of the stretchy_buffer
// (i like to keep this number even so struct members are 64-byte aligned)
#define SB_HEADER_COUNT 4

// the defaults for each of them, when initializing a new stretchy_buffer
// (the first two should always be 0; all values after that are udata)
// (the number of elements should match SB_HEADER_COUNT)
#define SB_HEADER_DEFAULT_UDATA_INSTEAD -1
#define SB_HEADER_DEFAULT_UDATA_SEGADDR 0
#define SB_HEADER_DEFAULTS { 0, 0, SB_HEADER_DEFAULT_UDATA_INSTEAD, SB_HEADER_DEFAULT_UDATA_SEGADDR }

// aliases for each
#define sb_udata_instead stb__sb2
#define sb_udata_segaddr stb__sb3

#define sb_array(TYPE, NAME) TYPE *NAME

#define sb_foreach(V, CODE) for (int eachIndex = 0; eachIndex < sb_count(V); ++eachIndex) { typeof((V)[0]) *each = &(V)[eachIndex]; CODE }

#define sb_foreach_backwards(V, CODE) for (int eachIndex = sb_count(V); eachIndex > 0; --eachIndex) { typeof((V)[0]) *each = &(V)[eachIndex - 1]; CODE }

#define sb_foreach_named(V, NAME, CODE) for (int NAME##Index = 0; NAME##Index < sb_count(V); ++NAME##Index) { typeof((V)[0]) *NAME = &(V)[NAME##Index]; CODE }

#ifndef NO_STRETCHY_BUFFER_SHORT_NAMES
#define sb_free   stb_sb_free
#define sb_push   stb_sb_push
#define sb_insert stb_sb_insert
#define sb_remove stb_sb_remove
#define sb_pop    stb_sb_pop
#define sb_count  stb_sb_count
#define sb_add    stb_sb_add
#define sb_last   stb_sb_last
#define sb_clear  stb_sb_clear
#define sb_trim   stb_sb_trim
#define sb_new    stb_sb_new
#define sb_reverse stb_sb_reverse
#define sb_reverse_section stb_sb_reverse_section
#define sb_rotate stb_sb_rotate
#define sb_swap   stb_sb_swap
#define sb_swap_safe stb_sb_swap_safe
#define sb_contains stb_sb_contains
#define sb_new_size stb_sb_new_size
#define sb_contains_ref stb_sb_contains_ref
#define sb_contains_copy stb_sb_contains_copy
#define sb_find_decl stb_sb_find_decl
#define sb_find_impl stb_sb_find_impl
#endif

#define stb_sb_free(a)         ((a) ? free(stb__sbraw(a)),0 : 0)
#define stb_sb_push(a,v)       (stb__sbmaybegrow(a,1), (a)[stb__sbn(a)++] = (v))
//#define stb_sb_push(a,v)       {stb__sbmaybegrow(a,1); (a)[stb__sbn(a)++] = (v);} // safer?
#define stb_sb_pop(a)          (((a) && stb__sbn(a)) ? (a)[--stb__sbn(a)] : (typeof(*(a))){0})
#define stb_sb_count(a)        ((a) ? stb__sbn(a) : 0)
#define stb_sb_add(a,n)        (stb__sbmaybegrow(a,n), stb__sbn(a)+=(n), &(a)[stb__sbn(a)-(n)])
#define stb_sb_last(a)         ((a)[stb__sbn(a)-1])
//#define stb_sb_clear(a)        ((a) ? stb__sbn(a) = 0 : (void)0)
#define stb_sb_clear(a)        { if (a) stb__sbn(a) = 0; } // c++
#define stb_sb_trim(a,newsize) { if (a && stb__sbn(a) > newsize) stb__sbn(a) = newsize; }
#define stb_sb_new(a)          { (void)stb_sb_add(a, 1); stb_sb_clear(a); }
#define stb_sb_new_size(a,n)   { (void)stb_sb_add(a, ((n)+1)); stb_sb_clear(a); }
//#define stb_sb_new_size(a,n)   { stb__sbgrow(a, ((n)+1)); stb_sb_clear(a); } // just experimenting
//#define stb_sb_new_size(a,n)   { for (int i = 0; i < ((n) + 1); ++i) sb_push(a, (typeof(*(a))){0}); stb_sb_clear(a); }
#define stb_sb_contains(HAYSTACK, NEEDLE) ((sb_find_index)(HAYSTACK, NEEDLE, sizeof(*(NEEDLE))) >= 0)
#define stb_sb_contains_ref(HAYSTACK, NEEDLE) ((sb_find_ref)(HAYSTACK, NEEDLE, sizeof(&(NEEDLE))) >= 0)
#define stb_sb_contains_copy(HAYSTACK, NEEDLE) (sb_find_copy(HAYSTACK, NEEDLE) >= 0)
#ifdef __cplusplus
	// C++ has no typeof, so work around it
	#define stb_sb_swap(a,index0,index1) { \
		sb_push(a, a[index1]); \
		a[index1] = a[index0]; \
		a[index0] = sb_last(a); \
		--stb__sbn(a); \
	}
#else
	#define stb_sb_swap(a,index0,index1) { \
		(typeof(*(a))) tmp = a[index1]; \
		a[index1] = a[index0]; \
		a[index0] = tmp; \
	}
#endif
#define stb_sb_swap_safe(a,index0,index1) { \
	if (a \
		&& index0 >= 0 && index0 < sb_count(a) \
		&& index1 >= 0 && index1 < sb_count(a) \
	) { \
		stb_sb_swap(a, index0, index1) \
	} \
}

#define stb_sb_find_decl(FUNCNAME, HAYSTACK_TYPE, NEEDLE_TYPE) \
	HAYSTACK_TYPE *FUNCNAME(sb_array(HAYSTACK_TYPE, haystack), NEEDLE_TYPE needle)
#define stb_sb_find_impl(FUNCNAME, HAYSTACK_TYPE, NEEDLE_TYPE, CRITERIA) \
	stb_sb_find_decl(FUNCNAME, HAYSTACK_TYPE, NEEDLE_TYPE) { \
		sb_foreach(haystack, { \
			if (CRITERIA) \
				return each; \
		}) \
		return 0; \
	}

#define stb_sb_insert(a, v, index) { \
	stb__sbmaybegrow(a,1); \
	for (int i = stb__sbn(a); i > index; --i) \
		(a)[i] = (a)[(i) - 1]; \
	(a)[index] = (v); \
	stb__sbn(a)++; \
}

#define stb_sb_remove(a, index) { \
	for (int i = index; i < stb__sbn(a) - 1; ++i) \
		(a)[i] = (a)[i + 1]; \
	stb__sbn(a)--; \
}

#define stb_sb_reverse_section(X, START, END) if (stb_sb_count(X) > 1) { \
	int start = START; \
	int end = END; \
	while (start < end) { \
		typeof(*(X)) tmp = X[start]; \
		X[start] = X[end]; \
		X[end] = tmp; \
		++start; \
		--end; \
	} \
}

#define stb_sb_reverse(X) stb_sb_reverse_section(X, 0, stb_sb_count(X) - 1)
/*
#define stb_sb_reverse(X) if (sb_count(X) > 1) { \
	int num = sb_count(X); \
	for (int i = 0; i < num / 2; ++i) { \
		typeof(*(X)) tmp = X[i]; \
		X[i] = X[(num - 1) - i]; \
		X[(num - 1) - i] = tmp; \
	} \
}
*/

// moves array left so that index 'D' is moved to beginning
#define stb_sb_rotate(X, D) if (sb_count(X) > 1) { \
	int size = sb_count(X); \
	int nudgeIndex = (D) % size; \
	sb_reverse_section(X, 0, nudgeIndex - 1); \
	sb_reverse_section(X, nudgeIndex, size - 1); \
	sb_reverse(X); \
}

#define stb__sbraw(a) ((int *) (void *) (a) - SB_HEADER_COUNT)
#define stb__sbm(a)   stb__sbraw(a)[0]
#define stb__sbn(a)   stb__sbraw(a)[1]
#define stb__sb2(a)   stb__sbraw(a)[2]
#define stb__sb3(a)   stb__sbraw(a)[3]

#define stb__sbneedgrow(a,n)  ((a)==0 || stb__sbn(a)+(n) >= stb__sbm(a))
#define stb__sbmaybegrow(a,n) (stb__sbneedgrow(a,(n)) ? stb__sbgrow(a,n) : 0)
#define stb__sbgrow(a,n)      (*((void **)&(a)) = stb__sbgrowf((a), (n), sizeof(*(a))))

#include <stdlib.h>
#include <stdint.h>

// expects needle to reference haystack[0..n]
#define sb_find_index(HAYSTACK, NEEDLE) sb_find_index(HAYSTACK, NEEDLE, sizeof(*(NEEDLE)))
static int (sb_find_index)(const void *haystack, const void *needle, const int sizeofEach)
{
	int len = sb_count(haystack);
	const uint8_t *addr = (const uint8_t*)haystack;
	
	for (int i = 0; i < len; ++i, addr += sizeofEach)
		if ((const void*)addr == needle)
			return i;
	
	return -1;
}

// expects haystack[0..n] to be reference to needle
#define sb_find_ref(HAYSTACK, NEEDLE) sb_find_ref(HAYSTACK, NEEDLE, sizeof(*(NEEDLE)))
static int (sb_find_ref)(const void *haystack, const void *needle, const int sizeofEach)
{
	int len = sb_count(haystack);
	const uint8_t *addr = (const uint8_t*)haystack;
	
	assert(sizeofEach == sizeof(void*));
	
	for (int i = 0; i < len; ++i, addr += sizeofEach)
		if (*(const uintptr_t*)addr == (uintptr_t)needle)
			return i;
	
	return -1;
}

// expects haystack[0..n] to be reference to needle
#define sb_find_copy(HAYSTACK, NEEDLE) sb_find_copy(HAYSTACK, (const void*)&(NEEDLE), sizeof(NEEDLE))
static int (sb_find_copy)(const void *haystack, const void *needle, const int sizeofEach)
{
	int len = sb_count(haystack);
	const uint8_t *addr = (const uint8_t*)haystack;
	
	for (int i = 0; i < len; ++i, addr += sizeofEach)
		if (!memcmp(addr, needle, sizeofEach))
			return i;
	
	return -1;
}

static void * stb__sbgrowf(void *arr, int increment, int itemsize)
{
   int dbl_cur = arr ? 2*stb__sbm(arr) : 0;
   int min_needed = stb_sb_count(arr) + increment;
   int m = dbl_cur > min_needed ? dbl_cur : min_needed;
   int *p = (int *) realloc(arr ? stb__sbraw(arr) : 0, itemsize * m + sizeof(int)*SB_HEADER_COUNT);
   if (p) {
      const int defaults[] = SB_HEADER_DEFAULTS;
      assert(sizeof(defaults) / sizeof(*defaults) == SB_HEADER_COUNT);
      if (!arr)
         for (int i = 1; i < SB_HEADER_COUNT; ++i)
            p[i] = defaults[i];
      p[0] = m;
      return p + SB_HEADER_COUNT;
   } else {
      #ifdef STRETCHY_BUFFER_OUT_OF_MEMORY
      STRETCHY_BUFFER_OUT_OF_MEMORY ;
      #endif
      return (void *) (SB_HEADER_COUNT*sizeof(int)); // try to force a NULL pointer exception later
   }
}
#endif // STB_STRETCHY_BUFFER_H_INCLUDED


/*
------------------------------------------------------------------------------
This software is available under 2 licenses -- choose whichever you prefer.
------------------------------------------------------------------------------
ALTERNATIVE A - MIT License
Copyright (c) 2017 Sean Barrett
Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
------------------------------------------------------------------------------
ALTERNATIVE B - Public Domain (www.unlicense.org)
This is free and unencumbered software released into the public domain.
Anyone is free to copy, modify, publish, use, compile, sell, or distribute this
software, either in source code form or as a compiled binary, for any purpose,
commercial or non-commercial, and by any means.
In jurisdictions that recognize copyright laws, the author or authors of this
software dedicate any and all copyright interest in the software to the public
domain. We make this dedication for the benefit of the public at large and to
the detriment of our heirs and successors. We intend this dedication to be an
overt act of relinquishment in perpetuity of all present and future rights to
this software under copyright law.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
------------------------------------------------------------------------------
*/
