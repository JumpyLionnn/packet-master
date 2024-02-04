#include "test.h"
#include <stdio.h>

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
int expect_status(Result result, ResultStatus status) {
    if (result.status == status) {
        return 0;
    }
    else {
        printf("Test failed: Expected result to be '%s' but got '%s'.\n", status_to_string(status), status_to_string(result.status));
        return 1;
    }
}
