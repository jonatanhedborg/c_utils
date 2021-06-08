#ifndef strmap_h
#define strmap_h

// To make strmap thread safe, do this before include: #define STRMAP_THREAD_SAFE

typedef struct strmap_t strmap_t;

strmap_t* strmap_create( int item_size );
void strmap_destroy( strmap_t* strmap );
void strmap_clear( strmap_t* strmap );
void strmap_insert( strmap_t* strmap, str_t key, void const* item );
void strmap_remove( strmap_t* strmap, str_t key );
bool strmap_update( strmap_t* strmap, str_t key, void const* item );
bool strmap_find( strmap_t* strmap, str_t key, void* item );

#endif /* strmap_h */


#ifdef STRMAP_IMPLEMENTATION
#undef STRMAP_IMPLEMENTATION

#include <stdlib.h>
#include <string.h>
#include "hashtable.h"


static uint32_t strmap_hash_u32( uint32_t key ) {
    key = ~key + ( key << 15 );
    key = key ^ ( key >> 12 );
    key = key + ( key << 2 );
    key = key ^ ( key >> 4 );
    key = (key + ( key << 3 ) ) + ( key << 11 );
    key = key ^ ( key >> 16);
    return key;
}


typedef struct strmap_t {
    hashtable_t hashtable;
    int item_size;
    #ifdef STRMAP_THREAD_SAFE
        thread_mutex_t mutex;
    #endif
} strmap_t;


#ifdef STRMAP_THREAD_SAFE
    #define STRMAP_MUTEX_LOCK(x) thread_mutex_lock( (x) )
    #define STRMAP_MUTEX_UNLOCK(x) thread_mutex_unlock( (x) )
#else
    #define STRMAP_MUTEX_LOCK(x) 
    #define STRMAP_MUTEX_UNLOCK(x) 
#endif


strmap_t* strmap_create( int item_size ) {
    strmap_t* strmap = (strmap_t*) malloc( sizeof( strmap_t ) );
    int const initial_capacity = 256;
    hashtable_init( &strmap->hashtable, sizeof( str_t ), item_size,  initial_capacity, NULL );
    #ifdef STRMAP_THREAD_SAFE
        thread_mutex_init( &strmap->mutex );
    #endif
    strmap->item_size = item_size;
    return strmap;
}


void strmap_destroy( strmap_t* strmap ) {
    STRMAP_MUTEX_LOCK( &strmap->mutex );
    hashtable_term( &strmap->hashtable );
    STRMAP_MUTEX_UNLOCK( &strmap->mutex );    
    #ifdef STRMAP_THREAD_SAFE
        thread_mutex_term( &strmap->mutex );
    #endif
    free( strmap );
}


void strmap_clear( strmap_t* strmap ) {
    STRMAP_MUTEX_LOCK( &strmap->mutex );
    hashtable_clear( &strmap->hashtable );
    STRMAP_MUTEX_UNLOCK( &strmap->mutex );    
}


void strmap_insert( strmap_t* strmap, str_t key, void const* item ) {
    STRMAP_MUTEX_LOCK( &strmap->mutex );
    hashtable_insert( &strmap->hashtable, strmap_hash_u32( key ), &key, item );
    STRMAP_MUTEX_UNLOCK( &strmap->mutex );    
}


void strmap_remove( strmap_t* strmap, str_t key ) {
    STRMAP_MUTEX_LOCK( &strmap->mutex );
    hashtable_remove( &strmap->hashtable, strmap_hash_u32( key ), &key );
    STRMAP_MUTEX_UNLOCK( &strmap->mutex );    
}


bool strmap_update( strmap_t* strmap, str_t key, void const* item ) {
    STRMAP_MUTEX_LOCK( &strmap->mutex );
    void* result = hashtable_find( &strmap->hashtable, strmap_hash_u32( key ), &key );
    if( result ) {
        memcpy( result, item, (size_t) strmap->item_size );
    }
    STRMAP_MUTEX_UNLOCK( &strmap->mutex );    
    return result != NULL;
}


bool strmap_find( strmap_t* strmap, str_t key, void* item ) {
    STRMAP_MUTEX_LOCK( &strmap->mutex );
    void const* result = hashtable_find( &strmap->hashtable, strmap_hash_u32( key ), &key );
    if( result ) {
        memcpy( item, result, (size_t) strmap->item_size );
    }
    STRMAP_MUTEX_UNLOCK( &strmap->mutex );    
    return result != NULL;
}


#undef STRMAP_MUTEX_LOCK
#undef STRMAP_MUTEX_UNLOCK


#endif /* STRMAP_IMPLEMENTATION */
