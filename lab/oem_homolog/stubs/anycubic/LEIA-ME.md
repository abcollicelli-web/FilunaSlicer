# Anycubic — cola de compilação da Filuna (Linux)

`filuna-linux.patch`: 3 linhas em 2 arquivos de **interface/plugins** da Anycubic (`protocol/check_protocol.cpp`,
`plugins/info_manage/info_manage_plugin.cpp`) que a própria Anycubic nunca compilou no Linux: include com
maiúscula errada (`SemVer.hpp`), `Slic3r::format` sem include e um `?:` com `std::string`/`wxString`.
Aplicado pela receita (`build/compilar.sh`) só durante a compilação e desfeito ao fim; a árvore no git fica limpa.
Motor (libslic3r/G-code), perfis e rede: intocados. Nada disto vai para módulo.
