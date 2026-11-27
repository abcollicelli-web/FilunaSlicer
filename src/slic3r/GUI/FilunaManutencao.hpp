#pragma once

// Manutenção do Filuna Slicer pelo servidor Filuna:
//  - DIAGNÓSTICO: todo erro das telas e do fluxo de dispositivos vai sozinho para o servidor
//    (/api/slicer/v1/diagnostico) com etapa, mensagem, versão instalada e o fim do registro do programa.
//    O dono vê em /admin/diagnostico — acabou o teste às cegas com print de tela.
//  - ATUALIZAÇÃO DO PROGRAMA: consulta /api/slicer/v1/app/atualizacao?build=<o que está instalado>, confere a
//    assinatura Ed25519 do documento, baixa SÓ o pacote do que mudou, confere o SHA-256, extrai numa pasta
//    de espera e, ao aplicar, fecha o programa, copia os arquivos e abre de novo (Windows).
//  - PRIMEIRA INSTALAÇÃO: abrir com o Windows, manter a conta, atualizar suporte a impressoras sozinho.
// Fica fora do código do Orca: só dois ganchos em GUI_App (iniciar e o comando da página inicial).

#include <string>
#include <nlohmann/json.hpp>

namespace Slic3r { namespace GUI { namespace FilunaManutencao {

void iniciar();                        // uma vez, na abertura
void conta(const std::string& token);  // entrou/saiu da conta: verifica de novo
std::string build_local();             // id do build instalado (filuna-build.json ao lado do executável)
std::string versao_do_pacote();        // versão do pacote da Microsoft Store ("2.5.93.0"); vazio fora da Store

// Erro para o servidor. Não repete o mesmo erro em 60 s; sem conta, só registra no log local.
void diagnostico(const std::string& etapa, const std::string& mensagem, const nlohmann::json& contexto = nlohmann::json::object());

// Ações das telas: "diagnostico.enviar", "atualizacao.estado|verificar|aplicar", "preferencias.ler|gravar".
// Devolve false quando a ação não é daqui.
bool acao(const std::string& nome, const nlohmann::json& dados, nlohmann::json& resposta);
// Página inicial (homepage): {"acao": "...", "dados": {...}} em texto.
void comando_home(const std::string& corpo_json);

}}} // namespace Slic3r::GUI::FilunaManutencao
