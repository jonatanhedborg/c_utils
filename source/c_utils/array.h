#ifndef array_h
#define array_h

// To make array thread safe, do this before include: #define ARRAY_THREAD_SAFE

typedef struct array_t array_t;

array_t* array_create( int item_size );
void array_destroy( array_t* array );
void array_add( array_t* array, void* item );
void array_remove( array_t* array, int index );
void array_remove_ordered( array_t* array, int index );
bool array_get( array_t* array, int index, void* item );
bool array_set( array_t* array, int index, void const* item );
int array_count( array_t* array );
void array_sort( array_t* array, int (*compare)( void const*, void const* ) );
int array_bsearch( array_t* array, void* key, int (*compare)( void const*, void const* ) );

#ifndef ARRAY_THREAD_SAFE
    void* array_item( array_t* array, int index );
#endif


#endif /* array_h */


#ifdef ARRAY_IMPLEMENTATION
#undef ARRAY_IMPLEMENTATION

#include <stdlib.h>
#include <string.h>

typedef struct array_t {
    int item_size;
    int capacity;
    int count;
    void* items;
    #ifdef ARRAY_THREAD_SAFE
        thread_mutex_t mutex;
    #endif
} array_t;


#ifdef ARRAY_THREAD_SAFE
    #define ARRAY_MUTEX_LOCK(x) thread_mutex_lock( (x) )
    #define ARRAY_MUTEX_UNLOCK(x) thread_mutex_unlock( (x) )
#else
    #define ARRAY_MUTEX_LOCK(x) 
    #define ARRAY_MUTEX_UNLOCK(x) 
#endif

array_t* array_create( int item_size ) {
    array_t* array = (array_t*) malloc( sizeof( array_t ) );
    array->item_size = item_size;
    array->capacity = 256;
    array->count = 0;
    array->items = malloc( array->capacity * item_size );
    #ifdef ARRAY_THREAD_SAFE
        thread_mutex_init( &array->mutex );
    #endif
    return array;
}


void array_destroy( array_t* array ) {
    ARRAY_MUTEX_LOCK( &array->mutex );
    free( array->items );
    ARRAY_MUTEX_UNLOCK( &array->mutex );
    #ifdef ARRAY_THREAD_SAFE
        thread_mutex_term( &array->mutex );
    #endif
    free( array );
}


void array_add( array_t* array, void* item ) {
    ARRAY_MUTEX_LOCK( &array->mutex );
    if( array->count >= array->capacity ) {
        array->capacity *= 2;
        array->items = realloc( array->items, array->capacity * array->item_size );
    }
    memcpy( (void*)( ( (uintptr_t) array->items ) + array->count * array->item_size ), item, array->item_size );
    ++array->count;
    ARRAY_MUTEX_UNLOCK( &array->mutex );
}


void array_remove( array_t* array, int index ) {
    ARRAY_MUTEX_LOCK( &array->mutex );
    if( index >= 0 && index < array->count ) {
        --array->count;
        memmove( (void*)( ( (uintptr_t) array->items ) + index * array->item_size ), 
            (void*)( ( (uintptr_t) array->items ) + array->count  * array->item_size ), array->item_size );
    }
    ARRAY_MUTEX_UNLOCK( &array->mutex );
}

void array_remove_ordered( array_t* array, int index ) {
    ARRAY_MUTEX_LOCK( &array->mutex );
    if( index >= 0 && index < array->count ) {
        --array->count;
        memmove( (void*)( ( (uintptr_t) array->items ) + index * array->item_size ), 
            (void*)( ( (uintptr_t) array->items ) + ( index + 1 ) * array->item_size ), array->item_size * ( array->count - index ) );
    }
    ARRAY_MUTEX_UNLOCK( &array->mutex );
}

bool array_get( array_t* array, int index, void* item ) {
    ARRAY_MUTEX_LOCK( &array->mutex );
    bool result = index >= 0 && index < array->count;
    if( result ) {
        memcpy( item, (void*)( ( (uintptr_t) array->items ) + index * array->item_size ), array->item_size );
    }
    ARRAY_MUTEX_UNLOCK( &array->mutex );
    return result;
}

bool array_set( array_t* array, int index, void const* item ) {
    ARRAY_MUTEX_LOCK( &array->mutex );
    bool result = index >= 0 && index < array->count;
    if( result ) {
        memcpy( (void*)( ( (uintptr_t) array->items ) + index * array->item_size ), item, array->item_size );
    }
    ARRAY_MUTEX_UNLOCK( &array->mutex );
    return result;
}

int array_count( array_t* array ) {
    ARRAY_MUTEX_LOCK( &array->mutex );
    int count = array->count;
    ARRAY_MUTEX_UNLOCK( &array->mutex );
    return count;
}


void array_sort( array_t* array, int (*compare)( void const*, void const* ) ) {
    ARRAY_MUTEX_LOCK( &array->mutex );
    qsort( array->items, array->count, array->item_size, compare );
    ARRAY_MUTEX_UNLOCK( &array->mutex );
}

int array_bsearch( array_t* array, void* key, int (*compare)( void const*, void const* ) ) {
    ARRAY_MUTEX_LOCK( &array->mutex );
    void* item = bsearch( key, array->items, array->count, array->item_size, compare );
    int result = -1;
    if( item ) {
        result = ( ((uintptr_t)item) - ((uintptr_t)array->items) ) / array->item_size;
    }       
    ARRAY_MUTEX_UNLOCK( &array->mutex );
    return result;
}


#ifndef ARRAY_THREAD_SAFE
    void* array_item( array_t* array, int index ) {
        if(  index >= 0 && index < array->count ) {
            return (void*)( ( (uintptr_t) array->items ) + index * array->item_size );
        } else {
            return NULL;
        }
    }
#endif

#undef ARRAY_MUTEX_LOCK
#undef ARRAY_MUTEX_UNLOCK

#endif /* ARRAY_IMPLEMENTATION */