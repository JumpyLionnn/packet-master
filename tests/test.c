#include "test.h"
#include <stdio.h>

#define expect_eq_impl(type, type_name, format)\
void expect_ ## type_name ## _eq_impl(type left, type right, const char* left_text, const char* right_text, const char* file, size_t line) {\
    if (left == right) {\
        g_tests_state.passed++;\
    }\
    else {\
        g_tests_state.failed++;\
        fprintf(stderr, "%s:%zu: Test failed, expected left to equal right:\n", file, line);\
        fprintf(stderr, "\tleft("format"): %s\n", left, left_text);\
        fprintf(stderr, "\tright("format"): %s\n", right, right_text);\
    }\
}

TestsState g_tests_state = {
    .failed = 0,
    .passed = 0
};

expect_eq_impl(int, int, "%i")
expect_eq_impl(uint8_t, uint8, "%hhu")

void expect_bool_eq_impl(bool left, bool right, const char* left_text, const char* right_text, const char* file, size_t line) {
    if (left == right) {
        g_tests_state.passed++;
    }
    else {
        fprintf(stderr, "%s:%zu: Test failed, expected left to equal right:\n", file, line);
        fprintf(stderr, "\tleft(%s): %s\n", left ? "true" : "right", left_text);
        fprintf(stderr, "\tright(%s): %s\n", right ? "true" : "right", right_text);
        g_tests_state.failed++;
    }
}

expect_eq_impl(size_t, size, "%zu")

void expect_success_impl(Result result, const char* result_text, const char* file, size_t line) {
    if (result.status == Status_Success) {
        g_tests_state.passed++;
    }
    else {
        fprintf(stderr, "%s:%zu: Test failed, Result was failure: %s in %s.\n", file, line, status_to_string(result.status), result_text);
        g_tests_state.failed++;
    }
}

void expect_status_impl(Result result, ResultStatus status, const char* result_text, const char* status_text, const char* file, size_t line) {
    if (result.status == status) {
        g_tests_state.passed++;
    }
    else {
        fprintf(stderr, "%s:%zu: Test failed, expected left to equal right:\n", file, line);
        fprintf(stderr, "\tleft(%s): %s\n", status_to_string(result.status), result_text);
        fprintf(stderr, "\tright(%s): %s\n", status_to_string(status), status_text);
        g_tests_state.failed++;
    }
}
