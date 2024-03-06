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

    Result() = default;
    Result(ResultStatus s) : status(s) {} 
};

#define ENABLE_ASSERT
#ifdef ENABLE_ASSERT
void assert_impl(bool value, const char* expression, size_t line, const char* file);
#define assert(condition) assert_impl((condition), #condition, __LINE__, __FILE__)
#else
#define assert(condition)
#endif

size_t max(size_t a, size_t b);

#include <string.h>

template<typename T>
class Vector {
    public:
        Vector(Allocator* allocator) : m_data(nullptr), m_length(0), m_capacity(0), m_allocator(allocator) {}
        ~Vector() {
            m_allocator->free(m_data, m_capacity * sizeof(T));
        }
        Vector(const Vector&) = delete;

        // returns nullptr on failure
        T* push(T value) {
            if (m_capacity <= m_length) {
                size_t old_capacity = m_capacity;
                m_capacity = max(m_capacity * 2, m_length + 1);
                if (m_data == nullptr) {
                    m_data = (T*)m_allocator->alloc(m_capacity * sizeof(T));
                    if (m_data == nullptr) {
                        return nullptr;
                    }
                }
                else {
                    T* data = (T*)m_allocator->realloc(m_data, old_capacity * sizeof(T), m_capacity * sizeof(T));
                    if (data == nullptr) {
                        return nullptr;
                    }
                    m_data = data;
                }
            }
            m_data[m_length] = value;
            return m_data + (m_length++);
        }

        T* push_many(T* data, size_t count) {
            if (count > m_capacity - m_length) {
                // expand
                size_t old_capacity = m_capacity;
                m_capacity = max(m_capacity * 2, m_length + count);
                m_data = (T*)m_allocator->realloc(m_data, old_capacity * sizeof(T), m_capacity * sizeof(T));
                if (m_data == nullptr) {
                    return nullptr;
                }
                #ifdef NDEBUG
                    memset(m_data + m_length + count, 0, (m_capacity - m_length - count) * sizeof(T));
                #endif
            }
            memcpy(m_data + m_length, data, count * sizeof(T));
            T* res = m_data + m_length;
            m_length += count;
            return res;
        }

        bool remove(size_t index) {
            assert(index < m_length);
            if (index < m_length - 1) {
                size_t right = m_length - index - 1;
                void* move_res = memmove(m_data + index, m_data + index + 1, right * sizeof(T));
                m_length--;
                return move_res != nullptr;
            }
            m_length--;
            return true;
        }

        bool remove_many(size_t index, size_t count) {
            assert(index + count < m_length);
            if (index + count < m_length - 1) {
                size_t right = m_length - index - count;
                void* move_res = memmove(m_data + index, m_data + index + count, right * sizeof(T));
                m_length -= count;
                return move_res != nullptr;
            }
            m_length -= count;
            return true;
        }

        void clear() {
            m_length = 0;
        }

        T* first() {
            if (m_length > 0 && m_data != nullptr) {
                return m_data;
            }
            return nullptr;
        }

        T& operator[](size_t index) {
            assert(index < m_length);
            return m_data[index];
        }

        inline T* ptr() const { return m_data; }
        inline size_t length() const { return m_length; }
        inline size_t capacity() const { return m_capacity; }
    private:
        T* m_data;
        size_t m_length;
        size_t m_capacity;
        Allocator* m_allocator;
};


int count_leading_zeros_uint_fallback(unsigned int num);
int count_leading_zeros_uint(unsigned int num);

uint8_t max_bits_u8(uint8_t bits);
uint8_t max_u8(uint8_t number);

class Serializer {
    public:
        Serializer(Writer* writer, Allocator* allocator);
        ~Serializer();

        // serialize uint8_t
        Result serialize_uint8(uint8_t value);
        // serialize uint8_t with max amount of bits specified in order to reduce the required storage space
        // NOTE: passing a value with more bits than the max bits is an undefined behaviour, this is not a validator
        Result serialize_uint8_max(uint8_t value, uint8_t max_bits);

        // serialize uint16_t
        Result serialize_uint16(uint16_t value);
        // serialize uint16_t with max amount of bits specified in order to reduce the required storage space
        // NOTE: passing a value with more bits than the max bits is an undefined behaviour, this is not a validator
        Result serialize_uint16_max(uint16_t value, uint8_t max_bits);

        // serialize uint32_t
        Result serialize_uint32(uint32_t value);
        // serialize uint32_t with max amount of bits specified in order to reduce the required storage space with some extra size optimizations
        // NOTE: passing a value with more bits than the max bits is an undefined behaviour, this is not a validator
        Result serialize_uint32_max(uint32_t value, uint8_t max_bits);

        // serializes a boolean value
        Result serialize_bool(bool value);

        // flushes the buffers and resets the serializer
        // after calling this method it is possible to reuse the same instance of the serializer
        Result finalize();

    private:
        Result push_bit(uint8_t value);
        Result push_bits(uint64_t value, size_t count);

        Result flush_buffer();

        Result get_free_bits(SerializerFreeBits** result);
    private:
        Writer* m_writer;
        // Allocator* m_allocator;
        size_t m_start_index;
        Vector<uint8_t> m_buffer;
        Vector<SerializerFreeBits> m_free_bits;
};

class Deserializer {
    public:
        Deserializer(Reader* reader, Allocator* allocator);
        ~Deserializer();

        // Deserialize uint8_t, returns 0 on failure with an error in the result
        Result deserialize_uint8(uint8_t* value);
        // deserialize uint8_t with max amount of bits specified. returns 0 on failure with an error in the result
        // NOTE: passing a value with more bits than the max bits is an undefined behaviour, this is not a validator
        Result deserialize_uint8_max(uint8_t max_bits, uint8_t* value);

        // deserialize uint16_t
        Result deserialize_uint16(uint16_t* value);
        // deserialize uint16_t with max amount of bits specified in order to reduce the required storage space
        Result deserialize_uint16_max(uint8_t max_bits, uint16_t* value);

        // deserialize uint32_t
        Result deserialize_uint32(uint32_t* value);
        // deserialize uint32_t with max amount of bits specified in order to reduce the required storage space
        Result deserialize_uint32_max(uint8_t max_bits, uint32_t* value);

        // Deserialize bool, returns false on failure with an error in the result
        Result deserialize_bool(bool* value);
    private:
        Result read_bit(uint8_t* value);
        Result read_bits(size_t count, uint64_t* bits);

        Result get_free_bits(DeserializerFreeBits** out_free_bits);
    private:
        Reader* m_reader;
        Allocator* m_allocator;
        Vector<DeserializerFreeBits> m_free_bits;
};