#include <stdlib.h>
#include <stdio.h>
#include <packet_master.h>
#include <string.h>
#include "test/test.h"


static Allocator allocator = {
     .alloc = malloc,
    .realloc = realloc,
    .free = free
};

typedef struct {
    Buffer* buffer;
    size_t index;
} BufferReader;

BufferReader read_buffer(Buffer* buffer) {
    return (BufferReader) {
        .buffer = buffer,
        .index = 0
    };
}

int write_data(void* data, uint8_t* incoming_data, size_t size) {
    return buffer_push_bytes((Buffer*)data, incoming_data, size, &allocator);
}

uint8_t* read_data(void* data, size_t data_size) {
    BufferReader* reader = data;
    if (data_size > reader->buffer->size - reader->index) {
        return NULL;
    }
    uint8_t* bytes = reader->buffer->data + reader->index;
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
        serialize_uint8(serializer, 2, &result);
        ts_expect_success(result);
    }
    {
        Result result;
        serialize_uint8_max(serializer, 3, max_bits_u8(7), &result);
        ts_expect_success(result);
    }
    {
        Result result;
        serialize_bool(serializer, true, &result);
        ts_expect_success(result);
    }
    {
        Result result;
        serialize_bool(serializer, false, &result);
        ts_expect_success(result);
    }
    {
        Result result;
        serialize_bool(serializer, true, &result);
        ts_expect_success(result);
    }
    {
        Result result;
        // 5 = 0b101
        serialize_uint8_max(serializer, 5, max_bits_u8(4), &result);
        ts_expect_success(result);
    }
    {
        Result result;
        // 5 = 0b101
        serialize_uint16(serializer, 1023, &result);
        ts_expect_success(result);
    }
    {
        Result result;
        serializer_finalize(serializer, &result);
        ts_expect_success(result);
    }
}


void validate_serialized_data(Buffer* buffer) {
    ts_assert(buffer->capacity >= buffer->size);
    ts_expect_uint8_eq(buffer->data[0], 2);
    ts_expect_uint8_eq(buffer->data[1], 0b10000011);
    ts_expect_uint8_eq(buffer->data[2], 0b01010110);
    ts_expect_uint8_eq(buffer->data[3], 0b11111111);
    ts_expect_uint8_eq(buffer->data[4], 0b00000011);
    ts_expect_size_eq(buffer->size, 5);
}


void test_deserializer(Deserializer* deserializer) {
    {
        Result result;
        ts_expect_uint8_eq(deserialize_uint8(deserializer, &result), 2);
        ts_expect_success(result);
    }
    {
        Result result;
        ts_expect_uint8_eq(max_u8(127), 7);
        ts_expect_uint8_eq(deserialize_uint8_max(deserializer, max_u8(127), &result), 3);
        ts_expect_success(result);
    }
    {
        Result result;
        ts_expect_bool_eq(deserialize_bool(deserializer, &result), true);
        ts_expect_success(result);
    }
    {
        Result result;
        ts_expect_bool_eq(deserialize_bool(deserializer, &result), false);
        ts_expect_success(result);
    }
    {
        Result result;
        ts_expect_bool_eq(deserialize_bool(deserializer, &result), true);
        ts_expect_success(result);
    }

    {
        Result result;
        ts_expect_uint8_eq(deserialize_uint8_max(deserializer, max_bits_u8(4), &result), 5);
        ts_expect_status(result, Status_Success);
    }
    // {
    //     Result result;
    //     ts_expect_uint8_eq(deserialize_uint8(deserializer, &result), 0);
    //     ts_expect_status(result, Status_ReadFailed);
    // }
}

int main() {
    ts_start_testing();

    Buffer buffer = create_buffer(10, &allocator);
    {
        Writer writer = {
            .write = write_data,
            .data = &buffer
        };
        Serializer serializer = create_serializer(&writer, allocator);
        TS_RUN_TEST(test_serializer, &serializer);
        free_serializer(&serializer);
    }

    TS_RUN_TEST(validate_serialized_data, &buffer);

    {
        BufferReader buf_reader = read_buffer(&buffer);
        Reader reader = {
            .read = read_data,
            .data = &buf_reader  
        };
        Deserializer deserializer = create_deserializer(&reader, allocator);
        TS_RUN_TEST(test_deserializer, &deserializer);
        free_deserializer(&deserializer);
    }
    free_buffer(&buffer, &allocator);

    TS_RUN_TEST(test_count_bits);

    return ts_finish_testing();
}