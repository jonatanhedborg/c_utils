#ifndef str_h
#define str_h

#include <stdint.h>
#include <stdbool.h>

typedef uint32_t str_t;

// create a str_t from a c string
str_t str( char const* string );

// return the c string for a str_t
char const* cstr( str_t string );

// give the length of a string
int len( str_t string );

// concatenate string a and string b
str_t concat( str_t a, str_t b );

// returns 0 if strings are equal, <0 if a comes before b, >0 if b comes before a 
int compare( str_t a, str_t b );

// remove leading and trailing whitespace
str_t trim( str_t string );

// remove leading whitespace 
str_t ltrim( str_t string );

// remove trailing whitespace
str_t rtrim( str_t string );

// return the leftmost characters of a string
str_t left( str_t source, int number );

// return the rightmost characters of a string
str_t right( str_t source, int number );

// return a number of characters from the middle of a string
str_t mid( str_t source, int offset, int number );

// search for occurrences of one string within another string
int instr( str_t haystack, str_t needle, int start );

// search haystack for next occurrence of any char from needles
int any( str_t haystack, str_t needles, int start );

// returns true if a string starts with the specified substring
bool starts_with( str_t string, str_t start );

// convert a string of text to upper case
str_t upper( str_t string );

// convert a string of text to lower case
str_t lower( str_t string );

// convert a number into a string
str_t string_from_int( int x );

// convert a number into a string
str_t string_from_float( float x );

// convert a string of digits into a floating point value
float float_from_string( str_t string );

// convert a string of digits into an integer value
int int_from_string( str_t string );

// printf style formatting
str_t format( str_t format_string, ... );


#endif /* str_h */


#ifdef STR_IMPLEMENTATION
#undef STR_IMPLEMENTATION

#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <stdarg.h>
#include "strpool.h"

typedef struct strsys_t {
    strpool_t pool;
    int temp_capacity;
    char* temp_buffer;
    #ifdef STR_THREAD_SAFE
        thread_mutex_t mutex;
    #endif
} strsys_t;

thread_atomic_ptr_t g_strsys;

#ifdef STR_THREAD_SAFE
    #define STR_MUTEX_LOCK(x) thread_mutex_lock( (x) )
    #define STR_MUTEX_UNLOCK(x) thread_mutex_unlock( (x) )
#else
    #define STR_MUTEX_LOCK(x) 
    #define STR_MUTEX_UNLOCK(x) 
#endif

typedef uint32_t str_t;


static void cleanup_strsys( void ) {
    strsys_t* strsys = (strsys_t*) thread_atomic_ptr_load( &g_strsys );
    if( strsys ) {
        STR_MUTEX_LOCK( &strsys->mutex );
        strpool_term( &strsys->pool );
        STR_MUTEX_UNLOCK( &strsys->mutex );
        #ifdef STR_THREAD_SAFE
            thread_mutex_term( &strsys->mutex );
        #endif
        free( strsys->temp_buffer );
        free( strsys );
    }
}


static strsys_t* get_strsys( void ) {
    strsys_t* pool = thread_atomic_ptr_load( &g_strsys );
    if( pool ) {
        return pool;
    } else {
        strsys_t* strsys = (struct  strsys_t*) malloc( sizeof( strsys_t ) );
        strpool_config_t config = strpool_default_config;
        config.counter_bits = 0;
        strpool_init( &strsys->pool, &config );
        #ifdef STR_THREAD_SAFE
            thread_mutex_init( &strsys->mutex );
        #endif
        strsys->temp_capacity = 256;
        strsys->temp_buffer = (char*) malloc( strsys->temp_capacity );

        if( thread_atomic_ptr_compare_and_swap( &g_strsys, NULL, strsys ) == NULL ) {
            atexit( cleanup_strsys );
            return strsys;
        } else {
            strpool_term( &strsys->pool );
            #ifdef STR_THREAD_SAFE
                thread_mutex_term( &strsys->mutex );
            #endif
            free( strsys->temp_buffer );
            free( strsys );
            return thread_atomic_ptr_load( &g_strsys );
        }
    }
}


static char* get_strtemp( strsys_t* strsys, int required_capacity ) {
    if( strsys->temp_capacity < required_capacity ) {
        free( strsys->temp_buffer );
        while( strsys->temp_capacity < required_capacity ) {
            strsys->temp_capacity *= 2;
        }
        strsys->temp_buffer = (char*) malloc( strsys->temp_capacity );
    }
    return strsys->temp_buffer;
}


// create a str_t from a c string
str_t str( char const* string ) {
    strsys_t* strsys = get_strsys();
    STR_MUTEX_LOCK( &strsys->mutex );
    STRPOOL_U64 handle = strpool_inject( &strsys->pool, string ? string : "", string ? (int) strlen( string ) : 0 );
    STR_MUTEX_UNLOCK( &strsys->mutex );
    return (str_t) handle;
}


// return the c string for a str_t
char const* cstr( str_t string ) {
    strsys_t* strsys = get_strsys();
    STR_MUTEX_LOCK( &strsys->mutex );
    char const* result = strpool_cstr( &strsys->pool, (STRPOOL_U64) string );
    STR_MUTEX_UNLOCK( &strsys->mutex );
    return result ? result : "";
}

// give the length of a string
int len( str_t string ) {
    strsys_t* strsys = get_strsys();
    STR_MUTEX_LOCK( &strsys->mutex );
    int result = strpool_length( &strsys->pool, (STRPOOL_U64) string );
    STR_MUTEX_UNLOCK( &strsys->mutex );
    return result;
}


// concatenate string a and string b
str_t concat( str_t a, str_t b ) {
    strsys_t* strsys = get_strsys();
    STR_MUTEX_LOCK( &strsys->mutex );
    int len_a = strpool_length( &strsys->pool, (STRPOOL_U64) a );
    int len_b = strpool_length( &strsys->pool, (STRPOOL_U64) b );
    char* temp = get_strtemp( strsys, len_a + len_b );
    char const* cstr_a = strpool_cstr( &strsys->pool, (STRPOOL_U64) a );
    char const* cstr_b = strpool_cstr( &strsys->pool, (STRPOOL_U64) b );
    memcpy( temp, cstr_a, len_a );
    memcpy( temp + len_a, cstr_b, len_b );
    STRPOOL_U64 handle = strpool_inject( &strsys->pool, temp, len_a + len_b );
    STR_MUTEX_UNLOCK( &strsys->mutex );
    return (str_t) handle;
}


// returns 0 if strings are equal, <0 if a comes before b, >0 if b comes before a 
int compare( str_t a, str_t b ) {
    if( a == b ) {
        return 0;
    }
    strsys_t* strsys = get_strsys();
    STR_MUTEX_LOCK( &strsys->mutex );
    char const* cstr_a = strpool_cstr( &strsys->pool, (STRPOOL_U64) a );
    char const* cstr_b = strpool_cstr( &strsys->pool, (STRPOOL_U64) b );
    int result = strcmp( cstr_a, cstr_b );
    STR_MUTEX_UNLOCK( &strsys->mutex );
    return result;
}


// remove leading and trailing whitespace
str_t trim( str_t string ) {
    strsys_t* strsys = get_strsys();
    STR_MUTEX_LOCK( &strsys->mutex );
    int length = strpool_length( &strsys->pool, (STRPOOL_U64) string );
    char const* start = strpool_cstr( &strsys->pool, (STRPOOL_U64) string );
    start = start ? start : "";
	char const* end = start + length - 1;
	while( *start && *start <= ' ' ) {
		++start;
    }
	while( end > start && *end <= ' ' ) {
		--end;
    }
    STRPOOL_U64 handle = strpool_inject( &strsys->pool, start, end - start + 1 );
    STR_MUTEX_UNLOCK( &strsys->mutex );
    return (str_t) handle;
}

// remove leading whitespace 
str_t ltrim( str_t string ) {
    strsys_t* strsys = get_strsys();
    STR_MUTEX_LOCK( &strsys->mutex );
    int length = strpool_length( &strsys->pool, (STRPOOL_U64) string );
    char const* start = strpool_cstr( &strsys->pool, (STRPOOL_U64) string );
    start = start ? start : "";
	char const* end = start + length - 1;
	while( *start && *start <= ' ' ) {
		++start;
    }
    STRPOOL_U64 handle = strpool_inject( &strsys->pool, start, end - start + 1 );
    STR_MUTEX_UNLOCK( &strsys->mutex );
    return (str_t) handle;
}


// remove trailing whitespace
str_t rtrim( str_t string ) {
    strsys_t* strsys = get_strsys();
    STR_MUTEX_LOCK( &strsys->mutex );
    int length = strpool_length( &strsys->pool, (STRPOOL_U64) string );
    char const* start = strpool_cstr( &strsys->pool, (STRPOOL_U64) string );
    start = start ? start : "";
	char const* end = start + length - 1;
	while( end > start && *end <= ' ' ) {
		--end;
    }
    STRPOOL_U64 handle = strpool_inject( &strsys->pool, start, end - start + 1 );
    STR_MUTEX_UNLOCK( &strsys->mutex );
    return (str_t) handle;
}


// return the leftmost characters of a string
str_t left( str_t source, int number ) {
    strsys_t* strsys = get_strsys();
    STR_MUTEX_LOCK( &strsys->mutex );
    int length = strpool_length( &strsys->pool, (STRPOOL_U64) source );
    number = number < 0 ? 0 : number > length ? length : number;
    char const* string = strpool_cstr( &strsys->pool, (STRPOOL_U64) source );
    STRPOOL_U64 handle = strpool_inject( &strsys->pool, string, number );
    STR_MUTEX_UNLOCK( &strsys->mutex );
    return (str_t) handle;
}


// return the rightmost characters of a string
str_t right( str_t source, int number ) {
    strsys_t* strsys = get_strsys();
    STR_MUTEX_LOCK( &strsys->mutex );
    int length = strpool_length( &strsys->pool, (STRPOOL_U64) source );
    number = number < 0 ? 0 : number > length ? length : number;
    char const* string = strpool_cstr( &strsys->pool, (STRPOOL_U64) source );
    STRPOOL_U64 handle = strpool_inject( &strsys->pool, string + ( length - number ), number );
    STR_MUTEX_UNLOCK( &strsys->mutex );
    return (str_t) handle;
}

// return a number of characters from the middle of a string
str_t mid( str_t source, int offset, int number ) {
    strsys_t* strsys = get_strsys();
    STR_MUTEX_LOCK( &strsys->mutex );
    int length = strpool_length( &strsys->pool, (STRPOOL_U64) source );
    offset = offset < 0 ? 0 : offset > length ? length : offset;
    number = number < 0 ? length - offset : number;
    number = offset + number > length ? length - offset : number;
    char const* string = strpool_cstr( &strsys->pool, (STRPOOL_U64) source );
    STRPOOL_U64 handle = strpool_inject( &strsys->pool, string + offset, number );
    STR_MUTEX_UNLOCK( &strsys->mutex );
    return (str_t) handle;
}


// search for occurrences of one string within another string
int instr( str_t haystack, str_t needle, int start ) {
    strsys_t* strsys = get_strsys();
    STR_MUTEX_LOCK( &strsys->mutex );
    char const* cstr_a = strpool_cstr( &strsys->pool, (STRPOOL_U64) haystack );
    char const* cstr_b = strpool_cstr( &strsys->pool, (STRPOOL_U64) needle );
    int length_a = strpool_length( &strsys->pool, (STRPOOL_U64) haystack );
	start = start < 0 ? 0 : start > length_a ? length_a : start;
	char const* find = strstr( cstr_a + start, cstr_b );
	int result = (int)( find ? ( find - cstr_a ) : -1 ); 
    STR_MUTEX_UNLOCK( &strsys->mutex );
    return result;
}


// search haystack for next occurrence of any char from needles
int any( str_t haystack, str_t needles, int start ) {
    strsys_t* strsys = get_strsys();
    STR_MUTEX_LOCK( &strsys->mutex );
    char const* cstr_a = strpool_cstr( &strsys->pool, (STRPOOL_U64) haystack );
    char const* cstr_b = strpool_cstr( &strsys->pool, (STRPOOL_U64) needles );
    int length_a = strpool_length( &strsys->pool, (STRPOOL_U64) haystack );
    int length_b = strpool_length( &strsys->pool, (STRPOOL_U64) needles );
	start = start < 0 ? 0 : start > length_a ? length_a : start;
	for( int i = start; i < length_a; ++i ) {
		for( int j = 0; j < length_b; ++j ) {
			if( cstr_a[ i ] == cstr_b[ j ] ) {
                STR_MUTEX_UNLOCK( &strsys->mutex );
				return i;            
            }
        }
    }

    STR_MUTEX_UNLOCK( &strsys->mutex );
	return -1; 
}


// returns true if a string starts with the specified substring
bool starts_with( str_t string, str_t start ) {
    strsys_t* strsys = get_strsys();
    STR_MUTEX_LOCK( &strsys->mutex );
    char const* cstr_a = strpool_cstr( &strsys->pool, (STRPOOL_U64) string );
    char const* cstr_b = strpool_cstr( &strsys->pool, (STRPOOL_U64) start );
    int length_b = strpool_length( &strsys->pool, (STRPOOL_U64) start );
	bool result = strncmp( cstr_a, cstr_b, length_b ) == 0; 
    STR_MUTEX_UNLOCK( &strsys->mutex );
    return result;
}


// convert a string of text to upper case
str_t upper( str_t string ) {
    strsys_t* strsys = get_strsys();
    STR_MUTEX_LOCK( &strsys->mutex );
    int length = strpool_length( &strsys->pool, (STRPOOL_U64) string );
    char* temp = get_strtemp( strsys, length );
	char const* src = strpool_cstr( &strsys->pool, (STRPOOL_U64) string );	
    char* dst = temp;
	for( int i = 0; i < length; ++i ) {
		*dst++ = (char) toupper( (int)*src++ );
    }
    STRPOOL_U64 handle = strpool_inject( &strsys->pool, temp, length );
    STR_MUTEX_UNLOCK( &strsys->mutex );
    return (str_t) handle;
}


// convert a string of text to lower case
str_t lower( str_t string ) {
    strsys_t* strsys = get_strsys();
    STR_MUTEX_LOCK( &strsys->mutex );
    int length = strpool_length( &strsys->pool, (STRPOOL_U64) string );
    char* temp = get_strtemp( strsys, length );
	char const* src = strpool_cstr( &strsys->pool, (STRPOOL_U64) string );	
    char* dst = temp;
	for( int i = 0; i < length; ++i ) {
		*dst++ = (char) tolower( (int)*src++ );
    }
    STRPOOL_U64 handle = strpool_inject( &strsys->pool, temp, length );
    STR_MUTEX_UNLOCK( &strsys->mutex );
    return (str_t) handle;
}


// convert a number into a string
str_t string_from_int( int x ) {
    strsys_t* strsys = get_strsys();
    STR_MUTEX_LOCK( &strsys->mutex );
    char* temp = get_strtemp( strsys, 256 );
	int length = snprintf( temp, 256, "%d", x );
    STRPOOL_U64 handle = strpool_inject( &strsys->pool, temp, length );
    STR_MUTEX_UNLOCK( &strsys->mutex );
    return (str_t) handle;
}


// convert a number into a string
str_t string_from_float( float x ) {
    strsys_t* strsys = get_strsys();
    STR_MUTEX_LOCK( &strsys->mutex );
    char* temp = get_strtemp( strsys, 256 );
	int length = snprintf( temp, 256, "%f", x );
    STRPOOL_U64 handle = strpool_inject( &strsys->pool, temp, length );
    STR_MUTEX_UNLOCK( &strsys->mutex );
    return (str_t) handle;
}


// convert a string of digits into a floating point value
float float_from_string( str_t string ) {
    strsys_t* strsys = get_strsys();
    STR_MUTEX_LOCK( &strsys->mutex );
	char const* c_str = strpool_cstr( &strsys->pool, (STRPOOL_U64) string );	
	float result = (float) atof( c_str );
    STR_MUTEX_UNLOCK( &strsys->mutex );
    return result;
}


// convert a string of digits into an integer value
int int_from_string( str_t string ) {
    strsys_t* strsys = get_strsys();
    STR_MUTEX_LOCK( &strsys->mutex );
	char const* c_str = strpool_cstr( &strsys->pool, (STRPOOL_U64) string );	
	int result = atoi( c_str );
    STR_MUTEX_UNLOCK( &strsys->mutex );
    return result;
}


// printf style formatting
 str_t format( str_t format_string, ... ) {
    strsys_t* strsys = get_strsys();
    STR_MUTEX_LOCK( &strsys->mutex );
    char* temp = get_strtemp( strsys, 256 );
    char const* format_cstr = strpool_cstr( &strsys->pool, (STRPOOL_U64) format_string );
    
	va_list args;
	va_start( args, format_string );
	#ifdef _WIN32
		int size = _vscprintf( format_cstr, args );
	#else
	    int size = vsnprintf( temp, 0, format_cstr, args );
	#endif
	va_end( args );
    
	temp = get_strtemp( strsys, size );
	va_start( args, format_string );
	#ifdef _WIN32
		_vsnprintf( temp, (size_t) size + 1, format_cstr, args );
	#else
		vsnprintf( temp, (size_t) size + 1, format_cstr, args );
	#endif
	va_end( args );
    
    STRPOOL_U64 handle = strpool_inject( &strsys->pool, temp, (int)size );
    STR_MUTEX_UNLOCK( &strsys->mutex );
    return (str_t) handle;
    
}

#undef STR_MUTEX_LOCK
#undef STR_MUTEX_UNLOCK

#endif /* STR_IMPLEMENTATION */