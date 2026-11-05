# Filuna Slicer

Filuna Slicer é um fork do **OrcaSlicer**: o motor de fatiamento, os perfis e os protocolos de impressão são os do Orca, sem alterações. Por cima disso fica uma camada visual e de produto Filuna. Ele também traz módulos de fabricante (Elegoo, Anycubic e outros) para envio e controle nativos.

## Visão
- Fatiar e imprimir **igual ao Orca** desde o primeiro minuto.
- Um único programa para Orca, Bambu, Elegoo, Creality, Flashforge, Qidi, Snapmaker e Anycubic.
- Aparência própria (Filuna), com o Orca de verdade por baixo.

## Regras do projeto
1. **O upstream fica intacto.** A branch `upstream-mirror` é uma cópia exata do OrcaSlicer e nunca é editada à mão.
2. **A camada Filuna fica isolada.** Ela vive em `FILUNA/`, com o mínimo de pontos de extensão no código do Orca. Cada um desses pontos é listado em ARCHITECTURE.md.
3. **Os módulos de fabricante são separados e versionados**, conforme MODULES.md.
4. **Toda atualização de upstream** passa por merge controlado, CI de teste e aceite (UPDATE-PLAYBOOK.md).
5. **Nenhuma impressão começa sem confirmação explícita do usuário.**
6. **Pasta de configuração própria `Filuna`**, separada do Orca, do mesmo jeito que o Bambu Studio faz. Não se misturam perfis com um Orca instalado.
7. **Nada é considerado pronto** antes de passar no TEST-ACCEPTANCE.md (Elegoo Centauri Carbon 2 e Anycubic Kobra 3 V2).
8. **Sem plugins de rede proprietários empacotados** (Bambu e Flashforge), por compatibilidade com a AGPL.

## Licença
GNU **AGPL-3.0**, a mesma do OrcaSlicer (ver `LICENSE.txt` na raiz). O código-fonte do Filuna é público. Cada binário distribuído deve indicar onde obter o fonte correspondente.

## Créditos
Slic3r (Alessandro Ranellucci e comunidade) → PrusaSlicer (Prusa Research) → Bambu Studio (Bambu Lab) → OrcaSlicer (SoftFever e comunidade) → Filuna Slicer.
Os módulos de fabricante derivam de ElegooSlicer, AnycubicSlicerNext e outros forks AGPL, com crédito aos respectivos autores.
