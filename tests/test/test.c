#include "expect_tests.h"
#include "test_core.h"

#include <stdio.h>
#include <time.h>

#define TAB_CHARS "    "

#define expect_eq_impl(type, type_name, format)\
bool _ts_expect_ ## type_name ## _eq_impl(type left, type right, const char* left_text, const char* right_text, const char* file, size_t line) {\
    bool equal = left == right;\
    if (!equal) {\
        fail_test();\
        fprintf(stderr, TAB_CHARS"%s:%zu: Test failed, expected left to equal right:\n", file, line);\
        fprintf(stderr, TAB_CHARS TAB_CHARS"left("format"): %s\n", left, left_text);\
        fprintf(stderr, TAB_CHARS TAB_CHARS"right("format"): %s\n", right, right_text);\
    }\
    return equal;\
}

#define expect_eq_impl_format(type, type_name, format, format_fn)\
bool _ts_expect_ ## type_name ## _eq_impl(type left, type right, const char* left_text, const char* right_text, const char* file, size_t line) {\
    bool equal = left == right;\
    if (!equal) {\
        fail_test();\
        fprintf(stderr, TAB_CHARS"%s:%zu: Test failed, expected left to equal right:\n", file, line);\
        fprintf(stderr, TAB_CHARS TAB_CHARS"left("format"): %s\n", (format_fn)(left), left_text);\
        fprintf(stderr, TAB_CHARS TAB_CHARS"right("format"): %s\n", (format_fn)(right), right_text);\
    }\
    return equal;\
}

#define CONSOLE_ANSI_RESET "\033[0m"
#define CONSOLE_ANSI_FG_GREEN "\033[32m"
#define CONSOLE_ANSI_FG_RED "\033[91m"

#define COLORED_OK CONSOLE_ANSI_FG_GREEN"ok"CONSOLE_ANSI_RESET
#define COLORED_FAILED CONSOLE_ANSI_FG_RED"FAILED"CONSOLE_ANSI_RESET



TSTestsState g_ts_tests_state = {
    .total = 0,
    .failed = 0,
    .is_running_test = false,
    .is_success = false,
    .current_test_name = NULL
};

void fail_test() {
    if (g_ts_tests_state.is_success) {
        g_ts_tests_state.failed++;
        printf("test %s ... %s\n", g_ts_tests_state.current_test_name, COLORED_FAILED);
        g_ts_tests_state.is_success = false;
    }
}

bool _ts_expect_impl(bool value, const char* text, const char* file, size_t line) {
    if (!value) {
        fail_test();
        fprintf(stderr, TAB_CHARS"%s:%zu: Test failed, %s was false.\n", file, line, text);
    }
    return value;
}

expect_eq_impl(int, int, "%i")
expect_eq_impl(uint8_t, uint8, "%hhu")

const char* bool_to_str(bool value) {
    if (value) {
        return "true";
    }
    else {
        return "false";
    }
}


expect_eq_impl_format(bool, bool, "%s", bool_to_str)

expect_eq_impl(size_t, size, "%zu")

bool _ts_expect_success_impl(Result result, const char* result_text, const char* file, size_t line) {
    bool equal = result.status == Status_Success;
    if (!equal) {
        fail_test();
        fprintf(stderr, TAB_CHARS"%s:%zu: Test failed, Result was failure: %s in %s.\n", file, line, status_to_string(result.status), result_text);
    }
    return equal;
}

expect_eq_impl_format(ResultStatus, status, "%s", status_to_string)



void _ts_start_test(const char* test_name) {
    g_ts_tests_state.total++;
    g_ts_tests_state.is_running_test = true;
    g_ts_tests_state.is_success = true;
    g_ts_tests_state.current_test_name = test_name;
}

void _ts_end_test() {
    if (g_ts_tests_state.is_success) {
        printf("test %s ... %s\n", g_ts_tests_state.current_test_name, COLORED_OK);
    }
    g_ts_tests_state.is_running_test = false;
}

double timespecDiff(struct timespec *timeA_p, struct timespec *timeB_p)
{
  return ((double)(((timeA_p->tv_sec * 1000000000) + timeA_p->tv_nsec) -
           ((timeB_p->tv_sec * 1000000000) + timeB_p->tv_nsec))) / 1000000000;
}

void ts_start_testing() {
    clock_gettime(CLOCK_MONOTONIC, &g_ts_tests_state.start_time);
    printf("Running tests...\n");
}
int ts_finish_testing() {
    struct timespec end_time;
    clock_gettime(CLOCK_MONOTONIC, &end_time);
    double time_taken = timespecDiff(&end_time, &g_ts_tests_state.start_time);
    printf("\ntest result: %s. %zu passed; %zu failed; finished in %.3lfs\n", g_ts_tests_state.failed == 0 ? COLORED_OK : COLORED_FAILED,  g_ts_tests_state.total - g_ts_tests_state.failed, g_ts_tests_state.failed, time_taken);
    if (g_ts_tests_state.failed == 0) {
        return EXIT_SUCCESS;
    }
    else {
        return EXIT_FAILURE;
    }
}