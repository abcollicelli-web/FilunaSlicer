// Filuna — substituto para a régua de paridade (ver README.md). Nunca conecta.
#pragma once
#include <functional>
#include <memory>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace Slic3r { namespace GUI {

class QDSDevice;

class QIDIDeviceWebSocket
{
public:
    std::function<void(const std::string&, const std::string&)>    on_status_changed;
    std::function<void(const std::string&, const nlohmann::json&)> on_message_received;

    bool connect(const std::string&, std::shared_ptr<QDSDevice>) { return false; }
    void stopConnection(const std::string&) {}
    void sendSubscribeMessage(const std::string&) {}
    void getAllErrorList(const std::string&) {}
    void sendCommand(const std::string&, const std::string&) {}
    bool sendCommand(const std::string&, const std::string&, const std::string&, const std::string&) { return false; }
    bool sendCommand(const std::string&, const nlohmann::json&, const std::string&) { return false; }
    void sendActionCommand(const std::string&, const std::string&) {}
    std::vector<std::string> getConnectionIds() const { return {}; }
};

}} // namespace Slic3r::GUI
