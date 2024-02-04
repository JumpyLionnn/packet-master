#include "packet_master.h"
#include <string.h>

int write(Writer* writer, uint8_t* value, size_t size) {
    return writer->write(writer->data, value, size);
}

int write_byte(Writer* writer, uint8_t value) {
    return writer->write(writer->data, &value, 1);
}

uint8_t* read(Reader* reader, size_t size) {
    return reader->read(reader->data, size);
}


#define BIT_MASK(start, count, type) (~((type)(~(type)0) << (type)(count)) << (type)(start))

#define ENABLE_ASSERT
#ifdef ENABLE_ASSERT
#include <stdio.h>
void assert_impl(bool value, const char* expression, size_t line, const char* file) {
    if (!value) {
        fprintf(stderr, "%s:%u: Assertion failed: %s.\n", file, (uint32_t)line, expression);
    }
}
#define assert(condition) assert_impl((condition), #condition, __LINE__, __FILE__)
#else
#define assert(condition)
#endif

const char* status_to_string(ResultStatus status) {
    switch (status)
    {
    case Status_Success:
        return "success";
    case Status_MemoryAllocationFailed:
        return "memory_allocation_failed";
    case Status_MemoryOperationFailed:
        return "memory_operation_failed";
    case Status_WriteFailed:
        return "write_failed";
    case Status_ReadFailed:
        return "read_failed";
    default:
        return "unknown";
    }  
}

Buffer create_buffer(size_t capacity, Allocator* allocator) {
    return (Buffer) {
        .data = allocator->alloc(capacity),
        .capacity = capacity,
        .size = 0
    };
}

void free_buffer(Buffer* buffer, Allocator* allocator) {
    allocator->free(buffer->data);
}

size_t min(size_t a, size_t b) {
    if (a > b) {
        return b;
    }
    else {
        return a;
    }
}

size_t max(size_t a, size_t b) {
    if (a > b) {
        return a;
    }
    else {
        return b;
    }
}

int buffer_push_bytes(Buffer* buffer, uint8_t* data, size_t size, Allocator* allocator) {
    assert(buffer->size <= buffer->capacity);
    // printf("size: %u, capacity: %u", (uint32_t));
    if (size > buffer->capacity - buffer->size) {
        // expand
        buffer->capacity = max(buffer->capacity * 1.5, buffer->size + size);
        buffer->data = allocator->realloc(buffer->data, buffer->capacity);
        assert(buffer->data != NULL);
        memset(buffer->data + buffer->size + size, 0, buffer->capacity - buffer->size - size);
    }
    memcpy(buffer->data + buffer->size, data, size);
    buffer->size += size;
    return 0;
}
void buffer_clear(Buffer* buffer) {
    buffer->size = 0;
}

int buffer_remove(Buffer* buffer, size_t start, size_t count) {
    assert(start + count < buffer->size);
    size_t end = start + count;
    void* move_res = memmove(buffer->data + start, buffer->data + end, buffer->size - end);
    buffer->size -= count;
    return (size_t)move_res > 0 ? 1 : 0;
}


int buffer_push_byte(Buffer* buffer, uint8_t byte, Allocator* allocator) {
    return buffer_push_bytes(buffer, &byte, 1, allocator);
}

FreeBitsVector create_free_bits_vector(size_t capacity, Allocator* allocator) {
    return (FreeBitsVector) {
        .capacity = capacity,
        .count = 0,
        .data = capacity == 0 ? NULL : allocator->alloc(capacity * sizeof(FreeBits))
    };
}

FreeBits* vector_push(FreeBitsVector* vector, FreeBits value, Allocator* allocator) {
    if (vector->capacity <= vector->count) {
        vector->capacity = max(vector->capacity * 1.5, vector->count + 1);
        if (vector->data == NULL) {
            vector->data = allocator->alloc(vector->capacity);
            if (vector->data == NULL) {
                return NULL;
            }
        }
        else {
            FreeBits* data = allocator->realloc(vector->data, vector->capacity * sizeof(FreeBits));
            if (data == NULL) {
                return NULL;
            }
            vector->data = data;
        }
    }
    vector->data[vector->count] = value;
    return vector->data + (vector->count++);
}

int vector_remove(FreeBitsVector* vector, size_t index) {
    assert(index < vector->count);
    void* move_res = memmove(vector->data + index, vector->data + index + 1, vector->count - index - 1);
    vector->count--;
    return (size_t)move_res > 0 ? 1 : 0;
}

inline FreeBits* vector_first(FreeBitsVector* vector) {
    if (vector->count > 0) {
        assert(vector->data != NULL && vector->capacity > 0);
        return vector->data;
    }
    return NULL;
}

void vector_clear(FreeBitsVector* vector) {
    vector->count = 0;
}

void free_vector(FreeBitsVector* vector, Allocator* allocator) {
    allocator->free(vector->data);
}

Serializer create_serializer(Writer* writer, Allocator allocator) {
    return (Serializer) {
        .writer = writer,
        .allocator = allocator
    };
}

void free_serializer(Serializer* serializer) {
    free_buffer(&serializer->buffer, &serializer->allocator);
    free_vector(&serializer->free_bits, &serializer->allocator);
}


Deserializer create_deserializer(Reader* reader, Allocator allocator) {
    return (Deserializer) {
        .reader = reader,
        .allocator = allocator
    };
}

void free_deserializer(Deserializer* deserializer) {
}

FreeBits* find_free_bits(Serializer* serializer) {
   return vector_first(&serializer->free_bits);
}

void serialize_uint8(Serializer* serializer, uint8_t value, Result* result) {
    int push_res = buffer_push_byte(&serializer->buffer, value, &serializer->allocator);
    if (push_res != 0) {
        result->status = Status_MemoryAllocationFailed;
        return;
    }
    flush_buffer(serializer, result);
    if (result->status != Status_Success) {
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

void serialize_bool(Serializer* serializer, bool value, Result* result) {
    FreeBits* free_bits = find_free_bits(serializer);
    if (free_bits == NULL) {
        FreeBits value = {
            .end = 8,
            .start = 0,
            .index = serializer->buffer.size + serializer->start_index
        };
        int pushed = buffer_push_byte(&serializer->buffer, 0, &serializer->allocator);
        free_bits = vector_push(&serializer->free_bits, value, &serializer->allocator);
        if (free_bits == NULL || pushed != 0) {
            result->status = Status_MemoryAllocationFailed;
            return;
        }
    }
    uint8_t mask = BIT_MASK(free_bits->start, value, uint8_t);
    size_t byte_index = free_bits->index - serializer->start_index;
    serializer->buffer.data[byte_index] |= mask;
    free_bits->start++;
    // if this byte is full then we can remove it and flush the buffer until the next free bits index
    if (free_bits->start >= free_bits->end) {
        if (vector_remove(&serializer->free_bits, 0) != 0) {
            result->status = Status_MemoryOperationFailed;
            return;
        }
        flush_buffer(serializer, result);
    }
}


void flush_buffer(Serializer* serializer, Result* result) {
    FreeBits* free_bits = vector_first(&serializer->free_bits);
    if (free_bits == NULL) {
        // flushing the entire buffer
        int write_result = write(serializer->writer, serializer->buffer.data, serializer->buffer.size);
        if (write_result != 0) {
            result->status = Status_WriteFailed;
            result->error_info.write_error = write_result;
        }
        serializer->start_index += serializer->buffer.size;
        buffer_clear(&serializer->buffer);
    }
    else {
        // flushing until the free_bits index
        size_t count = free_bits->index - serializer->start_index;
        if (count > 0) {
            int write_result = write(serializer->writer, serializer->buffer.data, count);
            if (write_result != 0) {
                result->status = Status_WriteFailed;
                result->error_info.write_error = write_result;
            }
            if (buffer_remove(&serializer->buffer, 0, count) != 0) {
                result->status = Status_MemoryOperationFailed;
            }
            serializer->start_index = free_bits->index;
        }
    }
}


void serializer_finalize(Serializer* serializer, Result* result) {
    vector_clear(&serializer->free_bits);
    flush_buffer(serializer, result);
}