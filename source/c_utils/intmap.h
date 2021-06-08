#ifndef intmap_h
#define intmap_h

// To make intmap thread safe, do this before include: #define INTMAP_THREAD_SAFE

typedef struct intmap_t intmap_t;

intmap_t* intmap_create( int item_size );
void intmap_destroy( intmap_t* intmap );
void intmap_clear( intmap_t* intmap );
void intmap_insert( intmap_t* intmap, int key, void const* item );
void intmap_remove( intmap_t* intmap, int key );
bool intmap_update( intmap_t* intmap, int key, void const* item );
bool intmap_find( intmap_t* intmap, int key, void* item );

#endif /* intmap_h */


#ifdef INTMAP_IMPLEMENTATION
#undef INTMAP_IMPLEMENTATION

#include <stdlib.h>
#include <string.h>
#include "hashtable.h"

static uint32_t intmap_hash_u32( uint32_t key ) {
    key = ~key + ( key << 15 );
    key = key ^ ( key >> 12 );
    key = key + ( key << 2 );
    key = key ^ ( key >> 4 );
    key = (key + ( key << 3 ) ) + ( key << 11 );
    key = key ^ ( key >> 16);
    return key;
}


typedef struct intmap_t {
    hashtable_t hashtable;
    int item_size;
    #ifdef INTMAP_THREAD_SAFE
        thread_mutex_t mutex;
    #endif
} intmap_t;


#ifdef INTMAP_THREAD_SAFE
    #define INTMAP_MUTEX_LOCK(x) thread_mutex_lock( (x) )
    #define INTMAP_MUTEX_UNLOCK(x) thread_mutex_unlock( (x) )
#else
    #define INTMAP_MUTEX_LOCK(x) 
    #define INTMAP_MUTEX_UNLOCK(x) 
#endif


intmap_t* intmap_create( int item_size ) {
    intmap_t* intmap = (intmap_t*) malloc( sizeof( intmap_t ) );
    int const initial_capacity = 256;
    hashtable_init( &intmap->hashtable, sizeof( int ), item_size,  initial_capacity, NULL );
    #ifdef INTMAP_THREAD_SAFE
        thread_mutex_init( &intmap->mutex );
    #endif
    intmap->item_size = item_size;
    return intmap;
}


void intmap_destroy( intmap_t* intmap ) {
    INTMAP_MUTEX_LOCK( &intmap->mutex );
    hashtable_term( &intmap->hashtable );
    INTMAP_MUTEX_UNLOCK( &intmap->mutex );    
    #ifdef INTMAP_THREAD_SAFE
        thread_mutex_term( &intmap->mutex );
    #endif
    free( intmap );
}


void intmap_clear( intmap_t* intmap ) {
    INTMAP_MUTEX_LOCK( &intmap->mutex );
    hashtable_clear( &intmap->hashtable );
    INTMAP_MUTEX_UNLOCK( &intmap->mutex );    
}


void intmap_insert( intmap_t* intmap, int key, void const* item ) {
    INTMAP_MUTEX_LOCK( &intmap->mutex );
    hashtable_insert( &intmap->hashtable, intmap_hash_u32( key ), &key, item );
    INTMAP_MUTEX_UNLOCK( &intmap->mutex );    
}


void intmap_remove( intmap_t* intmap, int key ) {
    INTMAP_MUTEX_LOCK( &intmap->mutex );
    hashtable_remove( &intmap->hashtable, intmap_hash_u32( key ), &key );
    INTMAP_MUTEX_UNLOCK( &intmap->mutex );    
}


bool intmap_update( intmap_t* intmap, int key, void const* item ) {
    INTMAP_MUTEX_LOCK( &intmap->mutex );
    void* result = hashtable_find( &intmap->hashtable, intmap_hash_u32( key ), &key );
    if( result ) {
        memcpy( result, item, (size_t) intmap->item_size );
    }
    INTMAP_MUTEX_UNLOCK( &intmap->mutex );    
    return result != NULL;
}


bool intmap_find( intmap_t* intmap, int key, void* item ) {
    INTMAP_MUTEX_LOCK( &intmap->mutex );
    void const* result = hashtable_find( &intmap->hashtable, intmap_hash_u32( key ), &key );
    if( result ) {
        memcpy( item, result, (size_t) intmap->item_size );
    }
    INTMAP_MUTEX_UNLOCK( &intmap->mutex );    
    return result != NULL;
}


#undef INTMAP_MUTEX_LOCK
#undef INTMAP_MUTEX_UNLOCK


#endif /* INTMAP_IMPLEMENTATION */
