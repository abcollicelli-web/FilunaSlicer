#pragma once

// Impressão pelo motor de rede da marca (camada ÚNICA para todas as marcas que conectam pelo Filuna).
//
// O Orca tem dois caminhos para o botão Imprimir: o da Bambu (janela "Enviar para impressora") e o antigo,
// de "host de impressão" (OctoPrint, Klipper…), que só habilita o botão quando o perfil tem um endereço
// preenchido. As marcas do Filuna não usam nenhum dos dois: a impressora é achada na rede e conectada pelo
// agente da marca (IPrinterAgent). Este arquivo liga o botão Imprimir e a sincronização de filamentos a
// esse agente, de um jeito só, para qualquer marca — nada aqui conhece Elegoo, Anycubic ou outra.
//
// Contrato com os agentes (todos os Filuna*Agent já seguem):
//   start_local_print(params)          params.filename = arquivo .gcode local; params.ams_mapping = "[b0,b1,…]"
//                                      (posição = filamento do projeto, valor = entrada global unidade*4+slot,
//                                      -1 = filamento não usado nesta mesa)
//   start_send_gcode_to_sdcard(params) só envia o arquivo

#include <functional>
#include <string>

#include <nlohmann/json.hpp>

namespace Slic3r { namespace GUI { namespace FilunaImpressao {

// A impressora do perfil ativo fala por um agente de marca do Filuna (não vale para a Bambu, que tem janela própria).
bool agente_filuna();

// Janela de envio (impressora, arquivo, entrada de cada filamento) e envio do G-code da mesa.
void imprimir(int plate_idx);

// filament_id de um perfil de filamento compatível com a impressora ativa, por MARCA e tipo: cada marca de
// filamento tem o seu perfil de impressão. `marca` é o que a impressora leu do carretel ("ELEGOO PLA+",
// "Anycubic", sku…); pode vir vazia. Ordem: perfil da marca do filamento e da mesma linha, perfil da marca do
// filamento, perfil da marca da impressora, genérico, qualquer um do tipo. Vazio quando não há nenhum.
// O protocolo da marca imprime o filamento N pela entrada N: a sincronização mantém as entradas vazias no lugar.
bool agente_entrada_fixa();

// Filuna Farm: o Slicer imprime as mesas da fila do site pela impressora conectada (fila, fatiamento sem tela,
// envio e andamento). Desliga com filuna_farm_auto=0 no app_config.
void farm_iniciar();

// Farm local (a tela da Farm no Slicer): "farm.escolher" (arquivo), "farm.analisar" (peças, cores, medidas e foto, com o
// farm_arquivos.py do site no Python do programa), "farm.medir" (tempo e gramas fatiando cada arquivo com os perfis da
// impressora conectada) e "farm.salvar" (projeto planejado no computador guardado no site para acompanhar). As de
// trabalho longo respondem depois pelo evento.
nlohmann::json farm_acao(const std::string& acao, const nlohmann::json& d, std::function<void(const std::string&, const nlohmann::json&)> evento);

std::string filamento_id_do_tipo(const std::string& tipo, const std::string& marca = std::string());

}}} // namespace Slic3r::GUI::FilunaImpressao
