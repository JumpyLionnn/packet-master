#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>

// A writer interface to write output data to in a generic way.
// The write method should return 0 on success or any other number on failure.
typedef struct {
    int(*write)(void*,uint8_t*,size_t);
    void* data;
} Writer;

// A generic way to allocate memory without requiring libc
typedef struct {
    // allocate memory, returning 0 on failure
    void*(*alloc)(size_t);
    void*(*realloc)(void*, size_t);
    void(*free)(void*);
} Allocator;

typedef struct {
    Writer* writer;
    Allocator allocator;
} Serializer;


typedef enum {
    Status_Success = 0,
    // Indicates that an attempt to allocate memory has failed (alloc function returned 0)
    Status_MemoryAllocationFailed,
    // Indicates that output was failed to be written into the writer, error code in write_error
    Status_WriteFailed
} ResultStatus;
const char* status_to_string(ResultStatus status);

typedef struct {
    ResultStatus status;
    union {
        int write_error;
        int test;
    } error_info;
} Result;

Result serialize_uint8(Serializer* serializer, uint8_t value);
