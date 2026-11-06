#pragma once
#include "IPrinterAdapter.hpp"
namespace filuna::oem {
class PendingVendorAdapter : public IPrinterAdapter {
  std::string brand_;
public:
  explicit PendingVendorAdapter(std::string brand) : brand_(std::move(brand)) {}
  std::vector<Device> discover() override { return {}; }
  Outcome pair(const Device&, const std::string&) override { return {false,false,brand_+": native pairing not implemented"}; }
  Outcome status_stream() override { return {false,false,brand_+": status not implemented"}; }
  Outcome send(const std::string&) override { return {false,false,brand_+": native transfer not implemented"}; }
  Outcome start(const std::string&) override { return {false,false,brand_+": start not implemented"}; }
  Outcome pause() override { return {false,false,brand_+": pause not implemented"}; }
  Outcome resume() override { return {false,false,brand_+": resume not implemented"}; }
  Outcome stop() override { return {false,false,brand_+": stop not implemented"}; }
  Outcome camera() override { return {false,false,brand_+": camera not implemented"}; }
  Outcome materials() override { return {false,false,brand_+": materials not implemented"}; }
};
}
