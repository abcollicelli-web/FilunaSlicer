// The test binary links precisely the same filuna_oem_* static library targets as the application.
// Startup is a link/constructor check, NOT a T3-T9 PASS.
#include "IPrinterAdapter.hpp"
#include "PendingVendorAdapter.hpp"
#include <iostream>
#include <memory>
#include <string>
namespace filuna::oem {
std::unique_ptr<IPrinterAdapter> create_bambu(const std::string&);
std::unique_ptr<IPrinterAdapter> create_elegoo();
std::unique_ptr<IPrinterAdapter> create_flashforge();
std::unique_ptr<IPrinterAdapter> create_qidi();
std::unique_ptr<IPrinterAdapter> create_creality();
std::unique_ptr<IPrinterAdapter> create_snapmaker();
std::unique_ptr<IPrinterAdapter> create_prusa();
std::unique_ptr<IPrinterAdapter> create_anycubic();
}
int main(int argc, char **argv) {
  using namespace filuna::oem;
  if(argc<2 || std::string(argv[1])!="--list") { std::cerr<<"Usage: filuna_batch_test --list (native CMake linkage only)\n"; return 64; }
  const char *name[] = {"bambu","elegoo","flashforge","qidi","creality","snapmaker","prusa","anycubic"};
  std::unique_ptr<IPrinterAdapter> a[]={
    create_bambu(""),
    create_elegoo(),
    create_flashforge(),
    create_qidi(),
    create_creality(),
    create_snapmaker(),
    create_prusa(),
    create_anycubic()
  };
  bool pending = false;
  for(unsigned i=0;i<8;++i) {
    if(!a[i]) { std::cerr << name[i] << " ADAPTER_MISSING\\n"; return 1; }
    // RTTI-only readiness check: do not invoke a network-capable adapter method.
    if(dynamic_cast<PendingVendorAdapter*>(a[i].get()) != nullptr) {
      std::cout << name[i] << " PENDING (LINKED, NOT IMPLEMENTED)\\n";
      pending = true;
    } else {
      std::cout << name[i] << " LINKED (CONTRACT UNVERIFIED)\\n";
    }
  }
  // Do not allow a green laboratory result to be mistaken for OEM readiness.
  return pending ? 2 : 0;
}
