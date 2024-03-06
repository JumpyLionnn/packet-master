#include "packet_master.h"

#define BYTE_SIZE 8
#define BIT_MASK(start, count, type) ((type)~((type)(~(type)0) << (type)(count)) << (type)(start))

#ifdef ENABLE_ASSERT
#include <stdio.h>
void assert_impl(bool value, const char* expression, size_t line, const char* file) {
    if (!value) {
        fprintf(stderr, "%s:%u: Assertion failed: %s.\n", file, (uint32_t)line, expression);
    }
}
#endif

#define unimplemented() fprintf(stderr, "%s:%u: Attempt to run code marked as unimplemented.\n", __FILE__, __LINE__)

#define U8_FREE_BITS_STORAGE_MIN 4

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

uint32_t closest_power_of_two(uint32_t value) {
    return 1 << count_used_bits_uint32(value - 1);
}

uint8_t max_bits_u8(uint8_t bits) {
    return bits;
}
uint8_t max_u8(uint8_t number) {
    return BYTE_SIZE - (uint8_t)(count_leading_zeros_uint((unsigned int)number) - (sizeof(unsigned int) - 1) * BYTE_SIZE);
}

Serializer::Serializer(Writer* writer, Allocator* allocator) 
    : m_writer(writer), m_start_index(0), m_buffer(allocator), m_free_bits(allocator) {
}

Serializer::~Serializer() {}

Result Serializer::serialize_uint8(uint8_t value) {
    return serialize_uint8_max(value, BYTE_SIZE);
}
Result Serializer::serialize_uint8_max(uint8_t value, uint8_t max_bits) {
    if (max_bits <= U8_FREE_BITS_STORAGE_MIN) {
        return push_bits((uint32_t)value, max_bits);
    }
    else {
        if (max_bits < 8) {
            SerializerFreeBits free_bits{};
            free_bits.end = BYTE_SIZE;
            free_bits.start = max_bits;
            free_bits.index = m_buffer.length() + m_start_index;
            if (m_free_bits.push(free_bits) == nullptr) {
                return Result(ResultStatus::MemoryAllocationFailed);
            }
        }
        if (m_buffer.push(value) == nullptr) {
            return Result(ResultStatus::MemoryAllocationFailed);
        }
    }
    return flush_buffer();
}

Result Serializer::serialize_uint8_opt(uint8_t value, UintOptions options) {
    assert(options.max_bits <= sizeof(uint8_t) * BYTE_SIZE);
    assert(options.segments_hint <= sizeof(uint8_t) * BYTE_SIZE);
    uint32_t used_bits = count_used_bits_uint32((uint32_t)value);
    assert(used_bits <= options.max_bits);
    uint32_t segment_count = closest_power_of_two(options.segments_hint);
    uint32_t small_segment_bit_count = options.max_bits / segment_count;
    uint32_t big_segment_bit_count = small_segment_bit_count + 1;
    uint32_t big_segment_count = options.max_bits % segment_count;
    // uint32_t small_segment_count = segment_count - big_segment_count;
    uint32_t segment_storage_size = count_used_bits_uint32(segment_count - 1);

    uint32_t used_big_segments = min(big_segment_count, ceil_divide(used_bits, big_segment_bit_count));
    uint32_t used_segments = used_big_segments;
    uint32_t used_bits_by_big_segments = used_big_segments * big_segment_bit_count;
    uint32_t final_used_bits = used_bits_by_big_segments;
    if (used_bits_by_big_segments < used_bits) {
        uint32_t used_small_segments = ceil_divide(used_bits - used_bits_by_big_segments, small_segment_bit_count);
        used_segments += used_small_segments;
        final_used_bits += used_small_segments * small_segment_bit_count;
    }

    Result result = push_bits(used_segments - 1, segment_storage_size);
    if (result.status != ResultStatus::Success) {
        return result;
    }
    // internal assert to make sure the algorithm is right
    assert(final_used_bits <= sizeof(uint8_t) * BYTE_SIZE);
    if (m_buffer.push(value) == nullptr) {
        return Result(ResultStatus::MemoryAllocationFailed);
    }

    uint32_t free_bits_start = final_used_bits % BYTE_SIZE;
    if (free_bits_start > 0) {
        SerializerFreeBits free_bits;
        free_bits.start = free_bits_start;
        free_bits.end = BYTE_SIZE;
        free_bits.index = m_buffer.length() - 1 + m_start_index;
        if (m_free_bits.push(free_bits) == nullptr) {
            return Result(ResultStatus::MemoryAllocationFailed);
        }
    }
    return flush_buffer();
}

Result Serializer::serialize_uint16(uint16_t value) {
    return serialize_uint16_max(value, 16);
}
Result Serializer::serialize_uint16_max(uint16_t value, uint8_t max_bits) {
    if (max_bits <= 8) {
        return serialize_uint8_max((uint8_t)value, max_bits);
    }
    else {
        uint32_t used_bits = count_used_bits_uint32(value);
        assert(used_bits <= max_bits);
        uint32_t used_bytes = max(ceil_divide(used_bits, BYTE_SIZE), 1);
        uint32_t byte_count_size = count_used_bits_uint32(sizeof(uint16_t) - 1);
        assert(byte_count_size == 1); // should be 1 for uint16_t
        uint16_t little_endian = native_endianness_to_little_endian(value);
        Result res = push_bits(used_bytes - 1, byte_count_size);
        if (res.status != ResultStatus::Success) {
            return res;
        }
        if (m_buffer.push_many((uint8_t*)&little_endian, used_bytes) == nullptr) {
            return Result(ResultStatus::MemoryAllocationFailed);
        }
        if (used_bytes * BYTE_SIZE > max_bits) {
            uint32_t free_bits_start = max_bits % BYTE_SIZE;
            SerializerFreeBits free_bits{};
            free_bits.start = free_bits_start;
            free_bits.end = BYTE_SIZE;
            free_bits.index = m_buffer.length() - 1 + m_start_index;
            if (m_free_bits.push(free_bits) == nullptr) {
                return Result(ResultStatus::MemoryAllocationFailed);
            }
        }
        return flush_buffer();
    }
}

Result Serializer::serialize_uint32(uint32_t value) {
    return serialize_uint32_max(value, 32);
}
Result Serializer::serialize_uint32_max(uint32_t value, uint8_t max_bits) {
    if (max_bits <= 16) {
        return serialize_uint16_max((uint16_t)value, max_bits);
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
        Result result;
        if (count_used_bits_uint32(max_bytes - 2) != byte_count_size) {
            // if the max bytes is 3 and the used bytes is 2
            uint8_t middle_byte = (max_bytes + 1) / 2;
            if (used_bytes == middle_byte) {
                result = push_bit(1);
            }
            else {
                uint32_t byte_count = used_bytes;
                if (byte_count > middle_byte) {
                    byte_count--;
                }
                // the first bit is zero to indicate there are some length bits after it
                result = push_bits((byte_count - 1) << 1, byte_count_size);
            }
        }
        else {
            result = push_bits(used_bytes - 1, byte_count_size);
        }
        if (result.status != ResultStatus::Success) {
            return result;
        }

        uint32_t little_endian = native_endianness_to_little_endian(value);
        if (m_buffer.push_many((uint8_t*)&little_endian, used_bytes) == nullptr) {
            return Result(ResultStatus::MemoryAllocationFailed);
        }
        if (used_bytes * BYTE_SIZE > max_bits) {
            uint32_t free_bits_start = max_bits % BYTE_SIZE;
            SerializerFreeBits free_bits{};
            free_bits.start = free_bits_start;
            free_bits.end = BYTE_SIZE;
            free_bits.index = m_buffer.length() - 1 + m_start_index;
            if (m_free_bits.push(free_bits) == nullptr) {
                return Result(ResultStatus::MemoryAllocationFailed);
            }
        }
        return flush_buffer();
    }
}

Result Serializer::serialize_bool(bool value) {
    return push_bit((uint8_t)value);
}

Result Serializer::finalize() {
    m_free_bits.clear();
    Result result = flush_buffer();
    m_start_index = 0;
    return result;
}

void Serializer::reset() {
    m_free_bits.clear();
    m_buffer.clear();
    m_start_index = 0;
}

Result Serializer::push_bit(uint8_t value) {
    SerializerFreeBits* free_bits = nullptr;
    Result result = get_free_bits(&free_bits);
    if (result.status != ResultStatus::Success) {
        return result;
    }
    uint8_t mask = BIT_MASK(free_bits->start, value, uint8_t);
    size_t byte_index = free_bits->index - m_start_index;
    m_buffer[byte_index] |= mask;
    free_bits->start++;
    // if this byte is full then we can remove it and flush the buffer until the next free bits index
    if (free_bits->start >= free_bits->end) {
        if (!m_free_bits.remove(0)) {
            return Result(ResultStatus::MemoryOperationFailed);
        }
        return flush_buffer();
    }
    return Result(ResultStatus::Success);
}

Result Serializer::push_bits(uint32_t value, size_t count) {
    while (count > 0) {
        SerializerFreeBits* free_bits = nullptr; 
        Result result = get_free_bits(&free_bits);
        if (result.status != ResultStatus::Success) {
            return result;
        }

        size_t write_count = min(free_bits->end - free_bits->start, count);
        uint8_t data = (uint8_t)(value & BIT_MASK(0, (uint32_t)write_count, uint32_t));
        m_buffer[free_bits->index - m_start_index] |= data << free_bits->start;
        free_bits->start += write_count;
        value >>= write_count;
        count -= write_count;

        assert(free_bits->end >= free_bits->start);
        if (free_bits->start >= free_bits->end) {
            if (!m_free_bits.remove(0)) {
                return Result(ResultStatus::MemoryOperationFailed);
            }
            Result result = flush_buffer();
            if (result.status != ResultStatus::Success) {
                return result;
            }
        }
    }
    return Result(ResultStatus::Success);
}

Result Serializer::flush_buffer() {
    SerializerFreeBits* free_bits = m_free_bits.first();
    if (free_bits == nullptr) {
        // flushing the entire buffer
        int write_result = m_writer->write(m_buffer.ptr(), m_buffer.length());
        if (write_result != 0) {
            Result result{};
            result.status = ResultStatus::WriteFailed;
            result.error_info.write_error = write_result;
            return result;
        }
        m_start_index += m_buffer.length();
        m_buffer.clear();
    }
    else {
        // flushing until the free_bits index
        size_t count = free_bits->index - m_start_index;
        if (count > 0) {
            int write_result = m_writer->write(m_buffer.ptr(), count);
            if (write_result != 0) {
                Result result{};
                result.status = ResultStatus::WriteFailed;
                result.error_info.write_error = write_result;
                return result;
            }
            if (!m_buffer.remove_many(0, count)) {
                return Result(ResultStatus::MemoryOperationFailed);
            }
            m_start_index = free_bits->index;
        }
    }
    return Result(ResultStatus::Success);
}

Result Serializer::get_free_bits(SerializerFreeBits** out_free_bits) {
   SerializerFreeBits* free_bits = m_free_bits.first();
   if (free_bits == nullptr) {
        SerializerFreeBits value{};
        value.start = 0;
        value.end = BYTE_SIZE;
        value.index = m_buffer.length() + m_start_index;
        uint8_t* pushed = m_buffer.push(0);
        free_bits = m_free_bits.push(value);
        if (free_bits == nullptr || pushed == nullptr) {
            return Result(ResultStatus::MemoryAllocationFailed);
        }
    }
    *out_free_bits = free_bits;
    return Result(ResultStatus::Success);
}



Deserializer::Deserializer(Reader* reader, Allocator* allocator) : m_reader(reader), m_allocator(allocator), m_free_bits(allocator) {}

Deserializer::~Deserializer() {}

Result Deserializer::deserialize_uint8(uint8_t* value) {
    return deserialize_uint8_max(BYTE_SIZE, value);
}
Result Deserializer::deserialize_uint8_max(uint8_t max_bits, uint8_t* value) {
    *value = 0;
    if (max_bits <= U8_FREE_BITS_STORAGE_MIN) {
        uint64_t bits;
        Result res = read_bits(max_bits, &bits);
        *value = (uint8_t)bits;
        return res;
    }
    else {
        uint8_t* byte = m_reader->read(1);
        if (byte == NULL) {
            return Result(ResultStatus::ReadFailed);
        }
        if (max_bits < 8) {
            DeserializerFreeBits free_bits{};
            free_bits.byte = *byte;
            free_bits.start = max_bits;
            free_bits.end = 8;
            if (m_free_bits.push(free_bits) == nullptr) {
                return Result(ResultStatus::MemoryAllocationFailed);
            }
        }
        *value = *byte & BIT_MASK(0, max_bits, uint8_t);
        return Result(ResultStatus::Success);
    }
}

Result Deserializer::deserialize_uint16(uint16_t* value) {
    return deserialize_uint16_max(16, value);
}
Result Deserializer::deserialize_uint16_max(uint8_t max_bits, uint16_t* value) {
    *value = 0;
    if (max_bits <= BYTE_SIZE) {
        uint8_t uvalue;
        Result res = deserialize_uint8_max(max_bits, &uvalue);
        *value = uvalue;
        return res;
    }
    else {
        uint32_t byte_count_size = count_used_bits_uint32(sizeof(uint16_t) - 1);
        assert(byte_count_size == 1);
        uint64_t byte_count;
        Result result = read_bits(byte_count_size, &byte_count);
        byte_count += 1;
        if (result.status != ResultStatus::Success) {
            return result;
        }
        uint8_t* bytes = m_reader->read((size_t)byte_count);
        if (bytes == NULL) {
            return Result(ResultStatus::ReadFailed);
        }

        uint16_t little_endian = (*(uint16_t*)bytes) & BIT_MASK((uint16_t)0, min(byte_count * 8, max_bits), uint16_t);
        *value = little_endian_to_native_endianness_uint16(little_endian);
        if (byte_count * BYTE_SIZE > max_bits) {
            uint8_t start = max_bits % BYTE_SIZE;
            DeserializerFreeBits free_bits{};
            free_bits.start = start;
            free_bits.end = BYTE_SIZE;
            free_bits.byte = bytes[byte_count - 1];
            if (m_free_bits.push(free_bits) == nullptr) {
                return Result(ResultStatus::MemoryAllocationFailed);
            }
        }
        return Result(ResultStatus::Success);
    }
}

Result Deserializer::deserialize_uint32(uint32_t* value) {
    return deserialize_uint32_max(32, value);
}
Result Deserializer::deserialize_uint32_max(uint8_t max_bits, uint32_t* value) {
    *value = 0;
    if (max_bits <= 16) {
        uint16_t uint;
        Result result = deserialize_uint16_max(max_bits, &uint);
        *value = uint;
        return result;
    }
    else {
        uint8_t max_bytes = ceil_divide(max_bits, BYTE_SIZE);
        uint32_t byte_count_size = count_used_bits_uint32(max_bytes - 1);
        size_t byte_count;
        if (count_used_bits_uint32(max_bytes - 2) != byte_count_size) {
            uint8_t is_middle; 
            Result result = read_bit(&is_middle);
            if (result.status != ResultStatus::Success) {
                return result;
            }
            uint8_t middle_byte = (max_bytes + 1) / 2;
            if (is_middle) {
                byte_count = middle_byte;
            }
            else {
                // middle byte = 2
                // count is 1 or 0
                // actual   3 or 1
                uint64_t count;
                Result result = read_bits(byte_count_size - 1, &count);
                count += 1;
                if (result.status != ResultStatus::Success) {
                    return result;
                }
                if (count + 1 >= middle_byte) {
                    count++;
                }
                byte_count = count;
            }
        }
        else {
            uint64_t count;
            Result result = read_bits(byte_count_size, &count);
            byte_count = count + 1;
            if (result.status != ResultStatus::Success) {
                return result;
            }
        }

        uint8_t* bytes = m_reader->read(byte_count);
        if (bytes == NULL) {
            return Result(ResultStatus::ReadFailed);
        }
        uint32_t max = min((uint32_t)max_bits, (uint32_t)(byte_count * BYTE_SIZE));
        uint32_t little_endian = (*(uint32_t*)bytes) & BIT_MASK(0, min(max, 31), uint32_t);
        *value = little_endian_to_native_endianness_uint32(little_endian);

        if (byte_count * BYTE_SIZE > max_bits) {
            uint8_t start = max_bits % 8;
            DeserializerFreeBits free_bits{};
            free_bits.byte = bytes[byte_count - 1];
            free_bits.start = start;
            free_bits.end = 8;
            if (m_free_bits.push(free_bits) == nullptr) {
                return Result(ResultStatus::MemoryAllocationFailed);
            }
        }

        return Result(ResultStatus::Success);
    }
}

Result Deserializer::deserialize_bool(bool* value) {
    return read_bit((uint8_t*)value);
}

void Deserializer::reset() {
    m_free_bits.clear();
}

Result Deserializer::read_bit(uint8_t* value) {
    *value = 0;
    DeserializerFreeBits* free_bits; 
    Result result = get_free_bits(&free_bits);
    if (result.status != ResultStatus::Success) {
        return result;
    }
    *value = (free_bits->byte & BIT_MASK(free_bits->start, 1, uint8_t)) >> free_bits->start;
    free_bits->start++;
    if (free_bits->start >= free_bits->end) {
        if (!m_free_bits.remove(0)) {
            return Result(ResultStatus::MemoryOperationFailed);
        }
    }
    return Result(ResultStatus::Success);
}

Result Deserializer::read_bits(size_t count, uint64_t* value) {
    *value = 0;
    size_t index = 0;
    while (count > 0) {
        DeserializerFreeBits* free_bits = nullptr; 
        Result result = get_free_bits(&free_bits);
        if (free_bits == nullptr) {
            assert(result.status != ResultStatus::Success);
            return result;
        }
        size_t read_count = min(free_bits->end - free_bits->start, count);
        uint64_t data = (free_bits->byte & BIT_MASK(free_bits->start, read_count, uint64_t)) >> free_bits->start;
        *value |= data << index;
        index += read_count;
        count -= read_count;
        free_bits->start += (uint8_t)read_count;
        if (free_bits->start >= free_bits->end) {
            if (!m_free_bits.remove(0)) {
                assert(false);
                return Result(ResultStatus::MemoryOperationFailed);
            }
        }
    }
    return Result(ResultStatus::Success);
}

Result Deserializer::get_free_bits(DeserializerFreeBits** out_free_bits) {
    DeserializerFreeBits* free_bits = m_free_bits.first();
    if (free_bits == NULL) {
        uint8_t* byte = m_reader->read(1);
        if (byte == NULL) {
            return Result(ResultStatus::ReadFailed);
        }
        DeserializerFreeBits value{};
        value.start = 0;
        value.end = BYTE_SIZE;
        value.byte = *byte;
        free_bits = m_free_bits.push(value);
        if (free_bits == NULL) {
            return Result(ResultStatus::MemoryAllocationFailed);
        }
    }
    *out_free_bits = free_bits;
    return Result(ResultStatus::Success);
}
