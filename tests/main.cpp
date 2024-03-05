#include <stdlib.h>
#include <stdio.h>
#include <packet_master.h>
#include <string.h>
#include "test/test.h"


void* my_malloc(size_t size, void* ctx) {
    (void)ctx;
    return malloc(size);
}

void* my_realloc(void* ptr, size_t old_size, size_t new_size, void* ctx) {
    (void)old_size;
    (void)ctx;
    return realloc(ptr, new_size);
}

void my_free(void* ptr, size_t size, void* ctx) {
    (void)size;
    (void)ctx;
    free(ptr);
}

static Allocator allocator = {
    my_malloc,
    my_realloc,
    my_free,
    NULL
};

typedef struct {
    Vector<uint8_t>* buffer;
    size_t index;
} BufferReader;

BufferReader read_buffer(Vector<uint8_t>* buffer) {
    BufferReader reader{};
    reader.buffer = buffer;
    return reader;
}

int write_data(void* data, uint8_t* incoming_data, size_t size) {
    return ((Vector<uint8_t>*)data)->push_many(incoming_data, size) == nullptr ? 1 : 0;
}

uint8_t* read_data(void* data, size_t data_size) {
    BufferReader* reader = (BufferReader*)data;
    if (data_size > reader->buffer->length() - reader->index) {
        return NULL;
    }
    uint8_t* bytes = reader->buffer->ptr() + reader->index;
    reader->index += data_size;
    return bytes;
}

void test_count_bits() {
    // native
    ts_expect_int_eq(count_leading_zeros_uint(0b1), 31);
    ts_expect_int_eq(count_leading_zeros_uint(0b10), 30);
    ts_expect_int_eq(count_leading_zeros_uint(0b100), 29);
    ts_expect_int_eq(count_leading_zeros_uint(0b1000), 28);
    ts_expect_int_eq(count_leading_zeros_uint(0b1110000), 25);
    ts_expect_int_eq(count_leading_zeros_uint(0b111001100), 23);
    ts_expect_int_eq(count_leading_zeros_uint(0), 32);

    ts_expect_int_eq(count_leading_zeros_uint_fallback(0b1), 31);
    ts_expect_int_eq(count_leading_zeros_uint_fallback(0b10), 30);
    ts_expect_int_eq(count_leading_zeros_uint_fallback(0b100), 29);
    ts_expect_int_eq(count_leading_zeros_uint_fallback(0b1000), 28);
    ts_expect_int_eq(count_leading_zeros_uint_fallback(0b1110000), 25);
    ts_expect_int_eq(count_leading_zeros_uint_fallback(0b111001100), 23);
    ts_expect_int_eq(count_leading_zeros_uint_fallback(0), 32);
}

void test_serializer(Serializer* serializer) {
    {
        Result result;
        serializer->serialize_uint8(2, &result);
        ts_expect_success(result);
    }
    {
        Result result;
        serializer->serialize_uint8(0, &result);
        ts_expect_success(result);
    }
    {
        Result result;
        serializer->serialize_uint8_max(3, max_bits_u8(7), &result);
        ts_expect_success(result);
    }
    {
        Result result;
        serializer->serialize_bool(true, &result);
        ts_expect_success(result);
    }
    {
        Result result;
        serializer->serialize_bool(false, &result);
        ts_expect_success(result);
    }
    {
        Result result;
        serializer->serialize_bool(true, &result);
        ts_expect_success(result);
    }
    {
        Result result;
        // 5 = 0b101
        serializer->serialize_uint8_max(5, max_bits_u8(4), &result);
        ts_expect_success(result);
    }
    {
        Result result;
        serializer->serialize_uint16(1023, &result);
        ts_expect_success(result);
    }
    {
        Result result;
        serializer->serialize_uint16(127, &result);
        ts_expect_success(result);
    }
    {
        Result result;
        serializer->serialize_uint16_max(511, max_bits_u8(10), &result);
        ts_expect_success(result);
    }
    {
        Result result;
        serializer->serialize_uint32(0b00001111000000000000000000000000, &result);
        ts_expect_success(result);
    }
    {
        Result result;
        serializer->serialize_uint32_max(0b100000001111111100000000, max_bits_u8(24), &result);
        ts_expect_success(result);
    }
    {
        Result result;
        serializer->serialize_uint32_max(0b000000001111111111111111, max_bits_u8(24), &result);
        ts_expect_success(result);
    }
    {
        Result result;
        serializer->serialize_uint16(0, &result);
        ts_expect_success(result);
    }
    {
        Result result;
        serializer->serialize_uint32(0, &result);
        ts_expect_success(result);
    }
    {
        Result result;
        serializer->finalize(&result);
        ts_expect_success(result);
    }
}


void validate_serialized_data(Vector<uint8_t>& buffer) {
    ts_assert(buffer.capacity() >= buffer.length());
    ts_expect_uint8_eq(buffer[0], 2);
    ts_expect_uint8_eq(buffer[1], 0);
    ts_expect_uint8_eq(buffer[2], 0b10000011);
    ts_expect_uint8_eq(buffer[3], 0b01010110);
    ts_expect_uint8_eq(buffer[4], 0b11111111);
    ts_expect_uint8_eq(buffer[5], 0b00000011);
    ts_expect_uint8_eq(buffer[6], 0b01111111);
    ts_expect_uint8_eq(buffer[7], 0b00110111);
    ts_expect_uint8_eq(buffer[8], 0b11111111);
    ts_expect_uint8_eq(buffer[9], 0b01);

    // 00001111_0000000_00000000_000000000
    ts_expect_uint8_eq(buffer[10], 0b00000000);
    ts_expect_uint8_eq(buffer[11], 0b00000000);
    ts_expect_uint8_eq(buffer[12], 0b00000000);
    ts_expect_uint8_eq(buffer[13], 0b00001111);

    // 0b10000000_11111111_00000000
    ts_expect_uint8_eq(buffer[14], 0b00000000);
    ts_expect_uint8_eq(buffer[15], 0b11111111);
    ts_expect_uint8_eq(buffer[16], 0b10000000);

    // 0b000000001111111111111111
    ts_expect_uint8_eq(buffer[17], 0b11111111);
    ts_expect_uint8_eq(buffer[18], 0b11111111);
    
    ts_expect_uint8_eq(buffer[19], 0);
    ts_expect_uint8_eq(buffer[20], 0);

    ts_expect_size_eq(buffer.length(), 21);
}


void test_deserializer(Deserializer* deserializer) {
    {
        Result result;
        ts_expect_uint8_eq(deserializer->deserialize_uint8(&result), 2);
        ts_expect_success(result);
    }
    {
        Result result;
        ts_expect_uint8_eq(deserializer->deserialize_uint8(&result), 0);
        ts_expect_success(result);
    }
    {
        Result result;
        ts_expect_uint8_eq(max_u8(127), 7);
        ts_expect_uint8_eq(deserializer->deserialize_uint8_max(max_u8(127), &result), 3);
        ts_expect_success(result);
    }
    {
        Result result;
        ts_expect_bool_eq(deserializer->deserialize_bool(&result), true);
        ts_expect_success(result);
    }
    {
        Result result;
        ts_expect_bool_eq(deserializer->deserialize_bool(&result), false);
        ts_expect_success(result);
    }
    {
        Result result;
        ts_expect_uint16_eq(deserializer->deserialize_bool(&result), true);
        ts_expect_success(result);
    }

    {
        Result result;
        ts_expect_uint8_eq(deserializer->deserialize_uint8_max(max_bits_u8(4), &result), 5);
        ts_expect_status(result, ResultStatus::Success);
    }
    {
        Result result;
        ts_expect_uint16_eq(deserializer->deserialize_uint16(&result), 1023);
        ts_expect_status(result, ResultStatus::Success);
    }
    {
        Result result;
        ts_expect_uint16_eq(deserializer->deserialize_uint16(&result), 127);
        ts_expect_status(result, ResultStatus::Success);
    }
    {
        Result result;
        ts_expect_uint16_eq(deserializer->deserialize_uint16_max(max_bits_u8(10), &result), 511);
        ts_expect_status(result, ResultStatus::Success);
    }
    {
        Result result;
        ts_expect_uint32_eq(deserializer->deserialize_uint32(&result), 0b00001111000000000000000000000000);
        ts_expect_status(result, ResultStatus::Success);
    }
    {
        Result result;
        ts_expect_uint32_eq(deserializer->deserialize_uint32_max(max_bits_u8(24), &result), 0b100000001111111100000000);
        ts_expect_status(result, ResultStatus::Success);
    }
    {
        Result result;
        ts_expect_uint32_eq(deserializer->deserialize_uint32_max(max_bits_u8(24), &result), 0b000000001111111111111111);
        ts_expect_status(result, ResultStatus::Success);
    }
    {
        Result result;
        ts_expect_uint16_eq(deserializer->deserialize_uint16(&result), 0);
        ts_expect_status(result, ResultStatus::Success);
    }
    {
        Result result;
        ts_expect_uint32_eq(deserializer->deserialize_uint32(&result), 0);
        ts_expect_status(result, ResultStatus::Success);
    }
    {
        Result result;
        ts_expect_uint8_eq(deserializer->deserialize_uint8(&result), 0);
        ts_expect_status(result, ResultStatus::ReadFailed);
    }
}

int main() {
    ts_start_testing();

    Vector<uint8_t> buffer(&allocator);
    
    Writer writer{};
    writer.write_callback = write_data;
    writer.ctx = &buffer;
    Serializer serializer(&writer, &allocator);
    TS_RUN_TEST(test_serializer, &serializer);
    

    TS_RUN_TEST(validate_serialized_data, buffer);

    
    BufferReader buf_reader = read_buffer(&buffer);
    Reader reader{};
    reader.read_callback = read_data;
    reader.ctx = &buf_reader;
    Deserializer deserializer(&reader, &allocator);
    TS_RUN_TEST(test_deserializer, &deserializer);
    

    TS_RUN_TEST(test_count_bits);

    return ts_finish_testing();
}