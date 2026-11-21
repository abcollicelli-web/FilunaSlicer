#pragma once
#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <bcrypt.h>
#include <cstdint>
#include <memory>
#include <string>

#ifdef _MSC_VER
#pragma comment(lib, "bcrypt.lib")
#endif

namespace Filuna { namespace Update {

// Owns the exact file handle whose bytes were hashed. This is only a digest
// check, not signature, provenance, authorization or safe-path verification.
// The caller supplies a path inside its trusted staging directory and must
// keep this object alive and consume this handle, rather than reopen the path.
class HashedWindowsPayload {
public:
    ~HashedWindowsPayload() { CloseHandle(file_); }
    HashedWindowsPayload(const HashedWindowsPayload&) = delete;
    HashedWindowsPayload& operator=(const HashedWindowsPayload&) = delete;
    HANDLE file_handle() const { return file_; }

    static std::unique_ptr<HashedWindowsPayload> open(
        const std::wstring& staging_path, std::uint64_t expected_size,
        const std::string& expected_sha256, const char*& error) {
        error = "INVALID_SHA256";
        if (expected_sha256.size() != 64)
            return nullptr;
        for (char c : expected_sha256)
            if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f')))
                return nullptr;

        // Deny writes/deletion while held; an already-open writer also makes
        // this open fail. Do not follow a reparse point at the final component.
        HANDLE file = CreateFileW(staging_path.c_str(), GENERIC_READ, FILE_SHARE_READ,
            nullptr, OPEN_EXISTING, FILE_FLAG_OPEN_REPARSE_POINT | FILE_FLAG_SEQUENTIAL_SCAN, nullptr);
        if (file == INVALID_HANDLE_VALUE) {
            error = "PAYLOAD_OPEN_FAILED";
            return nullptr;
        }
        std::unique_ptr<HashedWindowsPayload> payload(new HashedWindowsPayload(file));
        BY_HANDLE_FILE_INFORMATION info{};
        if (GetFileType(file) != FILE_TYPE_DISK || !GetFileInformationByHandle(file, &info) ||
            (info.dwFileAttributes & (FILE_ATTRIBUTE_REPARSE_POINT | FILE_ATTRIBUTE_DIRECTORY)) ||
            info.nNumberOfLinks != 1) {
            error = "PAYLOAD_NOT_REGULAR_SINGLE_LINK";
            return nullptr;
        }
        const std::uint64_t size = (std::uint64_t(info.nFileSizeHigh) << 32) | info.nFileSizeLow;
        if (size != expected_size) {
            error = "PAYLOAD_SIZE_MISMATCH";
            return nullptr;
        }
        Algorithm algorithm;
        Hash hash;
        if (BCryptOpenAlgorithmProvider(&algorithm.handle, BCRYPT_SHA256_ALGORITHM, nullptr, 0) < 0 ||
            BCryptCreateHash(algorithm.handle, &hash.handle, nullptr, 0, nullptr, 0, 0) < 0) {
            error = "SHA256_PROVIDER_FAILED";
            return nullptr;
        }
        unsigned char chunk[64 * 1024];
        std::uint64_t total = 0;
        for (;;) {
            DWORD count = 0;
            if (!ReadFile(file, chunk, sizeof(chunk), &count, nullptr)) {
                error = "PAYLOAD_READ_FAILED";
                return nullptr;
            }
            if (count == 0)
                break;
            if (total > expected_size || count > expected_size - total) {
                error = "PAYLOAD_SIZE_MISMATCH";
                return nullptr;
            }
            total += count;
            if (BCryptHashData(hash.handle, chunk, count, 0) < 0) {
                error = "SHA256_HASH_FAILED";
                return nullptr;
            }
        }
        unsigned char digest[32];
        if (total != expected_size || BCryptFinishHash(hash.handle, digest, sizeof(digest), 0) < 0) {
            error = "SHA256_FINISH_FAILED";
            return nullptr;
        }
        static constexpr char hex[] = "0123456789abcdef";
        std::string actual;
        actual.reserve(64);
        for (unsigned char byte : digest) {
            actual.push_back(hex[byte >> 4]);
            actual.push_back(hex[byte & 15]);
        }
        if (actual != expected_sha256) {
            error = "PAYLOAD_SHA256_MISMATCH";
            return nullptr;
        }
        LARGE_INTEGER beginning{};
        if (!SetFilePointerEx(file, beginning, nullptr, FILE_BEGIN)) {
            error = "PAYLOAD_REWIND_FAILED";
            return nullptr;
        }
        error = "SHA256_ONLY_VERIFIED";
        return payload;
    }

private:
    struct Algorithm {
        BCRYPT_ALG_HANDLE handle = nullptr;
        ~Algorithm() { if (handle) BCryptCloseAlgorithmProvider(handle, 0); }
    };
    struct Hash {
        BCRYPT_HASH_HANDLE handle = nullptr;
        ~Hash() { if (handle) BCryptDestroyHash(handle); }
    };
    explicit HashedWindowsPayload(HANDLE file) : file_(file) {}
    HANDLE file_;
};

}} // namespace Filuna::Update
#endif
