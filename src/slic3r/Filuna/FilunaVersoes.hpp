#pragma once

// Identificação dos motores internos do Filuna (decisão do proprietário, 13/11/2026).
// O produto é um só — Filuna Slicer — mas cada motor tem nome e versão próprios, para que a
// atualização pelo servidor (guia-v1/03 §5, pacotes de recursos por id) possa evoluir um motor
// sem recompilar os outros e para que o suporte saiba exatamente o que o cliente está rodando.
// Regras:
// - a versão de um motor só muda junto com uma mudança real nele, registrada no
//   FILUNA/REGISTRO-CONTINUIDADE-ETAPAS.md;
// - ids são estáveis: são a chave dos pacotes de atualização e nunca mudam;
// - o esquema do banco local não vem daqui: é o PRAGMA user_version do FilunaDb.

#include <string>
#include <vector>

namespace Slic3r::Filuna {

// Versão da camada Filuna do produto (não é a versão do Orca que serve de base).
inline constexpr const char* VERSAO_FILUNA_SLICER = "1.3.0-beta";

struct Motor
{
    const char* id;     // chave do pacote de atualização (nunca muda)
    const char* nome;   // como aparece para o cliente
    const char* versao;
};

// Os motores internos e suas versões atuais.
inline const std::vector<Motor>& motores_filuna()
{
    static const std::vector<Motor> lista = {
        {"cad", "Filuna CAD", "1.1.0"},                   // aba Design: seleção, esboço, receita (base Orca + camada Filuna)
        {"dados", "Filuna Dados", "1.2.0"},               // banco local, fórmulas de estoque e leitor de G-code
        {"sincronizacao", "Filuna Sincronização", "1.0.0"}, // fila, enviar/receber e restauração
        {"telas", "Filuna Telas", "1.3.0"},               // páginas web internas (Meu espaço, Estoque, Dispositivos, Farm, Modelos 3D)
        {"inicio", "Filuna Início", "1.0.2"},             // tela Início (resources/web/homepage e o que ela usa de web/include)
        {"atualizacao", "Filuna Atualização", "1.2.0"},   // catálogo assinado, download, SHA-256, modo Store e correção de telas por geração
    };
    return lista;
}

// Versão embutida de um motor ("" quando o id não existe).
inline std::string versao_embutida(const std::string& id)
{
    for (const Motor& m : motores_filuna())
        if (id == m.id)
            return m.versao;
    return {};
}

} // namespace Slic3r::Filuna
