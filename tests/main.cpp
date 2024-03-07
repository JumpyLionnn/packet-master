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

void test_serializer_bool(Serializer* serializer) {
    ts_expect_success(serializer->serialize_bool(true));
    ts_expect_success(serializer->serialize_bool(false));
    ts_expect_success(serializer->serialize_bool(true));
    ts_expect_success(serializer->finalize());
}
void validate_serialized_data_bool(Vector<uint8_t>& buffer) {
    ts_assert(buffer.capacity() >= buffer.length());
    ts_expect_uint8_eq(buffer[0], 0b00000101);
    ts_expect_size_eq(buffer.length(), 1);
}
void test_deserializer_bool(Deserializer* deserializer) {
    {
        bool value;
        ts_expect_success(deserializer->deserialize_bool(&value));
        ts_expect_bool_eq(value, true);
    }
    {
        bool value;
        ts_expect_success(deserializer->deserialize_bool(&value));
        ts_expect_bool_eq(value, false);
    }
    {
        bool value;
        ts_expect_success(deserializer->deserialize_bool(&value));
        ts_expect_bool_eq(value, true);
    }
    {
        uint8_t value;
        ts_expect_status(deserializer->deserialize_uint8(u8_default_options(), &value), ResultStatus::ReadFailed);
        ts_expect_uint8_eq(value, 0);
    }
}


void test_serializer_uint8(Serializer* serializer) {
    ts_expect_success(serializer->serialize_uint8(2, u8_default_options()));
    ts_expect_success(serializer->serialize_uint8(0, u8_default_options()));
    ts_expect_success(serializer->serialize_uint8(3, max_bits_u8(4)));
    // 12 : 0b00001100
    ts_expect_success(serializer->serialize_uint8(1, max_u8(12)));
    // 102 : 0b01100110
    ts_expect_success(serializer->serialize_uint8(36, max_u8(102)));
    UintOptions options;
    options.max_bits = 8;
    options.segments_hint = 3;
    // 0b00101000
    ts_expect_success(serializer->serialize_uint8(40, prepare_uint_options(options)));
    options.max_bits = 7;
    options.segments_hint = 2;
    ts_expect_success(serializer->serialize_uint8(120, prepare_uint_options(options)));
    ts_expect_success(serializer->finalize());
}
void validate_serialized_data_uint8(Vector<uint8_t>& buffer) {
    ts_assert(buffer.capacity() >= buffer.length());
    ts_expect_uint8_eq(buffer[0], 0b00000010);
    ts_expect_uint8_eq(buffer[1], 0b00000000);
    ts_expect_uint8_eq(buffer[2], 0b01100011);
    ts_expect_uint8_eq(buffer[3], 0b00000001);
    ts_expect_uint8_eq(buffer[4], 0b00100100);
    ts_expect_uint8_eq(buffer[5], 0b00101000);
    ts_expect_uint8_eq(buffer[6], 0b01111000);
    ts_expect_size_eq(buffer.length(), 7);
}
void test_deserializer_uint8(Deserializer* deserializer) {
    {
        uint8_t value;
        ts_expect_success(deserializer->deserialize_uint8(u8_default_options(), &value));
        ts_expect_uint8_eq(value, 2);
    }
    {
        uint8_t value;
        ts_expect_success(deserializer->deserialize_uint8(u8_default_options(), &value));
        ts_expect_uint8_eq(value, 0);
    }
    {
        uint8_t value;
        ts_expect_success(deserializer->deserialize_uint8(max_bits_u8(4), &value));
        ts_expect_uint8_eq(value, 3);
    }
    {
        uint8_t value;
        ts_expect_success(deserializer->deserialize_uint8(max_u8(12), &value));
        ts_expect_uint8_eq(value, 1);
    }
    {
        uint8_t value;
        ts_expect_success(deserializer->deserialize_uint8(max_u8(102), &value));
        ts_expect_uint8_eq(value, 36);
    }
    UintOptions options;
    {
        options.max_bits = 8;
        options.segments_hint = 3;
        uint8_t value;
        ts_expect_success(deserializer->deserialize_uint8(prepare_uint_options(options), &value));
        ts_expect_uint8_eq(value, 40);
    }
    {
        options.max_bits = 7;
        options.segments_hint = 2;
        uint8_t value;
        ts_expect_success(deserializer->deserialize_uint8(prepare_uint_options(options), &value));
        ts_expect_uint8_eq(value, 120);
    }
    {
        uint8_t value;
        ts_expect_status(deserializer->deserialize_uint8(u8_default_options(), &value), ResultStatus::ReadFailed);
        ts_expect_uint8_eq(value, 0);
    }
}



void test_serializer(Serializer* serializer) {
    ts_expect_success(serializer->serialize_uint8(2, u8_default_options()));
    ts_expect_success(serializer->serialize_uint8(0, u8_default_options()));
    ts_expect_success(serializer->serialize_uint8(3, max_bits_u8(7)));
    ts_expect_success(serializer->serialize_bool(true));
    ts_expect_success(serializer->serialize_bool(false));
    ts_expect_success(serializer->serialize_bool(true));
    // 5 = 0b101
    ts_expect_success(serializer->serialize_uint8(5, max_bits_u8(4)));
    ts_expect_success(serializer->serialize_uint16(1023));
    ts_expect_success(serializer->serialize_uint16(127));
    ts_expect_success(serializer->serialize_uint16_max(511, 10));
    ts_expect_success(serializer->serialize_uint32(0b00001111000000000000000000000000));
    ts_expect_success(serializer->serialize_uint16(0));
    ts_expect_success(serializer->serialize_uint32(0));
    ts_expect_success(serializer->finalize());
}

void validate_serialized_data(Vector<uint8_t>& buffer) {
    ts_assert(buffer.capacity() >= buffer.length());
    ts_expect_uint8_eq(buffer[0], 2);
    ts_expect_uint8_eq(buffer[1], 0);
    ts_expect_uint8_eq(buffer[2], 0b10000011);
    ts_expect_uint8_eq(buffer[3], 0b01110110);
    ts_expect_uint8_eq(buffer[4], 0b00000101);
    ts_expect_uint8_eq(buffer[5], 0b11111111);
    ts_expect_uint8_eq(buffer[6], 0b00000011);
    ts_expect_uint8_eq(buffer[7], 0b01111111);
    ts_expect_uint8_eq(buffer[8], 0b11111111);
    ts_expect_uint8_eq(buffer[9], 0b01);

    // 00001111_0000000_00000000_000000000
    ts_expect_uint8_eq(buffer[10], 0b00000000);
    ts_expect_uint8_eq(buffer[11], 0b00000000);
    ts_expect_uint8_eq(buffer[12], 0b00000000);
    ts_expect_uint8_eq(buffer[13], 0b00001111);

    ts_expect_uint8_eq(buffer[14], 0b00000000);
    ts_expect_uint8_eq(buffer[15], 0b00000000);

    ts_expect_size_eq(buffer.length(),16);
}

void test_deserializer(Deserializer* deserializer) {
    {
        uint8_t value;
        ts_expect_success(deserializer->deserialize_uint8(u8_default_options(), &value));
        ts_expect_uint8_eq(value, 2);
    }
    {
        uint8_t value;
        ts_expect_success(deserializer->deserialize_uint8(u8_default_options(), &value));
        ts_expect_uint8_eq(value, 0);
    }
    {
        uint8_t value;
        ts_expect_uint8_eq(max_u8(127).max_bits, 7);
        ts_expect_success(deserializer->deserialize_uint8(max_u8(127), &value));
        ts_expect_uint8_eq(value, 3);
    }
    {
        bool value;
        ts_expect_success(deserializer->deserialize_bool(&value));
        ts_expect_bool_eq(value, true);
    }
    {
        bool value;
        ts_expect_success(deserializer->deserialize_bool(&value));
        ts_expect_bool_eq(value, false);
    }
    {
        bool value;
        ts_expect_success(deserializer->deserialize_bool(&value));
        ts_expect_bool_eq(value, true);
    }

    {
        uint8_t value;
        ts_expect_success(deserializer->deserialize_uint8(max_bits_u8(4), &value));
        ts_expect_uint8_eq(value, 5);
    }
    {
        uint16_t value;
        ts_expect_success(deserializer->deserialize_uint16(&value));
        ts_expect_uint16_eq(value, 1023);
    }
    {
        uint16_t value;
        ts_expect_success(deserializer->deserialize_uint16(&value));
        ts_expect_uint16_eq(value, 127);
    }
    {
        uint16_t value;
        ts_expect_success(deserializer->deserialize_uint16_max(10, &value));
        ts_expect_uint16_eq(value, 511);
    }
    {
        uint32_t value;
        ts_expect_success(deserializer->deserialize_uint32(&value));
        ts_expect_uint32_eq(value, 0b00001111000000000000000000000000);
    }
    {
        uint16_t value;
        ts_expect_success(deserializer->deserialize_uint16(&value));
        ts_expect_uint16_eq(value, 0);
    }
    {
        uint32_t value;
        ts_expect_success(deserializer->deserialize_uint32(&value));
        ts_expect_uint32_eq(value, 0);
    }
    {
        uint8_t value;
        ts_expect_status(deserializer->deserialize_uint8(u8_default_options(), &value), ResultStatus::ReadFailed);
        ts_expect_uint8_eq(value, 0);
    }
}

int main() {
    ts_start_testing();

    Vector<uint8_t> buffer(&allocator);
    
    Writer writer{};
    writer.write_callback = write_data;
    writer.ctx = &buffer;
    Serializer serializer(&writer, &allocator);

    BufferReader buf_reader = read_buffer(&buffer);
    Reader reader{};
    reader.read_callback = read_data;
    reader.ctx = &buf_reader;
    Deserializer deserializer(&reader, &allocator);

    TS_RUN_TEST(test_serializer_bool, &serializer);
    TS_RUN_TEST(validate_serialized_data_bool, buffer);
    TS_RUN_TEST(test_deserializer_bool, &deserializer);

    buffer.clear();
    serializer.reset();
    deserializer.reset();
    buf_reader.index = 0;

    TS_RUN_TEST(test_serializer_uint8, &serializer);
    TS_RUN_TEST(validate_serialized_data_uint8, buffer);
    TS_RUN_TEST(test_deserializer_uint8, &deserializer);

    buffer.clear();
    serializer.reset();
    deserializer.reset();
    buf_reader.index = 0;
    
    TS_RUN_TEST(test_serializer, &serializer);
    TS_RUN_TEST(validate_serialized_data, buffer);
    TS_RUN_TEST(test_deserializer, &deserializer);

    TS_RUN_TEST(test_count_bits);

    return ts_finish_testing();
}