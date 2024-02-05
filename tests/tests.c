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

int test_serializer() {
    int failed = 0;

    Buffer buffer = create_buffer(10, &allocator);
    Writer writer = {
        .write = write_data,
        .data = &buffer
    };
    Serializer serializer = create_serializer(&writer, allocator);
    {
        Result result;
        serialize_uint8(&serializer, 2, &result);
        failed += expect_success(result);
    }
    {
        Result result;
        serialize_uint8_max(&serializer, 3, 7, &result);
        failed += expect_success(result);
    }
    {
        Result result;
        serialize_bool(&serializer, true, &result);
        failed += expect_success(result);
    }
    {
        Result result;
        serialize_bool(&serializer, false, &result);
        failed += expect_success(result);
    }
    {
        Result result;
        serialize_bool(&serializer, true, &result);
        failed += expect_success(result);
    }
    {
        Result result;
        serializer_finalize(&serializer, &result);
        failed += expect_success(result);
    }

    failed += expect_uint8_eq(*(buffer.data + 0), 2);
    failed += expect_uint8_eq(*(buffer.data + 1), 0b10000011);
    failed += expect_uint8_eq(*(buffer.data + 2), 0b10);
    failed += expect_size_eq(buffer.size, 3);


    BufferReader buf_reader = read_buffer(&buffer);
    Reader reader = {
        .read = read_data,
        .data = &buf_reader  
    };
    Deserializer deserializer = create_deserializer(&reader, allocator);

    {
        Result result;
        failed += expect_uint8_eq(deserialize_uint8(&deserializer, &result), 2);
        failed += expect_success(result);
    }
    {
        Result result;
        failed += expect_uint8_eq(deserialize_uint8_max(&deserializer, 7, &result), 3);
        failed += expect_success(result);
    }
    {
        Result result;
        failed += expect_bool_eq(deserialize_bool(&deserializer, &result), true);
        failed += expect_success(result);
    }
    {
        Result result;
        failed += expect_bool_eq(deserialize_bool(&deserializer, &result), false);
        failed += expect_success(result);
    }
    {
        Result result;
        failed += expect_bool_eq(deserialize_bool(&deserializer, &result), true);
        failed += expect_success(result);
    }

    {
        Result result;
        failed += expect_uint8_eq(deserialize_uint8(&deserializer, &result), 0);
        failed += expect_status(result, Status_ReadFailed);
    }

    free_deserializer(&deserializer);
    free_serializer(&serializer);
    free_buffer(&buffer, &allocator);
    return failed;
}


int main() {
    int failed = 0;

    failed += test_serializer();

    if (failed == 0) {
        printf("All tests passed\n");
        return EXIT_SUCCESS;
    }
    else {
        printf("%i tests failed\n", failed);
        return EXIT_FAILURE;
    }
}