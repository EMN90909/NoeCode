#include "noe.hpp"
#include "abi.h"

#include <chrono>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace noe {
namespace {
thread_local std::string lastAbiError;

void setError(noqeri_abi_error* error, int code, const std::string& message) {
    lastAbiError = message;
    if (error) {
        error->code = code;
        error->message = lastAbiError.c_str();
    }
}

std::string abiValueToString(const noqeri_abi_value& value) {
    switch (value.tag) {
        case NOQERI_ABI_NULL: return "null";
        case NOQERI_ABI_BOOL: return value.as.boolean ? "true" : "false";
        case NOQERI_ABI_INT: return std::to_string(value.as.integer);
        case NOQERI_ABI_FLOAT: return std::to_string(value.as.floating);
        case NOQERI_ABI_STRING:
            return value.as.string.data ? std::string(value.as.string.data, value.as.string.size) : std::string{};
        default: return "<invalid>";
    }
}

int32_t builtinPrint(void*, const noqeri_abi_value* args, size_t argc, noqeri_abi_value* result, noqeri_abi_error*) {
    for (size_t i = 0; i < argc; ++i) {
        if (i) std::cout << ' ';
        std::cout << abiValueToString(args[i]);
    }
    std::cout << '\n';
    if (result) result->tag = NOQERI_ABI_NULL;
    return 0;
}

int32_t builtinClockMillis(void*, const noqeri_abi_value*, size_t argc, noqeri_abi_value* result, noqeri_abi_error* error) {
    if (argc != 0) { setError(error, 2, "clockMillis expects no arguments"); return 2; }
    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    result->tag = NOQERI_ABI_INT;
    result->as.integer = static_cast<int64_t>(now);
    return 0;
}

int32_t builtinPlatform(void*, const noqeri_abi_value*, size_t argc, noqeri_abi_value* result, noqeri_abi_error* error) {
    if (argc != 0) { setError(error, 2, "platform expects no arguments"); return 2; }
#if defined(_WIN32)
    static constexpr char text[] = "windows";
#elif defined(__APPLE__)
    static constexpr char text[] = "macos";
#elif defined(__linux__)
    static constexpr char text[] = "linux";
#else
    static constexpr char text[] = "unknown";
#endif
    result->tag = NOQERI_ABI_STRING;
    result->as.string = {text, sizeof(text) - 1};
    return 0;
}

int32_t builtinTextLength(void*, const noqeri_abi_value* args, size_t argc, noqeri_abi_value* result, noqeri_abi_error* error) {
    if (argc != 1 || args[0].tag != NOQERI_ABI_STRING) {
        setError(error, 2, "textLength expects one string argument");
        return 2;
    }
    result->tag = NOQERI_ABI_INT;
    result->as.integer = static_cast<int64_t>(args[0].as.string.size);
    return 0;
}

const noqeri_abi_function_entry builtinEntries[] = {
    {"print", builtinPrint, nullptr},
    {"clockMillis", builtinClockMillis, nullptr},
    {"platform", builtinPlatform, nullptr},
    {"textLength", builtinTextLength, nullptr},
};

const noqeri_host_api builtinApi = {
    NOQERI_ABI_VERSION,
    sizeof(builtinEntries) / sizeof(builtinEntries[0]),
    builtinEntries
};

noqeri_abi_value toAbi(const NirValue& value) {
    noqeri_abi_value out{};
    if (std::holds_alternative<std::monostate>(value)) out.tag = NOQERI_ABI_NULL;
    else if (auto v = std::get_if<bool>(&value)) { out.tag = NOQERI_ABI_BOOL; out.as.boolean = *v ? 1 : 0; }
    else if (auto v = std::get_if<std::int64_t>(&value)) { out.tag = NOQERI_ABI_INT; out.as.integer = *v; }
    else if (auto v = std::get_if<double>(&value)) { out.tag = NOQERI_ABI_FLOAT; out.as.floating = *v; }
    else if (auto v = std::get_if<std::string>(&value)) {
        out.tag = NOQERI_ABI_STRING;
        out.as.string = {v->data(), v->size()};
    }
    return out;
}

NirValue fromAbi(const noqeri_abi_value& value) {
    switch (value.tag) {
        case NOQERI_ABI_NULL: return std::monostate{};
        case NOQERI_ABI_BOOL: return value.as.boolean != 0;
        case NOQERI_ABI_INT: return static_cast<std::int64_t>(value.as.integer);
        case NOQERI_ABI_FLOAT: return value.as.floating;
        case NOQERI_ABI_STRING:
            return value.as.string.data ? std::string(value.as.string.data, value.as.string.size) : std::string{};
        default: throw std::runtime_error("host returned invalid ABI value tag");
    }
}
}

const noqeri_host_api* defaultHostApi() { return &builtinApi; }

std::optional<NirValue> callAbiFunction(const noqeri_host_api* host, const std::string& name,
                                        const std::vector<NirValue>& args, std::string& error) {
    if (!host) return std::nullopt;
    if (host->abi_version != NOQERI_ABI_VERSION) {
        error = "host ABI version mismatch";
        return std::nullopt;
    }
    const noqeri_abi_function_entry* entry = nullptr;
    for (size_t i = 0; i < host->function_count; ++i) {
        if (host->functions[i].name && name == host->functions[i].name) {
            entry = &host->functions[i];
            break;
        }
    }
    if (!entry) return std::nullopt;

    std::vector<noqeri_abi_value> converted;
    converted.reserve(args.size());
    for (const auto& value : args) converted.push_back(toAbi(value));
    noqeri_abi_value result{};
    noqeri_abi_error abiError{};
    int32_t status = entry->invoke(entry->user_data, converted.data(), converted.size(), &result, &abiError);
    if (status != 0) {
        error = abiError.message ? abiError.message : "host function failed";
        return std::nullopt;
    }
    return fromAbi(result);
}

} // namespace noe

extern "C" uint32_t noqeri_abi_version(void) { return NOQERI_ABI_VERSION; }

extern "C" int32_t noqeri_run_source(const char* source, const noqeri_host_api* host, noqeri_abi_error* error) {
    if (!source) {
        if (error) { error->code = 1; error->message = "source must not be null"; }
        return 1;
    }
    auto result = noe::compileSource(source);
    if (result.diagnostics.hasErrors()) {
        if (error) { error->code = 2; error->message = "source failed to compile"; }
        return 2;
    }
    int status = noe::Interpreter(host).run(result.nir);
    if (status != 0 && error) { error->code = 3; error->message = "runtime execution failed"; }
    return status == 0 ? 0 : 3;
}
