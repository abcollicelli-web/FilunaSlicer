#pragma once

// Filuna — a janela que aparece quando o cliente manda imprimir, em QUALQUER impressora (filamento ou resina).
//
// À esquerda, os passos do envio, que vão se trocando. À direita, o desenho da impressora: o bico e a mesa vão
// esquentando, mudando de cor até o vermelho, com a temperatura de verdade lida da máquina em cima de cada um.
// No fim diz que a impressão começou e oferece acompanhar pelo Dispositivo ou deixar esta janela aberta.
//
// Quem manda imprimir só precisa de três coisas: criar a janela, ir dizendo em que passo está (passo()) e
// avisar quando terminou (concluir()). As temperaturas a janela mesma busca na impressora conectada.

#include <wx/dialog.h>
#include <wx/string.h>

#include <memory>
#include <string>

class wxTimer;
class wxTimerEvent;

namespace Slic3r { namespace GUI {

class JanelaEnvio : public wxDialog
{
public:
    // Os passos do envio, na ordem em que acontecem. A janela mostra um de cada vez, à esquerda.
    enum class Passo {
        PREPARANDO,   // separando o arquivo da mesa
        ENVIANDO,     // mandando o arquivo (tem porcentagem)
        CONFERINDO,   // o arquivo chegou inteiro?
        FALANDO,      // pedindo o início da impressão
        AQUECENDO,    // a impressora está esquentando o bico e a mesa
        IMPRIMINDO,   // começou
    };

    // dev_id: a impressora conectada, de onde saem as temperaturas de verdade (vazio = sem leitura ao vivo).
    JanelaEnvio(wxWindow* pai, const std::string& dev_id, const wxString& impressora, const wxString& arquivo);
    ~JanelaEnvio() override;

    void passo(Passo p, int porcento = -1);        // em que pé está o envio
    void concluir(bool deu_certo, const wxString& recado = {}); // fim: mostra o resultado e os botões
    bool cancelou() const { return m_cancelou; }   // o cliente pediu para parar

    // true quando o cliente escolheu começar um projeto novo ao fechar.
    bool quer_projeto_novo() const { return m_projeto_novo; }

private:
    class Desenho;   // a impressora desenhada, à direita
    class Coluna;    // os passos, à esquerda

    void ao_relogio(wxTimerEvent&);
    void ler_temperaturas();
    void montar_fim(bool deu_certo, const wxString& recado);

    std::string m_dev_id;
    wxString    m_impressora, m_arquivo;
    Desenho*    m_desenho{nullptr};
    Coluna*     m_coluna{nullptr};
    wxTimer*    m_relogio{nullptr};
    bool        m_cancelou{false}, m_projeto_novo{false}, m_terminou{false};
};

}} // namespace Slic3r::GUI
