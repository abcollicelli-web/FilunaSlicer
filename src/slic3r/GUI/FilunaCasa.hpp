#pragma once

// Filuna Home no Slicer (tela Dispositivo › Casa): o Slicer roda dentro da casa do cliente, então é ele que procura
// os aparelhos na rede (TVs, tomadas, luzes, roteador, central) e conversa com eles. O site (/casa) só mostra a
// lista que o Slicer manda e guarda a configuração da conta.
//
// Busca: SSDP (TVs LG e Samsung, Fire TV, roteador com UPnP), mDNS (Google TV e Chromecast, Fire TV, Shelly, Home
// Assistant, Philips Hue), broadcast das tomadas TP-Link (Kasa na porta 9999, Tapo na 20002) e das Smart Life
// (Tuya, portas 6666 e 6667), mais o roteador pela rota padrão e o Wi-Fi do computador.
// Ações: Shelly (estado, ligar, desligar, teste), TV LG (parear e aviso na tela, protocolo SSAP do webOS),
// Home Assistant (teste do MQTT e publicação das impressoras) e diagnóstico do roteador e do Wi-Fi.

#include <nlohmann/json.hpp>
#include <string>

namespace Slic3r { namespace GUI { namespace FilunaCasa {

using json = nlohmann::json;

// Começa a busca em segundo plano (uns 6 s). Devolve na hora; o resultado vem por resultado().
json procurar();
// {rodando, aparelhos: [{chave, tipo, nome, marca, modelo, ip, via, estado, fatos, alerta?, checagens?, dados}], rede}
json resultado();
// Ação num aparelho, em segundo plano: {acao, pedido, ...}. Devolve {aceito}; o fim chega à página no evento
// "casa.acao" com {pedido, ok, msg, ...}.
json acao(const json& pedido);
// Chamada a cada leitura das impressoras: publica o estado delas no Home Assistant, se o cliente ligou.
void publicar_home_assistant(const json& estado_impressoras);

}}} // namespace Slic3r::GUI::FilunaCasa
