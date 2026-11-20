/* expat_config.h for LDD64 (not part of the expat release): written by hand in place of the file
   CMake generates, for the two targets of LDD64. Settings as in LDD 4.3.11 (namespaces, DTDs,
   1024 bytes of context). Entropy for the hash salt: rand_s on Windows (random_rand_s.c),
   arc4random_buf on macOS and on glibc >= 2.36 (random_arc4random_buf.c). */
#ifndef EXPAT_CONFIG_H
#  define EXPAT_CONFIG_H 1

#  define BYTEORDER 1234
#  define XML_CONTEXT_BYTES 1024
#  define XML_DTD 1
#  define XML_GE 1
#  define XML_NS 1

#  define HAVE_STDINT_H 1
#  define HAVE_STDLIB_H 1
#  define HAVE_STRING_H 1
#  define HAVE_INTTYPES_H 1
#  define HAVE_SYS_STAT_H 1
#  define HAVE_SYS_TYPES_H 1
#  define STDC_HEADERS 1

#  if defined(_WIN32)
/* rand_s: random_rand_s.c */
#  else
#    define HAVE_ARC4RANDOM_BUF 1
#    define HAVE_DLFCN_H 1
#    define HAVE_FCNTL_H 1
#    define HAVE_GETPAGESIZE 1
#    define HAVE_MMAP 1
#    define HAVE_STRINGS_H 1
#    define HAVE_UNISTD_H 1
#  endif

#  define PACKAGE "expat"
#  define PACKAGE_BUGREPORT "https://github.com/libexpat/libexpat/issues"
#  define PACKAGE_NAME "expat"
#  define PACKAGE_STRING "expat 2.10.0"
#  define PACKAGE_TARNAME "expat"
#  define PACKAGE_URL ""
#  define PACKAGE_VERSION "2.10.0"

#endif /* ndef EXPAT_CONFIG_H */
