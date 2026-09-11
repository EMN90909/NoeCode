#include "noe.hpp"

namespace noe {
bool NativeBackend::emitAssembly(const NirProgram&, const std::filesystem::path&, Diagnostics& diagnostics) const {
    diagnostics.error(
        "NOE-N5000", {},
        "native machine-code emission is not enabled in bootstrap phase 3/8",
        "use 'noe run' for the NIR interpreter or 'noe nir' to inspect lowered code; the custom native backend is the next implementation milestone"
    );
    return false;
}
} // namespace noe
