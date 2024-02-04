#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>

// A writer interface to write output data in a generic way.
// The write method should return NULL on success or any other number on failure.
typedef struct {
    int(*write)(void*,uint8_t*,size_t);
    void* data;
} Writer;

// A reader interface to read data in a generic way.
// The read method should return NULL on success or any other number on failure.
// It takes the number of bytes to be read
typedef struct {
    uint8_t*(*read)(void*,size_t);
    void* data;
} Reader;

// A generic way to allocate memory without requiring libc
typedef struct {
    // allocate memory, returning NULL on failure
    void*(*alloc)(size_t);
    void*(*realloc)(void*, size_t);
    void(*free)(void*);
} Allocator;



typedef struct {
    Writer* writer;
    Allocator allocator;
} Serializer;

typedef struct {
    Reader* reader;
    Allocator allocator;
} Deserializer;

typedef enum {
    Status_Success = 0,
    // Indicates that an attempt to allocate memory has failed (alloc function returned 0)
    Status_MemoryAllocationFailed,
    // Indicates that output has failed to be written into the writer, error code in write_error
    Status_WriteFailed,
    // Indicates that input has failed to be read from the reader
    Status_ReadFailed
} ResultStatus;
const char* status_to_string(ResultStatus status);

typedef struct {
    ResultStatus status;
    union {
        int write_error;
    } error_info;
} Result;

typedef struct {
    uint8_t* data;
    size_t capacity;
    size_t size;
} Buffer;

Buffer create_buffer(size_t capacity);
void free_buffer(Buffer* buffer);
int push_bytes(Buffer* buffer, uint8_t* data, size_t size);

Serializer create_serializer(Writer* writer, Allocator allocator);
Deserializer create_deserializer(Reader* reader, Allocator allocator);

// serialize uint8_t
void serialize_uint8(Serializer* serializer, uint8_t value, Result* result);

// Deserialize uint8_t, returns 0 on failure with an error in the result
uint8_t deserialize_uint8(Deserializer* deserializer, Result* result);