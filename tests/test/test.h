#pragma once

#include <stdint.h>
#include <stddef.h>
#include <setjmp.h>
#include <time.h>
#include "expect_tests.h"
#include "test_core.h"

#define ts_expect(condition) (void)_ts_expect_impl((condition), #condition, __FILE__, __LINE__)
#define ts_expect_int_eq(left, right) (void)_ts_expect_int_eq_impl((left), (right), #left, #right, __FILE__, __LINE__)
#define ts_expect_uint8_eq(left, right) (void)_ts_expect_uint8_eq_impl((left), (right), #left, #right, __FILE__, __LINE__)
#define ts_expect_bool_eq(left, right) (void)_ts_expect_bool_eq_impl((left), (right), #left, #right, __FILE__, __LINE__)
#define ts_expect_size_eq(left, right) (void)_ts_expect_size_eq_impl((left), (right), #left, #right, __FILE__, __LINE__)

#define ts_expect_success(result) (void)_ts_expect_success_impl((result), #result, __FILE__, __LINE__)
#define ts_expect_status(result, _status) (void)_ts_expect_status_eq_impl(((result).status), (_status), #result, #_status, __FILE__, __LINE__)

void _ts_start_test(const char* test_name);
void _ts_end_test();
static jmp_buf s_ts_assert_jmp_buf = {0};

#define TS_RUN_TEST(test, ...) do {\
    _ts_start_test(#test);\
    if (!setjmp(s_ts_assert_jmp_buf)) {\
        (void)(test)(__VA_ARGS__);\
    }\
    _ts_end_test();\
} while(0)

static void _ts_jump_back() {
    if (g_ts_tests_state.is_running_test) {
        longjmp(s_ts_assert_jmp_buf, 1);
    }
}

#define ts_assert(value) do {if (!_ts_expect_impl(value, #value, __FILE__, __LINE__)) {_ts_jump_back();}} while(0)


// public api
void ts_start_testing();
int ts_finish_testing();
