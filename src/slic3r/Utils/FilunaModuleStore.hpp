#pragma once

// Filuna — módulos de fabricante baixados sob demanda (REGRA-CANONICA-MODULOS-FABRICANTE-SOB-DEMANDA).
//
// O servidor guarda, por marca, o motor, os perfis, a rede e o sistema de cores. O Slicer pede o
// catálogo, confere a assinatura Ed25519 sobre o texto exato do catálogo com a chave pública
// embutida, escolhe o módulo da marca da impressora escolhida pelo cliente e baixa só ele,
// conferindo o SHA-256 de cada pacote antes de extrair.
//
// Esta camada não depende da interface: a busca HTTP entra como função, para os testes e para a
// GUI chamarem do jeito delas.

#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <vector>

#include <boost/filesystem/path.hpp>

namespace Slic3r { namespace Filuna {

// Chave pública do catálogo (FILUNA/fabricantes/chave-modulos-ed25519-1.pub), Ed25519 bruta em base64.
extern const char* const MODULE_CATALOG_KEY_ID;
extern const char* const MODULE_CATALOG_PUBLIC_KEY_B64;

struct ModuleComponent
{
    std::string              type;     // "profiles", "engine", "network"...
    std::string              file;     // nome do pacote
    std::string              sha256;   // hex minúsculo
    std::string              url;      // caminho relativo no site
    std::uint64_t            bytes = 0;
    std::vector<std::string> printers; // impressoras que o pacote cobre
};

struct ModuleInfo
{
    std::string                  id;      // "snapmaker", "bambu"...
    std::string                  name;
    std::string                  version;
    std::string                  state;   // "HOMOLOGADO_INTERNO" ou "EM_TESTE"
    std::vector<ModuleComponent> components;
};

struct ModuleCatalog
{
    std::vector<ModuleInfo>  modules; // somente os módulos visíveis para a conta
    std::string              key_id;
};

// Confere a assinatura Ed25519 (base64) de 'text' com a chave pública bruta (base64).
bool verify_ed25519(const std::string& text, const std::string& signature_b64, const std::string& public_key_b64);

std::string sha256_hex(const std::string& data);

// Lê a resposta de GET /api/slicer/v1/modulos: {"catalogo": "<texto>", "assinatura": "...", "visiveis": [...]}.
// Recusa catálogo sem assinatura válida. 'public_key_b64' vazio usa a chave embutida.
std::optional<ModuleCatalog> parse_catalog_response(const std::string& body, std::string& error,
                                                    const std::string& public_key_b64 = {});

// Módulo e componente que cobrem a impressora escolhida (nome do preset de máquina do sistema).
const ModuleInfo* module_for_printer(const ModuleCatalog& catalog, const std::string& printer_name);

// Confere o SHA-256 do pacote e extrai o .zip em 'dest_dir' (criado se preciso). Recusa caminhos
// absolutos ou com "..". Não deixa nada extraído pela metade: extrai numa pasta ao lado e renomeia.
bool install_component(const ModuleComponent& component, const std::string& package_bytes,
                       const boost::filesystem::path& dest_dir, std::string& error);

// Fluxo completo para uma impressora: catálogo -> módulo -> pacotes -> <root>/<id>/<versão>/<tipo>.
// 'fetch(path, body, error)' faz o GET autenticado no site e devolve false em erro.
using ModuleFetch = std::function<bool(const std::string& path, std::string& body, std::string& error)>;

struct ModuleInstallResult
{
    bool                     ok = false;
    std::string              module_id;
    std::string              version;
    std::vector<std::string> installed; // tipos de componente instalados
    std::string              error;
};

ModuleInstallResult ensure_module_for_printer(const std::string& printer_name, const boost::filesystem::path& root,
                                              const ModuleFetch& fetch, const std::string& public_key_b64 = {});

}} // namespace Slic3r::Filuna
