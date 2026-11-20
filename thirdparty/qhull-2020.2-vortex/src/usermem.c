/*<html><pre>  -<a                             href="qh-user.htm"
  >-------------------------------</a><a name="TOP">-</a>

   usermem.c
   user redefinable functions -- qh_exit, qh_free, and qh_malloc

   See README.txt.

   If you redefine one of these functions you must redefine all of them.
   If you recompile and load this file, then usermem.o will not be loaded
   from qhull.a or qhull.lib

   See libqhull.h for data structures, macros, and user-callable functions.
   See user.c for qhull-related, redefinable functions
   see user.h for user-definable constants
   See userprintf.c for qh_fprintf and userprintf_rbox.c for qh_fprintf_rbox

   Please report any errors that you fix to qhull@qhull.org
*/

#include "libqhull.h"

#include <stdarg.h>
#include <stdlib.h>

/* VORTEX: CM Labs' Vortex routes all qhull memory through its MeMemoryAPI table. Define
   qh_VORTEX_MEMORY to the header that declares it and qh_MEcreate / qh_MEcreateZeroed /
   qh_MEdestroy to its entries; qh_VORTEX (defined by the projects) selects Me/MeMemory.h and the
   MeMemoryAPI entries.
   Without it the CRT functions are used, which is what Vortex's default table does. qhull itself
   then has no direct malloc / calloc / free call left (mem.c and global.c call qh_calloc). */
#if defined(qh_VORTEX) && !defined(qh_VORTEX_MEMORY)
#define qh_VORTEX_MEMORY "Me/MeMemory.h"
#define qh_MEcreate(n)       MeMemoryAPI.create(n)
#define qh_MEcreateZeroed(n) MeMemoryAPI.createZeroed(n)
#define qh_MEdestroy(p)      MeMemoryAPI.destroy(p)
#endif
#if defined(qh_VORTEX_MEMORY)
#include qh_VORTEX_MEMORY
#endif
#ifndef qh_MEcreate
#define qh_MEcreate(size)         malloc(size)
#define qh_MEcreateZeroed(size)   calloc((size_t)1, size)
#define qh_MEdestroy(mem)         free(mem)
#endif

/*-<a                             href="qh-user.htm#TOC"
  >-------------------------------</a><a name="qh_exit">-</a>

  qh_exit( exitcode )
    exit program
    the exitcode must be 255 or less.  Zero indicates success.
    Note: Exit status ('$?') in bash reports 256 as 0

  notes:
    qh_exit() is called when qh_errexit() and longjmp() are not available.

    This is the only use of exit() in Qhull
    To replace qh_exit with 'throw', see libqhullcpp/usermem_r-cpp.cpp
*/
void qh_exit(int exitcode) {
    exit(exitcode);
} /* exit */

/*-<a                             href="qh-user.htm#TOC"
  >-------------------------------</a><a name="qh_fprintf_stderr">-</a>

  qh_fprintf_stderr( msgcode, format, list of args )
    fprintf to stderr with msgcode (non-zero)

  notes:
    qh_fprintf_stderr() is called when qh.ferr is not defined, usually due to an initialization error
    if msgcode is a MSG_ERROR (6000), caller should set qh.last_errcode (like qh_fprintf) or variable 'last_errcode'
    
    It is typically followed by qh_errexit().

    Redefine this function to avoid using stderr

    Use qh_fprintf [userprintf.c] for normal printing
*/
void qh_fprintf_stderr(int msgcode, const char *fmt, ... ) {
    va_list args;

    va_start(args, fmt);
    if(msgcode)
      fprintf(stderr, "QH%.4d ", msgcode);
    vfprintf(stderr, fmt, args);
    va_end(args);
} /* fprintf_stderr */

/*-<a                             href="qh-user.htm#TOC"
>-------------------------------</a><a name="qh_free">-</a>

  qh_free( mem )
    free memory

  notes:
    same as free()
    No calls to qh_errexit() 
*/
void qh_free(void *mem) {
    qh_MEdestroy(mem);  /* VORTEX: was free(mem) */
} /* free */

/*-<a                             href="qh-user.htm#TOC"
    >-------------------------------</a><a name="qh_malloc">-</a>

    qh_malloc( mem )
      allocate memory

    notes:
      same as malloc()
*/
void *qh_malloc(size_t size) {
    return qh_MEcreate(size);  /* VORTEX: was malloc(size) */
} /* malloc */

/*-<a                             href="qh-user.htm#TOC"
    >-------------------------------</a><a name="qh_calloc">-</a>

    qh_calloc( nmemb, size )
      allocate zeroed memory

    notes:
      VORTEX: added by CM Labs; replaces the calloc() calls of qh_meminit and qh_initflags ('Hn,n,n')
*/
void *qh_calloc(size_t nmemb, size_t size) {
    return qh_MEcreateZeroed(nmemb * size);
} /* calloc */


