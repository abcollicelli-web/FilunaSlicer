#include "NativeWindowsPayloadHash.hpp"
#include <cassert>
#include <iostream>
#include <string>

using namespace Filuna::Update;

int main() {
    // CREATE_NEW prevents replacing an existing file. The test writes only its
    // own two names in the current scratch/CI directory, then removes them.
    const std::wstring path = L"filuna-hash-test-" + std::to_wstring(GetCurrentProcessId()) + L".bin";
    const std::wstring link = path + L".link";
    HANDLE file = CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
    assert(file != INVALID_HANDLE_VALUE);
    DWORD written = 0;
    assert(WriteFile(file, "abc", 3, &written, nullptr) && written == 3);
    assert(CloseHandle(file));
    const std::string digest = "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad";
    const char* error = nullptr;
    assert(!HashedWindowsPayload::open(path, 3, "not-a-hash", error));
    assert(std::string(error) == "INVALID_SHA256");
    assert(!HashedWindowsPayload::open(path, 4, digest, error));
    assert(std::string(error) == "PAYLOAD_SIZE_MISMATCH");
    assert(!HashedWindowsPayload::open(path, 3, std::string(64, '0'), error));
    assert(std::string(error) == "PAYLOAD_SHA256_MISMATCH");

    // An existing writer must prevent a successful proof.
    file = CreateFileW(path.c_str(), GENERIC_WRITE, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
    assert(file != INVALID_HANDLE_VALUE);
    assert(!HashedWindowsPayload::open(path, 3, digest, error));
    assert(CloseHandle(file));

    auto payload = HashedWindowsPayload::open(path, 3, digest, error);
    assert(payload && std::string(error) == "SHA256_ONLY_VERIFIED");
    file = CreateFileW(path.c_str(), GENERIC_WRITE, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
    assert(file == INVALID_HANDLE_VALUE); // proof remains bound to unchanged bytes
    assert(!DeleteFileW(path.c_str()));
    char bytes[3]{};
    DWORD count = 0;
    assert(ReadFile(payload->file_handle(), bytes, sizeof(bytes), &count, nullptr));
    assert(count == 3 && std::string(bytes, 3) == "abc"); // same verified handle, rewound
    payload.reset();

    assert(CreateHardLinkW(link.c_str(), path.c_str(), nullptr));
    assert(!HashedWindowsPayload::open(path, 3, digest, error));
    assert(std::string(error) == "PAYLOAD_NOT_REGULAR_SINGLE_LINK");
    assert(DeleteFileW(link.c_str()));
    // Cross the hashing buffer boundary with an independently known digest.
    file = CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
    assert(file != INVALID_HANDLE_VALUE);
    const std::string large(65537, 'a');
    assert(WriteFile(file, large.data(), static_cast<DWORD>(large.size()), &written, nullptr));
    assert(written == large.size());
    assert(CloseHandle(file));
    const std::string large_digest = "008ffc88d3c96a9f307524eb361e47c5222a887fc45fa0c1fb8d429c5c23b430";
    payload = HashedWindowsPayload::open(path, large.size(), large_digest, error);
    assert(payload);
    payload.reset();
    assert(!HashedWindowsPayload::open(path, large.size(), digest, error));
    assert(std::string(error) == "PAYLOAD_SHA256_MISMATCH");
    assert(DeleteFileW(path.c_str()));
    assert(!HashedWindowsPayload::open(path, 3, digest, error));
    std::cout << "NATIVE_WINDOWS_PAYLOAD_HASH_PASS\n";
}
