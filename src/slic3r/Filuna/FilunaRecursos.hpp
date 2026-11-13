#pragma once

// Recursos atualizáveis do Filuna (guia-v1/03 §5): pacotes baixados do servidor ficam em
// <data_dir>/filuna/recursos/<id>/<versão>/, e o arquivo <id>/atual aponta a versão em uso.
// A anterior é guardada para poder voltar; nada é aplicado pela metade. Este arquivo é a parte
// pura (sem rede e sem interface): quem baixa é o FilunaAtualizacao, na camada GUI.

#include <string>
#include <utility>
#include <vector>

namespace Slic3r::Filuna::Recursos {

// Ids e versões viram nomes de pasta e pedaços de URL, então são validados antes de qualquer uso.
bool id_valido(const std::string& id);         // [a-z0-9-], 1 a 40
bool versao_valida(const std::string& versao); // [A-Za-z0-9._-], 1 a 40, sem ".."

std::string dir_versao(const std::string& raiz, const std::string& id, const std::string& versao);

// Versão apontada por <raiz>/<id>/atual ("" quando não há).
std::string versao_atual(const std::string& raiz, const std::string& id);

// Pasta da versão em uso ("" quando o ponteiro não existe ou a pasta sumiu).
std::string dir_atual(const std::string& raiz, const std::string& id);

// Troca o ponteiro de forma atômica (escreve atual.novo e renomeia) e apaga as versões antigas,
// guardando a recém-ativada e a imediatamente anterior.
void ativar(const std::string& raiz, const std::string& id, const std::string& versao);

// Todos os recursos instalados (id, versão apontada).
std::vector<std::pair<std::string, std::string>> instalados(const std::string& raiz);

// Base64 padrão; vazio quando o texto não decodifica.
std::vector<unsigned char> base64_decodificar(const std::string& texto);

// Confere a assinatura Ed25519 (OpenSSL) da mensagem exata: assinatura em base64 e a chave
// pública de 32 bytes em hexadecimal (a chave do catálogo vem fixa no binário).
bool verificar_ed25519(const std::string& mensagem, const std::string& assinatura_b64, const std::string& chave_publica_hex);

} // namespace Slic3r::Filuna::Recursos
