#pragma once

// Filuna — Assistente de impressão (aprovado pelo dono em 20/11/2026, telas em FILUNA/ASSISTENTE.md).
//
// O caminho da impressão em 5 etapas (Peça, Conferir, Fatiar, Ficha, Imprimir) fica no rodapé da Preparação e no
// painel "assistente.html", à direita da mesa. As telas decidem o que mostrar; aqui ficam só as ações que mexem no
// programa: conferir a peça, ler e mudar alguns ajustes do processo (lista fechada), orientar, fatiar, imprimir e a
// pasta automática. Tudo o que a IA sugere passa pela mesma lista e só é aplicado com o clique do cliente.

#include <optional>
#include <string>

#include <nlohmann/json.hpp>
#include <wx/panel.h>

class wxWebView;
class wxWebViewEvent;

namespace Slic3r { namespace GUI {

class FilunaPanel;

// O painel à direita da mesa: a tela "assistente.html" e, embaixo dela, o site da IA do próprio cliente (Claude,
// ChatGPT ou Gemini), aberto como num navegador, com o login e a assinatura dele. A Filuna não usa chave de API nem
// crédito: a IA do cliente lê o fatiador e sugere pelo conector Filuna (filuna3d.com/api/ia/mcp), e as sugestões voltam
// para a tela, onde o cliente aprova. Com a IA aberta, a tela encolhe para as abas e a barra de navegação.
class FilunaAssistentePainel : public wxPanel
{
public:
    explicit FilunaAssistentePainel(wxWindow* parent);
    ~FilunaAssistentePainel() override;

    static FilunaAssistentePainel* atual();

    void abrir_ia(const wxString& url); // cria o navegador na primeira vez
    void fechar_ia();
    bool ia_aberta() const { return m_ia_aberta; }
    void ia_comando(const std::string& comando); // "voltar", "recarregar", "navegador"

private:
    void on_new_window(wxWebViewEvent& e);

    FilunaPanel* m_tela{nullptr};
    wxWebView*   m_ia{nullptr};
    bool         m_ia_aberta{false};
};

// Ações "assistente.*", "peca.*", "processo.*", "fatiamento.estado|avisos", "fatiar", "imprimir" e "pasta.*" da ponte
// das telas Filuna. Devolve nada quando a ação não é do assistente.
std::optional<nlohmann::json> filuna_assistente_acao(const std::string& acao, const nlohmann::json& dados);

// Chamado pela Plater quando um fatiamento termina: avisa todas as telas ("fatiado") para o rodapé e o painel
// atualizarem sem esperar a próxima consulta.
void filuna_assistente_fatiado(bool ok);

}} // namespace Slic3r::GUI
