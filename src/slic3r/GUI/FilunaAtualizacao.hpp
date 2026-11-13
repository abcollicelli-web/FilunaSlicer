#pragma once

// Atualização de recursos e módulos pelo servidor Filuna (FILUNA/guia-v1/03 §5 e Fase 7).
// Uma thread própria consulta o catálogo assinado (/api/slicer/v1/modulos) ao abrir o programa e a
// cada 6 horas, baixa o que mudou para <data_dir>/filuna/recursos/<id>/<versão>/, confere o
// SHA-256 de cada pacote e só então troca o ponteiro (FilunaRecursos). Falhou qualquer parte:
// descarta e fica como está — nunca aplica pela metade. O aviso de versão nova do programa
// continua no canal que já existe (check_new_version_sf lendo /api/slicer/v1/versao).

#include <string>
#include <vector>

namespace Slic3r { namespace GUI {

class FilunaAtualizacao
{
public:
    struct Estado
    {
        bool                     ligado = false;  // há conta e a thread roda
        std::string              ultimo_ok;       // hora UTC da última verificação completa
        std::string              ultimo_erro;     // vazio quando a última deu certo
        std::vector<std::string> baixados;        // "id versão" aplicados desde que o programa abriu
    };

    static FilunaAtualizacao& instancia();

    // <raiz> = <data_dir>/filuna/recursos. Chamar uma vez; as outras chamadas são ignoradas.
    void iniciar(const std::string& raiz, const std::string& token);
    // Conta entrou ou saiu: verifica de novo em alguns segundos. Token vazio = parado.
    void avisar(const std::string& token);
    void parar();
    Estado estado() const;

private:
    FilunaAtualizacao() = default;
    ~FilunaAtualizacao();
    FilunaAtualizacao(const FilunaAtualizacao&)            = delete;
    FilunaAtualizacao& operator=(const FilunaAtualizacao&) = delete;

    struct Impl;
    Impl* m = nullptr;
};

}} // namespace Slic3r::GUI
