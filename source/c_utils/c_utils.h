#ifndef c_utils_h
#define c_utils_h

#ifdef C_UTILS_THREAD_SAFE
    #define STR_THREAD_SAFE
    #define ARRAY_THREAD_SAFE
    #define STRMAP_THREAD_SAFE
    #define INTMAP_THREAD_SAFE
    #define BUFFER_THREAD_SAFE
#endif

#include "thread.h"
#include "str.h"
#include "array.h"
#include "strmap.h"
#include "intmap.h"
#include "buffer.h"

#endif /* c_utils_h */


#ifdef C_UTILS_IMPLEMENTATION
#undef C_UTILS_IMPLEMENTATION

#define STR_IMPLEMENTATION
#include "str.h"

#define ARRAY_IMPLEMENTATION
#include "array.h"

#define STRMAP_IMPLEMENTATION
#include "strmap.h"

#define INTMAP_IMPLEMENTATION
#include "intmap.h"

#define BUFFER_IMPLEMENTATION
#include "buffer.h"

#define THREAD_IMPLEMENTATION
#include "thread.h"

#define STRPOOL_IMPLEMENTATION
#include "strpool.h"

#define HASHTABLE_IMPLEMENTATION
#include "hashtable.h"

#endif /* C_UTILS_IMPLEMENTATION */