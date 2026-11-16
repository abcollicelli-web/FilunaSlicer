#pragma once

// Filuna — várias impressoras ao vivo ao mesmo tempo.
//
// O programa (como o Orca) mantém UMA impressora selecionada no DeviceManager; as outras ficavam "desligadas" para o
// site, o app e a Farm. Aqui cada impressora que já foi conectada uma vez ganha a própria instância do agente da
// marca, conectada em segundo plano, e o relatório dela (formato "print" que todos os agentes Filuna e o Moonraker
// entregam: gcode_state, mc_percent, temperaturas, AMS) vira o estado ao vivo. A impressora selecionada no programa
// sai deste acompanhamento enquanto estiver selecionada (uma conexão por impressora).

#include <nlohmann/json.hpp>

#include <string>
#include <vector>

namespace Slic3r {
class MachineObject;
namespace GUI { namespace FilunaAoVivo {

// Liga o gerente (uma vez) e carrega as impressoras guardadas. Linha da tela.
void iniciar();
// Guarda como reconectar esta impressora (IP, agente, código). Chamar para a impressora ativa já conectada. Linha da tela.
void lembrar(MachineObject* o);
// Impressora selecionada no programa (vazio = nenhuma): o acompanhamento paralelo dela é desligado na hora.
void ativa(const std::string& dev_id);
// Estado ao vivo de uma impressora acompanhada (campos do cartão: ligada, estado, progresso, restante_s, bico, mesa,
// cores, ao_vivo); objeto vazio se não está sendo acompanhada.
nlohmann::json estado_de(const std::string& dev_id);
// Séries das impressoras acompanhadas.
std::vector<std::string> acompanhadas();

}} // namespace GUI::FilunaAoVivo
} // namespace Slic3r
