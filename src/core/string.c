/*
 * Copyright (C) 2015 Michael Brown <mbrown@fensystems.co.uk>.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 * You can also choose to distribute this program under the terms of
 * the Unmodified Binary Distribution Licence (as given in the file
 * COPYING.UBDL), provided that you have satisfied its requirements.
 */

FILE_LICENCE ( GPL2_OR_LATER_OR_UBDL );

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <errno.h>

/** @file
 *
 * String functions
 *
 */

/**
 * Fill memory region
 *
 * @v dest		Destination region
 * @v character		Fill character
 * @v len		Length
 * @ret dest		Destination region
 */
void * generic_memset ( void *dest, int character, size_t len ) {
	uint8_t *dest_bytes = dest;

	while ( len-- )
		*(dest_bytes++) = character;
	return dest;
}

/**
 * Copy memory region (forwards)
 *
 * @v dest		Destination region
 * @v src		Source region
 * @v len		Length
 * @ret dest		Destination region
 */
void * generic_memcpy ( void *dest, const void *src, size_t len ) {
	const uint8_t *src_bytes = src;
	uint8_t *dest_bytes = dest;

	while ( len-- )
		*(dest_bytes++) = *(src_bytes++);
	return dest;
}

/**
 * Copy memory region (backwards)
 *
 * @v dest		Destination region
 * @v src		Source region
 * @v len		Length
 * @ret dest		Destination region
 */
void * generic_memcpy_reverse ( void *dest, const void *src, size_t len ) {
	const uint8_t *src_bytes = ( src + len );
	uint8_t *dest_bytes = ( dest + len );

	while ( len-- )
		*(--dest_bytes) = *(--src_bytes);
	return dest;
}

/**
 * Copy (possibly overlapping) memory region
 *
 * @v dest		Destination region
 * @v src		Source region
 * @v len		Length
 * @ret dest		Destination region
 */
void * generic_memmove ( void *dest, const void *src, size_t len ) {

	if ( dest < src ) {
		return generic_memcpy ( dest, src, len );
	} else {
		return generic_memcpy_reverse ( dest, src, len );
	}
}

/**
 * Compare memory regions
 *
 * @v first		First region
 * @v second		Second region
 * @v len		Length
 * @ret diff		Difference
 */
int memcmp ( const void *first, const void *second, size_t len ) {
	const uint8_t *first_bytes = first;
	const uint8_t *second_bytes = second;
	int diff;

	while ( len-- ) {
		diff = ( *(first_bytes++) - *(second_bytes++) );
		if ( diff )
			return diff;
	}
	return 0;
}

/**
 * Find character within a memory region
 *
 * @v src		Source region
 * @v character		Character to find
 * @v len		Length
 * @ret found		Found character, or NULL if not found
 */
void * memchr ( const void *src, int character, size_t len ) {
	const uint8_t *src_bytes = src;

	for ( ; len-- ; src_bytes++ ) {
		if ( *src_bytes == character )
			return ( ( void * ) src_bytes );
	}
	return NULL;
}

/**
 * Swap memory regions
 *
 * @v first		First region
 * @v second		Second region
 * @v len		Length
 * @ret first		First region
 */
void * memswap ( void *first, void *second, size_t len ) {
	uint8_t *first_bytes = first;
	uint8_t *second_bytes = second;
	uint8_t temp;

	for ( ; len-- ; first_bytes++, second_bytes++ ) {
		temp = *first_bytes;
		*first_bytes = *second_bytes;
		*second_bytes = temp;
	}
	return first;
}

/**
 * Compare strings
 *
 * @v first		First string
 * @v second		Second string
 * @ret diff		Difference
 */
int strcmp ( const char *first, const char *second ) {

	return strncmp ( first, second, ~( ( size_t ) 0 ) );
}

/**
 * Compare strings
 *
 * @v first		First string
 * @v second		Second string
 * @v max		Maximum length to compare
 * @ret diff		Difference
 */
int strncmp ( const char *first, const char *second, size_t max ) {
	const uint8_t *first_bytes = ( ( const uint8_t * ) first );
	const uint8_t *second_bytes = ( ( const uint8_t * ) second );
	int diff;

	for ( ; max-- ; first_bytes++, second_bytes++ ) {
		diff = ( *first_bytes - *second_bytes );
		if ( diff )
			return diff;
		if ( ! *first_bytes )
			return 0;
	}
	return 0;
}

/**
 * Compare case-insensitive strings
 *
 * @v first		First string
 * @v second		Second string
 * @ret diff		Difference
 */
int strcasecmp ( const char *first, const char *second ) {

	return strncasecmp ( first, second, ~( ( size_t ) 0 ) );
}

/**
 * Compare case-insensitive strings
 *
 * @v first		First string
 * @v second		Second string
 * @v max		Maximum length to compare
 * @ret diff		Difference
 */
int strncasecmp ( const char *first, const char *second, size_t max ) {
	const uint8_t *first_bytes = ( ( const uint8_t * ) first );
	const uint8_t *second_bytes = ( ( const uint8_t * ) second );
	int diff;

	for ( ; max-- ; first_bytes++, second_bytes++ ) {
		diff = ( toupper ( *first_bytes ) -
			 toupper ( *second_bytes ) );
		if ( diff )
			return diff;
		if ( ! *first_bytes )
			return 0;
	}
	return 0;
}

/**
 * Get length of string
 *
 * @v src		String
 * @ret len		Length
 */
size_t strlen ( const char *src ) {

	return strnlen ( src, ~( ( size_t ) 0 ) );
}

/**
 * Get first position of any symbol included in str2 in str1
 *
 * @v str1		String
 * @v str2		String
 * @ret pos		Position
 */
size_t strcspn (const char * str1, const char * str2)
{
    size_t i = 0;
    size_t l1 = strlen(str1);
    size_t l2 = strlen(str2);
 
    for ( ; i < l1; ++i)
        for (size_t j = 0; j < l2; ++j)
            if (str1[i] == str2[j])
                return i;
 
    return -1;
}

/**
 * Get first position of any symbol not included in str2 in str1
 *
 * @v str		String
 * @v chars		String
 * @ret pos		Position
 */
size_t strspn(const char *str, const char *chars) {
    size_t i = 0;
    while (str[i] && strchr(chars, str[i]))
        i++;
    return i;
}

/**
 * Get length of string
 *
 * @v src		String
 * @v max		Maximum length
 * @ret len		Length
 */
size_t strnlen ( const char *src, size_t max ) {
	const uint8_t *src_bytes = ( ( const uint8_t * ) src );
	size_t len = 0;

	while ( max-- && *(src_bytes++) )
		len++;
	return len;
}

/**
 * Find character within a string
 *
 * @v src		String
 * @v character		Character to find
 * @ret found		Found character, or NULL if not found
 */
char * strchr ( const char *src, int character ) {
	const uint8_t *src_bytes = ( ( const uint8_t * ) src );

	for ( ; ; src_bytes++ ) {
		if ( *src_bytes == character )
			return ( ( char * ) src_bytes );
		if ( ! *src_bytes )
			return NULL;
	}
}

/**
 * Find rightmost character within a string
 *
 * @v src		String
 * @v character		Character to find
 * @ret found		Found character, or NULL if not found
 */
char * strrchr ( const char *src, int character ) {
	const uint8_t *src_bytes = ( ( const uint8_t * ) src );
	const uint8_t *start = src_bytes;

	while ( *src_bytes )
		src_bytes++;
	for ( src_bytes-- ; src_bytes >= start ; src_bytes-- ) {
		if ( *src_bytes == character )
			return ( ( char * ) src_bytes );
	}
	return NULL;
}

/**
 * Find substring
 *
 * @v haystack		String
 * @v needle		Substring
 * @ret found		Found substring, or NULL if not found
 */
char * strstr ( const char *haystack, const char *needle ) {
	size_t len = strlen ( needle );

	for ( ; *haystack ; haystack++ ) {
		if ( memcmp ( haystack, needle, len ) == 0 )
			return ( ( char * ) haystack );
	}
	return NULL;
}

/**
 * Copy string
 *
 * @v dest		Destination string
 * @v src		Source string
 * @ret dest		Destination string
 */
char * strcpy ( char *dest, const char *src ) {
	const uint8_t *src_bytes = ( ( const uint8_t * ) src );
	uint8_t *dest_bytes = ( ( uint8_t * ) dest );

	/* We cannot use strncpy(), since that would pad the destination */
	for ( ; ; src_bytes++, dest_bytes++ ) {
		*dest_bytes = *src_bytes;
		if ( ! *dest_bytes )
			break;
	}
	return dest;
}

/**
 * Copy string
 *
 * @v dest		Destination string
 * @v src		Source string
 * @v max		Maximum length
 * @ret dest		Destination string
 */
char * strncpy ( char *dest, const char *src, size_t max ) {
	const uint8_t *src_bytes = ( ( const uint8_t * ) src );
	uint8_t *dest_bytes = ( ( uint8_t * ) dest );

	for ( ; max ; max--, src_bytes++, dest_bytes++ ) {
		*dest_bytes = *src_bytes;
		if ( ! *dest_bytes )
			break;
	}
	while ( max-- )
		*(dest_bytes++) = '\0';
	return dest;
}

/**
 * Concatenate string
 *
 * @v dest		Destination string
 * @v src		Source string
 * @ret dest		Destination string
 */
char * strcat ( char *dest, const char *src ) {

	strcpy ( ( dest + strlen ( dest ) ), src );
	return dest;
}

/**
 * Duplicate string
 *
 * @v src		Source string
 * @ret dup		Duplicated string, or NULL if allocation failed
 */
char * strdup ( const char *src ) {

	return strndup ( src, ~( ( size_t ) 0 ) );
}

/**
 * Duplicate string
 *
 * @v src		Source string
 * @v max		Maximum length
 * @ret dup		Duplicated string, or NULL if allocation failed
 */
char * strndup ( const char *src, size_t max ) {
	size_t len = strnlen ( src, max );
        char *dup;

        dup = malloc ( len + 1 /* NUL */ );
        if ( dup ) {
		memcpy ( dup, src, len );
		dup[len] = '\0';
        }
        return dup;
}

/**
 * Calculate digit value
 *
 * @v character		Digit character
 * @ret digit		Digit value
 *
 * Invalid digits will be returned as a value greater than or equal to
 * the numeric base.
 */
unsigned int digit_value ( unsigned int character ) {

	if ( character >= 'a' )
		return ( character - ( 'a' - 10 ) );
	if ( character >= 'A' )
		return ( character - ( 'A' - 10 ) );
	if ( character <= '9' )
		return ( character - '0' );
	return character;
}

/**
 * Preprocess string for strtoul() or strtoull()
 *
 * @v string		String
 * @v negate		Final value should be negated
 * @v base		Numeric base
 * @ret string		Remaining string
 */
static const char * strtoul_pre ( const char *string, int *negate, int *base ) {

	/* Skip any leading whitespace */
	while ( isspace ( *string ) )
		string++;

	/* Process arithmetic sign, if present */
	*negate = 0;
	if ( *string == '-' ) {
		string++;
		*negate = 1;
	} else if ( *string == '+' ) {
		string++;
	}

	/* Process base, if present */
	if ( *base == 0 ) {
		*base = 10;
		if ( *string == '0' ) {
			string++;
			*base = 8;
			if ( ( *string & ~0x20 ) == 'X' ) {
				string++;
				*base = 16;
			}
		}
	}

	return string;
}

/**
 * Convert string to numeric value
 *
 * @v string		String
 * @v endp		End pointer (or NULL)
 * @v base		Numeric base (or zero to autodetect)
 * @ret value		Numeric value
 */
unsigned long strtoul ( const char *string, char **endp, int base ) {
	unsigned long value = 0;
	unsigned int digit;
	int negate;

	/* Preprocess string */
	string = strtoul_pre ( string, &negate, &base );

	/* Process digits */
	for ( ; ; string++ ) {
		digit = digit_value ( *string );
		if ( digit >= ( unsigned int ) base )
			break;
		value = ( ( value * base ) + digit );
	}

	/* Negate value if, applicable */
	if ( negate )
		value = -value;

	/* Fill in end pointer, if applicable */
	if ( endp )
		*endp = ( ( char * ) string );

	return value;
}

/**
 * Convert string to numeric value
 *
 * @v string		String
 * @v endp		End pointer (or NULL)
 * @v base		Numeric base (or zero to autodetect)
 * @ret value		Numeric value
 */
unsigned long long strtoull ( const char *string, char **endp, int base ) {
	unsigned long long value = 0;
	unsigned int digit;
	int negate;

	/* Preprocess string */
	string = strtoul_pre ( string, &negate, &base );

	/* Process digits */
	for ( ; ; string++ ) {
		digit = digit_value ( *string );
		if ( digit >= ( unsigned int ) base )
			break;
		value = ( ( value * base ) + digit );
	}

	/* Negate value if, applicable */
	if ( negate )
		value = -value;

	/* Fill in end pointer, if applicable */
	if ( endp )
		*endp = ( ( char * ) string );

	return value;
}
/**
 * Convert string to numeric value
 *
 * @v string		String
 * @v endp		End pointer (or NULL)
 * @v base		Numeric base (or zero to autodetect)
 * @ret value		Numeric value
 */
long int strtol(const char *string, char **endPtr, int base)
{
    const char *p;
    long int result;

    /*
     * Skip any leading blanks.
     */
    p = string;
    while (isspace(*p)) {
    p += 1;
    }

    /*
     * Check for a sign.
     */
    if (*p == '-') {
    p += 1;
    result = -1*(strtoul(p, endPtr, base));
    } else {
    if (*p == '+') {
        p += 1;
    }
    result = strtoul(p, endPtr, base);
    }
    // if ((result == 0) && (endPtr != 0) && (*endPtr == p)) {
    // *endPtr = string;
    // }
    return result;
}


/* 
 * strtod.c --
 *
 *	Source code for the "strtod" library procedure.
 *
 * Copyright (c) 1988-1993 The Regents of the University of California.
 * Copyright (c) 1994 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) $Id: strtod.c,v 1.1.1.4 2003/03/06 00:09:04 landonf Exp $
 */

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

static int maxExponent = 511;	/* Largest possible base 10 exponent.  Any
				 * exponent larger than this will already
				 * produce underflow or overflow, so there's
				 * no need to worry about additional digits.
				 */
static double powersOf10[] = {	/* Table giving binary powers of 10.  Entry */
    10.,			/* is 10^2^i.  Used to convert decimal */
    100.,			/* exponents into floating-point numbers. */
    1.0e4,
    1.0e8,
    1.0e16,
    1.0e32,
    1.0e64,
    1.0e128,
    1.0e256
};
/*
 *----------------------------------------------------------------------
 *
 * strtod --
 *
 *	This procedure converts a floating-point number from an ASCII
 *	decimal representation to internal double-precision format.
 *
 * Results:
 *	The return value is the double-precision floating-point
 *	representation of the characters in string.  If endPtr isn't
 *	NULL, then *endPtr is filled in with the address of the
 *	next character after the last one that was part of the
 *	floating-point number.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

double strtod(string, endPtr)
    const char *string;		/* A decimal ASCII floating-point number,
				 * optionally preceded by white space.
				 * Must have form "-I.FE-X", where I is the
				 * integer part of the mantissa, F is the
				 * fractional part of the mantissa, and X
				 * is the exponent.  Either of the signs
				 * may be "+", "-", or omitted.  Either I
				 * or F may be omitted, or both.  The decimal
				 * point isn't necessary unless F is present.
				 * The "E" may actually be an "e".  E and X
				 * may both be omitted (but not just one).
				 */
    char **endPtr;		/* If non-NULL, store terminating character's
				 * address here. */
{
    int sign, expSign = FALSE;
    double fraction, dblExp, *d;
    const char *p;
    int c;
    int exp = 0;		/* Exponent read from "EX" field. */
    int fracExp = 0;		/* Exponent that derives from the fractional
				 * part.  Under normal circumstatnces, it is
				 * the negative of the number of digits in F.
				 * However, if I is very long, the last digits
				 * of I get dropped (otherwise a long I with a
				 * large negative exponent could cause an
				 * unnecessary overflow on I alone).  In this
				 * case, fracExp is incremented one for each
				 * dropped digit. */
    int mantSize;		/* Number of digits in mantissa. */
    int decPt;			/* Number of mantissa digits BEFORE decimal
				 * point. */
    const char *pExp;		/* Temporarily holds location of exponent
				 * in string. */

    /*
     * Strip off leading blanks and check for a sign.
     */

    p = string;
    while (isspace((*p))) {
	p += 1;
    }
    if (*p == '-') {
	sign = TRUE;
	p += 1;
    } else {
	if (*p == '+') {
	    p += 1;
	}
	sign = FALSE;
    }

    /*
     * Count the number of digits in the mantissa (including the decimal
     * point), and also locate the decimal point.
     */

    decPt = -1;
    for (mantSize = 0; ; mantSize += 1)
    {
	c = *p;
	if (!isdigit(c)) {
	    if ((c != '.') || (decPt >= 0)) {
		break;
	    }
	    decPt = mantSize;
	}
	p += 1;
    }

    /*
     * Now suck up the digits in the mantissa.  Use two integers to
     * collect 9 digits each (this is faster than using floating-point).
     * If the mantissa has more than 18 digits, ignore the extras, since
     * they can't affect the value anyway.
     */
    
    pExp  = p;
    p -= mantSize;
    if (decPt < 0) {
	decPt = mantSize;
    } else {
	mantSize -= 1;			/* One of the digits was the point. */
    }
    if (mantSize > 18) {
	fracExp = decPt - 18;
	mantSize = 18;
    } else {
	fracExp = decPt - mantSize;
    }
    if (mantSize == 0) {
	fraction = 0.0;
	p = string;
	goto done;
    } else {
	int frac1, frac2;
	frac1 = 0;
	for ( ; mantSize > 9; mantSize -= 1)
	{
	    c = *p;
	    p += 1;
	    if (c == '.') {
		c = *p;
		p += 1;
	    }
	    frac1 = 10*frac1 + (c - '0');
	}
	frac2 = 0;
	for (; mantSize > 0; mantSize -= 1)
	{
	    c = *p;
	    p += 1;
	    if (c == '.') {
		c = *p;
		p += 1;
	    }
	    frac2 = 10*frac2 + (c - '0');
	}
	fraction = (1.0e9 * frac1) + frac2;
    }

    /*
     * Skim off the exponent.
     */

    p = pExp;
    if ((*p == 'E') || (*p == 'e')) {
	p += 1;
	if (*p == '-') {
	    expSign = TRUE;
	    p += 1;
	} else {
	    if (*p == '+') {
		p += 1;
	    }
	    expSign = FALSE;
	}
	if (!isdigit((*p))) {
	    p = pExp;
	    goto done;
	}
	while (isdigit((*p))) {
	    exp = exp * 10 + (*p - '0');
	    p += 1;
	}
    }
    if (expSign) {
	exp = fracExp - exp;
    } else {
	exp = fracExp + exp;
    }

    /*
     * Generate a floating-point number that represents the exponent.
     * Do this by processing the exponent one bit at a time to combine
     * many powers of 2 of 10. Then combine the exponent with the
     * fraction.
     */
    
    if (exp < 0) {
	expSign = TRUE;
	exp = -exp;
    } else {
	expSign = FALSE;
    }
    if (exp > maxExponent) {
	exp = maxExponent;
	errno = ERANGE;
    }
    dblExp = 1.0;
    for (d = powersOf10; exp != 0; exp >>= 1, d += 1) {
	if (exp & 01) {
	    dblExp *= *d;
	}
    }
    if (expSign) {
	fraction /= dblExp;
    } else {
	fraction *= dblExp;
    }

done:
    if (endPtr != NULL) {
	*endPtr = (char *) p;
    }

    if (sign) {
	return -fraction;
    }
    return fraction;
}

/*-
 * Copyright (c) 2014 The Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. [rescinded 22 July 1999]
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

// typedef unsigned long long unsigned long long;
// typedef long long long long;

#ifndef ULLONG_MAX
#define ULLONG_MAX (~(unsigned long long)0) /* 0xFFFFFFFFFFFFFFFF */
#endif

#ifndef LLONG_MAX
#define LLONG_MAX ((long long)(ULLONG_MAX >> 1)) /* 0x7FFFFFFFFFFFFFFF */
#endif

#ifndef LLONG_MIN
#define LLONG_MIN (~LLONG_MAX) /* 0x8000000000000000 */
#endif

/*
 * Convert a string to a long long integer.
 *
 * Ignores `locale' stuff.  Assumes that the upper and lower case
 * alphabets and digits are each contiguous.
 */
long long strtoll(const char *nptr, char **endptr, int base)
{
	const char *s = nptr;
	unsigned long long acc;
	int c;
	unsigned long long cutoff;
	int neg = 0, any, cutlim;

	/*
	 * Skip white space and pick up leading +/- sign if any.
	 * If base is 0, allow 0x for hex and 0 for octal, else
	 * assume decimal; if base is already 16, allow 0x.
	 */
	do {
		c = *s++;
	} while (isspace(c));
	if (c == '-') {
		neg = 1;
		c = *s++;
	} else if (c == '+')
		c = *s++;
	if ((base == 0 || base == 16) &&
	    c == '0' && (*s == 'x' || *s == 'X')) {
		c = s[1];
		s += 2;
		base = 16;
	}
	if (base == 0)
		base = c == '0' ? 8 : 10;

	/*
	 * Compute the cutoff value between legal numbers and illegal
	 * numbers.  That is the largest legal value, divided by the
	 * base.  An input number that is greater than this value, if
	 * followed by a legal input character, is too big.  One that
	 * is equal to this value may be valid or not; the limit
	 * between valid and invalid numbers is then based on the last
	 * digit.  For instance, if the range for longs is
	 * [-2147483648..2147483647] and the input base is 10,
	 * cutoff will be set to 214748364 and cutlim to either
	 * 7 (neg==0) or 8 (neg==1), meaning that if we have accumulated
	 * a value > 214748364, or equal but the next digit is > 7 (or 8),
	 * the number is too big, and we will return a range error.
	 *
	 * Set any if any `digits' consumed; make it negative to indicate
	 * overflow.
	 */
	cutoff = neg ? -(unsigned long long)LLONG_MIN : LLONG_MAX;
	cutlim = cutoff % (unsigned long long)base;
	cutoff /= (unsigned long long)base;
	for (acc = 0, any = 0;; c = *s++) {
		if (isdigit(c))
			c -= '0';
		else if (isalpha(c))
			c -= isupper(c) ? 'A' - 10 : 'a' - 10;
		else
			break;
		if (c >= base)
			break;
		if (any < 0 || acc > cutoff || (acc == cutoff && c > cutlim))
			any = -1;
		else {
			any = 1;
			acc *= base;
			acc += c;
		}
	}
	if (any < 0) {
		acc = neg ? LLONG_MIN : LLONG_MAX;
		errno = ERANGE;
	} else if (neg)
		acc = -acc;
	if (endptr != 0)
		*endptr = (char *) (any ? s - 1 : nptr);
	return (acc);
}
