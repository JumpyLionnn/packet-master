#include "packet_master.h"
#include <string.h>

int Writer::write(uint8_t * value, size_t size) {
    return this->write_callback(this->ctx, value, size);
}

int Writer::write_byte(uint8_t value) {
    return this->write_callback(this->ctx, &value, 1);
}

uint8_t* Reader::read(size_t size) {
    return this->read_callback(this->ctx, size);
}

void* Allocator::alloc(size_t size) {
    return this->alloc_callback(size, this->ctx);
}
void* Allocator::realloc(void* ptr, size_t old_size, size_t new_size) {
    return this->realloc_callback(ptr, old_size, new_size, this->ctx);
}
void Allocator::free(void* ptr, size_t size) {
    this->free_callback(ptr, size, this->ctx);
}

#define BYTE_SIZE 8
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

#define unimplemented() fprintf(stderr, "%s:%u: Attempt to run code marked as unimplemented.\n", __FILE__, __LINE__)



const char* status_to_string(ResultStatus status) {
    switch (status)
    {
    case ResultStatus::Success:
        return "success";
    case ResultStatus::MemoryAllocationFailed:
        return "memory_allocation_failed";
    case ResultStatus::MemoryOperationFailed:
        return "memory_operation_failed";
    case ResultStatus::WriteFailed:
        return "write_failed";
    case ResultStatus::ReadFailed:
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
#include <intrin0.inl.h>
#endif

int count_leading_zeros_uint(unsigned int num) {
    #if defined(__GNUC__) || defined(__GNUG__) || defined(__clang__)
        // supported in clang and gcc
        // handling zero as it is an undefined behaviour for clz
        if (num == 0) {
            return sizeof(num) * BYTE_SIZE;
        }
        else {
            return __builtin_clz(num);
        }
    #elif defined(_MSC_VER) 
        if (num == 0) {
            return sizeof(num) * BYTE_SIZE;
        }
        else {
            return __lzcnt(num);
        }
    #else
        return count_leading_zeros_uint_fallback(num);
    #endif
}

uint64_t min(uint64_t a, uint64_t b) {
    if (a > b) {
        return b;
    }
    else {
        return a;
    }
}

uint16_t min(uint16_t a, uint16_t b) {
    if (a > b) {
        return b;
    }
    else {
        return a;
    }
}
uint32_t min(uint32_t a, uint32_t b) {
    if (a > b) {
        return b;
    }
    else {
        return a;
    }
}

uint64_t max(uint64_t a, uint64_t b) {
    if (a > b) {
        return a;
    }
    else {
        return b;
    }
}
uint32_t max(uint32_t a, uint32_t b) {
    if (a > b) {
        return a;
    }
    else {
        return b;
    }
}

uint32_t ceil_divide(uint32_t a, uint32_t b) {
    return (a + b - 1) / b;
}
uint8_t ceil_divide(uint8_t a, uint8_t b) {
    return (a + b - 1) / b;
}

enum Endianness {
    LittleEndian = 0,
    BigEndian
};

enum Endianness detect_endianness() {
    #if defined(__BYTE_ORDER) && __BYTE_ORDER == __BIG_ENDIAN || \
        defined(__BIG_ENDIAN__) || \
        defined(__ARMEB__) || \
        defined(__THUMBEB__) || \
        defined(__AARCH64EB__) || \
        defined(_MIBSEB) || defined(__MIBSEB) || defined(__MIBSEB__)
        return BigEndian;
    #elif defined(__BYTE_ORDER) && __BYTE_ORDER == __LITTLE_ENDIAN || \
        defined(__LITTLE_ENDIAN__) || \
        defined(__ARMEL__) || \
        defined(__THUMBEL__) || \
        defined(__AARCH64EL__) || \
        defined(_MIPSEL) || defined(__MIPSEL) || defined(__MIPSEL__)
        return LittleEndian;
    #else
        // fallback, it might have a small runtime cost
        int n = 1;
        if(*(char*)&n == 1) {
            return LittleEndian;
        }
        else {
            return BigEndian;
        }
    #endif
}

uint16_t swap_byte_order_fallback(uint16_t value) {
    return ((value & 0x00FF) << BYTE_SIZE) | ((value & 0xFF00) >> BYTE_SIZE);
}

uint16_t swap_byte_order(uint16_t value){
    #if defined(__GNUC__) || defined(__GNUG__) || defined(__clang__)
        return __builtin_bswap16(value);
    #elif defined(_MSC_VER)
        // all of those conditions are evaluated during compile time
        #pragma warning( push )
        #pragma warning( disable : 4127 )
            if (sizeof(uint16_t) == sizeof(unsigned short)) {
                return (uint16_t)_byteswap_ushort(value);
            }
            else if (sizeof(uint16_t) == sizeof(unsigned long)) {
                return (uint16_t)_byteswap_ulong(value);
            }
            else if (sizeof(uint16_t) == sizeof(unsigned __int64)) {
                return (uint16_t)_byteswap_uint64(value);
            }
            else {
                return swap_byte_order_fallback(value);
            }
        #pragma warning( pop )
    #else
        return swap_byte_order_fallback(value);
    #endif
}

uint32_t swap_byte_order_fallback(uint32_t value) {
    return ((value & 0x000000FF) << (BYTE_SIZE * 3)) | ((value & 0x0000FF00) << BYTE_SIZE) | ((value & 0x00FF0000) >> BYTE_SIZE) | ((value & 0xFF000000) >> (BYTE_SIZE * 3));
}

uint32_t swap_byte_order(uint32_t value){
    #if defined(__GNUC__) || defined(__GNUG__) || defined(__clang__)
        return __builtin_bswap32(value);
    #elif defined(_MSC_VER) 
        #pragma warning( push )
        #pragma warning( disable : 4127 )
            if (sizeof(uint32_t) == sizeof(unsigned short)) {
                return (uint32_t)_byteswap_ushort((unsigned short)value);
            }
            else if (sizeof(uint32_t) == sizeof(unsigned long)) {
                return (uint32_t)_byteswap_ulong(value);
            }
            else if (sizeof(uint32_t) == sizeof(unsigned __int64)) {
                return (uint32_t)_byteswap_uint64(value);
            }
            else {
                return swap_byte_order_fallback(value);
            }
        #pragma warning( pop )
    #else
        return swap_byte_order_fallback(value);
    #endif
}


uint16_t native_endianness_to_little_endian(uint16_t value) {
    if (detect_endianness() == BigEndian) {
        return swap_byte_order(value);
    }
    else {
        return value;
    }
}

uint32_t native_endianness_to_little_endian(uint32_t value) {
    if (detect_endianness() == BigEndian) {
        return swap_byte_order(value);
    }
    else {
        return value;
    }
}

uint16_t little_endian_to_native_endianness_uint16(uint16_t value) {
    if (detect_endianness() == BigEndian) {
        return swap_byte_order(value);
    }
    else {
        return value;
    }
}
uint32_t little_endian_to_native_endianness_uint32(uint32_t value) {
    if (detect_endianness() == BigEndian) {
        return swap_byte_order(value);
    }
    else {
        return value;
    }
}

uint32_t count_used_bits_uint32(uint32_t value) {
    return (uint32_t)(sizeof(unsigned int) * BYTE_SIZE - count_leading_zeros_uint((unsigned int)value));
} 


uint8_t max_bits_u8(uint8_t bits) {
    return bits;
}
uint8_t max_u8(uint8_t number) {
    return BYTE_SIZE - (uint8_t)(count_leading_zeros_uint((unsigned int)number) - (sizeof(unsigned int) - 1) * BYTE_SIZE);
}

Buffer create_buffer(size_t capacity, Allocator* allocator) {
    Buffer buffer{};
    buffer.data = (uint8_t*)allocator->alloc(capacity);
    buffer.capacity = capacity;
    buffer.size = 0;
    return buffer;
}

void free_buffer(Buffer* buffer, Allocator* allocator) {
    allocator->free(buffer->data, buffer->capacity);
}

int buffer_push_bytes(Buffer* buffer, uint8_t* data, size_t size, Allocator* allocator) {
    assert(buffer->size <= buffer->capacity);
    if (size > buffer->capacity - buffer->size) {
        // expand
        size_t old_size = buffer->capacity;
        buffer->capacity = max((size_t)((double)buffer->capacity * 1.5), buffer->size + size);
        buffer->data = (uint8_t*)allocator->realloc(buffer->data, old_size, buffer->capacity);
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
    return move_res == NULL;
}


int buffer_push_byte(Buffer* buffer, uint8_t byte, Allocator* allocator) {
    return buffer_push_bytes(buffer, &byte, 1, allocator);
}

SerializerFreeBitsVector ser_create_free_bits_vector(size_t capacity, Allocator* allocator) {
    SerializerFreeBitsVector free_bits;
    free_bits.capacity = capacity;
    free_bits.count = 0;
    free_bits.data = capacity == 0 ? NULL : (SerializerFreeBits*)allocator->alloc(capacity * sizeof(SerializerFreeBits));
    return free_bits;
}

SerializerFreeBits* ser_free_bits_push(SerializerFreeBitsVector* vector, SerializerFreeBits value, Allocator* allocator) {
    if (vector->capacity <= vector->count) {
        size_t old_size = vector->capacity * sizeof(vector->data[0]);
        vector->capacity = max((size_t)((double)vector->capacity * 1.5), vector->count + 1);
        if (vector->data == NULL) {
            vector->data = (SerializerFreeBits*)allocator->alloc(vector->capacity * sizeof(SerializerFreeBits));
            if (vector->data == NULL) {
                return NULL;
            }
        }
        else {
            SerializerFreeBits* data = (SerializerFreeBits*)allocator->realloc(vector->data, old_size, vector->capacity * sizeof(SerializerFreeBits));
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
        void* move_res = memmove(vector->data + index, vector->data + index + 1, (vector->count - index - 1) * sizeof(vector->data[0]));
        vector->count--;
        return move_res == NULL;
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
    allocator->free(vector->data, sizeof(vector->data[0]) * vector->capacity);
}


DeserializerFreeBitsVector deser_create_free_bits_vector(size_t capacity, Allocator* allocator) {
    DeserializerFreeBitsVector free_bits{};
    free_bits.capacity = capacity;
    free_bits.count = 0;
    free_bits.data = capacity == 0 ? NULL : (DeserializerFreeBits*)allocator->alloc(capacity * sizeof(DeserializerFreeBits));
    return free_bits;
}

DeserializerFreeBits* deser_free_bits_push(DeserializerFreeBitsVector* vector, DeserializerFreeBits value, Allocator* allocator) {
    if (vector->capacity <= vector->count) {
        size_t old_size = vector->capacity * sizeof(vector->data[0]);
        vector->capacity = max((size_t)((double)vector->capacity * 1.5), vector->count + 1);
        if (vector->data == NULL) {
            vector->data = (DeserializerFreeBits*)allocator->alloc(vector->capacity * sizeof(DeserializerFreeBits));
            if (vector->data == NULL) {
                return NULL;
            }
        }
        else {
            DeserializerFreeBits* data = (DeserializerFreeBits*)allocator->realloc(vector->data, old_size, vector->capacity * sizeof(DeserializerFreeBits));
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
        size_t right = vector->count - index - 1;
        void* move_res = memmove(vector->data + index, vector->data + index + 1, right * sizeof(vector->data[0]));
        vector->count--;
        return move_res == NULL ? 1 : 0;
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
    allocator->free(vector->data, vector->capacity * sizeof(vector->data[0]));
}

#define U8_FREE_BITS_STORAGE_MIN 4

Serializer::Serializer(Writer* writer, Allocator* allocator) 
    : m_writer(writer), m_allocator(allocator), m_start_index(0), m_buffer(create_buffer(0, allocator)), m_free_bits(ser_create_free_bits_vector(0, allocator)) {
}

Serializer::~Serializer() {
    free_buffer(&m_buffer, m_allocator);
    ser_free_bits_free(&m_free_bits, m_allocator);
}

void Serializer::serialize_uint8(uint8_t value, Result* result) {
    serialize_uint8_max(value, BYTE_SIZE, result);
}
void Serializer::serialize_uint8_max(uint8_t value, uint8_t max_bits, Result* result) {
    result->status = ResultStatus::Success;
    if (max_bits <= U8_FREE_BITS_STORAGE_MIN) {
        push_bits((uint64_t)value, max_bits, result);
    }
    else {
        if (max_bits < 8) {
            SerializerFreeBits free_bits{};
            free_bits.end = 8;
            free_bits.start = max_bits;
            free_bits.index = m_buffer.size + m_start_index;
            SerializerFreeBits* free_bits_push_res = ser_free_bits_push(&m_free_bits, free_bits, m_allocator);
            if (free_bits_push_res == NULL) {
                result->status = ResultStatus::MemoryAllocationFailed;
                return;
            }
        }
        int byte_push_res = buffer_push_byte(&m_buffer, value, m_allocator);
        if (byte_push_res != 0) {
            result->status = ResultStatus::MemoryAllocationFailed;
            return;
        }
    }
    flush_buffer(result);
}

void Serializer::serialize_uint16(uint16_t value, Result* result) {
    serialize_uint16_max(value, 16, result);
}
void Serializer::serialize_uint16_max(uint16_t value, uint8_t max_bits, Result* result) {
    result->status = ResultStatus::Success;
    if (max_bits <= 8) {
        serialize_uint8_max((uint8_t)value, max_bits, result);
    }
    else {
        uint32_t used_bits = count_used_bits_uint32(value);
        assert(used_bits <= max_bits);
        uint32_t used_bytes = max(ceil_divide(used_bits, BYTE_SIZE), 1);
        uint32_t byte_count_size = count_used_bits_uint32(sizeof(uint16_t) - 1);
        assert(byte_count_size == 1); // should be 1 for uint16_t
        uint16_t little_endian = native_endianness_to_little_endian(value);
        push_bits(used_bytes - 1, byte_count_size, result);
        if (result->status != ResultStatus::Success) {
            return;
        }
        if (buffer_push_bytes(&m_buffer, (uint8_t*)&little_endian, used_bytes, m_allocator) != 0) {
            result->status = ResultStatus::MemoryAllocationFailed;
            return;
        }
        if (used_bytes * BYTE_SIZE > max_bits) {
            uint32_t free_bits_start = max_bits % BYTE_SIZE;
            SerializerFreeBits free_bits{};
            free_bits.start = free_bits_start;
            free_bits.end = BYTE_SIZE;
            free_bits.index = m_buffer.size - 1 + m_start_index;
            if (ser_free_bits_push(&m_free_bits, free_bits, m_allocator) == NULL) {
                result->status = ResultStatus::MemoryAllocationFailed;
                return;
            }
        }
        flush_buffer(result);
    }
}

void Serializer::serialize_uint32(uint32_t value, Result* result) {
    serialize_uint32_max(value, 32, result);
}
void Serializer::serialize_uint32_max(uint32_t value, uint8_t max_bits, Result* result) {
    result->status = ResultStatus::Success;
    if (max_bits <= 16) {
        serialize_uint16_max((uint16_t)value, max_bits, result);
    }
    else {
        uint32_t used_bits = count_used_bits_uint32(value);
        assert(used_bits <= max_bits);
        uint32_t used_bytes = max(ceil_divide(used_bits, BYTE_SIZE), 1);
        uint8_t max_bytes = ceil_divide(max_bits, BYTE_SIZE);
        uint32_t byte_count_size = count_used_bits_uint32(max_bytes - 1);
        // example:
        // max_bits = 20 // 3 bytes max
        // used_bytes 3 // 01...
        // used_bytes 2 // 1...
        // used_bytes 1 // 00...

        // need to check whether -1 is going to change the amount of stored bits
        // if it does and if the byte count is the middle store only 1 otherwise store 0 and then the rest of the number (-1 if the number is above the middle)
        // otherwise store the byte count normally
        // why: the middle number has the highest probability(without any extra data) to exist therefore we optimize the size even further and remove 1 unnecessary bit

        // generic way to be easy to port to different types
        if (count_used_bits_uint32(max_bytes - 2) != byte_count_size) {
            // if the max bytes is 3 and the used bytes is 2
            uint8_t middle_byte = (max_bytes + 1) / 2;
            if (used_bytes == middle_byte) {
                push_bit(1, result);
            }
            else {
                uint32_t byte_count = used_bytes;
                if (byte_count > middle_byte) {
                    byte_count--;
                }
                // the first bit is zero to indicate there are some length bits after it
                push_bits((byte_count - 1) << 1, byte_count_size, result);
            }
        }
        else {
            push_bits(used_bytes - 1, byte_count_size, result);
        }
        if (result->status != ResultStatus::Success) {
            return;
        }

        uint32_t little_endian = native_endianness_to_little_endian(value);
        if (buffer_push_bytes(&m_buffer, (uint8_t*)&little_endian, used_bytes, m_allocator) != 0) {
            result->status = ResultStatus::MemoryAllocationFailed;
            return;
        }
        if (used_bytes * BYTE_SIZE > max_bits) {
            uint32_t free_bits_start = max_bits % BYTE_SIZE;
            SerializerFreeBits free_bits{};
            free_bits.start = free_bits_start;
            free_bits.end = BYTE_SIZE;
            free_bits.index = m_buffer.size - 1 + m_start_index;
            if (ser_free_bits_push(&m_free_bits, free_bits, m_allocator) == NULL) {
                result->status = ResultStatus::MemoryAllocationFailed;
                return;
            }
        }
        flush_buffer(result);
    }
}

void Serializer::serialize_bool(bool value, Result* result) {
    result->status = ResultStatus::Success;
    push_bit((uint8_t)value, result);
}

void Serializer::finalize(Result* result) {
    result->status = ResultStatus::Success;
    ser_free_bits_clear(&m_free_bits);
    flush_buffer(result);
    m_start_index = 0;
}

void Serializer::push_bit(uint8_t value, Result* result) {
    SerializerFreeBits* free_bits = get_free_bits(result);
    if (result->status != ResultStatus::Success) {
        return;
    }
    uint8_t mask = BIT_MASK(free_bits->start, value, uint8_t);
    size_t byte_index = free_bits->index - m_start_index;
    m_buffer.data[byte_index] |= mask;
    free_bits->start++;
    // if this byte is full then we can remove it and flush the buffer until the next free bits index
    if (free_bits->start >= free_bits->end) {
        if (ser_free_bits_remove(&m_free_bits, 0) != 0) {
            result->status = ResultStatus::MemoryOperationFailed;
            return;
        }
        flush_buffer(result);
    }
}

void Serializer::push_bits(uint64_t value, size_t count, Result* result) {
    while (count > 0) {
        SerializerFreeBits* free_bits = get_free_bits(result);
        if (result->status != ResultStatus::Success) {
            return;
        }

        size_t write_count = min(free_bits->end - free_bits->start, count);
        uint8_t data = (uint8_t)(value & BIT_MASK(0, (uint64_t)write_count, uint64_t));
        m_buffer.data[free_bits->index - m_start_index] |= data << free_bits->start;
        free_bits->start += write_count;
        value >>= write_count;
        count -= write_count;

        if (free_bits->start >= free_bits->end) {
            if (ser_free_bits_remove(&m_free_bits, 0) != 0) {
                result->status = ResultStatus::MemoryOperationFailed;
                return;
            }
            flush_buffer(result);
        }
    }
}

void Serializer::flush_buffer(Result* result) {
    SerializerFreeBits* free_bits = ser_free_bits_first(&m_free_bits);
    if (free_bits == NULL) {
        // flushing the entire buffer
        int write_result = m_writer->write(m_buffer.data, m_buffer.size);
        if (write_result != 0) {
            result->status = ResultStatus::WriteFailed;
            result->error_info.write_error = write_result;
        }
        m_start_index += m_buffer.size;
        buffer_clear(&m_buffer);
    }
    else {
        // flushing until the free_bits index
        size_t count = free_bits->index - m_start_index;
        if (count > 0) {
            int write_result = m_writer->write(m_buffer.data, count);
            if (write_result != 0) {
                result->status = ResultStatus::WriteFailed;
                result->error_info.write_error = write_result;
            }
            if (buffer_remove(&m_buffer, 0, count) != 0) {
                result->status = ResultStatus::MemoryOperationFailed;
            }
            m_start_index = free_bits->index;
        }
    }
}

SerializerFreeBits* Serializer::get_free_bits(Result* result) {
   SerializerFreeBits* free_bits = ser_free_bits_first(&m_free_bits);
   if (free_bits == NULL) {
        SerializerFreeBits value{};
        value.start = 0;
        value.end = BYTE_SIZE;
        value.index = m_buffer.size + m_start_index;
        int pushed = buffer_push_byte(&m_buffer, 0, m_allocator);
        free_bits = ser_free_bits_push(&m_free_bits, value, m_allocator);
        if (free_bits == NULL || pushed != 0) {
            result->status = ResultStatus::MemoryAllocationFailed;
            return NULL;
        }
    }
    return free_bits;
}



Deserializer::Deserializer(Reader* reader, Allocator* allocator) : m_reader(reader), m_allocator(allocator), m_free_bits(deser_create_free_bits_vector(0, allocator)) {}

Deserializer::~Deserializer() {
    deser_free_bits_free(&m_free_bits, m_allocator);
}

uint8_t Deserializer::deserialize_uint8(Result* result) {
    return deserialize_uint8_max(BYTE_SIZE, result);
}
uint8_t Deserializer::deserialize_uint8_max(uint8_t max_bits, Result* result) {
    result->status = ResultStatus::Success;
    if (max_bits <= U8_FREE_BITS_STORAGE_MIN) {
        return (uint8_t)read_bits(max_bits, result);
    }
    else {
        uint8_t* byte = m_reader->read(1);
        if (byte == NULL) {
            result->status = ResultStatus::ReadFailed;
            return 0;
        }
        if (max_bits < 8) {
            DeserializerFreeBits free_bits{};
            free_bits.byte = *byte;
            free_bits.start = max_bits;
            free_bits.end = 8;
            deser_free_bits_push(&m_free_bits, free_bits, m_allocator);
        }
        uint8_t value = *byte & BIT_MASK(0, max_bits, uint8_t);
        return value;
    }
}

uint16_t Deserializer::deserialize_uint16(Result* result) {
    return deserialize_uint16_max(16, result);
}
uint16_t Deserializer::deserialize_uint16_max(uint8_t max_bits, Result* result) {
    result->status = ResultStatus::Success;
    if (max_bits <= BYTE_SIZE) {
        return deserialize_uint8_max(max_bits, result);
    }
    else {
        uint32_t byte_count_size = count_used_bits_uint32(sizeof(uint16_t) - 1);
        assert(byte_count_size == 1);
        uint32_t byte_count = (uint32_t)read_bits(byte_count_size, result) + 1;
        if (result->status != ResultStatus::Success) {
            return 0;
        }
        uint8_t* bytes = m_reader->read(byte_count);
        if (bytes == NULL) {
            result->status = ResultStatus::ReadFailed;
            return 0;
        }

        uint16_t little_endian = (*(uint16_t*)bytes) & BIT_MASK((uint16_t)0, min(byte_count * 8, max_bits), uint16_t);
        uint16_t value = little_endian_to_native_endianness_uint16(little_endian);
        if (byte_count * BYTE_SIZE > max_bits) {
            uint8_t start = max_bits % BYTE_SIZE;
            DeserializerFreeBits free_bits{};
            free_bits.start = start;
            free_bits.end = BYTE_SIZE;
            free_bits.byte = bytes[byte_count - 1];
            if (deser_free_bits_push(&m_free_bits, free_bits, m_allocator) == NULL) {
                result->status = ResultStatus::MemoryAllocationFailed;
                return 0;
            }
        }
        return value;
    }
}

uint32_t Deserializer::deserialize_uint32(Result* result) {
    return deserialize_uint32_max(32, result);
}
uint32_t Deserializer::deserialize_uint32_max(uint8_t max_bits, Result* result) {
    result->status = ResultStatus::Success;
    if (max_bits <= 16) {
        return deserialize_uint16_max(max_bits, result);
    }
    else {
        uint8_t max_bytes = ceil_divide(max_bits, BYTE_SIZE);
        uint32_t byte_count_size = count_used_bits_uint32(max_bytes - 1);
        size_t byte_count;
        if (count_used_bits_uint32(max_bytes - 2) != byte_count_size) {
            uint8_t is_middle = read_bit(result);
            if (result->status != ResultStatus::Success) {
                return 0;
            }
            uint8_t middle_byte = (max_bytes + 1) / 2;
            if (is_middle) {
                byte_count = middle_byte;
            }
            else {
                // middle byte = 2
                // count is 1 or 0
                // actual   3 or 1
                uint64_t count = read_bits(byte_count_size - 1, result) + 1;
                if (result->status != ResultStatus::Success) {
                    return 0;
                }
                if (count + 1 >= middle_byte) {
                    count++;
                }
                byte_count = count;
            }
        }
        else {
            byte_count = read_bits(byte_count_size, result) + 1;
            if (result->status != ResultStatus::Success) {
                return 0;
            }
        }

        uint8_t* bytes = m_reader->read(byte_count);
        if (bytes == NULL) {
            result->status = ResultStatus::ReadFailed;
            return 0;
        }
        uint32_t max = min((uint32_t)max_bits, (uint32_t)(byte_count * BYTE_SIZE));
        uint32_t little_endian = (*(uint32_t*)bytes) & BIT_MASK(0, min(max, 31), uint32_t);
        uint32_t value = little_endian_to_native_endianness_uint32(little_endian);

        if (byte_count * BYTE_SIZE > max_bits) {
            uint8_t start = max_bits % 8;
            DeserializerFreeBits free_bits{};
            free_bits.byte = bytes[byte_count - 1];
            free_bits.start = start;
            free_bits.end = 8;
            if (deser_free_bits_push(&m_free_bits, free_bits, m_allocator) == NULL) {
                result->status = ResultStatus::MemoryAllocationFailed;
                return 0;
            }
        }

        return value;
    }
}

bool Deserializer::deserialize_bool(Result* result) {
    result->status = ResultStatus::Success;
    return (bool)read_bit(result);
}

uint8_t Deserializer::read_bit(Result* result) {
    DeserializerFreeBits* free_bits = get_free_bits(result);
    if (result->status != ResultStatus::Success) {
        return 0;
    }
    uint8_t value = (free_bits->byte & BIT_MASK(free_bits->start, 1, uint8_t)) >> free_bits->start;
    free_bits->start++;
    if (free_bits->start >= free_bits->end) {
        int res = deser_free_bits_remove(&m_free_bits, 0);
        if (res != 0) {
            result->status = ResultStatus::MemoryOperationFailed;
            return 0;
        }
    }
    return value;
}

uint64_t Deserializer::read_bits(size_t count, Result* result) {
    uint64_t value = 0;
    size_t index = 0;
    while (count > 0) {
        DeserializerFreeBits* free_bits = get_free_bits(result);
        if (free_bits == NULL) {
            assert(result->status != ResultStatus::Success);
            return 0;
        }
        size_t read_count = min(free_bits->end - free_bits->start, count);
        uint64_t data = (free_bits->byte & BIT_MASK(free_bits->start, read_count, uint64_t)) >> free_bits->start;
        value |= data << index;
        index += read_count;
        count -= read_count;
        free_bits->start += (uint8_t)read_count;
        if (free_bits->start >= free_bits->end) {
            if (deser_free_bits_remove(&m_free_bits, 0) != 0) {
                result->status = ResultStatus::MemoryAllocationFailed;
                return 0;
            }
        }
    }
    return value;
}

DeserializerFreeBits* Deserializer::get_free_bits(Result* result) {
    DeserializerFreeBits* free_bits = deser_free_bits_first(&m_free_bits);
    if (free_bits == NULL) {
        uint8_t* byte = m_reader->read(1);
        if (byte == NULL) {
            result->status = ResultStatus::ReadFailed;
            return NULL;
        }
        DeserializerFreeBits value{};
        value.start = 0;
        value.end = BYTE_SIZE;
        value.byte = *byte;
        free_bits = deser_free_bits_push(&m_free_bits, value, m_allocator);
        if (free_bits == NULL) {
            result->status = ResultStatus::MemoryAllocationFailed;
            return NULL;
        }
    }
    return free_bits;
}
