// Filuna batch laboratory placeholder: native OEM SDK linkage NOT YET IMPLEMENTED.
// Do not count the compilation of this file as a contract test PASS.
#include "../../PendingVendorAdapter.hpp"
#include <memory>
namespace filuna::oem {
std::unique_ptr<IPrinterAdapter> create_creality() { return std::make_unique<PendingVendorAdapter>("creality"); }
}
