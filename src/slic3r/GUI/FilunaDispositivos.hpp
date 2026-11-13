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
// Outro meio: por endereço IP (e código), quando a descoberta não achou.
json conectar_ip(const std::string& modulo_id, const std::string& modelo, const std::string& ip, const std::string& codigo,
                 const std::string& nome);
// Passo a passo da ponte LAN assistida da marca (resources/filuna/ponte-lan.json).
json ponte_instrucoes(const std::string& modulo_id);
// Cadastra a impressora conectada no banco local (sincroniza com a conta e o app) e guarda o vínculo.
json registrar(const std::string& nome, const std::string& modulo_id, const std::string& modelo, const std::string& dev_id);
// Estado ao vivo de todas as impressoras (cards): ligada/imprimindo/livre/desligada, cores, temperaturas, câmera.
json estado();
// Troca a impressora ativa (Preparar + agente + perfil) para a cadastrada.
json selecionar(const std::string& dev_id);
// Envia o estado resumido para a conta (servidor → app Android). Pouco tráfego: chamar a cada ~30 s.
void sincronizar_estado(const json& estado, const std::string& token);

}}} // namespace Slic3r::GUI::FilunaDispositivos
