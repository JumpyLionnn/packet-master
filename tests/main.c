#include <stdlib.h>
#include <stdio.h>
#include <packet_master.h>
#include <string.h>
#include "test.h"


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
    expect_int_eq(count_leading_zeros_uint(0b1), 31);
    expect_int_eq(count_leading_zeros_uint(0b10), 30);
    expect_int_eq(count_leading_zeros_uint(0b100), 29);
    expect_int_eq(count_leading_zeros_uint(0b1000), 28);
    expect_int_eq(count_leading_zeros_uint(0b1110000), 25);
    expect_int_eq(count_leading_zeros_uint(0b111001100), 23);
    expect_int_eq(count_leading_zeros_uint(0), 32);

    expect_int_eq(count_leading_zeros_uint_fallback(0b1), 31);
    expect_int_eq(count_leading_zeros_uint_fallback(0b10), 30);
    expect_int_eq(count_leading_zeros_uint_fallback(0b100), 29);
    expect_int_eq(count_leading_zeros_uint_fallback(0b1000), 28);
    expect_int_eq(count_leading_zeros_uint_fallback(0b1110000), 25);
    expect_int_eq(count_leading_zeros_uint_fallback(0b111001100), 23);
    expect_int_eq(count_leading_zeros_uint_fallback(0), 32);
}

void test_serializer() {
    Buffer buffer = create_buffer(10, &allocator);
    Writer writer = {
        .write = write_data,
        .data = &buffer
    };
    Serializer serializer = create_serializer(&writer, allocator);
    {
        Result result;
        serialize_uint8(&serializer, 2, &result);
        expect_success(result);
    }
    {
        Result result;
        serialize_uint8_max(&serializer, 3, max_bits_u8(7), &result);
        expect_success(result);
    }
    {
        Result result;
        serialize_bool(&serializer, true, &result);
        expect_success(result);
    }
    {
        Result result;
        serialize_bool(&serializer, false, &result);
        expect_success(result);
    }
    {
        Result result;
        serialize_bool(&serializer, true, &result);
        expect_success(result);
    }
    {
        Result result;
        serializer_finalize(&serializer, &result);
        expect_success(result);
    }

    expect_uint8_eq(*(buffer.data + 0), 2);
    expect_uint8_eq(*(buffer.data + 1), 0b10000011);
    expect_uint8_eq(*(buffer.data + 2), 0b10);
    expect_size_eq(buffer.size, 3);


    BufferReader buf_reader = read_buffer(&buffer);
    Reader reader = {
        .read = read_data,
        .data = &buf_reader  
    };
    Deserializer deserializer = create_deserializer(&reader, allocator);

    {
        Result result;
        expect_uint8_eq(deserialize_uint8(&deserializer, &result), 2);
        expect_success(result);
    }
    {
        Result result;
        expect_uint8_eq(max_u8(127), 7);
        expect_uint8_eq(deserialize_uint8_max(&deserializer, max_u8(127), &result), 3);
        expect_success(result);
    }
    {
        Result result;
        expect_bool_eq(deserialize_bool(&deserializer, &result), true);
        expect_success(result);
    }
    {
        Result result;
        expect_bool_eq(deserialize_bool(&deserializer, &result), false);
        expect_success(result);
    }
    {
        Result result;
        expect_bool_eq(deserialize_bool(&deserializer, &result), true);
        expect_success(result);
    }

    {
        Result result;
        expect_uint8_eq(deserialize_uint8(&deserializer, &result), 0);
        expect_status(result, Status_ReadFailed);
    }

    free_deserializer(&deserializer);
    free_serializer(&serializer);
    free_buffer(&buffer, &allocator);
}


int main() {

    // tests
    test_serializer();
    test_count_bits();

    printf("\nTest result: %s. %zu passed; %zu failed;\n", g_tests_state.failed == 0 ? "ok" : "FAILED",  g_tests_state.passed, g_tests_state.failed);
    if (g_tests_state.failed == 0) {
        return EXIT_SUCCESS;
    }
    else {
        return EXIT_FAILURE;
    }
}