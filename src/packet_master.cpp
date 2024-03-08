#include "packet_master.h"

#define BYTE_SIZE 8
#define BIT_MASK(start, count, type) ((type)~(bit_shift_left((type)(~(type)0), (type)(count))) << (type)(start))

#ifdef ENABLE_ASSERT
#include <stdio.h>
void assert_impl(bool value, const char* expression, size_t line, const char* file) {
    if (!value) {
        fprintf(stderr, "%s:%u: Assertion failed: %s.\n", file, (uint32_t)line, expression);
    }
}
#endif

#define unimplemented() fprintf(stderr, "%s:%u: Attempt to run code marked as unimplemented.\n", __FILE__, __LINE__)

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

// safer implementation of a bit shift to allow shifting by the type bit size without causing undefined behaviour
// bit shifting left by the bit size of the number will return 0
template<typename T>
static inline T bit_shift_left(T a, T b);

#ifdef UINT16_MAX
template<>
inline uint8_t bit_shift_left<uint8_t>(uint8_t a, uint8_t b) {
    return (uint16_t)a << b;
}
#else
template<>
inline uint8_t bit_shift_left<uint8_t>(uint8_t a, uint8_t b) {
    if (b >= sizeof(a) * BYTE_SIZE) {
        return 0;
    }
    return a << b;
}
#endif

#ifdef UINT32_MAX
template<>
inline uint16_t bit_shift_left<uint16_t>(uint16_t a, uint16_t b) {
    return (uint32_t)a << b;
}
#else
template<>
inline uint16_t bit_shift_left<uint16_t>(uint16_t a, uint16_t b) {
    if (b >= sizeof(a) * BYTE_SIZE) {
        return 0;
    }
    return a << b;
}
#endif

#ifdef UINT64_MAX
template<>
inline uint32_t bit_shift_left<uint32_t>(uint32_t a, uint32_t b) {
    return (uint64_t)a << b;
}
#else
template<>
inline uint32_t bit_shift_left<uint32_t>(uint32_t a, uint32_t b) {
    if (b >= sizeof(a) * BYTE_SIZE) {
        return 0;
    }
    return a << b;
}
#endif


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

// TODO: change this function to have a fixed size input and output, uint32_t
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

uint16_t little_endian_to_native_endianness(uint16_t value) {
    if (detect_endianness() == BigEndian) {
        return swap_byte_order(value);
    }
    else {
        return value;
    }
}
uint32_t little_endian_to_native_endianness(uint32_t value) {
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

PreparedUintOptions uint8_default_options() {
    UintOptions options;
    options.max_bits = sizeof(uint8_t) * BYTE_SIZE;
    options.segments_hint = 0; // default behaviour
    return prepare_uint_options(options);
}

PreparedUintOptions uint8_max_bits(uint32_t bits) {
    UintOptions options;
    options.max_bits = bits;
    options.segments_hint = 0; // default behaviour
    return prepare_uint_options(options);
}
PreparedUintOptions uint8_max(uint8_t number) {
    return uint8_max_bits(count_used_bits_uint32((uint32_t)number));
}

PreparedUintOptions uint16_default_options() {
    UintOptions options;
    options.max_bits = sizeof(uint16_t) * BYTE_SIZE;
    options.segments_hint = 0; // default behaviour
    return prepare_uint_options(options);
}
PreparedUintOptions uint16_max_bits(uint32_t bits) {
    UintOptions options;
    options.max_bits = bits;
    options.segments_hint = 0; // default behaviour
    return prepare_uint_options(options);
}
PreparedUintOptions uint16_max(uint16_t number) {
    return uint16_max_bits(count_used_bits_uint32((uint32_t)number));
}

PreparedUintOptions uint32_default_options() {
    UintOptions options;
    options.max_bits = sizeof(uint32_t) * BYTE_SIZE;
    options.segments_hint = 0; // default behaviour
    return prepare_uint_options(options);
}
PreparedUintOptions uint32_max_bits(uint32_t bits) {
    UintOptions options;
    options.max_bits = bits;
    options.segments_hint = 0; // default behaviour
    return prepare_uint_options(options);
}
PreparedUintOptions uint32_max(uint32_t number) {
    return uint32_max_bits(count_used_bits_uint32(number));
}

PreparedUintOptions prepare_uint_options(UintOptions options) {
    PreparedUintOptions result;
    result.max_bits = options.max_bits;
    if (options.segments_hint == 0) {
        result.segment_count = closest_power_of_two(ceil_divide(options.max_bits, BYTE_SIZE));
    } else {
        result.segment_count = closest_power_of_two(options.segments_hint);
    }
    result.small_segment_size = options.max_bits / result.segment_count;
    result.big_segment_size = result.small_segment_size + 1;
    result.big_segment_count = options.max_bits % result.segment_count;
    result.segments_storage_size = count_used_bits_uint32(result.segment_count - 1);
    return result;
}

Serializer::Serializer(Writer* writer, Allocator* allocator) 
    : m_writer(writer), m_start_index(0), m_buffer(allocator), m_free_bits(allocator) {
}

Serializer::~Serializer() {}

Result Serializer::serialize_uint8(uint8_t value, PreparedUintOptions options) {
    assert(options.max_bits <= sizeof(value) * BYTE_SIZE);
    uint32_t used_bits = count_used_bits_uint32((uint32_t)value);
    assert(used_bits <= options.max_bits);

    uint32_t used_big_segments = min(options.big_segment_count, ceil_divide(used_bits, options.big_segment_size));
    uint32_t used_segments = used_big_segments;
    uint32_t used_bits_by_big_segments = used_big_segments * options.big_segment_size;
    uint32_t final_used_bits = used_bits_by_big_segments;
    if (used_bits_by_big_segments < used_bits) {
        uint32_t used_small_segments = ceil_divide(used_bits - used_bits_by_big_segments, options.small_segment_size);
        used_segments += used_small_segments;
        final_used_bits += used_small_segments * options.small_segment_size;
    }

    Result result = push_bits(used_segments - 1, options.segments_storage_size);
    if (result.status != ResultStatus::Success) {
        return result;
    }

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

Result Serializer::serialize_uint16(uint16_t value, PreparedUintOptions options) {
    assert(options.max_bits <= sizeof(value) * BYTE_SIZE);
    uint32_t used_bits = max(count_used_bits_uint32((uint32_t)value), 1);
    assert(used_bits <= options.max_bits);

    uint32_t used_big_segments = min(options.big_segment_count, ceil_divide(used_bits, options.big_segment_size));
    uint32_t used_segments = used_big_segments;
    uint32_t used_bits_by_big_segments = used_big_segments * options.big_segment_size;
    uint32_t final_used_bits = used_bits_by_big_segments;
    if (used_bits_by_big_segments < used_bits) {
        uint32_t used_small_segments = ceil_divide(used_bits - used_bits_by_big_segments, options.small_segment_size);
        used_segments += used_small_segments;
        final_used_bits += used_small_segments * options.small_segment_size;
    }

    Result result = push_bits(used_segments - 1, options.segments_storage_size);
    if (result.status != ResultStatus::Success) {
        return result;
    }

    uint32_t used_bytes = ceil_divide(final_used_bits, BYTE_SIZE);
    uint16_t little_endian = native_endianness_to_little_endian(value);
    if (m_buffer.push_many((uint8_t*)&little_endian, (size_t)used_bytes) == nullptr) {
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

Result Serializer::serialize_uint32(uint32_t value, PreparedUintOptions options) {
    assert(options.max_bits <= sizeof(value) * BYTE_SIZE);
    uint32_t used_bits = max(count_used_bits_uint32(value), 1);
    assert(used_bits <= options.max_bits);

    uint32_t used_big_segments = min(options.big_segment_count, ceil_divide(used_bits, options.big_segment_size));
    uint32_t used_segments = used_big_segments;
    uint32_t used_bits_by_big_segments = used_big_segments * options.big_segment_size;
    uint32_t final_used_bits = used_bits_by_big_segments;
    if (used_bits_by_big_segments < used_bits) {
        uint32_t used_small_segments = ceil_divide(used_bits - used_bits_by_big_segments, options.small_segment_size);
        used_segments += used_small_segments;
        final_used_bits += used_small_segments * options.small_segment_size;
    }

    Result result = push_bits(used_segments - 1, options.segments_storage_size);
    if (result.status != ResultStatus::Success) {
        return result;
    }

    uint32_t used_bytes = ceil_divide(final_used_bits, BYTE_SIZE);
    uint32_t little_endian = native_endianness_to_little_endian(value);
    if (m_buffer.push_many((uint8_t*)&little_endian, (size_t)used_bytes) == nullptr) {
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
    assert(count <= sizeof(uint32_t) * BYTE_SIZE);
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

Result Deserializer::deserialize_uint8(PreparedUintOptions options, uint8_t* value) {
    *value = 0;
    uint32_t used_segments;
    Result result = read_bits(options.segments_storage_size, &used_segments);
    used_segments += 1;
    if (result.status != ResultStatus::Success) {
        return result;
    }
    
    uint32_t used_bits;
    if (used_segments > options.big_segment_count) {
        uint32_t used_small_segments = used_segments - options.big_segment_count;
        used_bits = options.big_segment_count * options.big_segment_size + used_small_segments * options.small_segment_size;
    }
    else {
        used_bits = used_segments * options.big_segment_size;
    }
    
    uint32_t used_bytes = ceil_divide(used_bits, BYTE_SIZE);
    assert(used_bytes == 1); // for uint8_t

    uint8_t* byte = m_reader->read((size_t)used_bytes);
    if (byte == nullptr) {
        return Result(ResultStatus::ReadFailed);
    }
    *value = *byte & BIT_MASK(0, used_bits, uint8_t);
    uint32_t free_bits_start = used_bits % BYTE_SIZE;
    if (free_bits_start > 0) {
        DeserializerFreeBits free_bits;
        free_bits.start = free_bits_start;
        free_bits.end = BYTE_SIZE;
        free_bits.byte = *byte;
        if (m_free_bits.push(free_bits) == nullptr) {
            return Result(ResultStatus::MemoryAllocationFailed);
        }
    }
    return Result(ResultStatus::Success);
}

Result Deserializer::deserialize_uint16(PreparedUintOptions options, uint16_t* value) {
    *value = 0;
    uint32_t used_segments;
    Result result = read_bits(options.segments_storage_size, &used_segments);
    used_segments += 1;
    if (result.status != ResultStatus::Success) {
        return result;
    }
    
    uint32_t used_bits;
    if (used_segments > options.big_segment_count) {
        uint32_t used_small_segments = used_segments - options.big_segment_count;
        used_bits = options.big_segment_count * options.big_segment_size + used_small_segments * options.small_segment_size;
    }
    else {
        used_bits = used_segments * options.big_segment_size;
    }
    
    uint32_t used_bytes = ceil_divide(used_bits, BYTE_SIZE);
    uint8_t* byte = m_reader->read((size_t)used_bytes);
    if (byte == nullptr) {
        return Result(ResultStatus::ReadFailed);
    }
    uint16_t little_endian = *(uint16_t*)byte & BIT_MASK(0, used_bits, uint16_t);
    *value = little_endian_to_native_endianness(little_endian);
    uint32_t free_bits_start = used_bits % BYTE_SIZE;
    if (free_bits_start > 0) {
        DeserializerFreeBits free_bits;
        free_bits.start = free_bits_start;
        free_bits.end = BYTE_SIZE;
        free_bits.byte = *byte;
        if (m_free_bits.push(free_bits) == nullptr) {
            return Result(ResultStatus::MemoryAllocationFailed);
        }
    }
    return Result(ResultStatus::Success);
}

Result Deserializer::deserialize_uint32(PreparedUintOptions options, uint32_t* value) {
    *value = 0;
    uint32_t used_segments;
    Result result = read_bits(options.segments_storage_size, &used_segments);
    used_segments += 1;
    if (result.status != ResultStatus::Success) {
        return result;
    }
    

    uint32_t used_bits;
    if (used_segments > options.big_segment_count) {
        uint32_t used_small_segments = used_segments - options.big_segment_count;
        used_bits = options.big_segment_count * options.big_segment_size + used_small_segments * options.small_segment_size;
    }
    else {
        used_bits = used_segments * options.big_segment_size;
    }
    
    uint32_t used_bytes = ceil_divide(used_bits, BYTE_SIZE);
    uint8_t* byte = m_reader->read((size_t)used_bytes);
    if (byte == nullptr) {
        return Result(ResultStatus::ReadFailed);
    }
    uint32_t little_endian = *(uint32_t*)byte & BIT_MASK(0, used_bits, uint32_t);
    *value = little_endian_to_native_endianness(little_endian);
    uint32_t free_bits_start = used_bits % BYTE_SIZE;
    if (free_bits_start > 0) {
        DeserializerFreeBits free_bits;
        free_bits.start = free_bits_start;
        free_bits.end = BYTE_SIZE;
        free_bits.byte = *byte;
        if (m_free_bits.push(free_bits) == nullptr) {
            return Result(ResultStatus::MemoryAllocationFailed);
        }
    }
    return Result(ResultStatus::Success);
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

Result Deserializer::read_bits(size_t count, uint32_t* value) {
    assert(count <= sizeof(uint32_t) * BYTE_SIZE);
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
        uint32_t data = (free_bits->byte & BIT_MASK(free_bits->start, read_count, uint32_t)) >> free_bits->start;
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
