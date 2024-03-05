#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>

// A writer interface to write output data in a generic way.
// The write method should return NULL on success or any other number on failure.
struct Writer {
    int(*write_callback)(void*,uint8_t*,size_t);
    void* ctx;

    int write(uint8_t* value, size_t size);
    int write_byte(uint8_t value);
};

// A reader interface to read data in a generic way.
// The read method should return NULL on success or any other number on failure.
// It takes the number of bytes to be read
struct Reader{
    uint8_t*(*read_callback)(void*,size_t);
    void* ctx;

    uint8_t* read(size_t size);
};

// A generic way to allocate memory without requiring libc
struct Allocator{
    // allocate memory, returning NULL on failure
    // alloc(size, ctx)
    void* (*alloc_callback)(size_t, void*);

    // return null on failure, doesn't call free on failure
    // realloc(ptr, old_size, new_size, ctx)
    void* (*realloc_callback)(void*, size_t, size_t, void*);

    // size will should always be the size passed into alloc
    // free(ptr, size, ctx)
    void (*free_callback)(void*, size_t, void*);
    void* ctx;

    void* alloc(size_t size);
    void* realloc(void* ptr, size_t old_size, size_t new_size);
    void free(void* ptr, size_t size);
};

struct Buffer{
    uint8_t* data;
    size_t capacity;
    size_t size;
};

// Internal
// a reference to a byte in the buffer which contains some free bits
struct SerializerFreeBits{
    size_t index;
    size_t start;
    size_t end;
};

// Internal
// a byte with some free bits in it for read
struct DeserializerFreeBits {
    uint8_t byte;
    uint8_t start;
    uint8_t end;
};


// Internal
struct SerializerFreeBitsVector {
    SerializerFreeBits* data;
    size_t count;
    size_t capacity;
};

// Internal
struct DeserializerFreeBitsVector{
    DeserializerFreeBits* data;
    size_t count;
    size_t capacity;
};

// struct Serializer{
//     Writer* writer;
//     Allocator allocator;
//     Buffer buffer;
//     SerializerFreeBitsVector free_bits;
//     size_t start_index;
// };

struct Deserializer {
    Reader* reader;
    Allocator allocator;
    DeserializerFreeBitsVector free_bits;
};

enum class ResultStatus {
    Success = 0,
    // Indicates that an attempt to allocate memory has failed (alloc function returned 0)
    MemoryAllocationFailed,
    // Indicates that an attempt to perform an operation on memory(e.g. memmove) has failed
    MemoryOperationFailed,
    // Indicates that output has failed to be written into the writer, error code in write_error
    WriteFailed,
    // Indicates that input has failed to be read from the reader
    ReadFailed
};
const char* status_to_string(ResultStatus status);

struct Result {
    ResultStatus status;
    union {
        int write_error;
    } error_info;
};


int count_leading_zeros_uint_fallback(unsigned int num);
int count_leading_zeros_uint(unsigned int num);

uint8_t max_bits_u8(uint8_t bits);
uint8_t max_u8(uint8_t number);

Buffer create_buffer(size_t capacity, Allocator* allocator);
void free_buffer(Buffer* buffer, Allocator* allocator);
int buffer_push_bytes(Buffer* buffer, uint8_t* data, size_t size, Allocator* allocator);

SerializerFreeBitsVector create_free_bits_ref_vector(size_t capacity, Allocator* allocator);
SerializerFreeBits* vector_push(SerializerFreeBitsVector* vector, SerializerFreeBits value, Allocator* allocator);
SerializerFreeBits* vector_first(SerializerFreeBitsVector* vector);
int vector_remove(SerializerFreeBitsVector* vector, size_t index);
void vector_clear(SerializerFreeBitsVector* vector);
void free_vector(SerializerFreeBitsVector* vector, Allocator* allocator);


class Serializer {
    public:
        Serializer(Writer* writer, Allocator* allocator);
        ~Serializer();

        // serialize uint8_t
        void serialize_uint8(uint8_t value, Result* result);
        // serialize uint8_t with max amount of bits specified in order to reduce the required storage space
        // NOTE: passing a value with more bits than the max bits is an undefined behaviour, this is not a validator
        void serialize_uint8_max(uint8_t value, uint8_t max_bits, Result* result);

        // serialize uint16_t
        void serialize_uint16(uint16_t value, Result* result);
        // serialize uint16_t with max amount of bits specified in order to reduce the required storage space
        // NOTE: passing a value with more bits than the max bits is an undefined behaviour, this is not a validator
        void serialize_uint16_max(uint16_t value, uint8_t max_bits, Result* result);

        // serialize uint32_t
        void serialize_uint32(uint32_t value, Result* result);
        // serialize uint32_t with max amount of bits specified in order to reduce the required storage space with some extra size optimizations
        // NOTE: passing a value with more bits than the max bits is an undefined behaviour, this is not a validator
        void serialize_uint32_max(uint32_t value, uint8_t max_bits, Result* result);

        // serializes a boolean value
        void serialize_bool(bool value, Result* result);

        // flushes the buffers and resets the serializer
        // after calling this method it is possible to reuse the same instance of the serializer
        void finalize(Result* result);

    private:
        void push_bit(uint8_t value, Result* result);
        void push_bits(uint64_t value, size_t count, Result* result);

        void flush_buffer(Result* result);

        SerializerFreeBits* get_free_bits(Result* result);
    private:
        Writer* m_writer;
        Allocator* m_allocator;
        size_t m_start_index;
        Buffer m_buffer;
        SerializerFreeBitsVector m_free_bits;
};

Deserializer create_deserializer(Reader* reader, Allocator allocator);

void free_deserializer(Deserializer* deserializer);

// Deserialize uint8_t, returns 0 on failure with an error in the result
uint8_t deserialize_uint8(Deserializer* deserializer, Result* result);


// deserialize uint8_t with max amount of bits specified. returns 0 on failure with an error in the result
// NOTE: passing a value with more bits than the max bits is an undefined behaviour, this is not a validator
uint8_t deserialize_uint8_max(Deserializer* deserializer, uint8_t max_bits, Result* result);


// deserialize uint16_t
uint16_t deserialize_uint16(Deserializer* deserializer, Result* result);

// deserialize uint16_t with max amount of bits specified in order to reduce the required storage space
uint16_t deserialize_uint16_max(Deserializer* serializer, uint8_t max_bits, Result* result);

// deserialize uint32_t
uint32_t deserialize_uint32(Deserializer* deserializer, Result* result);

// deserialize uint32_t with max amount of bits specified in order to reduce the required storage space
uint32_t deserialize_uint32_max(Deserializer* deserializer, uint8_t max_bits, Result* result);

// Deserialize bool, returns false on failure with an error in the result
bool deserialize_bool(Deserializer* deserializer, Result* result);