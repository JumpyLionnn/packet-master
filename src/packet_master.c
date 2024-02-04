#include "packet_master.h"

int write(Writer* writer, uint8_t value) {
    return writer->write(writer->data, &value, 1);
}

uint8_t* read(Reader* reader, size_t size) {
    return reader->read(reader->data, size);
}


#define BIT_MASK(start, count) (~((~0) >> (count)) >> (start))

const char* status_to_string(ResultStatus status) {
    switch (status)
    {
    case Status_Success:
        return "success";
    case Status_MemoryAllocationFailed:
        return "memory_allocation_failed";
    case Status_WriteFailed:
        return "write_failed";
    default:
        return "unknown";
    }  
}

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

int push_bytes(Buffer* buffer, uint8_t* data, size_t size) {
    if (size > buffer->capacity - buffer->size) {
        // expand
        buffer->capacity = min(buffer->capacity * 1.5, buffer->size + size);
        buffer->data = realloc(buffer->data, buffer->capacity);
        memset(buffer->data + buffer->size + size, 0, buffer->capacity - buffer->size - size);
    }
    memcpy(buffer->data + buffer->size, data, size);
    buffer->size += size;
    return 0;
}

Serializer create_serializer(Writer* writer, Allocator allocator) {
    return (Serializer) {
        .writer = writer,
        .allocator = allocator
    };
}
Deserializer create_deserializer(Reader* reader, Allocator allocator) {
    return (Deserializer) {
        .reader = reader,
        .allocator = allocator
    };
}

void serialize_uint8(Serializer* serializer, uint8_t value, Result* result) {
    int success = write(serializer->writer, value);
    if (success != 0) {
        result->status = Status_WriteFailed;
        result->error_info.write_error = success;
        return;
    }
    result->status = Status_Success;
    return;
}

uint8_t deserialize_uint8(Deserializer* deserializer, Result* result) {
    uint8_t* value = read(deserializer->reader, 1);
    if (value == NULL) {
        result->status = Status_ReadFailed;
        return 0;
    }
    result->status = Status_Success;
    return *value;
}