// Ensaio do envio FTPS da Bambu contra o emulador (lab/oem_homolog/emuladores/bambu_lan.py).
// Uso: teste_ftps <novo|velho|novo-cancela> <porta> <arquivo> <codigo> [limite_s]
// Imprime "OK", "ERRO: <motivo>" ou, se passar de 25 s sem terminar, "TRAVOU".
#include <atomic>
#include <chrono>
#include <cstdio>
#include <functional>
#include <future>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>

#include "FilunaFtps.hpp"
#include <boost/asio/connect.hpp>

namespace velho {
struct PrintParams { std::string dev_ip, password, username; };
enum { PrintingStageUpload = 1 };
using OnUpdateStatusFn = std::function<void(int, int, std::string)>;
using WasCancelledFn   = std::function<bool()>;
struct Ports { unsigned short ftps = 990; bool tls = true; };
struct FilunaBambuLanAgent {
    std::recursive_mutex m_mutex;
    std::string          m_dev_ip, m_access_code;
    Ports                m_ports;
    bool upload_ftps(const PrintParams& p, const std::string& local_path, const std::string& remote_name,
                     OnUpdateStatusFn update_fn, WasCancelledFn cancel_fn, std::string& error);
};
#include "corpo_velho.inc"
} // namespace velho

int main(int argc, char** argv)
{
    const std::string modo = argv[1], arquivo = argv[3], codigo = argv[4];
    const auto        porta  = static_cast<unsigned short>(std::stoi(argv[2]));
    const int         limite = argc > 5 ? std::stoi(argv[5]) : 20;
    std::atomic<bool> cancelar{false};
    auto              tarefa = std::async(std::launch::async, [&]() -> std::string {
        if (modo == "velho") {
            velho::FilunaBambuLanAgent a;
            a.m_dev_ip = "127.0.0.1";
            a.m_access_code = codigo;
            a.m_ports.ftps  = porta;
            std::string erro;
            return a.upload_ftps({}, arquivo, "teste.3mf", nullptr, nullptr, erro) ? "" : erro;
        }
        Slic3r::FilunaFtps::Envio e;
        e.ip = "127.0.0.1";
        e.porta = porta;
        e.senha = codigo;
        e.arquivo_local = arquivo;
        e.nome_remoto   = "teste.3mf";
        e.limite_resposta = std::chrono::seconds(limite);
        e.limite_fim      = std::chrono::seconds(limite);
        e.cancelado = [&] { return cancelar.load(); };
        e.registro  = [](const std::string& s) { std::cerr << "  etapa: " << s << "\n"; };
        return Slic3r::FilunaFtps::enviar(e);
    });
    if (modo == "novo-cancela") {
        std::this_thread::sleep_for(std::chrono::seconds(2));
        cancelar = true;
    }
    const auto inicio = std::chrono::steady_clock::now();
    if (tarefa.wait_for(std::chrono::seconds(25)) != std::future_status::ready) {
        std::cout << "TRAVOU (mais de 25 s sem terminar)" << std::endl;
        std::_Exit(2);
    }
    const auto        s   = std::chrono::duration<double>(std::chrono::steady_clock::now() - inicio).count();
    const std::string res = tarefa.get();
    std::cout << (res.empty() ? std::string("OK") : "ERRO: " + res) << "  (" << s << " s)" << std::endl;
    return res.empty() ? 0 : 1;
}
