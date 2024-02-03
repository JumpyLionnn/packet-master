#include "packet_master.h"


int write(Writer* writer, uint8_t value) {
    printf("Writing data\n");
    writer->write(writer->data, &value, 1);
}

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

Result serialize_uint8(Serializer* serializer, uint8_t value) {
    int success = write(serializer->writer, value);
    if (success != 0) {
        return (Result){
            .status = Status_WriteFailed,
            .error_info.write_error = success
        };
    }
    return (Result){
        .status = Status_Success
    };
}