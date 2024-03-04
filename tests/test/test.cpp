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



TSTestsState g_ts_tests_state = {};

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
expect_eq_impl(uint16_t, uint16, "%hu")
expect_eq_impl(uint32_t, uint32, "%u")

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

// double timespecDiff(struct timespec *timeA_p, struct timespec *timeB_p)
// {
//   return ((double)(((timeA_p->tv_sec * 1'000'000'000) + timeA_p->tv_nsec) -
//            ((timeB_p->tv_sec * 1'000'000'000) + timeB_p->tv_nsec))) / 1'000'000'000;
// }

#ifdef _WIN32
#include <Windows.h>
#else
#include <sys/time.h>
//#include <ctime>
#endif

// Returns the amount of milliseconds elapsed since the UNIX epoch
uint64_t get_time_ms()
{
#ifdef _WIN32
    /* Windows */
    FILETIME ft;
    LARGE_INTEGER li;

    /* Get the amount of 100 nano seconds intervals elapsed since January 1, 1601 (UTC) and copy it
     * to a LARGE_INTEGER structure. */
    GetSystemTimeAsFileTime(&ft);
    li.LowPart = ft.dwLowDateTime;
    li.HighPart = ft.dwHighDateTime;

    uint64_t ret = li.QuadPart;
    ret -= 116444736000000000LL; /* Convert from file time to UNIX epoch time. */
    ret /= 10000; /* From 100 nano seconds (10^-7) to 1 millisecond (10^-3) intervals */

    return ret;
#else
    struct timeval tv;

    gettimeofday(&tv, NULL);

    uint64_t ret = tv.tv_usec;
    /* Convert from micro seconds (10^-6) to milliseconds (10^-3) */
    ret /= 1000;

    /* Adds the seconds (10^0) after converting them to milliseconds (10^-3) */
    ret += (tv.tv_sec * 1000);

    return ret;
#endif
}

void ts_start_testing() {
    //clock_gettime(CLOCK_MONOTONIC, &g_ts_tests_state.start_time);
    g_ts_tests_state.start_time = get_time_ms();
    printf("Running tests...\n");
}
int ts_finish_testing() {
    // struct timespec end_time;
    //clock_gettime(CLOCK_MONOTONIC, &end_time);
    // double time_taken = timespecDiff(&end_time, &g_ts_tests_state.start_time);
    double time_taken = (double)(get_time_ms() - g_ts_tests_state.start_time) / 1000;
    printf("\ntest result: %s. %zu passed; %zu failed; finished in %.3lfs\n", g_ts_tests_state.failed == 0 ? COLORED_OK : COLORED_FAILED,  g_ts_tests_state.total - g_ts_tests_state.failed, g_ts_tests_state.failed, time_taken);
    if (g_ts_tests_state.failed == 0) {
        return EXIT_SUCCESS;
    }
    else {
        return EXIT_FAILURE;
    }
}