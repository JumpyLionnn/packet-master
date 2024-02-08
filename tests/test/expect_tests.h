#include <stdint.h>
#include <stddef.h>
#include "packet_master.h"


bool _ts_expect_impl(bool value, const char* text, const char* file, size_t line);
bool _ts_expect_int_eq_impl(int left, int right, const char* left_text, const char* right_text, const char* file, size_t line);
bool _ts_expect_uint8_eq_impl(uint8_t left, uint8_t right, const char* left_text, const char* right_text, const char* file, size_t line);
bool _ts_expect_uint16_eq_impl(uint16_t left, uint16_t right, const char* left_text, const char* right_text, const char* file, size_t line);
bool _ts_expect_uint32_eq_impl(uint32_t left, uint32_t right, const char* left_text, const char* right_text, const char* file, size_t line);
bool _ts_expect_bool_eq_impl(bool left, bool right, const char* left_text, const char* right_text, const char* file, size_t line);
bool _ts_expect_size_eq_impl(size_t left, size_t right, const char* left_text, const char* right_text, const char* file, size_t line);

bool _ts_expect_success_impl(Result result, const char* result_text, const char* file, size_t line);
bool _ts_expect_status_eq_impl(ResultStatus a, ResultStatus b, const char* a_text, const char* b_text, const char* file, size_t line);