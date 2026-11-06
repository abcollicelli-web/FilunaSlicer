#pragma once
#include <map>
#include <string>
#include <vector>
#include <memory>

// Experimental C++ Filuna OEM contract. Same compiled adapter is used by
// the test executable and may be linked by the Filuna app after code review.
// No hardware printers: tests are restricted to the 10.77.0.0/24 lab.
namespace filuna::oem {
struct Device { std::string ip, serial, model, name; };
struct Outcome { bool ok=false; bool attempted=false; std::string evidence; };
class IPrinterAdapter {
public:
 virtual ~IPrinterAdapter()=default;
 virtual std::vector<Device> discover()=0;
 virtual Outcome pair(const Device&,const std::string& access_code)=0;
 virtual Outcome status_stream()=0;
 virtual Outcome send(const std::string& path)=0;
 virtual Outcome start(const std::string& file)=0;
 virtual Outcome pause()=0;
 virtual Outcome resume()=0;
 virtual Outcome stop()=0;
 virtual Outcome camera()=0;
 virtual Outcome materials()=0;
};
std::unique_ptr<IPrinterAdapter> create_bambu(const std::string& ca_file);
}
