#pragma once

// Dispositivos (REGRA-CANONICA-ARQUITETURA-V2 §6): fabricante pela logo oficial → modelos homologados
// (catálogo assinado) → foto oficial do Orca → "Baixar complemento" (update interno do módulo) →
// busca na rede LAN pelo agente da marca → outros meios (IP) → ponte LAN assistida → card com estado,
// cores do sistema de cores e câmera. Preparar troca de impressora/perfil/agente sozinho.
// Tudo aqui é código Filuna; o Orca fornece perfis, covers, DeviceManager e agentes de rede.

#include <nlohmann/json.hpp>
#include <string>

namespace Slic3r { namespace GUI { namespace FilunaDispositivos {

using json = nlohmann::json;

// id do módulo no catálogo ("anycubic") -> id do fabricante nos perfis do Orca ("Anycubic")
std::string vendor_orca(const std::string& modulo_id);
// id do módulo -> id do agente de rede da marca (IPrinterAgent::get_agent_info().id)
std::string agente_da_marca(const std::string& modulo_id, const std::string& modelo = {});

// Fabricantes do catálogo com logo, estado e contagem de modelos disponíveis/indisponíveis.
json fabricantes();
// Modelos do fabricante: nome, foto (caminho do cover do Orca), disponível, mensagem quando não.
json modelos(const std::string& modulo_id);
// Complemento (módulo) a baixar para o fabricante: tamanho, versão, se já está instalado.
json complemento(const std::string& modulo_id);
// Seleciona em Preparar o perfil de máquina do modelo (bico padrão), instala o agente da marca e
// começa a busca na LAN. Devolve {"perfil": ..., "agente": ...} ou {"erro": ...}.
json preparar(const std::string& modulo_id, const std::string& modelo);
// Impressoras vistas na LAN pelo agente ativo.
json descobertas();
// Conecta a uma impressora descoberta (código de acesso quando a marca exige).
json conectar(const std::string& dev_id, const std::string& codigo);
// Outro meio: por endereço IP (e código), quando a descoberta não achou. A Flashforge pede o número de série: o
// programa pergunta direto ao IP; 'serie' só é preciso se ela não responder.
json conectar_ip(const std::string& modulo_id, const std::string& modelo, const std::string& ip, const std::string& codigo,
                 const std::string& nome, const std::string& serie = {});
// ---- Assistência remota (lista fechada do suporte; só leitura, exceto onde dito) ----
// Rede até uma impressora: redes do computador, por qual endereço ele sai até o IP, se é a mesma rede, portas das
// marcas abertas no IP, nome e série da Flashforge (pergunta direta) e da Bambu (certificado), e um resumo.
json rede_diagnostico(const std::string& ip, const std::string& modulo_id);
// Procura impressoras nas redes do computador pelas portas das marcas (e pela pergunta direta da Flashforge).
json rede_procurar();
// Predefinições ativas no fatiador e se combinam com a impressora conectada.
json fatiador_estado();
// Acerta a predefinição de impressora do fatiador para o modelo da impressora (muda a escolha do cliente).
json fatiador_ajustar(const std::string& dev_id);
// Passo a passo da ponte LAN assistida da marca (resources/filuna/ponte-lan.json).
json ponte_instrucoes(const std::string& modulo_id);
// Cadastra a impressora conectada no banco local (sincroniza com a conta e o app) e guarda o vínculo.
json registrar(const std::string& nome, const std::string& modulo_id, const std::string& modelo, const std::string& dev_id);
// Estado ao vivo de todas as impressoras (cards): ligada/imprimindo/livre/desligada, cores, temperaturas, câmera.
json estado();
// Troca a impressora ativa (Preparar + agente + perfil) para a cadastrada.
json selecionar(const std::string& dev_id);
// Reparo pelo suporte remoto: tira a impressora da lista do programa, do acompanhamento ao vivo e do vínculo com a
// conta (cadastro errado, IP trocado). Não mexe na impressora aberta no programa.
// Exclui a impressora de vez, a pedido do cliente (Dispositivo › Excluir): rede, vínculo e cadastro.
json excluir(const std::string& dev_id, const std::string& cadastro_id);
json esquecer(const std::string& dev_id);
json comando(const std::string& dev_id, const std::string& cmd);
json controle(const std::string& dev_id, const nlohmann::json& dados);
// Liga o vigia das impressoras (uma vez; independe da aba aberta): avisa impressão concluída e envia o estado.
// o cliente respondeu a pergunta do estoque sobre as entradas com filamento trocado
json troca_vista(const std::string& dev_id);
void monitor_iniciar();

// Temperaturas de verdade do bico e da mesa de uma impressora conectada (a janela de envio desenha com elas).
// false quando a impressora não está conectada ou não informa.
bool temperaturas(const std::string& dev_id, double& bico, double& bico_alvo, double& mesa, double& mesa_alvo);
// Envia o estado resumido para a conta (servidor → app Android). Pouco tráfego: chamar a cada ~30 s.
void sincronizar_estado(const json& estado, const std::string& token);

}}} // namespace Slic3r::GUI::FilunaDispositivos
