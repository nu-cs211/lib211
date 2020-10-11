#pragma once

bool rt211_test_log_check(
        bool condition,
        char const* file,
        int line);

void rt211_test_log_error(
        char const* context,
        char const* file,
        int line);
