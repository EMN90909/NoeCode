#include "noe.hpp"

namespace noe {
bool LinkerDriver::link(const std::filesystem::path&, const std::filesystem::path&, Diagnostics& diagnostics) const {
    diagnostics.error("NOE-K5100", {}, "linker driver is not active until the native backend emits target objects");
    return false;
}
} // namespace noe
