#pragma once
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
#define NOQERI_ABI_VERSION 2u

typedef enum noqeri_abi_tag {
    NOQERI_ABI_NULL = 0,
    NOQERI_ABI_BOOL = 1,
    NOQERI_ABI_INT = 2,
    NOQERI_ABI_FLOAT = 3,
    NOQERI_ABI_STRING = 4
} noqeri_abi_tag;

typedef struct noqeri_abi_string { const char* data; size_t size; } noqeri_abi_string;
typedef struct noqeri_abi_value {
    uint32_t tag;
    union { int32_t boolean; int64_t integer; double floating; noqeri_abi_string string; } as;
} noqeri_abi_value;
typedef struct noqeri_abi_error { int32_t code; const char* message; } noqeri_abi_error;

typedef int32_t (*noqeri_abi_write_fn)(void* context, const char* data, size_t size);
typedef int64_t (*noqeri_abi_clock_millis_fn)(void* context);
typedef noqeri_abi_string (*noqeri_abi_platform_name_fn)(void* context);
typedef int32_t (*noqeri_abi_service_fn)(void* context, const noqeri_abi_value* args, size_t argc,
                                         noqeri_abi_value* result, noqeri_abi_error* error);

typedef struct noqeri_abi_service {
    const char* name;
    noqeri_abi_service_fn invoke;
    void* context;
} noqeri_abi_service;

typedef struct noqeri_abi {
    uint32_t abi_version;
    uint32_t struct_size;
    void* context;
    noqeri_abi_write_fn write;
    noqeri_abi_clock_millis_fn clock_millis;
    noqeri_abi_platform_name_fn platform_name;
    size_t service_count;
    const noqeri_abi_service* services;
} noqeri_abi;

uint32_t noqeri_abi_version(void);
int32_t noqeri_run_source(const char* source, const noqeri_abi* abi, noqeri_abi_error* error);
#ifdef __cplusplus
}
#endif
