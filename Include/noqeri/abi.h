#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define NOQERI_ABI_VERSION 1u

typedef enum noqeri_abi_tag {
    NOQERI_ABI_NULL = 0,
    NOQERI_ABI_BOOL = 1,
    NOQERI_ABI_INT = 2,
    NOQERI_ABI_FLOAT = 3,
    NOQERI_ABI_STRING = 4
} noqeri_abi_tag;

typedef struct noqeri_abi_string {
    const char* data;
    size_t size;
} noqeri_abi_string;

typedef struct noqeri_abi_value {
    uint32_t tag;
    union {
        int32_t boolean;
        int64_t integer;
        double floating;
        noqeri_abi_string string;
    } as;
} noqeri_abi_value;

typedef struct noqeri_abi_error {
    int32_t code;
    const char* message;
} noqeri_abi_error;

typedef int32_t (*noqeri_abi_function)(
    void* user_data,
    const noqeri_abi_value* args,
    size_t argc,
    noqeri_abi_value* result,
    noqeri_abi_error* error);

typedef struct noqeri_abi_function_entry {
    const char* name;
    noqeri_abi_function invoke;
    void* user_data;
} noqeri_abi_function_entry;

typedef struct noqeri_host_api {
    uint32_t abi_version;
    size_t function_count;
    const noqeri_abi_function_entry* functions;
} noqeri_host_api;

uint32_t noqeri_abi_version(void);
int32_t noqeri_run_source(const char* source, const noqeri_host_api* host, noqeri_abi_error* error);

#ifdef __cplusplus
}
#endif
