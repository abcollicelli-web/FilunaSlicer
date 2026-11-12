#include "FilunaModuleStore.hpp"

#include <algorithm>
#include <cstdio>
#include <memory>

#include <boost/filesystem/fstream.hpp>
#include <boost/filesystem/operations.hpp>
#include <nlohmann/json.hpp>
#include <openssl/evp.h>

#include "libslic3r/miniz_extension.hpp"

namespace fs = boost::filesystem;

namespace Slic3r { namespace Filuna {

const char* const MODULE_CATALOG_KEY_ID         = "filuna-modulos-ed25519-1";
const char* const MODULE_CATALOG_PUBLIC_KEY_B64 = "VOBQtEh237hzipkTqjI/ibTfTMIIE7qNeqjiI3BqQ7E=";

namespace {

bool base64_decode(const std::string& in, std::string& out)
{
    std::string s;
    for (char c : in)
        if (c != '\n' && c != '\r' && c != ' ')
            s.push_back(c);
    if (s.empty() || s.size() % 4 != 0)
        return false;
    out.assign(s.size() / 4 * 3, '\0');
    const int n = EVP_DecodeBlock(reinterpret_cast<unsigned char*>(out.data()), reinterpret_cast<const unsigned char*>(s.data()),
                                  int(s.size()));
    if (n < 0)
        return false;
    // EVP_DecodeBlock conta o enchimento '=' como bytes zero
    size_t pad = 0;
    if (s.back() == '=') ++pad;
    if (s.size() > 1 && s[s.size() - 2] == '=') ++pad;
    out.resize(size_t(n) - pad);
    return true;
}

bool safe_entry(const std::string& name)
{
    if (name.empty() || name.front() == '/' || name.front() == '\\' || name.find(':') != std::string::npos)
        return false;
    fs::path p(name);
    return std::none_of(p.begin(), p.end(), [](const fs::path& part) { return part == ".."; });
}

} // namespace

bool verify_ed25519(const std::string& text, const std::string& signature_b64, const std::string& public_key_b64)
{
    std::string key, sig;
    if (!base64_decode(public_key_b64, key) || key.size() != 32 || !base64_decode(signature_b64, sig) || sig.size() != 64)
        return false;
    std::unique_ptr<EVP_PKEY, decltype(&EVP_PKEY_free)> pkey(
        EVP_PKEY_new_raw_public_key(EVP_PKEY_ED25519, nullptr, reinterpret_cast<const unsigned char*>(key.data()), key.size()),
        &EVP_PKEY_free);
    if (!pkey)
        return false;
    std::unique_ptr<EVP_MD_CTX, decltype(&EVP_MD_CTX_free)> ctx(EVP_MD_CTX_new(), &EVP_MD_CTX_free);
    if (!ctx || EVP_DigestVerifyInit(ctx.get(), nullptr, nullptr, nullptr, pkey.get()) != 1)
        return false;
    return EVP_DigestVerify(ctx.get(), reinterpret_cast<const unsigned char*>(sig.data()), sig.size(),
                            reinterpret_cast<const unsigned char*>(text.data()), text.size()) == 1;
}

std::string sha256_hex(const std::string& data)
{
    unsigned char md[EVP_MAX_MD_SIZE];
    unsigned int  len = 0;
    EVP_Digest(data.data(), data.size(), md, &len, EVP_sha256(), nullptr);
    static const char* hex = "0123456789abcdef";
    std::string        out;
    for (unsigned int i = 0; i < len; ++i) {
        out.push_back(hex[md[i] >> 4]);
        out.push_back(hex[md[i] & 15]);
    }
    return out;
}

std::optional<ModuleCatalog> parse_catalog_response(const std::string& body, std::string& error, const std::string& public_key_b64)
{
    nlohmann::json resp = nlohmann::json::parse(body, nullptr, false);
    if (resp.is_discarded() || !resp.is_object() || !resp.contains("catalogo") || !resp["catalogo"].is_string() ||
        !resp.contains("assinatura") || !resp["assinatura"].is_string()) {
        error = "resposta do catálogo de módulos sem catálogo ou assinatura";
        return std::nullopt;
    }
    const std::string text = resp["catalogo"].get<std::string>();
    if (!verify_ed25519(text, resp["assinatura"].get<std::string>(), public_key_b64.empty() ? MODULE_CATALOG_PUBLIC_KEY_B64 : public_key_b64)) {
        error = "assinatura do catálogo de módulos inválida";
        return std::nullopt;
    }
    nlohmann::json cat = nlohmann::json::parse(text, nullptr, false);
    if (cat.is_discarded() || !cat.is_object() || !cat.contains("modulos") || !cat["modulos"].is_array()) {
        error = "catálogo de módulos ilegível";
        return std::nullopt;
    }
    std::vector<std::string> visible;
    if (resp.contains("visiveis") && resp["visiveis"].is_array())
        for (const auto& v : resp["visiveis"])
            if (v.is_string())
                visible.push_back(v.get<std::string>());

    ModuleCatalog out;
    out.key_id = cat.value("chave", std::string());
    for (const auto& m : cat["modulos"]) {
        if (!m.is_object())
            continue;
        ModuleInfo info;
        info.id = m.value("id", std::string());
        if (std::find(visible.begin(), visible.end(), info.id) == visible.end())
            continue;
        info.name    = m.value("nome", std::string());
        info.version = m.value("versao", std::string());
        info.state   = m.value("estado", std::string());
        if (m.contains("componentes") && m["componentes"].is_array())
            for (const auto& c : m["componentes"]) {
                if (!c.is_object())
                    continue;
                ModuleComponent comp;
                comp.type   = c.value("tipo", std::string());
                comp.file   = c.value("arquivo", std::string());
                comp.sha256 = c.value("sha256", std::string());
                comp.url    = c.value("url", std::string());
                comp.bytes  = c.value("bytes", std::uint64_t(0));
                if (c.contains("impressoras") && c["impressoras"].is_array())
                    for (const auto& p : c["impressoras"])
                        if (p.is_string())
                            comp.printers.push_back(p.get<std::string>());
                info.components.push_back(std::move(comp));
            }
        out.modules.push_back(std::move(info));
    }
    return out;
}

const ModuleInfo* module_for_printer(const ModuleCatalog& catalog, const std::string& printer_name)
{
    for (const auto& m : catalog.modules)
        for (const auto& c : m.components)
            if (std::find(c.printers.begin(), c.printers.end(), printer_name) != c.printers.end())
                return &m;
    return nullptr;
}

bool install_component(const ModuleComponent& component, const std::string& package_bytes, const fs::path& dest_dir, std::string& error)
{
    if (sha256_hex(package_bytes) != component.sha256) {
        error = "pacote " + component.file + " com SHA-256 diferente do catálogo";
        return false;
    }
    mz_zip_archive zip;
    mz_zip_zero_struct(&zip);
    if (!mz_zip_reader_init_mem(&zip, package_bytes.data(), package_bytes.size(), 0)) {
        error = "pacote " + component.file + " não é um zip válido";
        return false;
    }
    const fs::path tmp = dest_dir.parent_path() / (dest_dir.filename().string() + ".parcial");
    boost::system::error_code ec;
    fs::remove_all(tmp, ec);
    bool ok = true;
    const mz_uint n = mz_zip_reader_get_num_files(&zip);
    for (mz_uint i = 0; i < n && ok; ++i) {
        mz_zip_archive_file_stat st;
        if (!mz_zip_reader_file_stat(&zip, i, &st)) {
            ok = false, error = "entrada ilegível no pacote";
            break;
        }
        const std::string name = st.m_filename;
        if (!safe_entry(name)) {
            ok = false, error = "pacote com caminho inseguro: " + name;
            break;
        }
        const fs::path target = tmp / fs::path(name);
        if (mz_zip_reader_is_file_a_directory(&zip, i)) {
            fs::create_directories(target, ec);
            continue;
        }
        fs::create_directories(target.parent_path(), ec);
        size_t size = 0;
        void*  data = mz_zip_reader_extract_to_heap(&zip, i, &size, 0);
        if (!data) {
            ok = false, error = "falha ao extrair " + name;
            break;
        }
        fs::ofstream f(target, std::ios::binary | std::ios::trunc);
        f.write(static_cast<const char*>(data), std::streamsize(size));
        mz_free(data);
        if (!f) {
            ok = false, error = "falha ao gravar " + target.string();
            break;
        }
    }
    mz_zip_reader_end(&zip);
    if (!ok) {
        fs::remove_all(tmp, ec);
        return false;
    }
    fs::remove_all(dest_dir, ec);
    fs::create_directories(dest_dir.parent_path(), ec);
    fs::rename(tmp, dest_dir, ec);
    if (ec) {
        error = "falha ao instalar em " + dest_dir.string() + ": " + ec.message();
        fs::remove_all(tmp, ec);
        return false;
    }
    return true;
}

ModuleInstallResult ensure_module_for_printer(const std::string& printer_name, const fs::path& root, const ModuleFetch& fetch,
                                              const std::string& public_key_b64)
{
    ModuleInstallResult r;
    std::string         body;
    if (!fetch("/api/slicer/v1/modulos", body, r.error))
        return r;
    auto catalog = parse_catalog_response(body, r.error, public_key_b64);
    if (!catalog)
        return r;
    const ModuleInfo* m = module_for_printer(*catalog, printer_name);
    if (!m) {
        r.error = "nenhum módulo disponível para a impressora " + printer_name;
        return r;
    }
    r.module_id = m->id;
    r.version   = m->version;
    for (const auto& c : m->components) {
        const fs::path dest  = root / m->id / m->version / c.type;
        const fs::path stamp = dest / ".filuna-sha256";
        boost::system::error_code ec;
        if (fs::exists(stamp, ec)) {
            fs::ifstream s(stamp);
            std::string   have;
            s >> have;
            if (have == c.sha256) {
                r.installed.push_back(c.type);
                continue;
            }
        }
        std::string pkg;
        if (!fetch(c.url, pkg, r.error))
            return r;
        if (!install_component(c, pkg, dest, r.error))
            return r;
        fs::ofstream(stamp) << c.sha256 << '\n';
        r.installed.push_back(c.type);
    }
    r.ok = true;
    return r;
}

}} // namespace Slic3r::Filuna
