//#define C_UTILS_THREAD_SAFE
//#define C_UTILS_BIG_ENDIAN
#include "c_utils/c_utils.h"

#include <stdlib.h>
#include <stdio.h>

typedef struct myobj_t {
    str_t name;
    str_t desc;
    int count;
} myobj_t;


int main() {
    strmap_t* map = strmap_create( sizeof( myobj_t ) );
    
    myobj_t obj;
    obj.name = str("some name");
    obj.desc = str("description");
    obj.count = 42;

    strmap_insert( map, str("test"), &obj );
    
    myobj_t found;
    if( strmap_find( map, str("test"), &found ) ) {
        printf( "Found:\n\tname:%s\n\tdesc:%s\n\tcount:%d\n\n", cstr( found.name ), cstr( found.desc ), found.count );
        
        found.count++;
        strmap_update( map, str("test"), &found );
    }
    
    myobj_t found2;
    if( strmap_find( map, str("test"), &found2 ) ) {
        printf( "Found:\n\tname:%s\n\tdesc:%s\n\tcount:%d\n\n", cstr( found.name ), cstr( found.desc ), found.count );
    }

    printf( "Len: %d\n", len( found2.desc ) );

    printf( "concat: %s\n", cstr( concat( found2.name, found2.desc ) ) );

    printf( "trim: '%s'\n", cstr( trim( str( "   test  " ) ) ) );
    printf( "ltrim: '%s'\n", cstr( ltrim( str( "   test  " ) ) ) );
    printf( "rtrim: '%s'\n", cstr( rtrim( str( "   test  " ) ) ) );

    str_t formatted = format( str( "Format Test: %d '%s'" ), found2.count, cstr( found2.name ) );
    printf( "format: %s\n", cstr( formatted ) );

    printf( "left: '%s'\n", cstr( left( str("Mattias Gustavsson"), 7 ) ) );
    printf( "right: '%s'\n", cstr( right( str("Mattias Gustavsson"), 10 ) ) );
    printf( "mid: '%s'\n", cstr( mid( str("Mattias Gustavsson"), 6, 3 ) ) );
    printf( "mid: '%s'\n", cstr( mid( str("Mattias Gustavsson"), 6, -1 ) ) );
    printf( "instr: %d\n", instr( str("Mattias Gustavsson"), str( "Gus" ), 0 ) );
    printf( "any: %d\n", any( str("Mattias Gustavsson"), str( "ui" ), 0 ) );
    printf( "any: %d\n", any( str("Mattias Gustavsson"), str( "ui" ), 5 ) );
    printf( "starts_with: %s\n", starts_with( str("Mattias Gustavsson"), str( "Mattias" ) ) ? "TRUE" : "FALSE" );
    printf( "starts_with: %s\n", starts_with( str("Mattias Gustavsson"), str( "Gustavsson" ) ) ? "TRUE" : "FALSE" );
    printf( "starts_with: %s\n", starts_with( str("Mattias"), str( "Mattias Gustavsson" ) ) ? "TRUE" : "FALSE" );
    printf( "upper: %s\n", cstr( upper( str("Mattias Gustavsson") ) ) );
    printf( "lower: %s\n", cstr( lower( str("Mattias Gustavsson") ) ) );
    printf( "string_from_int: %s\n", cstr( string_from_int( 42 ) ) );
    printf( "string_from_float: %s\n", cstr( string_from_float( 13.37f ) ) );
    printf( "int_from_string: %d\n", int_from_string( str( "42" ) ) );
    printf( "float_from_string: %f\n", float_from_string( str( "13.37" ) ) );
    
    array_t* myarr = array_create( sizeof( myobj_t ) );
    
    obj.name = str("a");
    obj.desc = str("a");
    obj.count = 1;
    array_add( myarr, &obj );

    obj.name = str("b");
    obj.desc = str("b");
    obj.count = 2;
    array_add( myarr, &obj );

    obj.name = str("c");
    obj.desc = str("c");
    obj.count = 3;
    array_add( myarr, &obj );
    
    for( int i = 0; i < array_count( myarr ); ++i ) {
        myobj_t o;
        if( array_get( myarr, i, &o ) ) {
            printf( "%d:\n\tname:%s\n\tdesc:%s\n\tcount:%d\n\n", i, cstr( o.name ), cstr( o.desc ), o.count );
        }
    }
    
    obj.count = 7;
    array_set( myarr, 2, &obj );
    array_remove_ordered( myarr, 0 );
    
    for( int i = 0; i < array_count( myarr ); ++i ) {
        myobj_t o;
        if( array_get( myarr, i, &o ) ) {
            printf( "%d:\n\tname:%s\n\tdesc:%s\n\tcount:%d\n\n", i, cstr( o.name ), cstr( o.desc ), o.count );
        }
    }    
    
    array_destroy( myarr );
    
    array_t* intarr = array_create( sizeof( int ) );
    int x;
    x = 3; array_add( intarr, &x );
    x = 1; array_add( intarr, &x );
    x = 5; array_add( intarr, &x );
    x = 2; array_add( intarr, &x );
    x = 4; array_add( intarr, &x );
    
    for( int i = 0; i < array_count( intarr ); ++i ) {
        int v;
        if( array_get( intarr, i, &v ) ) {
            printf( "%d ", v );
        }
    }    
    printf( "\n" );

    array_sort( intarr, compare_int );
    
    for( int i = 0; i < array_count( intarr ); ++i ) {
        int v;
        if( array_get( intarr, i, &v ) ) {
            printf( "%d ", v );
        }
    }    
    printf( "\n\n" );
    x = 4;
    printf( "bsearch(4): %d\n", array_bsearch( intarr, &x, compare_int ) );
    array_destroy( intarr );
    
    strmap_destroy( map );
    
    buffer_t* buffer = buffer_create();
    str_t data = str( "This is some test data" );
    int length = len( data );
    buffer_write_i32( buffer, &length, 1 );
    buffer_write_char( buffer, cstr( data ), length );
    buffer_save( buffer, "test.bin" );
    buffer_destroy( buffer );
    
    buffer_t* inbuf = buffer_load( "test.bin" );
    int inlen = 0;
    buffer_read_i32( inbuf, &inlen, 1 );
    char* buf = (char*) malloc( inlen + 1 );
    buffer_read_char( inbuf, buf, inlen );
    buf[ inlen ] = '\0';
    buffer_destroy( inbuf );
    printf( "Buffer: %d %s", inlen, buf );
    free( buf );
    return 0;
}

#define C_UTILS_IMPLEMENTATION
#include "c_utils/c_utils.h"
