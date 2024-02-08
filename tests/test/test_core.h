#include <stddef.h>
#include <stdbool.h>

typedef struct TSTestsState {
    size_t failed;
    size_t total;
    bool is_success;
    bool is_running_test;
    const char* current_test_name;

    uint64_t start_time;
} TSTestsState;

extern TSTestsState g_ts_tests_state;