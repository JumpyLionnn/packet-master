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


#define BIT_MASK(start, count, type) ((type)~((type)(~(type)0) << (type)(count)) << (type)(start))

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

#define unimplemented() fprintf("%s:%u: Attempt to run code marked as unimplemented.\n", __FILE__, __LINE__)

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
    return (int)(size_t)move_res;
}


int buffer_push_byte(Buffer* buffer, uint8_t byte, Allocator* allocator) {
    return buffer_push_bytes(buffer, &byte, 1, allocator);
}

SerializerFreeBitsVector ser_create_free_bits_vector(size_t capacity, Allocator* allocator) {
    return (SerializerFreeBitsVector) {
        .capacity = capacity,
        .count = 0,
        .data = capacity == 0 ? NULL : allocator->alloc(capacity * sizeof(SerializerFreeBits))
    };
}

SerializerFreeBits* ser_free_bits_push(SerializerFreeBitsVector* vector, SerializerFreeBits value, Allocator* allocator) {
    if (vector->capacity <= vector->count) {
        vector->capacity = max(vector->capacity * 1.5, vector->count + 1);
        if (vector->data == NULL) {
            vector->data = allocator->alloc(vector->capacity);
            if (vector->data == NULL) {
                return NULL;
            }
        }
        else {
            SerializerFreeBits* data = allocator->realloc(vector->data, vector->capacity * sizeof(SerializerFreeBits));
            if (data == NULL) {
                return NULL;
            }
            vector->data = data;
        }
    }
    vector->data[vector->count] = value;
    return vector->data + (vector->count++);
}

int ser_free_bits_remove(SerializerFreeBitsVector* vector, size_t index) {
    assert(index < vector->count);
    if (index < vector->count - 1) {
        void* move_res = memmove(vector->data + index, vector->data + index + 1, vector->count - index - 1);
        return (int)(size_t)move_res;
    }
    vector->count--;
    return 0;
}

SerializerFreeBits* ser_free_bits_first(SerializerFreeBitsVector* vector) {
    if (vector->count > 0) {
        assert(vector->data != NULL && vector->capacity > 0);
        return vector->data;
    }
    return NULL;
}

void ser_free_bits_clear(SerializerFreeBitsVector* vector) {
    vector->count = 0;
}

void ser_free_bits_free(SerializerFreeBitsVector* vector, Allocator* allocator) {
    allocator->free(vector->data);
}


DeserializerFreeBitsVector deser_create_free_bits_vector(size_t capacity, Allocator* allocator) {
    return (DeserializerFreeBitsVector) {
        .capacity = capacity,
        .count = 0,
        .data = capacity == 0 ? NULL : allocator->alloc(capacity * sizeof(DeserializerFreeBits))
    };
}

DeserializerFreeBits* deser_free_bits_push(DeserializerFreeBitsVector* vector, DeserializerFreeBits value, Allocator* allocator) {
    if (vector->capacity <= vector->count) {
        vector->capacity = max(vector->capacity * 1.5, vector->count + 1);
        if (vector->data == NULL) {
            vector->data = allocator->alloc(vector->capacity);
            if (vector->data == NULL) {
                return NULL;
            }
        }
        else {
            DeserializerFreeBits* data = allocator->realloc(vector->data, vector->capacity * sizeof(DeserializerFreeBits));
            if (data == NULL) {
                return NULL;
            }
            vector->data = data;
        }
    }
    vector->data[vector->count] = value;
    return vector->data + (vector->count++);
}

int deser_free_bits_remove(DeserializerFreeBitsVector* vector, size_t index) {
    assert(index < vector->count);
    if (index < vector->count - 1) {
        void* move_res = memmove(vector->data + index, vector->data + index + 1, vector->count - index - 1);
        return (int)(size_t)move_res;
    }
    vector->count--;
    return 0;
}

DeserializerFreeBits* deser_free_bits_first(DeserializerFreeBitsVector* vector) {
    if (vector->count > 0) {
        assert(vector->data != NULL && vector->capacity > 0);
        return vector->data;
    }
    return NULL;
}

void deser_free_bits_free(DeserializerFreeBitsVector* vector, Allocator* allocator) {
    allocator->free(vector->data);
}




Serializer create_serializer(Writer* writer, Allocator allocator) {
    return (Serializer) {
        .writer = writer,
        .allocator = allocator,
        .free_bits = ser_create_free_bits_vector(0, &allocator)
    };
}

void free_serializer(Serializer* serializer) {
    free_buffer(&serializer->buffer, &serializer->allocator);
    ser_free_bits_free(&serializer->free_bits, &serializer->allocator);
}


Deserializer create_deserializer(Reader* reader, Allocator allocator) {
    return (Deserializer) {
        .reader = reader,
        .allocator = allocator
    };
}

void free_deserializer(Deserializer* deserializer) {
    deser_free_bits_free(&deserializer->free_bits, &deserializer->allocator);
}

SerializerFreeBits* find_free_bits(Serializer* serializer) {
   return ser_free_bits_first(&serializer->free_bits);
}

void serialize_uint8(Serializer* serializer, uint8_t value, Result* result) {
    serialize_uint8_max(serializer, value, 8, result);
}

uint8_t deserialize_uint8(Deserializer* deserializer, Result* result) {
    return deserialize_uint8_max(deserializer, 8, result);
}

void serialize_uint8_max(Serializer* serializer, uint8_t value, uint8_t max_bits, Result* result) {
    result->status = Status_Success;
    if (max_bits < 8) {
        SerializerFreeBits free_bits = {
            .end = 8,
            .start = max_bits,
            .index = serializer->buffer.size + serializer->start_index
        };
        SerializerFreeBits* free_bits_push_res = ser_free_bits_push(&serializer->free_bits, free_bits, &serializer->allocator);
        if (free_bits_push_res == NULL) {
            result->status = Status_MemoryAllocationFailed;
            return;
        }
    }
    int byte_push_res = buffer_push_byte(&serializer->buffer, value, &serializer->allocator);
    if (byte_push_res != 0) {
        result->status = Status_MemoryAllocationFailed;
        return;
    }
    flush_buffer(serializer, result);
    
}

uint8_t deserialize_uint8_max(Deserializer* deserializer, uint8_t max_bits, Result* result) {
    result->status = Status_Success;
    uint8_t* byte = read(deserializer->reader, 1);
    if (byte == NULL) {
        result->status = Status_ReadFailed;
        return 0;
    }
    if (max_bits < 8) {
        DeserializerFreeBits free_bits = {
            .byte = *byte,
            .start = max_bits,
            .end = 8
        };
        deser_free_bits_push(&deserializer->free_bits, free_bits, &deserializer->allocator);
    }
    uint8_t value = *byte & BIT_MASK(0, max_bits, uint8_t);
    return value;
}

void serializer_push_bit(Serializer* serializer, uint8_t value, Result* result);
void serialize_bool(Serializer* serializer, bool value, Result* result) {
    result->status = Status_Success;
    serializer_push_bit(serializer, (uint8_t)value, result);
}

uint8_t deserializer_read_bit(Deserializer* deserializer, Result* result);
bool deserialize_bool(Deserializer* deserializer, Result* result) {
    result->status = Status_Success;
    return (bool)deserializer_read_bit(deserializer, result);
}


void serializer_push_bit(Serializer* serializer, uint8_t value, Result* result) {
    SerializerFreeBits* free_bits = find_free_bits(serializer);
    if (free_bits == NULL) {
        SerializerFreeBits value = {
            .end = 8,
            .start = 0,
            .index = serializer->buffer.size + serializer->start_index
        };
        int pushed = buffer_push_byte(&serializer->buffer, 0, &serializer->allocator);
        free_bits = ser_free_bits_push(&serializer->free_bits, value, &serializer->allocator);
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
        if (ser_free_bits_remove(&serializer->free_bits, 0) != 0) {
            result->status = Status_MemoryOperationFailed;
            return;
        }
        flush_buffer(serializer, result);
    }
}

uint8_t deserializer_read_bit(Deserializer* deserializer, Result* result) {
    DeserializerFreeBits* free_bits = deser_free_bits_first(&deserializer->free_bits);
    if (free_bits == NULL) {
        uint8_t* byte = read(deserializer->reader, 1);
        free_bits = deser_free_bits_push(&deserializer->free_bits, (DeserializerFreeBits){
            .byte = *byte,
            .end = 8,
            .start = 0
        }, &deserializer->allocator);
        if (free_bits == NULL) {
            result->status = Status_MemoryAllocationFailed;
            return 0;
        }
    }
    uint8_t value = (free_bits->byte & BIT_MASK(free_bits->start, 1, uint8_t)) >> free_bits->start;
    free_bits->start++;
    if (free_bits->start >= free_bits->end) {
        int res = deser_free_bits_remove(&deserializer->free_bits, 0);
        if (res != 0) {
            result->status = Status_MemoryOperationFailed;
            return 0;
        }
    }
    return value;
}

void flush_buffer(Serializer* serializer, Result* result) {
    SerializerFreeBits* free_bits = ser_free_bits_first(&serializer->free_bits);
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
    result->status = Status_Success;
    ser_free_bits_clear(&serializer->free_bits);
    flush_buffer(serializer, result);
}