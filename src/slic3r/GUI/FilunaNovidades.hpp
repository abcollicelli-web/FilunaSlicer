#pragma once

// Filuna — Novidades (pedido do dono, 27/11/2026).
//
// Na primeira abertura depois de uma atualização, um cartão mostra ao cliente os recursos novos da versão, em
// linguagem de cliente (tela resources/web/filuna/novidades.html). Os textos vêm de resources/web/filuna/novidades.json
// (embutido, em português e inglês) e o site pode trocar os de uma versão pela rota /api/slicer/v1/novidades
// (Admin › Novidades), sem lançar outra.
//
// A versão é a do pacote da Microsoft Store ("2.5.93.0" vale como 2.5.93). Fora da Store (pacote de teste) vale a
// versão mais nova do arquivo. A última vista fica em app_config ("filuna_novidades_vistas"). Na primeira instalação
// não aparece nada: a versão instalada já fica marcada como vista.

#include <optional>
#include <string>

#include <nlohmann/json.hpp>

namespace Slic3r { namespace GUI { namespace FilunaNovidades {

// Uma vez, depois que a janela principal abriu. instalacao_nova: não havia configuração antes desta abertura ou o
// assistente de primeira configuração acabou de aparecer.
void ao_abrir(bool instalacao_nova);

// "O que há de novo" (menu Ajuda): mostra as novidades da versão instalada e das anteriores.
void mostrar();
// Nome do item do menu na língua do programa ("O que há de novo" / "What's new").
std::string rotulo_menu();

// Ações da tela novidades.html: "novidades.dados" e "novidades.fechar". Nada quando a ação não é daqui.
std::optional<nlohmann::json> acao(const std::string& nome, const nlohmann::json& dados);

}}} // namespace Slic3r::GUI::FilunaNovidades
