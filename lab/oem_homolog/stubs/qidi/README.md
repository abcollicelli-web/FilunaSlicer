# Substitutos da pasta fechada `src/slic3r/QIDI/` (QIDIStudio)

A fonte publicada do QIDIStudio (QIDITECH/QIDIStudio, tag `v02.07.02.73`) exclui do repositório
a pasta `src/slic3r/QIDI/` (linha `/src/slic3r/QIDI/` do `.gitignore`): rede da nuvem QIDI Maker,
MQTT, P2P, WebSocket do dispositivo, API HTTP da impressora e sincronização de predefinições.
Sem ela a interface não compila, e o fatiamento pela linha de comando mora no mesmo executável.

Estes arquivos são escritos pela Filuna, só para a régua de paridade do motor: declaram os tipos
que a interface usa e não fazem nada (toda chamada de rede responde "sem conexão"). O motor
(`src/libslic3r`) e o caminho de fatiamento da CLI (`src/QIDIStudio.cpp`) são os do fabricante,
intocados. A compilação usa `QDT_RELEASE_TO_PUBLIC=0`, que tira da interface as chamadas da nuvem;
em `libslic3r` essa opção só muda nomes de pasta de configuração e o SHA do git no cabeçalho.

Não fazem parte de nenhum módulo distribuído: a conexão Filuna com impressoras QIDI é o
`QidiPrinterAgent` (Moonraker), já homologado contra o emulador.
