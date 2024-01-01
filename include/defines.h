#pragma once

#include <stdint.h>

#define DEBUG
#define PATH_PREFIX "../"

#define GLM_FORCE_RADIANS

#define ASSERT(line, err, err_code) if (line) {     \
    printf(err);                                    \
    return err_code;                                \
}

#define ENSURE(line, err_code) if (line) {          \
    return err_code;                                \
}

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef i8 Err;
