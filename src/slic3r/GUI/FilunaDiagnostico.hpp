#pragma once

// Filuna — Raio X do equipamento (diagnóstico remoto).
//
// Leitura: junta tudo o que a impressora informa (relatório completo da marca, medidas extras, leitura profunda do
// agente — no Klipper objetos, placas, logs; na Prusa info e impressora; na Bambu versões e HMS; na SDCP atributos e
// histórico), a rede até ela e o catálogo de testes que ela permite, e registra no Raio X da conta.
//
// Testes: o executor fica AQUI, fora da IA e do site. Quem pede (cliente, suporte remoto ou a IA dele pelo conector)
// só escolhe um teste do catálogo; os limites são fixos neste arquivo:
//   - teste que aquece ou movimenta só roda com o cliente confirmando, NA TELA DO PROGRAMA, que está ao lado da
//     impressora (vale 20 min); o pedido remoto fica esperando essa confirmação;
//   - nunca com a impressora imprimindo; um teste por vez;
//   - bico no máximo 240 °C, mesa no máximo 70 °C, tempo máximo por teste;
//   - sem resposta da impressora, conexão caída ou comando recusado: desliga o calor e encerra;
//   - movimento só depois de zerar os eixos (G28) e dentro dos limites que a própria impressora informa; o teste de
//     vibração só roda com sensor de vibração instalado;
//   - nada grava configuração ou firmware (sem SAVE_CONFIG, sem mexer em driver).

#include <nlohmann/json.hpp>
#include <string>

namespace Slic3r { namespace GUI { namespace FilunaDiagnostico {

using json = nlohmann::json;

// Leitura completa (bloqueia alguns segundos: chamar fora da linha da tela). {"erro"} se a impressora não está ligada.
json ler(const std::string& dev_id, const std::string& origem);
// Testes que esta impressora permite, com risco ("leitura", "aquece", "move") e duração, e os guiados pela tela.
json testes(const std::string& dev_id);
// Começa um teste em segundo plano: {dev_id, teste, origem}. Responde na hora; o andamento vem por teste_estado()
// e pelo evento "diagnostico.teste".
json testar(const json& pedido);
// Andamento ou resultado do teste atual (ou do último).
json teste_estado();
// Interrompe o teste: desliga aquecedores e cooler (e, num teste que movimenta o Klipper, parada de emergência).
json parar();
// O cliente confirma, na tela do programa, que está ao lado da impressora (libera testes que aquecem/movem por 20 min).
json presenca(const std::string& dev_id);

}}} // namespace Slic3r::GUI::FilunaDiagnostico
