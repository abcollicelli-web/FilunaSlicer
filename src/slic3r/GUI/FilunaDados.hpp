#pragma once

// Ligação do banco local do Filuna (slic3r/Filuna/FilunaDb) com o programa: o banco aberto em
// <data_dir>/filuna/filuna.db, as ações da ponte das telas Filuna que leem e gravam nele
// (FILUNA/guia-v1/01-TELAS §5) e o diálogo "Confirmar bobinas" (02-DADOS §4).

#include <optional>
#include <string>

#include <nlohmann/json.hpp>

class wxWindow;

namespace Slic3r {
namespace Filuna {
class FilunaDb;
}

namespace GUI {

// Banco local, aberto na primeira chamada. Lança std::runtime_error se não conseguir abrir.
Filuna::FilunaDb& filuna_db();
// Pede à sincronização com o servidor que suba o que mudou (em 5 segundos, agrupando rajadas).
void filuna_sync_avisar();

// Executa uma ação de dados da ponte (estoque.*, bobina.*, historico.*, trabalho.*, produto.*, painel.*,
// previsao.*, manutencao.*, sync.*). Vazio quando a ação não é de dados.
std::optional<nlohmann::json> filuna_dados_acao(const std::string& acao, const nlohmann::json& dados);

// Depois de exportar ou enviar o G-code da mesa atual: se houver bobinas cadastradas, pergunta de quais
// bobinas sai cada filamento e registra o trabalho como 'enviado'. Sem bobinas ou sem fatiamento, não faz nada.
void filuna_registrar_envio(wxWindow* parent);

}} // namespace Slic3r::GUI
