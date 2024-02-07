#include "packet_master.h"
#include <string.h>

int write(Writer* writer, uint8_t* value, size_t size) {
    return writer->write(writer->data, value, size);
}

int write_byte(Writer* writer, uint8_t value) {
    return writer->write(writer->data, &value, 1);
}

uint8_t* read(Reader* reader, size_t size) {
    return reader->read(reader->data, size);
}


#define BIT_MASK(start, count, type) ((type)~((type)(~(type)0) << (type)(count)) << (type)(start))

#define ENABLE_ASSERT
#ifdef ENABLE_ASSERT
#include <stdio.h>
void assert_impl(bool value, const char* expression, size_t line, const char* file) {
    if (!value) {
        fprintf(stderr, "%s:%u: Assertion failed: %s.\n", file, (uint32_t)line, expression);
    }
}
#define assert(condition) assert_impl((condition), #condition, __LINE__, __FILE__)
#else
#define assert(condition)
#endif

#define unimplemented() fprintf("%s:%u: Attempt to run code marked as unimplemented.\n", __FILE__, __LINE__)

const char* status_to_string(ResultStatus status) {
    switch (status)
    {
    case Status_Success:
        return "success";
    case Status_MemoryAllocationFailed:
        return "memory_allocation_failed";
    case Status_MemoryOperationFailed:
        return "memory_operation_failed";
    case Status_WriteFailed:
        return "write_failed";
    case Status_ReadFailed:
        return "read_failed";
    default:
        return "unknown";
    }  
}

int count_leading_zeros_uint_fallback(unsigned int num) {
    int higher_bits = 32;
    // the center bit
    int center = 16;
    while (center != 0) {
        unsigned int higher_half = num >> center;
        if (higher_half != 0) {
            // checking the higher half of center
            higher_bits = higher_bits - center;
            num = higher_half;
        }
        // keeping the lower half
        center = center >> 1;
    }
    return higher_bits - num;
}

#ifdef _MSC_VER 
#include <immintrin.h>
#endif

int count_leading_zeros_uint(unsigned int num) {
    #if defined(__GNUC__) || defined(__GNUG__) || defined(__clang__)
        // supported in clang and gcc
        // handling zero as it is an undefined behaviour for clz
        if (num == 0) {
            return sizeof(num) * 8;
        }
        else {
            return __builtin_clz(num);
        }
    #elif defined(_MSC_VER) 
        // supported by msvc (not tested)
        // handling zero as it is an undefined behaviour for clz
        if (num == 0) {
            return sizeof(num) * 8;
        }
        else {
            return __lzcnt(num);
        }
    #else
        return count_leading_zeros_uint_fallback(num);
    #endif
}

size_t min(size_t a, size_t b) {
    if (a > b) {
        return b;
    }
    else {
        return a;
    }
}

size_t max(size_t a, size_t b) {
    if (a > b) {
        return a;
    }
    else {
        return b;
    }
}

uint32_t ceil_divide_uint32(uint32_t a, uint32_t b) {
    return (a + b - 1) / b;
}

enum Endianness {
    LittleEndian = 0,
    BigEndian
};

enum Endianness detect_endianness() {
    int n = 1;
    if(*(char *)&n == 1) {
        return LittleEndian;
    }
    else {
        return BigEndian;
    }
}

uint16_t swap_byte_order_uint16_fallback(uint16_t value) {
    return ((value & 0x00FF) << 8) | ((value & 0xFF00) >> 8);
}

uint16_t swap_byte_order_uint16(uint16_t value){
    // TODO: Add special case for msvc
    #if defined(__GNUC__) || defined(__GNUG__) || defined(__clang__)
        return __builtin_bswap16(value);
    #else
        return swap_byte_order_uint16_fallback(value);
    #endif
}

uint32_t swap_byte_order_uint32_fallback(uint32_t value) {
    return ((value & 0x000000FF) << 24) | ((value & 0x0000FF00) << 8) | ((value & 0x00FF0000) >> 8) | ((value & 0xFF000000) >> 24);
}

uint32_t swap_byte_order_uint32(uint32_t value){
    // TODO: Add special case for msvc
    #if defined(__GNUC__) || defined(__GNUG__) || defined(__clang__)
        return __builtin_bswap32(value);
    #else
        return swap_byte_order_uint16_fallback(value);
    #endif
}


uint16_t system_endianness_to_little_endian_uint16(uint16_t value) {
    if (detect_endianness() == BigEndian) {
        return swap_byte_order_uint16(value);
    }
    else {
        return value;
    }
}

uint32_t system_endianness_to_little_endian_uint32(uint32_t value) {
    if (detect_endianness() == BigEndian) {
        return swap_byte_order_uint32(value);
    }
    else {
        return value;
    }
}

uint32_t count_used_bits_uint32(uint32_t value) {
    return (uint32_t)(sizeof(unsigned int) * 8 - count_leading_zeros_uint((unsigned int)value));
} 


uint8_t max_bits_u8(uint8_t bits) {
    return bits;
}
uint8_t max_u8(uint8_t number) {
    return 8 - (uint8_t)(count_leading_zeros_uint((unsigned int)number) - (sizeof(unsigned int) - 1) * 8);
}

Buffer create_buffer(size_t capacity, Allocator* allocator) {
    return (Buffer) {
        .data = allocator->alloc(capacity),
        .capacity = capacity,
        .size = 0
    };
}

void free_buffer(Buffer* buffer, Allocator* allocator) {
    allocator->free(buffer->data);
}

int buffer_push_bytes(Buffer* buffer, uint8_t* data, size_t size, Allocator* allocator) {
    assert(buffer->size <= buffer->capacity);
    if (size > buffer->capacity - buffer->size) {
        // expand
        buffer->capacity = max(buffer->capacity * 1.5, buffer->size + size);
        buffer->data = allocator->realloc(buffer->data, buffer->capacity);
        assert(buffer->data != NULL);
        memset(buffer->data + buffer->size + size, 0, buffer->capacity - buffer->size - size);
    }
    memcpy(buffer->data + buffer->size, data, size);
    buffer->size += size;
    return 0;
}
void buffer_clear(Buffer* buffer) {
    buffer->size = 0;
}

int buffer_remove(Buffer* buffer, size_t start, size_t count) {
    assert(start + count < buffer->size);
    size_t end = start + count;
    void* move_res = memmove(buffer->data + start, buffer->data + end, buffer->size - end);
    buffer->size -= count;
    return (int)(size_t)move_res;
}


int buffer_push_byte(Buffer* buffer, uint8_t byte, Allocator* allocator) {
    return buffer_push_bytes(buffer, &byte, 1, allocator);
}

SerializerFreeBitsVector ser_create_free_bits_vector(size_t capacity, Allocator* allocator) {
    return (SerializerFreeBitsVector) {
        .capacity = capacity,
        .count = 0,
        .data = capacity == 0 ? NULL : allocator->alloc(capacity * sizeof(SerializerFreeBits))
    };
}

SerializerFreeBits* ser_free_bits_push(SerializerFreeBitsVector* vector, SerializerFreeBits value, Allocator* allocator) {
    if (vector->capacity <= vector->count) {
        vector->capacity = max(vector->capacity * 1.5, vector->count + 1);
        if (vector->data == NULL) {
            vector->data = allocator->alloc(vector->capacity);
            if (vector->data == NULL) {
                return NULL;
            }
        }
        else {
            SerializerFreeBits* data = allocator->realloc(vector->data, vector->capacity * sizeof(SerializerFreeBits));
            if (data == NULL) {
                return NULL;
            }
            vector->data = data;
        }
    }
    vector->data[vector->count] = value;
    return vector->data + (vector->count++);
}

int ser_free_bits_remove(SerializerFreeBitsVector* vector, size_t index) {
    assert(index < vector->count);
    if (index < vector->count - 1) {
        void* move_res = memmove(vector->data + index, vector->data + index + 1, vector->count - index - 1);
        return (int)(size_t)move_res;
    }
    vector->count--;
    return 0;
}

SerializerFreeBits* ser_free_bits_first(SerializerFreeBitsVector* vector) {
    if (vector->count > 0) {
        assert(vector->data != NULL && vector->capacity > 0);
        return vector->data;
    }
    return NULL;
}

void ser_free_bits_clear(SerializerFreeBitsVector* vector) {
    vector->count = 0;
}

void ser_free_bits_free(SerializerFreeBitsVector* vector, Allocator* allocator) {
    allocator->free(vector->data);
}


DeserializerFreeBitsVector deser_create_free_bits_vector(size_t capacity, Allocator* allocator) {
    return (DeserializerFreeBitsVector) {
        .capacity = capacity,
        .count = 0,
        .data = capacity == 0 ? NULL : allocator->alloc(capacity * sizeof(DeserializerFreeBits))
    };
}

DeserializerFreeBits* deser_free_bits_push(DeserializerFreeBitsVector* vector, DeserializerFreeBits value, Allocator* allocator) {
    if (vector->capacity <= vector->count) {
        vector->capacity = max(vector->capacity * 1.5, vector->count + 1);
        if (vector->data == NULL) {
            vector->data = allocator->alloc(vector->capacity);
            if (vector->data == NULL) {
                return NULL;
            }
        }
        else {
            DeserializerFreeBits* data = allocator->realloc(vector->data, vector->capacity * sizeof(DeserializerFreeBits));
            if (data == NULL) {
                return NULL;
            }
            vector->data = data;
        }
    }
    vector->data[vector->count] = value;
    return vector->data + (vector->count++);
}

int deser_free_bits_remove(DeserializerFreeBitsVector* vector, size_t index) {
    assert(index < vector->count);
    if (index < vector->count - 1) {
        void* move_res = memmove(vector->data + index, vector->data + index + 1, vector->count - index - 1);
        return (int)(size_t)move_res;
    }
    vector->count--;
    return 0;
}

DeserializerFreeBits* deser_free_bits_first(DeserializerFreeBitsVector* vector) {
    if (vector->count > 0) {
        assert(vector->data != NULL && vector->capacity > 0);
        return vector->data;
    }
    return NULL;
}

void deser_free_bits_free(DeserializerFreeBitsVector* vector, Allocator* allocator) {
    allocator->free(vector->data);
}




Serializer create_serializer(Writer* writer, Allocator allocator) {
    return (Serializer) {
        .writer = writer,
        .allocator = allocator,
        .free_bits = ser_create_free_bits_vector(0, &allocator)
    };
}

void free_serializer(Serializer* serializer) {
    free_buffer(&serializer->buffer, &serializer->allocator);
    ser_free_bits_free(&serializer->free_bits, &serializer->allocator);
}


Deserializer create_deserializer(Reader* reader, Allocator allocator) {
    return (Deserializer) {
        .reader = reader,
        .allocator = allocator
    };
}

void free_deserializer(Deserializer* deserializer) {
    deser_free_bits_free(&deserializer->free_bits, &deserializer->allocator);
}

SerializerFreeBits* serializer_get_free_bits(Serializer* serializer, Result* result) {
   SerializerFreeBits* free_bits = ser_free_bits_first(&serializer->free_bits);
   if (free_bits == NULL) {
        SerializerFreeBits value = {
            .end = 8,
            .start = 0,
            .index = serializer->buffer.size + serializer->start_index
        };
        int pushed = buffer_push_byte(&serializer->buffer, 0, &serializer->allocator);
        free_bits = ser_free_bits_push(&serializer->free_bits, value, &serializer->allocator);
        if (free_bits == NULL || pushed != 0) {
            result->status = Status_MemoryAllocationFailed;
            return NULL;
        }
    }
    return free_bits;
}

DeserializerFreeBits* deserializer_get_free_bits(Deserializer* deserializer, Result* result) {
   DeserializerFreeBits* free_bits = deser_free_bits_first(&deserializer->free_bits);
   if (free_bits == NULL) {
        uint8_t* byte = read(deserializer->reader, 1);
        if (byte == NULL) {
            result->status = Status_ReadFailed;
            return NULL;
        }
        DeserializerFreeBits value = {
            .end = 8,
            .start = 0,
            .byte = *byte
        };
        free_bits = deser_free_bits_push(&deserializer->free_bits, value, &deserializer->allocator);
        if (free_bits == NULL) {
            result->status = Status_MemoryAllocationFailed;
            return NULL;
        }
    }
    return free_bits;
}

void serialize_uint8(Serializer* serializer, uint8_t value, Result* result) {
    serialize_uint8_max(serializer, value, 8, result);
}

uint8_t deserialize_uint8(Deserializer* deserializer, Result* result) {
    return deserialize_uint8_max(deserializer, 8, result);
}

#define U8_FREE_BITS_STORAGE_MIN 4

void serializer_push_bits(Serializer* serializer, uint64_t value, size_t count, Result* result);
void serialize_uint8_max(Serializer* serializer, uint8_t value, uint8_t max_bits, Result* result) {
    result->status = Status_Success;
    if (max_bits <= U8_FREE_BITS_STORAGE_MIN) {
        serializer_push_bits(serializer, (uint64_t)value, max_bits, result);
    }
    else {
        if (max_bits < 8) {
            SerializerFreeBits free_bits = {
                .end = 8,
                .start = max_bits,
                .index = serializer->buffer.size + serializer->start_index
            };
            SerializerFreeBits* free_bits_push_res = ser_free_bits_push(&serializer->free_bits, free_bits, &serializer->allocator);
            if (free_bits_push_res == NULL) {
                result->status = Status_MemoryAllocationFailed;
                return;
            }
        }
        int byte_push_res = buffer_push_byte(&serializer->buffer, value, &serializer->allocator);
        if (byte_push_res != 0) {
            result->status = Status_MemoryAllocationFailed;
            return;
        }
    }
    flush_buffer(serializer, result);
}

uint64_t deserializer_read_bits(Deserializer* deserializer, size_t count, Result* result);
uint8_t deserialize_uint8_max(Deserializer* deserializer, uint8_t max_bits, Result* result) {
    result->status = Status_Success;
    if (max_bits <= U8_FREE_BITS_STORAGE_MIN) {
        return (uint8_t)deserializer_read_bits(deserializer, max_bits, result);
    }
    else {
        uint8_t* byte = read(deserializer->reader, 1);
        if (byte == NULL) {
            result->status = Status_ReadFailed;
            return 0;
        }
        if (max_bits < 8) {
            DeserializerFreeBits free_bits = {
                .byte = *byte,
                .start = max_bits,
                .end = 8
            };
            deser_free_bits_push(&deserializer->free_bits, free_bits, &deserializer->allocator);
        }
        uint8_t value = *byte & BIT_MASK(0, max_bits, uint8_t);
        return value;
    }
}

void serialize_uint16(Serializer* serializer, uint16_t value, Result* result) {
    serialize_uint16_max(serializer, value, 16, result);
}

void serialize_uint16_max(Serializer* serializer, uint16_t value, uint8_t max_bits, Result* result) {
    result->status = Status_Success;
    if (max_bits <= 8) {
        serialize_uint8_max(serializer, (uint8_t)value, max_bits, result);
    }
    else {
        uint32_t used_bits = count_used_bits_uint32(value);
        uint32_t used_bytes = ceil_divide_uint32(used_bits, 8);
        uint32_t byte_count_size = count_used_bits_uint32(sizeof(uint16_t) - 1); 
        assert(byte_count_size == 1); // should be 1 for uint16_t
        uint16_t little_endian = system_endianness_to_little_endian_uint16(value);
        printf("used_bytes: %u", used_bytes);
        serializer_push_bits(serializer, used_bytes - 1, byte_count_size, result);
        if (result->status != Status_Success) {
            return;
        }
        if (buffer_push_bytes(&serializer->buffer, (uint8_t*)&little_endian, used_bytes, &serializer->allocator) != 0) {
            result->status = Status_MemoryAllocationFailed;
            return;
        }
        uint32_t free_bits_start = used_bits % 8;
        uint32_t free_bits_count = 8 - free_bits_start;
        if (free_bits_count > 0) {
            SerializerFreeBits free_bits = {
                .start = free_bits_start,
                .end = 8,
                .index = serializer->buffer.size - 1
            };
            if (ser_free_bits_push(&serializer->free_bits, free_bits, &serializer->allocator) == NULL) {
                result->status = Status_MemoryAllocationFailed;
                return;
            }
        }
        flush_buffer(serializer, result);
    }
}


void serializer_push_bit(Serializer* serializer, uint8_t value, Result* result);
void serialize_bool(Serializer* serializer, bool value, Result* result) {
    result->status = Status_Success;
    serializer_push_bit(serializer, (uint8_t)value, result);
}

uint8_t deserializer_read_bit(Deserializer* deserializer, Result* result);
bool deserialize_bool(Deserializer* deserializer, Result* result) {
    result->status = Status_Success;
    return (bool)deserializer_read_bit(deserializer, result);
}


void serializer_push_bit(Serializer* serializer, uint8_t value, Result* result) {
    SerializerFreeBits* free_bits = serializer_get_free_bits(serializer, result);
    if (result->status != Status_Success) {
        return;
    }
    uint8_t mask = BIT_MASK(free_bits->start, value, uint8_t);
    size_t byte_index = free_bits->index - serializer->start_index;
    serializer->buffer.data[byte_index] |= mask;
    free_bits->start++;
    // if this byte is full then we can remove it and flush the buffer until the next free bits index
    if (free_bits->start >= free_bits->end) {
        if (ser_free_bits_remove(&serializer->free_bits, 0) != 0) {
            result->status = Status_MemoryOperationFailed;
            return;
        }
        flush_buffer(serializer, result);
    }
}
uint8_t deserializer_read_bit(Deserializer* deserializer, Result* result) {
    DeserializerFreeBits* free_bits = deserializer_get_free_bits(deserializer, result);
    if (result->status != Status_Success) {
        return 0;
    }
    uint8_t value = (free_bits->byte & BIT_MASK(free_bits->start, 1, uint8_t)) >> free_bits->start;
    free_bits->start++;
    if (free_bits->start >= free_bits->end) {
        int res = deser_free_bits_remove(&deserializer->free_bits, 0);
        if (res != 0) {
            result->status = Status_MemoryOperationFailed;
            return 0;
        }
    }
    return value;
}

void serializer_push_bits(Serializer* serializer, uint64_t value, size_t count, Result* result) {
    while (count > 0) {
        SerializerFreeBits* free_bits = serializer_get_free_bits(serializer, result);
        if (result->status != Status_Success) {
            return;
        }

        size_t write_count = min(free_bits->end - free_bits->start, count);
        uint8_t data = (uint8_t)(value & (uint64_t)BIT_MASK(0, write_count, size_t));
        serializer->buffer.data[free_bits->index - serializer->start_index] |= data << free_bits->start;
        free_bits->start += write_count;
        value >>= write_count;
        count -= write_count;

        if (free_bits->start >= free_bits->end) {
            if (ser_free_bits_remove(&serializer->free_bits, 0) != 0) {
                result->status = Status_MemoryOperationFailed;
                return;
            }
            flush_buffer(serializer, result);
        }
    }
}
uint64_t deserializer_read_bits(Deserializer* deserializer, size_t count, Result* result) {
    uint64_t value = 0;
    size_t index = 0;
    while (count > 0) {
        DeserializerFreeBits* free_bits = deserializer_get_free_bits(deserializer, result);
        if (free_bits == NULL) {
            assert(result->status != Status_Success);
            return 0;
        }
        size_t read_count = min(free_bits->end - free_bits->start, count);
        uint64_t data = (free_bits->byte & BIT_MASK(free_bits->start, read_count, uint64_t)) >> free_bits->start;
        value |= data << index;
        index += read_count;
        count -= read_count;
        free_bits->start += read_count;
        if (free_bits->start >= free_bits->end) {
            if (deser_free_bits_remove(&deserializer->free_bits, 0) != 0) {
                result->status = Status_MemoryAllocationFailed;
                return 0;
            }
        }
    }
    return value;
}

void flush_buffer(Serializer* serializer, Result* result) {
    SerializerFreeBits* free_bits = ser_free_bits_first(&serializer->free_bits);
    if (free_bits == NULL) {
        // flushing the entire buffer
        int write_result = write(serializer->writer, serializer->buffer.data, serializer->buffer.size);
        if (write_result != 0) {
            result->status = Status_WriteFailed;
            result->error_info.write_error = write_result;
        }
        serializer->start_index += serializer->buffer.size;
        buffer_clear(&serializer->buffer);
    }
    else {
        // flushing until the free_bits index
        size_t count = free_bits->index - serializer->start_index;
        if (count > 0) {
            int write_result = write(serializer->writer, serializer->buffer.data, count);
            if (write_result != 0) {
                result->status = Status_WriteFailed;
                result->error_info.write_error = write_result;
            }
            if (buffer_remove(&serializer->buffer, 0, count) != 0) {
                result->status = Status_MemoryOperationFailed;
            }
            serializer->start_index = free_bits->index;
        }
    }
}


void serializer_finalize(Serializer* serializer, Result* result) {
    result->status = Status_Success;
    ser_free_bits_clear(&serializer->free_bits);
    flush_buffer(serializer, result);
}