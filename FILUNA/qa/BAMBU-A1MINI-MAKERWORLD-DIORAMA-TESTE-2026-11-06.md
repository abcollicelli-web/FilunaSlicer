# FILUNA — TESTE MAKERWORLD A1 MINI: DIORAMA EVOLUÇÕES

**Data:** 2026-11-06 (Europe/Rome). **Origem:** 3MF anexado pelo proprietário; preservado sem alterar modelo, camas ou filamentos.
**Estado da sessão:** teste Bambu oficial em execução no laboratório; o documento só registra o que foi medido, não inventa G-code final ou módulos prontos.

## Fonte
- Arquivo: `Charmander-Charizard+evolutions+diorama.3mf`
- SHA-256 da transferência integral: `3649d011b82a52aceed643f7e82c22803a59001704f5e3e415e16a7ed590c365`
- Tamanho exato: 15.846.184 bytes
- Cópia de ensaio no host SentinelX: `/srv/site/incoming/filuna-qa-a1-mini-original.3mf`
- ZIP 3MF: 23 entradas, incluindo geometria, `Metadata/project_settings.config`, `Metadata/model_settings.config`, `Metadata/slice_info.config`, `Metadata/plate_1.json` e imagens de placa.
- **Impressora já definida no arquivo: Bambu Lab A1 mini**, `Bambu Lab A1 mini 0.4 nozzle`, **não** P2S. O proprietário determinou explicitamente conservar A1 mini; **não converter silenciosamente para P2S**.
- Processo `0.20mm Standard @BBL A1M`; altura de camada 0,2 mm; infill 10%; suporte habilitado; mesa texturizada.
- Cinquenta? Não: **cinco** materiais: `Bambu PETG-CF @BBL A1M 0.4 nozzle`, `Bambu PLA Matte @BBL A1M`, `Bambu PLA Basic @BBL A1M`, `Generic PETG @BBL A1M`, `Generic PLA @BBL A1M`, com diâmetro 1,75 mm. **Não assumir segurança de alternância de materiais em um único bico sem validar projeto, G-code e compatibilidade AMS**.
- Geometria do modelo principal: 1.055.850 faces (requer análise de memória e slicing prolongado); o conteúdo não é um cubo sintético.

## Versões fixadas
- Fonte oficial pública **Bambu Studio** clonada em `/srv/site/incoming/filuna-bambu-source-qa-20261106`, commit `355d71eee5a9ebc794693fe57e63674806019770`. Git clonado integra fontes abertas e recursos, **não autoriza distribuir ou integrar componentes fechados do fabricante**.
- Executável oficial Bambu Studio já presente no laboratório: `/var/tmp/filuna-qa-wine/official-bambu-v0290170/unpacked/bambu-studio.exe`, versão do pacote `02.09.01.70`; código-fonte clonado (master atual) NÃO está afirmado como binariamente idêntico à versão 02.09.01.70. Necessário pin do commit exato do build, se disponível.
- Filuna candidato: PR #5, `filuna/oem-gcode-routing-qa-20261106`, ainda em draft e sem novo binário Windows OEM validado.

## Sequência obrigatória de testes
1. **Bambu Studio oficial primeiro**: CLI com `--slice 0`, arquivo 3MF sem alterações, prefixo Wine `/var/tmp/filuna-qa-wine/prefix`, saída isolada `/var/tmp/filuna-qa-wine/qa-a1mini-evolutions-bambu-official`. Quando terminar, registrar G-code, saída/erro, tempo, filamento, configurações, integridade do container G-code. Se exigir geração de `.gcode.3mf`, preservar os dois outputs.
2. Integrar código OEM Bambu conforme contrato de rede 100% e fatiamento 100% de diferenças, **sem duplicar Orca comum**, preservando licenças, interfaces e firmware por modelo; registrar commits e dependências. **Clone de repositório ou pacote JSON sozinho não constitui integração.**
3. Build Windows completo e novo Filuna executável com o módulo A1 mini funcional selecionado por dispositivo cadastrado e modelo/firmware. Se o build falhar, registrar erro exato e corrigir.
4. Filuna com **mesmo 3MF sem alterar config**; comparar G-code semântico: XYZ/E/F/M204 por feature, retrações, deslocamentos, tempo, filamento, suportes, troca de material, pausas e `.gcode.3mf` incluindo `slice_info.config` e imagens de placa.
5. Testes de protocolos OEM de rede autenticados simulados (SSDP, MQTT/TLS, FTPS), e depois impressora física para validação final. Não iniciar impressão de forma automática.
6. A decisão de homologação, com números e evidência, pertence ao proprietário.

**Não fazer afirmações prematuras:** o manifesto do Bambu no Filuna ainda registra o motor completo como pendente; o ZIP de perfis Bambu em QA e os hooks A1 atuais NÃO resolvem a A1 mini nem equivalência OEM de todos os modelos.

## Testes em execução e resultados
A tentativa Bambu oficial do 3MF foi iniciada. A primeira invocação no Ubuntu/Wine atingiu o limite de 78 segundos sem finalizar, enquanto o processo consumia CPU e já criava `plate_1.gcode.tmp`. **Resultado G-code final ainda não verificado nesse instante.** Preservar processo/arquivo temporário e aguardar a conclusão do trabalho em curso antes de lançar outro slicing do mesmo objeto.
