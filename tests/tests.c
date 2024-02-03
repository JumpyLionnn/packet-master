#include <stdlib.h>
#include <stdio.h>
#include <packet_master.h>
#include <string.h>

int expect_int_eq(int a, int b) {
    if (a == b) {
        return 0;
    }
    else {
        printf("Test failed: expected %i == %i.\n", a, b);
        return 1;
    }
}

int expect_uint8_eq(uint8_t a, uint8_t b) {
    if (a == b) {
        return 0;
    }
    else {
        printf("Test failed: expected %hhu == %hhu.\n", a, b);
        return 1;
    }
}

int expect_size_eq(size_t a, size_t b) {
    if (a == b) {
        return 0;
    }
    else {
        printf("Test failed: expected %u == %u.\n", (uint32_t)a, (uint32_t)b);
        return 1;
    }
}

int expect_success(Result result) {
    if (result.status == Status_Success) {
        return 0;
    }
    else {
        printf("Test failed: Result was failure: %s.\n", status_to_string(result.status));
        return 1;
    }
}


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
    uint8_t* data;
    size_t capacity;
    size_t size;
} Buffer;

Buffer create_buffer(size_t capacity) {
    return (Buffer) {
        .data = calloc(capacity, 1),
        .capacity = capacity,
        .size = 0
    };
}

void free_buffer(Buffer* buffer) {
    free(buffer->data);
}

int write_data(void* data, uint8_t* incoming_data, size_t data_size) {
    Buffer* buffer = data;
    if (data_size > buffer->capacity - buffer->size) {
        // expand
        buffer->capacity = min(buffer->capacity * 1.5, buffer->size + data_size);
        buffer->data = realloc(buffer->data, buffer->capacity);
        memset(buffer->data + buffer->size + data_size, 0, buffer->capacity - buffer->size - data_size);
    }
    memcpy(buffer->data + buffer->size, incoming_data, data_size);
    buffer->size += data_size;
    return 0;
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
    failed += expect_success(serialize_uint8(&serializer, 2));
    failed += expect_uint8_eq(*(buffer.data + 0), 2);
    failed += expect_size_eq(buffer.size, 1);
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