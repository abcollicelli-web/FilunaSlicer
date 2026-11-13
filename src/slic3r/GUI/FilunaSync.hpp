#pragma once

// Sincronização do banco local do Filuna com filuna3d.com (FILUNA/guia-v1/03-SERVIDOR-E-BACKUP.md §4).
// Uma thread própria envia a sync_fila e recebe o que outros computadores, o site e o app mudaram.
// Nunca bloqueia a interface; sem login ou sem internet, a fila espera e sobe depois.

#include <cstdint>
#include <functional>
#include <string>

namespace Slic3r { namespace GUI {

class FilunaSync
{
public:
    struct Estado
    {
        bool         ligado = false;  // há conta (token) e a thread está rodando
        int          pendentes = 0;   // itens esperando envio
        std::int64_t rev = 0;         // último rev recebido do servidor
        std::string  ultimo_ok;       // hora UTC da última sincronização completa
        std::string  ultimo_erro;     // vazio quando a última deu certo
    };

    static FilunaSync& instancia();

    // Liga com o banco em <caminho_db> (conexão própria). Chamar uma vez; as outras chamadas são ignoradas.
    // [ao_receber] roda na thread da sincronização quando chegam mudanças de fora (use CallAfter para a tela).
    void iniciar(const std::string& caminho_db, const std::string& token, std::function<void()> ao_receber = {});
    // Algo mudou no banco ou na conta: sincroniza em 5 segundos (agrupa rajadas). Token vazio = sem conta.
    void avisar(const std::string& token);
    // Ao fechar o programa: última tentativa de envio, esperando no máximo 3 segundos.
    void parar();
    Estado estado() const;

private:
    FilunaSync() = default;
    ~FilunaSync();
    FilunaSync(const FilunaSync&)            = delete;
    FilunaSync& operator=(const FilunaSync&) = delete;

    struct Impl;
    Impl* m = nullptr;
};

}} // namespace Slic3r::GUI
