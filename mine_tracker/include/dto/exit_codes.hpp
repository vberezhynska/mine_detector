#pragma once

#include <cstdint>

namespace {
    enum class ExitCode : int8_t {
        Success = 0,
        FailedOnStartup = 1,
        RuntimeError = 1,
    };
} //namespace