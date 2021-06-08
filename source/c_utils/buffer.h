#ifndef buffer_h
#define buffer_h

// To make buffer thread safe, do this before include: #define BUFFER_THREAD_SAFE

typedef struct buffer_t buffer_t;

buffer_t* buffer_create( void );
buffer_t* buffer_load( const char* filename );
void buffer_destroy( buffer_t* buffer );
bool buffer_save( buffer_t* buffer, char const* filename );
int buffer_position( buffer_t* buffer );
int buffer_position_set( buffer_t* buffer, int position );
int buffer_size( buffer_t* buffer );

int buffer_read_char( buffer_t* buffer, char* value, int count );
int buffer_read_i8( buffer_t* buffer, int8_t* value, int count );
int buffer_read_i16( buffer_t* buffer, int16_t* value, int count );
int buffer_read_i32( buffer_t* buffer, int32_t* value, int count );
int buffer_read_i64( buffer_t* buffer, int64_t* value, int count );
int buffer_read_u8( buffer_t* buffer, uint8_t* value, int count );
int buffer_read_u16( buffer_t* buffer, uint16_t* value, int count );
int buffer_read_u32( buffer_t* buffer, uint32_t* value, int count );
int buffer_read_u64( buffer_t* buffer, uint64_t* value, int count );
int buffer_read_float( buffer_t* buffer, float* value, int count );
int buffer_read_double( buffer_t* buffer, double* value, int count );
int buffer_read_bool( buffer_t* buffer, bool* value, int count );

int buffer_write_char( buffer_t* buffer, char const* value, int count );
int buffer_write_i8( buffer_t* buffer, int8_t const* value, int count );
int buffer_write_i16( buffer_t* buffer, int16_t const* value, int count );
int buffer_write_i32( buffer_t* buffer, int32_t const* value, int count );
int buffer_write_i64( buffer_t* buffer, int64_t const* value, int count );
int buffer_write_u8( buffer_t* buffer, uint8_t const* value, int count );
int buffer_write_u16( buffer_t* buffer, uint16_t const* value, int count );
int buffer_write_u32( buffer_t* buffer, uint32_t const* value, int count );
int buffer_write_u64( buffer_t* buffer, uint64_t const* value, int count );
int buffer_write_float( buffer_t* buffer, float const* value, int count );
int buffer_write_double( buffer_t* buffer, double const* value, int count );
int buffer_write_bool( buffer_t* buffer, bool const* value, int count );

#endif /* buffer_h */


#ifdef BUFFER_IMPLEMENTATION
#undef BUFFER_IMPLEMENTATION

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static uint32_t buffer_pow2ceil( uint32_t v ) {
    --v;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    ++v;
    v += ( v == 0 );
    return v;
}


typedef struct buffer_t {
    int capacity;
    int size;
    int position;
    void* data;
    #ifdef BUFFER_THREAD_SAFE
        thread_mutex_t mutex;
    #endif
} buffer_t;



#ifdef BUFFER_THREAD_SAFE
    #define BUFFER_MUTEX_LOCK(x) thread_mutex_lock( (x) )
    #define BUFFER_MUTEX_UNLOCK(x) thread_mutex_unlock( (x) )
#else
    #define BUFFER_MUTEX_LOCK(x) 
    #define BUFFER_MUTEX_UNLOCK(x) 
#endif


buffer_t* buffer_create( void ) {
    buffer_t* buffer = (buffer_t*) malloc( sizeof( buffer_t) );
    buffer->capacity = 4096;
    buffer->size = 0;
    buffer->position = 0;
    buffer->data = malloc( buffer->capacity );
    #ifdef BUFFER_THREAD_SAFE
        thread_mutex_init( &buffer->mutex );
    #endif
    return buffer;
}


buffer_t* buffer_load( const char* filename ) {
    FILE* fp = fopen( filename, "rb" );
    if( !fp ) {
        return NULL;
    }
    fseek( fp, 0, SEEK_END );
    int size = (int) ftell( fp );
    fseek( fp, 0, SEEK_SET );
    if( size == 0 ) {
        fclose( fp );
        return NULL;
    }

    int pow2size = (int)buffer_pow2ceil( (uint32_t) size );
    void* data = malloc( pow2size );
    int read = (int) fread( data, 1, size, fp );
    fclose( fp );
    if( read != size ) {
        free( data );
        return NULL;
    }

    buffer_t* buffer = (buffer_t*) malloc( sizeof( buffer_t) );
    buffer->capacity = pow2size;
    buffer->size = size;
    buffer->position = 0;
    buffer->data = data;
    #ifdef BUFFER_THREAD_SAFE
        thread_mutex_init( &buffer->mutex );
    #endif
    return buffer;
}


void buffer_destroy( buffer_t* buffer ) {
    BUFFER_MUTEX_LOCK( &buffer->mutex );
    free( buffer->data );
    BUFFER_MUTEX_UNLOCK( &buffer->mutex );
    #ifdef BUFFER_THREAD_SAFE
        thread_mutex_term( &buffer->mutex );
    #endif
    free( buffer );
}


bool buffer_save( buffer_t* buffer, char const* filename ) {
    FILE* fp = fopen( filename, "wb" );
    if( !fp ) {
        return false;
    }
    BUFFER_MUTEX_LOCK( &buffer->mutex );
    int written = (int) fwrite( buffer->data, 1, buffer->size, fp ); 
    fclose( fp );
    return written == buffer->size;
    BUFFER_MUTEX_UNLOCK( &buffer->mutex );
}


int buffer_position( buffer_t* buffer ) {
    BUFFER_MUTEX_LOCK( &buffer->mutex );
    int result = buffer->position;
    BUFFER_MUTEX_UNLOCK( &buffer->mutex );
    return result;
}


int buffer_position_set( buffer_t* buffer, int position ) {
    BUFFER_MUTEX_LOCK( &buffer->mutex );
    buffer->position = position < 0 ? 0 : position > buffer->size ? buffer->size : position;
    int result = buffer->position;
    BUFFER_MUTEX_UNLOCK( &buffer->mutex );
    return result;
}


int buffer_size( buffer_t* buffer ) {
    BUFFER_MUTEX_LOCK( &buffer->mutex );
    int result = buffer->size;
    BUFFER_MUTEX_UNLOCK( &buffer->mutex );
    return result;
}

// TODO: endian swap
#define BUFFER_READ_IMPL \
    { \
        BUFFER_MUTEX_LOCK( &buffer->mutex ); \
        int result = 0; \
        for( int i = 0; i < count; ++i ) { \
            if( buffer->position + sizeof( *value ) > buffer->size ) { \
                BUFFER_MUTEX_UNLOCK( &buffer->mutex ); \
                return result; \
            } \
            memcpy( &value[ i ], (void*)( ( (uintptr_t) buffer->data ) + buffer->position ), sizeof( *value ) ); \
            buffer->position += sizeof( *value ); \
            ++result; \
        } \
        BUFFER_MUTEX_UNLOCK( &buffer->mutex ); \
        return result; \
    }

int buffer_read_char( buffer_t* buffer, char* value, int count ) BUFFER_READ_IMPL
int buffer_read_i8( buffer_t* buffer, int8_t* value, int count ) BUFFER_READ_IMPL
int buffer_read_i16( buffer_t* buffer, int16_t* value, int count ) BUFFER_READ_IMPL
int buffer_read_i32( buffer_t* buffer, int32_t* value, int count ) BUFFER_READ_IMPL
int buffer_read_i64( buffer_t* buffer, int64_t* value, int count ) BUFFER_READ_IMPL
int buffer_read_u8( buffer_t* buffer, uint8_t* value, int count ) BUFFER_READ_IMPL
int buffer_read_u16( buffer_t* buffer, uint16_t* value, int count ) BUFFER_READ_IMPL
int buffer_read_u32( buffer_t* buffer, uint32_t* value, int count ) BUFFER_READ_IMPL
int buffer_read_u64( buffer_t* buffer, uint64_t* value, int count ) BUFFER_READ_IMPL
int buffer_read_float( buffer_t* buffer, float* value, int count ) BUFFER_READ_IMPL
int buffer_read_double( buffer_t* buffer, double* value, int count ) BUFFER_READ_IMPL
int buffer_read_bool( buffer_t* buffer, bool* value, int count ) BUFFER_READ_IMPL

#undef BUFFER_READ_IMPL

	
// TODO: endian swap
#define BUFFER_WRITE_IMPL \
    { \
        BUFFER_MUTEX_LOCK( &buffer->mutex ); \
        int result = 0; \
        for( int i = 0; i < count; ++i ) { \
            if( buffer->position + sizeof( *value ) > buffer->size ) { \
                buffer->size = buffer->position + sizeof( *value ); \
                while( buffer->size > buffer->capacity ) { \
                    buffer->capacity *= 2; \
                } \
                buffer->data = realloc( buffer->data, buffer->capacity ); \
            } \
            memcpy( (void*)( ( (uintptr_t) buffer->data ) + buffer->position ), &value[ i ], sizeof( *value ) ); \
            buffer->position += sizeof( *value ); \
            ++result; \
        } \
        return result; \
        BUFFER_MUTEX_UNLOCK( &buffer->mutex ); \
    }

int buffer_write_char( buffer_t* buffer, char const* value, int count ) BUFFER_WRITE_IMPL
int buffer_write_i8( buffer_t* buffer, int8_t const* value, int count ) BUFFER_WRITE_IMPL
int buffer_write_i16( buffer_t* buffer, int16_t const* value, int count ) BUFFER_WRITE_IMPL
int buffer_write_i32( buffer_t* buffer, int32_t const* value, int count ) BUFFER_WRITE_IMPL
int buffer_write_i64( buffer_t* buffer, int64_t const* value, int count ) BUFFER_WRITE_IMPL
int buffer_write_u8( buffer_t* buffer, uint8_t const* value, int count ) BUFFER_WRITE_IMPL
int buffer_write_u16( buffer_t* buffer, uint16_t const* value, int count ) BUFFER_WRITE_IMPL
int buffer_write_u32( buffer_t* buffer, uint32_t const* value, int count ) BUFFER_WRITE_IMPL
int buffer_write_u64( buffer_t* buffer, uint64_t const* value, int count ) BUFFER_WRITE_IMPL
int buffer_write_float( buffer_t* buffer, float const* value, int count ) BUFFER_WRITE_IMPL
int buffer_write_double( buffer_t* buffer, double const* value, int count ) BUFFER_WRITE_IMPL
int buffer_write_bool( buffer_t* buffer, bool const* value, int count ) BUFFER_WRITE_IMPL 

#undef BUFFER_WRITE_IMPL

#undef BUFFER_MUTEX_LOCK
#undef BUFFER_MUTEX_UNLOCK

#endif /* BUFFER_IMPLEMENTATION */
