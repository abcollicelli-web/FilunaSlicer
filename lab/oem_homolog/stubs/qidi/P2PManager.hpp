// Filuna — substituto para a régua de paridade (ver README.md). Nunca conecta.
#pragma once
#include <atomic>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace Slic3r { namespace GUI {

class P2PManager
{
public:
    struct ConnectParams
    {
        std::string              client_id, server, device_id, user_token;
        std::vector<std::string> relay_list;
    };
    struct FileTransferOptions
    {
        std::atomic<bool>*                        cancel = nullptr;
        std::function<void(int64_t, int64_t)>     progress;
    };
    struct FileTransferResult
    {
        bool        ok        = false;
        bool        cancelled = false;
        std::string error;
    };

    static P2PManager& instance() { static P2PManager m; return m; }

    bool isInited() const { return false; }
    bool isConnected() const { return false; }
    void disconnect() {}
    void shutdown() {}
    void triggerReconnect() {}
    bool ensureConnection(const ConnectParams&, std::function<void(bool)>) { return false; }
    FileTransferResult uploadFile(const std::string&, const std::string&, const FileTransferOptions&) { return {}; }
};

}} // namespace Slic3r::GUI
