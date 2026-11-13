#include <catch2/catch_test_macros.hpp>

#include <map>
#include <memory>
#include <string>

#include <boost/filesystem/fstream.hpp>
#include <boost/filesystem/operations.hpp>
#include <openssl/evp.h>

#include "libslic3r/miniz_extension.hpp"
#include "slic3r/Utils/FilunaModuleStore.hpp"

namespace fs = boost::filesystem;
using namespace Slic3r::Filuna;

namespace {

std::string b64(const std::string& raw)
{
    std::string out(4 * ((raw.size() + 2) / 3), '\0');
    EVP_EncodeBlock(reinterpret_cast<unsigned char*>(out.data()), reinterpret_cast<const unsigned char*>(raw.data()), int(raw.size()));
    return out;
}

struct Key
{
    std::unique_ptr<EVP_PKEY, decltype(&EVP_PKEY_free)> pkey{nullptr, &EVP_PKEY_free};
    Key()
    {
        std::unique_ptr<EVP_PKEY_CTX, decltype(&EVP_PKEY_CTX_free)> ctx(EVP_PKEY_CTX_new_id(EVP_PKEY_ED25519, nullptr), &EVP_PKEY_CTX_free);
        EVP_PKEY* k = nullptr;
        EVP_PKEY_keygen_init(ctx.get());
        EVP_PKEY_keygen(ctx.get(), &k);
        pkey.reset(k);
    }
    std::string public_b64() const
    {
        std::string raw(32, '\0');
        size_t      n = raw.size();
        EVP_PKEY_get_raw_public_key(pkey.get(), reinterpret_cast<unsigned char*>(raw.data()), &n);
        return b64(raw);
    }
    std::string sign_b64(const std::string& text) const
    {
        std::unique_ptr<EVP_MD_CTX, decltype(&EVP_MD_CTX_free)> ctx(EVP_MD_CTX_new(), &EVP_MD_CTX_free);
        EVP_DigestSignInit(ctx.get(), nullptr, nullptr, nullptr, pkey.get());
        std::string sig(64, '\0');
        size_t      n = sig.size();
        EVP_DigestSign(ctx.get(), reinterpret_cast<unsigned char*>(sig.data()), &n, reinterpret_cast<const unsigned char*>(text.data()),
                       text.size());
        return b64(sig);
    }
};

std::string make_zip(const std::map<std::string, std::string>& files)
{
    mz_zip_archive zip;
    mz_zip_zero_struct(&zip);
    mz_zip_writer_init_heap(&zip, 0, 0);
    for (const auto& [name, data] : files)
        mz_zip_writer_add_mem(&zip, name.c_str(), data.data(), data.size(), MZ_DEFAULT_COMPRESSION);
    void*  buf  = nullptr;
    size_t size = 0;
    mz_zip_writer_finalize_heap_archive(&zip, &buf, &size);
    std::string out(static_cast<const char*>(buf), size);
    mz_zip_writer_end(&zip);
    return out;
}

std::string catalog_text(const std::string& pkg_sha)
{
    return R"J({"schema":1,"chave":"teste","modulos":[)J"
           R"J({"id":"snapmaker","nome":"Snapmaker","versao":"2.4.1","estado":"HOMOLOGADO_INTERNO","componentes":[)J"
           R"J({"tipo":"profiles","arquivo":"perfis-snapmaker-2.4.1.zip","sha256":")J" + pkg_sha +
           R"J(","bytes":1,"url":"/api/slicer/v1/modulos/snapmaker/2.4.1/perfis-snapmaker-2.4.1.zip","impressoras":["Snapmaker U1 (0.4 nozzle)"]}]},)J"
           R"J({"id":"bambu","nome":"Bambu Lab","versao":"02.09.01.70","estado":"EM_TESTE","componentes":[]}]})J";
}

std::string response(const Key& key, const std::string& text, const std::string& visible = R"(["snapmaker","bambu"])")
{
    std::string escaped;
    for (char c : text) {
        if (c == '"' || c == '\\')
            escaped.push_back('\\');
        escaped.push_back(c);
    }
    return R"({"catalogo":")" + escaped + R"(","assinatura":")" + key.sign_b64(text) + R"(","visiveis":)" + visible + "}";
}

fs::path temp_root()
{
    fs::path p = fs::temp_directory_path() / fs::unique_path("filuna-modulos-%%%%-%%%%");
    fs::create_directories(p);
    return p;
}

} // namespace

TEST_CASE("The module catalog is accepted only with a valid signature over its exact text", "[FilunaModuleStore]")
{
    Key               key;
    const std::string text = catalog_text(std::string(64, 'a'));
    std::string       err;

    auto ok = parse_catalog_response(response(key, text), err, key.public_b64());
    REQUIRE(ok);
    CHECK(ok->modules.size() == 2);

    std::string body = response(key, text);
    body.replace(body.find("2.4.1"), 5, "9.9.9"); // texto trocado, assinatura antiga
    CHECK_FALSE(parse_catalog_response(body, err, key.public_b64()));
    CHECK(err.find("assinatura") != std::string::npos);

    Key other;
    CHECK_FALSE(parse_catalog_response(response(key, text), err, other.public_b64()));
}

TEST_CASE("Only modules the account may see are kept, and the printer picks its module", "[FilunaModuleStore]")
{
    Key         key;
    std::string err;
    auto        cat = parse_catalog_response(response(key, catalog_text(std::string(64, 'a')), R"(["snapmaker"])"), err, key.public_b64());
    REQUIRE(cat);
    REQUIRE(cat->modules.size() == 1);
    const ModuleInfo* m = module_for_printer(*cat, "Snapmaker U1 (0.4 nozzle)");
    REQUIRE(m);
    CHECK(m->id == "snapmaker");
    CHECK(module_for_printer(*cat, "Bambu Lab X1 Carbon 0.4 nozzle") == nullptr);
}

TEST_CASE("A package is installed only when its SHA-256 matches and its paths stay inside the module", "[FilunaModuleStore]")
{
    const fs::path root = temp_root();
    std::string    err;

    const std::string good = make_zip({{"profiles/Snapmaker.json", "{}"}, {"profiles/Snapmaker/machine/U1.json", "{\"type\":\"machine\"}"}});
    ModuleComponent   c{"profiles", "p.zip", sha256_hex(good), "/x", good.size(), {}};
    REQUIRE(install_component(c, good, root / "snapmaker/2.4.1/profiles", err));
    CHECK(fs::exists(root / "snapmaker/2.4.1/profiles/profiles/Snapmaker/machine/U1.json"));

    ModuleComponent wrong = c;
    wrong.sha256          = std::string(64, '0');
    CHECK_FALSE(install_component(wrong, good, root / "other", err));
    CHECK_FALSE(fs::exists(root / "other"));

    const std::string evil = make_zip({{"../fora.txt", "x"}});
    ModuleComponent   e{"profiles", "e.zip", sha256_hex(evil), "/x", evil.size(), {}};
    CHECK_FALSE(install_component(e, evil, root / "evil", err));
    CHECK_FALSE(fs::exists(root / "fora.txt"));
    fs::remove_all(root);
}

TEST_CASE("Choosing a printer downloads its module once and reuses the installed copy", "[FilunaModuleStore]")
{
    Key               key;
    const fs::path    root = temp_root();
    const std::string pkg  = make_zip({{"profiles/Snapmaker.json", "{\"name\":\"Snapmaker\"}"}});
    const std::string body = response(key, catalog_text(sha256_hex(pkg)));
    int               package_fetches = 0;
    ModuleFetch       fetch = [&](const std::string& path, std::string& out, std::string& error) {
        if (path == "/api/slicer/v1/modulos") {
            out = body;
            return true;
        }
        if (path == "/api/slicer/v1/modulos/snapmaker/2.4.1/perfis-snapmaker-2.4.1.zip") {
            ++package_fetches;
            out = pkg;
            return true;
        }
        error = "404 " + path;
        return false;
    };

    auto r = ensure_module_for_printer("Snapmaker U1 (0.4 nozzle)", root, fetch, key.public_b64());
    REQUIRE(r.ok);
    CHECK(r.module_id == "snapmaker");
    CHECK(fs::exists(root / "snapmaker/2.4.1/profiles/profiles/Snapmaker.json"));
    CHECK(package_fetches == 1);

    r = ensure_module_for_printer("Snapmaker U1 (0.4 nozzle)", root, fetch, key.public_b64());
    REQUIRE(r.ok);
    CHECK(package_fetches == 1);

    r = ensure_module_for_printer("Impressora que não existe", root, fetch, key.public_b64());
    CHECK_FALSE(r.ok);
    fs::remove_all(root);
}
