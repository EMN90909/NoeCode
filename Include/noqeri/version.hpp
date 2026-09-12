#pragma once

namespace noe {

inline constexpr const char* NOQERI_LANGUAGE_VERSION = "1.5";
inline constexpr const char* NOQERI_COMPILER_VERSION = "1.5.0-ecosystem-foundation";
inline constexpr const char* NOQERI_EDITION = "2026";
inline constexpr const char* NOQERI_ABI_SERIES = "2";
inline constexpr const char* NOQERI_PACKAGE_FORMAT = "2";
inline constexpr const char* NOQERI_REGISTRY_PROTOCOL = "1";
inline constexpr const char* NOQERI_DEFAULT_TARGET = "x86_64-unknown-none";
inline constexpr const char* NOQERI_SUPPORTED_TARGET =
    "environment-neutral-frontend;target-adapters;x86_64-unknown-none;noqeri-abi-v2";

inline constexpr const char* NOE_LANGUAGE_VERSION = NOQERI_LANGUAGE_VERSION;
inline constexpr const char* NOE_COMPILER_VERSION = NOQERI_COMPILER_VERSION;
inline constexpr const char* NOE_SUPPORTED_TARGET = NOQERI_SUPPORTED_TARGET;

} // namespace noe
