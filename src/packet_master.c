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