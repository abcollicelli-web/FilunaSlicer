#pragma once

// Filuna Home: o motor de avisos. O Filuna Slicer está dentro da casa do cliente; quando uma impressora termina,
// dá erro, pausa ou troca de filamento (e quando o site manda uma venda ou um aviso pela API), este motor decide
// quais aparelhos avisam e como, pelas regras que o cliente montou em Dispositivo › Casa:
//   - quais avisos cada aparelho recebe, de quais impressoras e em que horário (horário silencioso);
//   - o texto de cada aviso (modelo com {impressora}, {peca}, {erro}…), a cor de cada aviso e o estilo do popup;
//   - a tomada que desliga sozinha quando a impressão termina e o bico esfria.
// Tudo fica gravado neste computador (funciona sem internet) e a página copia para a conta.
//
// Aparelhos ("motores"): este computador (popup do Filuna), TV LG, luz Kasa e WLED, Home Assistant (MQTT) e os
// plugins declarativos (ntfy, Telegram, Discord, webhook, Tasmota…): um plugin é só a receita do pedido HTTP, sem
// código, então qualquer serviço com API HTTP entra sem compilar o Filuna de novo.

#include <nlohmann/json.hpp>
#include <string>

namespace Slic3r { namespace GUI { namespace FilunaCasaAvisos {

using json = nlohmann::json;

// Todas as funções são chamadas na linha da tela.

// {regras: {chave: {aparelho, cfg}}}
json regras();
// {chave, aparelho: {tipo, nome, ip, protocolo, plugin?}, cfg} -> grava; {chave, remover: true} apaga
json salvar_regra(const json& pedido);
// Manda um aviso de teste (ou do evento pedido) para um aparelho, com a configuração que está na tela, sem gravar.
// {pedido, aparelho, cfg, evento?}. Devolve {aceito}; o fim chega pelo evento "casa.teste" {pedido, ok, msg}.
json testar(const json& pedido);
// Últimos avisos entregues (ou não) por aparelho: {itens: [{quando, evento, titulo, aparelho, ok, msg}]}
json historico();
// Aviso de um evento: tipo = concluida, erro, pausada, filamento, venda, mensagem. dados: {impressora, peca, erro,
// progresso, texto, titulo}.
void evento(const std::string& tipo, const json& dados);
// A cada leitura das impressoras: desliga as tomadas que esperam o bico esfriar e busca os avisos que chegaram
// pelo site (vendas e API), uma vez a cada 30 s.
void vigiar(const json& estado_impressoras, const std::string& token);

}}} // namespace Slic3r::GUI::FilunaCasaAvisos
