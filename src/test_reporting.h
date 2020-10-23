#pragma once

bool rt211_test_log_check(
        bool condition,
        char const* file,
        int line);

void rt211_test_log_error(
        char const* const file,
        int         const line,
        char const* const context,
        char const* const message);

void rt211_test_log_perror(
        char const* const file,
        int         const line,
        char const* const context);
