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

#include <string>

namespace Slic3r { namespace GUI { namespace FilunaImpressao {

// A impressora do perfil ativo fala por um agente de marca do Filuna (não vale para a Bambu, que tem janela própria).
bool agente_filuna();

// Janela de envio (impressora, arquivo, entrada de cada filamento) e envio do G-code da mesa.
void imprimir(int plate_idx);

// filament_id de um perfil de filamento compatível com a impressora ativa para o tipo ("PLA", "PETG"…).
// Prefere o perfil da própria marca, depois o genérico. Vazio quando não há nenhum.
std::string filamento_id_do_tipo(const std::string& tipo);

}}} // namespace Slic3r::GUI::FilunaImpressao
