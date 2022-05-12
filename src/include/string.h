#ifndef _STRING_H
#define _STRING_H

/** @file
 *
 * String functions
 *
 */

//#include <compiler.h>

// FILE_LICENCE ( GPL2_OR_LATER_OR_UBDL );

#include <stddef.h>

extern void * generic_memset ( void *dest, int character, 
			       size_t len ) __attribute__ (( nonnull ));
extern void * generic_memcpy ( void *dest, const void *src,
			       size_t len ) __attribute__ (( nonnull ));
extern void * generic_memcpy_reverse ( void *dest, const void *src,
				       size_t len ) __attribute__ (( nonnull ));
extern void * generic_memmove ( void *dest, const void *src,
				size_t len ) __attribute__ (( nonnull ));

#include <bits/string.h>

/* Architecture-specific code is expected to provide these functions,
 * but may instead explicitly choose to use the generic versions.
 */
void * memset ( void *dest, int character, size_t len ) __attribute__ (( nonnull ));
void * memcpy ( void *dest, const void *src, size_t len ) __attribute__ (( nonnull ));
void * memmove ( void *dest, const void *src, size_t len ) __attribute__ (( nonnull ));

extern int __attribute__ (( pure )) memcmp ( const void *first, const void *second,
			   size_t len ) __attribute__ (( nonnull ));
extern void * __attribute__ (( pure )) memchr ( const void *src, int character,
			      size_t len ) __attribute__ (( nonnull ));
extern void * memswap ( void *dest, void *src, size_t len ) __attribute__ (( nonnull ));
extern int __attribute__ (( pure )) strcmp ( const char *first, const char *second ) __attribute__ (( nonnull ));
extern int __attribute__ (( pure )) strncmp ( const char *first, const char *second,
			    size_t max ) __attribute__ (( nonnull ));
extern size_t __attribute__ (( pure )) strlen ( const char *src ) __attribute__ (( nonnull ));
extern size_t __attribute__ (( pure )) strcspn (const char * str1, const char * str2) __attribute__ (( nonnull ));
extern size_t __attribute__ (( pure )) strspn(const char *str, const char *chars) __attribute__ (( nonnull ));
extern size_t __attribute__ (( pure )) strnlen ( const char *src, size_t max ) __attribute__ (( nonnull ));
extern char * __attribute__ (( pure )) strchr ( const char *src, int character ) __attribute__ (( nonnull ));
extern char * __attribute__ (( pure )) strrchr ( const char *src, int character ) __attribute__ (( nonnull ));
extern char * __attribute__ (( pure )) strstr ( const char *haystack,
			      const char *needle ) __attribute__ (( nonnull ));
extern char * strcpy ( char *dest, const char *src ) __attribute__ (( nonnull ));
extern char * strncpy ( char *dest, const char *src, size_t max ) __attribute__ (( nonnull ));
extern char * strcat ( char *dest, const char *src ) __attribute__ (( nonnull ));
extern char * __attribute__ (( malloc )) strdup ( const char *src ) __attribute__ (( nonnull ));
extern char * __attribute__ (( malloc )) strndup ( const char *src, size_t max ) __attribute__ (( nonnull ));
extern char * __attribute__ (( pure )) strpbrk ( const char *string,
			       const char *delim ) __attribute__ (( nonnull ));
extern char * strsep ( char **string, const char *delim ) __attribute__ (( nonnull ));

extern char * __attribute__ (( pure )) strerror ( int errno );

#endif /* _STRING_H */
