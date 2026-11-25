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

// Comandos usados pelo motor de avisos (FilunaCasaAvisos). Todos bloqueiam alguns segundos: chamar numa linha
// própria, nunca na da tela. As chaves e senhas são lidas antes, na linha da tela, e passadas aqui.
bool tv_lg_aviso(const std::string& ip, const std::string& chave_lg, const std::string& texto, std::string& erro);
// protocolo: "shelly" ou "kasa"
bool tomada_ligar(const std::string& protocolo, const std::string& ip, bool ligar, std::string& erro);
json tomada_estado(const std::string& protocolo, const std::string& ip);
// Mostra a cor do aviso na luz por alguns segundos e volta ao que estava. protocolo: "kasa" ou "wled".
bool luz_aviso(const std::string& protocolo, const std::string& ip, const std::string& cor_hex, int segundos, std::string& erro);
// Publica o aviso no MQTT do Home Assistant (gatilho "Filuna" nas automações). cfg_ha: {broker, usuario, senha}.
bool ha_evento(const json& cfg_ha, const json& aviso, std::string& erro);
// Chave de pareamento da TV LG guardada neste computador (linha da tela).
std::string chave_lg(const std::string& ip);

}}} // namespace Slic3r::GUI::FilunaCasa
