// Direct LAN implementation using the official Elegoo Link SDK.
// This tool runs beside Filuna on Windows; errors are non-zero exit codes.
// It does not silently substitute the system-installed slicer.
#include <elegoo_link.h>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace {
bool good(const elink::ELINK_ERROR_CODE code) {
    return code == elink::ELINK_ERROR_CODE::SUCCESS;
}
void usage() {
    std::cerr << "Usage:\n  filuna-cc2-sdk discover [cache.tsv]\n"
                 "  filuna-cc2-sdk send <printer-id> <ip> <access-code> <gcode> [canvas:tray,...]\n";
}
int discover(elink::ElegooLink& sdk, const std::string& cache) {
    elink::PrinterDiscoveryParams params;
    params.timeoutMs = 8000;
    params.broadcastInterval = 2000;
    params.enableAutoRetry = true;
    const auto found = sdk.startPrinterDiscovery(params);
    if (!found.isSuccess() || !found.hasValue()) {
        std::cerr << "CC2_DISCOVERY_FAILED: " << found.message << '\n';
        return 3;
    }
    std::ofstream stored;
    if (!cache.empty()) {
        stored.open(cache, std::ios::trunc);
        if (!stored) {
            std::cerr << "CC2_CACHE_WRITE_FAILED: " << cache << '\n';
            return 4;
        }
    }
    unsigned count = 0;
    for (const auto& p : found.value().printers) {
        if (p.printerType != elink::PrinterType::ELEGOO_FDM_CC2)
            continue;
        // TSV is consumed by the native device integration (no secret stored).
        const std::string line = p.printerId + "\t" + p.host + "\t" + p.model + "\t" + p.serialNumber;
        std::cout << line << '\n';
        if (stored.is_open())
            stored << line << '\n';
        ++count;
    }
    std::cerr << "CC2_DISCOVERY_COUNT: " << count << '\n';
    return 0;
}
int send(elink::ElegooLink& sdk, int argc, char** argv) {
    if (argc < 6) { usage(); return 2; }
    const std::filesystem::path gcode = std::filesystem::u8path(argv[5]);
    if (!std::filesystem::is_regular_file(gcode) || std::filesystem::file_size(gcode) == 0) {
        std::cerr << "CC2_GCODE_MISSING_OR_EMPTY: " << gcode.string() << '\n';
        return 5;
    }
    elink::ConnectPrinterParams connect;
    connect.printerId = argv[2];
    connect.host = argv[3];
    connect.accessCode = argv[4];
    connect.authMode = "accessCode";
    connect.printerType = elink::PrinterType::ELEGOO_FDM_CC2;
    connect.brand = "Elegoo";
    connect.model = "Elegoo Centauri Carbon 2";
    connect.name = connect.model;
    connect.networkMode = elink::NetworkMode::LAN;
    connect.autoReconnect = true;
    connect.checkConnection = true;
    const auto connected = sdk.connectPrinter(connect);
    if (!connected.isSuccess() || !connected.hasValue() || !connected.value().isConnected) {
        std::cerr << "CC2_CONNECT_FAILED: " << connected.message << '\n';
        return 6;
    }
    const std::string id = connected.value().printerInfo.printerId;
    elink::FileUploadParams file;
    file.printerId = id;
    file.storageLocation = "local";
    file.localFilePath = gcode.u8string();
    file.fileName = gcode.filename().u8string();
    file.overwriteExisting = true;
    const auto uploaded = sdk.uploadFile(file, [](const elink::FileUploadProgressData& progress) {
        std::cerr << "CC2_UPLOAD: " << progress.uploadedBytes << "/" << progress.totalBytes << '\n';
        return true;
    });
    if (!uploaded.isSuccess()) {
        std::cerr << "CC2_UPLOAD_FAILED: " << uploaded.message << '\n';
        return 7;
    }
    std::cout << "CC2_UPLOAD_OK: " << file.fileName << '\n';
    elink::StartPrintParams task;
    task.printerId = id;
    task.storageLocation = "local";
    task.fileName = file.fileName;
    task.autoBedLeveling = false;
    task.enableTimeLapse = false;
    if (argc > 6) {
        // Map G-code T index to CANVAS and tray. Format: t:canvas:tray,t:canvas:tray
        std::string mappings = argv[6];
        std::size_t from = 0;
        try {
            while (from < mappings.size()) {
                auto to = mappings.find(',', from);
                const auto entry = mappings.substr(from, to == std::string::npos ? to : to - from);
                auto first = entry.find(':');
                auto second = entry.find(':', first == std::string::npos ? first : first + 1);
                if (first == std::string::npos || second == std::string::npos)
                    throw std::invalid_argument("invalid slot map");
                elink::SlotMapItem slot{};
                slot.t = std::stoi(entry.substr(0, first));
                slot.canvasId = std::stoi(entry.substr(first + 1, second - first - 1));
                slot.trayId = std::stoi(entry.substr(second + 1));
                task.slotMap.push_back(slot);
                if (to == std::string::npos) break;
                from = to + 1;
            }
        } catch (const std::exception& e) {
            std::cerr << "CC2_CANVAS_MAP_INVALID: " << e.what() << '\n';
            return 8;
        }
    }
    const auto started = sdk.startPrint(task);
    if (!started.isSuccess()) {
        std::cerr << "CC2_START_FAILED: " << started.message << '\n';
        return 9;
    }
    std::cout << "CC2_START_OK: " << task.fileName << '\n';
    return 0;
}
}
int main(int argc, char** argv) {
    if (argc < 2) { usage(); return 2; }
    elink::ElegooLink& sdk = elink::ElegooLink::getInstance();
    elink::ElegooLink::Config config;
    config.log.logLevel = 2;
    config.log.logEnableConsole = false;
    config.log.logEnableFile = false;
    if (!sdk.initialize(config)) {
        std::cerr << "CC2_SDK_INIT_FAILED\n";
        return 1;
    }
    int result = 2;
    const std::string command = argv[1];
    if (command == "discover")
        result = discover(sdk, argc > 2 ? argv[2] : "");
    else if (command == "send")
        result = send(sdk, argc, argv);
    else usage();
    sdk.cleanup();
    return result;
}
