#pragma once

// Filuna — várias impressoras ao vivo ao mesmo tempo.
//
// O programa (como o Orca) mantém UMA impressora selecionada no DeviceManager; as outras ficavam "desligadas" para o
// site, o app e a Farm. Aqui cada impressora que já foi conectada uma vez ganha a própria instância do agente da
// marca, conectada em segundo plano, e o relatório dela (formato "print" que todos os agentes Filuna e o Moonraker
// entregam: gcode_state, mc_percent, temperaturas, AMS) vira o estado ao vivo. A impressora selecionada no programa
// sai deste acompanhamento enquanto estiver selecionada (uma conexão por impressora).

#include <nlohmann/json.hpp>

#include "slic3r/Utils/bambu_networking.hpp"

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
// Para de acompanhar e apaga o que foi guardado desta impressora (reparo pelo suporte remoto). Linha da tela.
void esquecer(const std::string& dev_id);
// Séries das impressoras acompanhadas.
std::vector<std::string> acompanhadas();

// Para a Farm imprimir em todas as impressoras, não só na aberta: as acompanhadas que estão ligadas agora (relatório
// "print" recebido nos últimos 5 minutos), fora a impressora selecionada no programa.
struct Ligada
{
    std::string    dev_id, agente;
    nlohmann::json relatorio; // "print" acumulado: gcode_state, mc_percent, mc_remaining_time, ams…
};
std::vector<Ligada> ligadas();
// Manda o arquivo e começa a impressão pelo conector desta impressora (fora da linha da tela). 0 = enviada.
int imprimir(const std::string& dev_id, PrintParams p, std::string& erro);
// "pausar", "retomar" ou "cancelar" a impressão desta impressora. 0 = comando aceito.
int comando(const std::string& dev_id, const std::string& acao);

}} // namespace GUI::FilunaAoVivo
} // namespace Slic3r
