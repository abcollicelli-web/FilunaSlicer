#pragma once

// Filuna — janelas de filamento do menu lateral (canvas "Menu lateral Filuna Slicer", proposta 2):
//
// - JanelaFilamento: escolher o filamento de uma entrada. Começa pela cor; depois busca, material, o que tem no estoque
//   Filuna, os perfis da marca e as outras marcas. Usa os perfis do Orca já carregados (nada é lido do disco ao abrir).
// - JanelaAjustesFilamento: os ajustes que importam do perfil (temperaturas, ventoinha, fluxo, retração), gravados pela
//   aba de filamento do Orca, como se o cliente tivesse mudado lá; "Ajustes avançados" abre a aba completa.

#include <wx/dialog.h>

#include <memory>
#include <string>

namespace Slic3r { namespace GUI { namespace FilunaFilamento {

// Códigos de saída além de wxID_OK / wxID_CANCEL.
enum Saida { ABRIR_AJUSTES = wxID_HIGHEST + 900, CADASTRAR_NOVO, AJUSTES_AVANCADOS };

class JanelaFilamento : public wxDialog
{
public:
    JanelaFilamento(wxWindow* pai, int entrada);
    ~JanelaFilamento() override;
    const std::string& preset() const; // nome interno do perfil escolhido
    wxColour           cor() const;

private:
    struct Dados;
    std::unique_ptr<Dados> m;
};

class JanelaAjustesFilamento : public wxDialog
{
public:
    JanelaAjustesFilamento(wxWindow* pai, int entrada);
    ~JanelaAjustesFilamento() override;

private:
    struct Dados;
    std::unique_ptr<Dados> m;
};

// Faz da entrada a que a aba de filamento do Orca edita (o mesmo que o lápis do Orca faz antes de abrir a aba).
bool editar_entrada(int entrada);

}}} // namespace Slic3r::GUI::FilunaFilamento
