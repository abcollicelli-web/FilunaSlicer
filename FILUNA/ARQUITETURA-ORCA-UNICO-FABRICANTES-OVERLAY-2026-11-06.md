# FILUNA — ARQUITETURA DEFINITIVA DE UM MOTOR ORCA + OVERLAYS OEM POR FABRICANTE
**Decisão expressa do proprietário — 06/11/2026.** Substitui desenhos anteriores que fragmentavam recursos individuais sem um módulo OEM coeso.
**Status:** REGRA ARQUITETURAL APROVADA; implementação do carregador e equivalência OEM ainda em desenvolvimento/não homologados.

## Princípio
O Filuna Slicer **já existe e está compilado**, mantém uma única interface e um único engine Orca (`src/libslic3r`). Não distribuir/importar quatro fatiadores completos. Cada fabricante tem uma pasta isolada em `FILUNA/fabricantes/<fabricante>/` que contém **somente o delta necessário, tecnicamente compatível e reutilizável do fatiador oficial** em relação ao Orca fixado. Nunca duplicar módulos comuns ao Orca nem copiar executáveis completos.

Não confundir 'módulo completo de compatibilidade' com cópia indiscriminada de todos os arquivos: completo significa a soma de **todas as diferenças e dependências realmente necessárias ao funcionamento daquele modelo**, documentadas e verificadas. O delta pode abranger algoritmos, interpretação de perfis, scripts/macros G-code, materiais/multimaterial, movimento, purga, calibração, identificação, comunicação e recursos de hardware. O modelo/firmware determina quais opções ativar.

## Organização exigida
```text
FilunaSlicer/
├─ src/libslic3r/                     # UMA base Orca, compartilhada
├─ src/slic3r/GUI/                    # UMA interface Filuna/Orca
├─ FILUNA/fabricantes/
│  ├─ bambu/
│  │  ├─ manifest.json               # proveniência, versão, ABI, modelos, capacidades, gates
│  │  ├─ modelos/a1/                 # diferenças por modelo; NÃO duplica base
│  │  ├─ engine/                     # código fonte OEM livre portado, quando necessário
│  │  ├─ profiles/                   # somente deltas e overlays específicos
│  │  ├─ gcode/                      # macros específicas diferentes das já existentes
│  │  └─ transport/                  # protocolos LAN permitidos, sem plugin OEM fechado
│  ├─ anycubic/
│  ├─ flashforge/
│  ├─ elegoo/
│  └─ .../
└─ FILUNA/qa/                         # comparação binária/semântica e protocolo virtual
```
Arquivos compartilhados permanecem em Orca/Filuna. Os cinco macros A1 oficiais já idênticos aos do Filuna ficam referenciados por SHA, **não duplicados**.

## Caminho de execução no aplicativo
1. **Dispositivos:** identificar fabricante/modelo/firmware por serviços de descoberta disponíveis e autorizados, testar primeiro protocolos locais disponíveis/documentados, com sua autenticação. Só usar alternativa LAN com código de acesso da impressora quando os recursos anteriores forem insuficientes; o código pode autenticar o próprio MQTT/FTPS.
2. **Registro:** persistir impressoras do cliente com identidade estável, credenciais protegidas e vínculo de módulo/versão/capacidades. Uma impressora detectada sem pareamento não vira dispositivo registrado automaticamente.
3. **Preparar:** apresentar **sempre Orca Filuna**, selecionar automaticamente `fabricantes/<vendor>/manifest.json` e overlay compatível com o modelo/firmware da impressora registrada. Incluir **o pacote funcional completo de diferenças OEM**, não decidir pequenos recursos isoladamente no fluxo da UI. Incompatibilidade ou módulo ausente → status explícito/bloqueio para tarefas OEM, mantendo funções Orca genéricas de exportação offline. Não carregar um binário arbitrário ou permitir recursos desconhecidos.
4. **Fatiar:** as diferenças OEM compatíveis já compiladas ou carregáveis via ABI versionada são aplicadas ao mesmo pipeline Orca; registrar hash de módulo, preset efetivo, versão e decisões. Testar equivalência ao G-code original oficial (sem modificar referência), mesma geometria, bico, filamento e parâmetros.
5. **Imprimir:** listar somente dispositivos **registrados** e seu estado real; transferência e comando por protocolo OEM suportado, ACK real e confirmação expressa antes de iniciar.

## Compatibilidade C++ e carregamento
- **Data overlays** (presets/macros/matriz firmware) podem ser sincronizados como dados versionados, com validação de schema/proveniência.
- **Code overlays** dependentes de `libslic3r` devem ser integrados à compilação Filuna ou encapsulados por interface de plugin de ABI estável e assinada, conforme viabilidade; arquivos C++ numa pasta **não são executáveis** por si.
- Evitar incluir DLLs fechadas do OEM, contas/chaves/servidores. Para rede, utilizar APIs documentadas e implementações abertas/autorizadas.
- O Orca livre continua utilizável sem assinatura. Nunca atualizar módulos em uma sessão ativa sem salvar/reiniciar/rollback seguros.

## Processo de comparação de código e homologação
A. Pin de Orca e commit/fonte oficial OEM + licença/aviso por arquivo e dependência.
B. Comparar árvore OEM com Orca (path, hash, AST/API quando útil). Classificar: COMUM (reusar Orca), DELTA NECESSÁRIO (portar), DELTA NÃO NECESSÁRIO, BLOQUEADO LICENÇA, INCOMPATÍVEL/DEPENDÊNCIA PENDENTE.
C. Organizar os **deltas necessários como unidade completa por fabricante** e submodelo, não reescrever todas as features nem copiar tudo.
D. Resolver configurações herdadas e diferenças de engine; instrumentar o carregamento real do overlay no processo Filuna.
E. Testar: seleção registrada → Preparar → geração de G-code (semântica e hash quando aplicável) → envio em LAN virtual → testes Windows reais e hardware. FAIL/NOT RUN bloqueia equivalência, impressão remota e release.
F. Upstream Bambu/Orca ou outros fabricantes: comparar apenas novos commits/arquivos com manifesto de referência, portar **novos deltas necessários**, revalidar e atualizar manifestos/versão; sem reconstrução total automática ou merge não revisado.

## Primeiro piloto Bambu Lab A1
- Engine Orca pinado `a620d804efbe74162c96ad29d755cdab2b823530`.
- Cinco macros A1 0.4 idênticos aos oficiais (PASS de conteúdo, não PASS de fatiamento).
- Presets efetivos A1 ainda divergentes: 6 máquina, 8 filamento, 19 processo.
- Bambu Studio tem função de suavização de velocidade `smooth_coefficient`; Filuna ignora esta opção em `PrintConfig.cpp`. Um candidato matemático C++ está em `FILUNA/modules/bambu-a1/BambuSpeedMapping.hpp`, teste isolado PASS; **ainda não integrado ao pipeline**.
- G-code oficial × Filuna: MISMATCH; redes A1 apenas descoberta simulada SSDP; MQTT/TLS/FTPS e hardware NOT RUN.
- O port desse código candidato deve migrar para `fabricantes/bambu/engine/` apenas após validar todas as dependências (config, planner e G-code), mantendo referência ao local anterior até consolidar.

**Aceite definitivo:** um único Orca, um manifesto/overlay completo Bambu A1, rastreabilidade de código/ABI, carregamento interno realmente comprovado, G-code equivalente sob condições controladas e teste físico aprovado. Nenhum desses últimos gates muda para PASS só pela criação da pasta.


## Evidência de incorporação parcial real — Bambu A1 / N2S (06/11/2026)
- Fonte oficial: `bambulab/BambuStudio/resources/printers/N2S.json` (blob `3a5929b98244d7800ee06a7fe41662f0141306ce`) identifica **Bambu A1 = N2S**, SN prefixo 039. Snapshot mantido em `FILUNA/fabricantes/bambu/modelos/a1/N2S.json`, sem habilitar campos de firmware automaticamente.
- Código de descoberta SSDP nativo `FILUNA/fabricantes/bambu/transport/BambuLanDiscovery.hpp` valida `N2S`, prefixo 039 e resposta do protocolo. Entrada C++ `engine/BambuA1Module.hpp` compartilhada por registro de Dispositivos e Preparar via `FilunaOEMOverlay.hpp`; correções no `DevManager.cpp` e chamada em `Plater.cpp::reslice`. Ambos os caminhos **inseridos no código-fonte**, sem UI substituta nem outro fatiador instalado.
- Teste C++17 compilado e executado no Ubuntu: módulo/inicialização Dispositivos-Preparar PASS; resposta UDP real do **emulador LAN** 10.77.0.12:1990 → 10.77.0.1:2021, N2S serial 039LAB0000001 PASS. Esta prova não confirma compilação Windows integral nem impressão física.
- `FILUNA/qa/virtual-lab/bambu_a1_side_by_side.py` rodou no servidor e produziu `/var/tmp/filuna-qa-wine/bambu-side-by-side-20261106/result.json`: seleção nativa PASS, SSDP emulado PASS, **G-code Bambu Studio oficial anterior vs Filuna Orca anterior MISMATCH**, sem recompilar aplicativos nessa comparação. Estado global HOMOLOGATION_BLOCKED.
- Auditor `FILUNA/qa/verify_bambu_a1_capabilities.py` encontrou 5 campos diferentes entre a matriz oficial N2S e a matriz runtime Orca (AMS externo, timelapse, posição de detecção, imagem, SN prefixo). Nenhuma capacidade foi ativada automaticamente por causa deste delta.
- Geração de arquivos de impressão equivalente ao binário oficial e transporte MQTT/TLS 8883, FTPS 990, autenticação com código LAN, envio e ACK físico ainda **NOT RUN** no Filuna integrado. Na Bambu, o LAN Code é a credencial exigida pelos serviços MQTT/FTPS quando ativados, não um transporte independente. Sem credencial real fornecida pela própria impressora, não afirmar pareamento.
- Workflow dedicado `.github/workflows/filuna-oem-windows-qa.yml` foi registrado para tentar compilar a branch candidata; **não considerar build PASS até receber resultado verificável**. Nenhum merge/deploy autorizado ou executado.
