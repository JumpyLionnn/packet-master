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


size_t min(size_t a, size_t b) {
    if (a > b) {
        return b;
    }
    else {
        return a;
    }
}

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
    return push_bytes((Buffer*)data, incoming_data, size);
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

    Buffer buffer = create_buffer(10);
    Writer writer = {
        .write = write_data,
        .data = &buffer
    };
    Serializer serializer = {
        .writer = &writer,
        .allocator = allocator
    };
    {
        Result result;
        serialize_uint8(&serializer, 2, &result);
        failed += expect_success(result);
    }
    failed += expect_uint8_eq(*(buffer.data + 0), 2);
    failed += expect_size_eq(buffer.size, 1);


    BufferReader buf_reader = read_buffer(&buffer);
    Reader reader = {
        .read = read_data,
        .data = &buf_reader  
    };
    Deserializer deserializer = {
        .allocator = allocator,
        .reader = &reader 
    };

    {
        Result result;
        failed += expect_uint8_eq(deserialize_uint8(&deserializer, &result), 2);
        failed += expect_success(result);
    }

    {
        Result result;
        failed += expect_uint8_eq(deserialize_uint8(&deserializer, &result), 0);
        failed += expect_status(result, Status_ReadFailed);
    }

    free_buffer(&buffer);

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