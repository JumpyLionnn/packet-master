#pragma once

#include <stdint.h>
#include <stddef.h>
#include "packet_master.h"

int expect_int_eq(int a, int b);

int expect_uint8_eq(uint8_t a, uint8_t b);

int expect_size_eq(size_t a, size_t b);

int expect_success(Result result);
int expect_status(Result result, ResultStatus status);