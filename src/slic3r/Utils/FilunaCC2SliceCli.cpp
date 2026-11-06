// Filuna's bundled ElegooSlicer 1.5.3.5 Windows CLI bridge.
// Operates only on a freshly exported, profiled 3MF. The external system
// Elegoo/Orca installation is never searched or invoked.
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>
#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif

#ifdef _WIN32
static std::wstring quote_arg(const std::wstring& input)
{
    std::wstring output = L"\"";
    std::size_t slashes = 0;
    for (const wchar_t ch : input) {
        if (ch == L'\\') { ++slashes; continue; }
        if (ch == L'"') {
            output.append(slashes * 2 + 1, L'\\');
            output.push_back(ch);
        } else {
            output.append(slashes, L'\\');
            output.push_back(ch);
        }
        slashes = 0;
    }
    output.append(slashes * 2, L'\\');
    output += L'"';
    return output;
}
#endif

int main(int argc, char** argv)
{
#ifndef _WIN32
    std::cerr << "CC2_SLICE_WINDOWS_ONLY\n";
    return 16;
#else
    namespace fs = std::filesystem;
    if (argc != 3 && argc != 4) {
        std::cerr << "Usage: filuna-cc2-slice <profiled-project.3mf> <output-directory> [bundled-portable-exe]\n";
        return 2;
    }
    const fs::path project = fs::u8path(argv[1]);
    const fs::path output = fs::u8path(argv[2]);
    if (!fs::is_regular_file(project) || project.extension() != L".3mf") {
        std::cerr << "CC2_SLICE_INVALID_PROJECT: expected profiled 3mf\n";
        return 10;
    }
    fs::path portable;
    if (argc == 4) {
        portable = fs::u8path(argv[3]);
    } else {
        wchar_t module[MAX_PATH]{};
        const DWORD size = GetModuleFileNameW(nullptr, module, MAX_PATH);
        if (size == 0 || size >= MAX_PATH) {
            std::cerr << "CC2_SLICE_GET_MODULE_FAILED: " << GetLastError() << '\n';
            return 11;
        }
        portable = fs::path(module).parent_path() / "engines" / "elegoo-v1.5.3.5" / "elegoo-slicer.exe";
    }
    if (!fs::is_regular_file(portable)) {
        std::cerr << "CC2_SLICE_PORTABLE_NOT_BUNDLED: " << portable.u8string() << '\n';
        return 12;
    }
    fs::create_directories(output);
    const fs::path gcode = output / "plate_1.gcode";
    // A stale G-code is never accepted as a successful fresh slice.
    std::error_code ec;
    fs::remove(gcode, ec);
    if (ec) {
        std::cerr << "CC2_SLICE_STALE_OUTPUT_REMOVE_FAILED: " << ec.message() << '\n';
        return 13;
    }
    // Identical core CLI arguments to the prior official CC2 cube run:
    // --slice 0 --outputdir <dir> <profiled input.3mf>
    std::wstring command = quote_arg(portable.wstring())
        + L" --debug 3 --slice 0 --outputdir " + quote_arg(output.wstring())
        + L" " + quote_arg(project.wstring());
    std::vector<wchar_t> command_line(command.begin(), command.end());
    command_line.push_back(L'\0');
    STARTUPINFOW startup{};
    startup.cb = sizeof(startup);
    PROCESS_INFORMATION process{};
    if (!CreateProcessW(portable.c_str(), command_line.data(), nullptr, nullptr, FALSE,
                        CREATE_NO_WINDOW, nullptr, portable.parent_path().c_str(), &startup, &process)) {
        std::cerr << "CC2_SLICE_LAUNCH_FAILED: " << GetLastError() << '\n';
        return 14;
    }
    WaitForSingleObject(process.hProcess, INFINITE);
    DWORD code = 0;
    const bool got_code = GetExitCodeProcess(process.hProcess, &code) != 0;
    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);
    if (!got_code || code != 0) {
        std::cerr << "CC2_SLICE_EXIT_FAILED: " << code << '\n';
        return 15;
    }
    if (!fs::is_regular_file(gcode) || fs::file_size(gcode) == 0) {
        std::cerr << "CC2_SLICE_GCODE_MISSING: " << gcode.u8string() << '\n';
        return 17;
    }
    std::cout << "CC2_SLICE_OK: " << gcode.u8string() << '\n';
    return 0;
#endif
}
