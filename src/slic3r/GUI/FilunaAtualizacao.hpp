#pragma once

// Atualização de recursos e módulos pelo servidor Filuna (FILUNA/guia-v1/03 §5 e Fase 7).
// Uma thread própria consulta o catálogo assinado (/api/slicer/v1/modulos) ao abrir o programa e a
// cada 6 horas, baixa o que mudou para <data_dir>/filuna/recursos/<id>/<versão>/, confere o
// SHA-256 de cada pacote e só então troca o ponteiro (FilunaRecursos). Falhou qualquer parte:
// descarta e fica como está — nunca aplica pela metade. O aviso de versão nova do programa
// continua no canal que já existe (check_new_version_sf lendo /api/slicer/v1/versao).

#include <functional>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

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

    // Catálogo assinado mais recente (texto já verificado; vazio antes da primeira consulta ou sem
    // conta) e os ids visíveis para esta conta. Usado pela tela de Dispositivos.
    nlohmann::json catalogo() const;
    std::vector<std::string> visiveis() const;
    // Versão instalada de um módulo ("" se nenhuma) e se a versão do catálogo já está instalada.
    std::string versao_instalada(const std::string& id) const;
    bool        modulo_atualizado(const std::string& id) const;
    // Baixa e ativa agora o módulo <id> do catálogo, numa thread própria ("Baixar complemento").
    // 'progresso' recebe {"etapa": "...", "feito": n, "total": m, "ok": bool, "erro": "..."} e é
    // chamado na thread de trabalho — quem usa despacha para a interface.
    void instalar_modulo(const std::string& id, std::function<void(const nlohmann::json&)> progresso);

private:
    FilunaAtualizacao() = default;
    ~FilunaAtualizacao();
    FilunaAtualizacao(const FilunaAtualizacao&)            = delete;
    FilunaAtualizacao& operator=(const FilunaAtualizacao&) = delete;

    struct Impl;
    Impl* m = nullptr;
};

}} // namespace Slic3r::GUI
