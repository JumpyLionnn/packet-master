#pragma once

#include <stdint.h>
#include <stddef.h>
#include "packet_master.h"

void expect_int_eq_impl(int left, int right, const char* left_text, const char* right_text, const char* file, size_t line);
void expect_uint8_eq_impl(uint8_t left, uint8_t right, const char* left_text, const char* right_text, const char* file, size_t line);
void expect_bool_eq_impl(bool left, bool right, const char* left_text, const char* right_text, const char* file, size_t line);
void expect_size_eq_impl(size_t left, size_t right, const char* left_text, const char* right_text, const char* file, size_t line);

#define expect_int_eq(left, right) expect_int_eq_impl((left), (right), #left, #right, __FILE__, __LINE__)
#define expect_uint8_eq(left, right) expect_uint8_eq_impl((left), (right), #left, #right, __FILE__, __LINE__)
#define expect_bool_eq(left, right) expect_bool_eq_impl((left), (right), #left, #right, __FILE__, __LINE__)
#define expect_size_eq(left, right) expect_size_eq_impl((left), (right), #left, #right, __FILE__, __LINE__)


void expect_success_impl(Result result, const char* result_text, const char* file, size_t line);
void expect_status_impl(Result result, ResultStatus status, const char* result_text, const char* status_text, const char* file, size_t line);

#define expect_success(result) expect_success_impl((result), #result, __FILE__, __LINE__)
#define expect_status(result, status) expect_status_impl((result), (status), #result, #status, __FILE__, __LINE__)

typedef struct TestsState {
    size_t failed;
    size_t passed;
} TestsState;

extern TestsState g_tests_state;